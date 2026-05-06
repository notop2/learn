/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "queue.h"
#include "sensor_data.h"
#include "bme280.h"
#include "lcd.h"
#include "motor.h"
#include "adc.h"
#include "usart.h"
#include "filter.h"
#include "version.h"
#include "diagnostic.h"
#include "watchdog.h"
#include "storage.h"
#include <stdio.h>
#include <string.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
/* 全局共享传感器数据 - 发布订阅模式 */
static volatile SensorData_t g_sensor_data;
static osEventFlagsId_t sensorEventFlags;
#define FLAG_NEW_DATA  0x0001U
/* USER CODE END Variables */
/* 消息队列定义 */
osMessageQueueId_t cmdQueueHandle;

/* 任务定义 */
osThreadId_t sensorTaskHandle;
osThreadId_t lcdTaskHandle;
osThreadId_t uartTaskHandle;
osThreadId_t cmdTaskHandle;
osThreadId_t fanTaskHandle;
osThreadId_t monitorTaskHandle;

/* 任务属性 */
const osThreadAttr_t sensorTask_attributes = {
  .name = "sensorTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t lcdTask_attributes = {
  .name = "lcdTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};

const osThreadAttr_t uartTask_attributes = {
  .name = "uartTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t cmdTask_attributes = {
  .name = "cmdTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};

const osThreadAttr_t fanTask_attributes = {
  .name = "fanTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t monitorTask_attributes = {
  .name = "monitorTask",
  .stack_size = 256 * 2,
  .priority = (osPriority_t) osPriorityIdle,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartSensorTask(void *argument);
void StartLcdTask(void *argument);
void StartUartTask(void *argument);
void StartCmdTask(void *argument);
void StartFanTask(void *argument);
void StartMonitorTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* 初始化看门狗 */
  WDT_Init();

  /* 注册任务到看门狗监控 */
  WDT_RegisterTask(TASK_ID_SENSOR, "sensor", 1500, true);
  WDT_RegisterTask(TASK_ID_LCD, "lcd", 1500, false);
  WDT_RegisterTask(TASK_ID_UART, "uart", 1500, false);
  WDT_RegisterTask(TASK_ID_CMD, "cmd", 1500, true);
  WDT_RegisterTask(TASK_ID_FAN, "fan", 1500, false);
  WDT_RegisterTask(TASK_ID_MONITOR, "monitor", 10000, true);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* 初始化消息队列 */
  /* USER CODE BEGIN RTOS_QUEUES */
  cmdQueueHandle = osMessageQueueNew(5, sizeof(Command_t), NULL);
  /* USER CODE END RTOS_QUEUES */

  /* 创建任务 */
  /* creation of sensorTask */
  sensorTaskHandle = osThreadNew(StartSensorTask, NULL, &sensorTask_attributes);

  /* creation of lcdTask */
  lcdTaskHandle = osThreadNew(StartLcdTask, NULL, &lcdTask_attributes);

  /* creation of uartTask */
  uartTaskHandle = osThreadNew(StartUartTask, NULL, &uartTask_attributes);

  /* creation of cmdTask */
  cmdTaskHandle = osThreadNew(StartCmdTask, NULL, &cmdTask_attributes);

  /* creation of fanTask */
  fanTaskHandle = osThreadNew(StartFanTask, NULL, &fanTask_attributes);

  /* creation of monitorTask */
  monitorTaskHandle = osThreadNew(StartMonitorTask, NULL, &monitorTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  sensorEventFlags = osEventFlagsNew(NULL);
  /* USER CODE END RTOS_EVENTS */

}

/* 私有变量 - 滤波器实例 */
static MovingAvgFilter_t g_temp_filter;
static MovingAvgFilter_t g_humid_filter;
static MovingAvgFilter_t g_press_filter;
static MovingAvgFilter_t g_light_filter;
static MovingAvgFilter_t g_pm25_filter;

#define ADC_RESOLUTION     4095
#define ADC_VREF           3.3f

static inline float ADC_ToVoltage(uint16_t adc_value)
{
    return (float)adc_value * ADC_VREF / ADC_RESOLUTION;
}

#define LIGHT_MAX_LUX      1000.0f
#define LIGHT_VOLTAGE_MAX  3.3f

static inline float ADC_ToLux(float voltage)
{
    if (voltage <= 0.001f) return 0.0f;
    if (voltage >= LIGHT_VOLTAGE_MAX) return LIGHT_MAX_LUX;
    float ratio = voltage / LIGHT_VOLTAGE_MAX;
    return ratio * LIGHT_MAX_LUX;
}

#define PM25_VOLTAGE_CLEAN  0.6f
#define PM25_MAX_UGM3      500.0f
#define PM25_VOLTAGE_MAX   2.8f

static inline float ADC_ToPM25(float voltage)
{
    if (voltage <= PM25_VOLTAGE_CLEAN) return 0.0f;
    if (voltage >= PM25_VOLTAGE_MAX) return PM25_MAX_UGM3;
    float ratio = (voltage - PM25_VOLTAGE_CLEAN) / (PM25_VOLTAGE_MAX - PM25_VOLTAGE_CLEAN);
    return ratio * PM25_MAX_UGM3;
}

/* 风扇状态机 */
typedef enum {
    FAN_IDLE = 0,
    FAN_LOW,
    FAN_MEDIUM,
    FAN_HIGH,
    FAN_MAX
} FanState_t;

static FanState_t g_fan_state = FAN_IDLE;
static uint32_t g_state_entry_time = 0;
static const uint32_t FAN_HYSTERESIS_MS = 10000;

static int32_t fan_state_to_speed(FanState_t state)
{
    switch (state) {
        case FAN_IDLE:   return 0;
        case FAN_LOW:    return 25;
        case FAN_MEDIUM: return 50;
        case FAN_HIGH:   return 75;
        case FAN_MAX:    return 100;
    }
    return 0;
}

/* 传感器任务：读取所有传感器数据 */
void StartSensorTask(void *argument) {
  SensorData_t sensor_data;
  
  /* 初始化传感器 */
  BME280_Init();
  
  /* 初始化滤波器 */
  MovingAvg_Init(&g_temp_filter, 25.0f);
  MovingAvg_Init(&g_humid_filter, 50.0f);
  MovingAvg_Init(&g_press_filter, 1013.0f);
  MovingAvg_Init(&g_light_filter, ADC_ToLux(ADC_ToVoltage(2048)));
  MovingAvg_Init(&g_pm25_filter, ADC_ToPM25(ADC_ToVoltage(2048)));
  
  for (;;) {
    /* 读取 BME280 */
    if (BME280_ReadData(&sensor_data.temperature, &sensor_data.humidity, &sensor_data.pressure) == HAL_OK) {
      sensor_data.bme280_valid = true;
      /* 应用移动平均滤波 */
      sensor_data.temp_filtered = MovingAvg_Update(&g_temp_filter, sensor_data.temperature);
      sensor_data.humid_filtered = MovingAvg_Update(&g_humid_filter, sensor_data.humidity);
      sensor_data.press_filtered = MovingAvg_Update(&g_press_filter, sensor_data.pressure);
    } else {
      sensor_data.bme280_valid = false;
      sensor_data.temp_filtered = sensor_data.temperature;
      sensor_data.humid_filtered = sensor_data.humidity;
      sensor_data.press_filtered = sensor_data.pressure;
    }
    
    /* 读取 ADC (光敏和 PM2.5) */
    uint16_t adc_raw;
    float voltage;
    
    ADC_SelectChannel(ADC_CHANNEL_10);
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
      adc_raw = HAL_ADC_GetValue(&hadc1);
      voltage = ADC_ToVoltage(adc_raw);
      sensor_data.light = ADC_ToLux(voltage);
      sensor_data.light_valid = true;
      sensor_data.light_filtered = MovingAvg_Update(&g_light_filter, sensor_data.light);
    } else {
      sensor_data.light_valid = false;
      sensor_data.light = 0.0f;
      sensor_data.light_filtered = 0.0f;
    }
    HAL_ADC_Stop(&hadc1);
    
    ADC_SelectChannel(ADC_CHANNEL_12);
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
      adc_raw = HAL_ADC_GetValue(&hadc1);
      voltage = ADC_ToVoltage(adc_raw);
      sensor_data.pm25 = ADC_ToPM25(voltage);
      sensor_data.pm25_valid = true;
      sensor_data.pm25_filtered = MovingAvg_Update(&g_pm25_filter, sensor_data.pm25);
    } else {
      sensor_data.pm25_valid = false;
      sensor_data.pm25 = 0.0f;
      sensor_data.pm25_filtered = 0.0f;
    }
    HAL_ADC_Stop(&hadc1);
    
    /* 添加时间戳 */
    sensor_data.timestamp = osKernelGetTickCount();
    
    /* 写入全局共享数据，广播给所有消费者 */
    g_sensor_data = sensor_data;
    osEventFlagsSet(sensorEventFlags, FLAG_NEW_DATA);
    
    WDT_TaskAlive(TASK_ID_SENSOR);
    
    osDelay(500); /* 500ms 采样一次 */
  }
}

/* LCD 显示任务 */
void StartLcdTask(void *argument) {
  SensorData_t sensor_data;
  
  LCD_Init();
  LCD_Clear(COLOR_BLACK);
  
  LCD_ShowString(10, 10, "Sensor Monitor", COLOR_WHITE, COLOR_BLACK, 16);
  LCD_DrawLine(10, 28, 310, 28, COLOR_GRAY);
  
  for (;;) {
    /* 等待新数据事件 */
    osEventFlagsWait(sensorEventFlags, FLAG_NEW_DATA, osFlagsWaitAny, osWaitForever);
    sensor_data = g_sensor_data;
    
    /* 显示温度 */
    LCD_ShowString(10, 40, "Temp:", COLOR_WHITE, COLOR_BLACK, 16);
    if (sensor_data.bme280_valid) {
      LCD_ShowFloat(80, 40, sensor_data.temp_filtered, 2, COLOR_GREEN, COLOR_BLACK, 16);
      LCD_ShowString(180, 40, "C", COLOR_WHITE, COLOR_BLACK, 16);
    } else {
      LCD_ShowString(80, 40, "---", COLOR_RED, COLOR_BLACK, 16);
    }
    
    /* 显示湿度 */
    LCD_ShowString(10, 60, "Humid:", COLOR_WHITE, COLOR_BLACK, 16);
    if (sensor_data.bme280_valid) {
      LCD_ShowFloat(80, 60, sensor_data.humid_filtered, 1, COLOR_GREEN, COLOR_BLACK, 16);
      LCD_ShowString(140, 60, "%", COLOR_WHITE, COLOR_BLACK, 16);
    } else {
      LCD_ShowString(80, 60, "---", COLOR_RED, COLOR_BLACK, 16);
    }
    
    /* 显示气压 */
    LCD_ShowString(10, 80, "Press:", COLOR_WHITE, COLOR_BLACK, 16);
    if (sensor_data.bme280_valid) {
      LCD_ShowFloat(80, 80, sensor_data.press_filtered, 1, COLOR_GREEN, COLOR_BLACK, 16);
      LCD_ShowString(180, 80, "hPa", COLOR_WHITE, COLOR_BLACK, 16);
    } else {
      LCD_ShowString(80, 80, "---", COLOR_RED, COLOR_BLACK, 16);
    }
    
    /* 显示光敏 */
    LCD_ShowString(10, 100, "Light:", COLOR_WHITE, COLOR_BLACK, 16);
    if (sensor_data.light_valid) {
      LCD_ShowNumber(80, 100, (uint16_t)sensor_data.light, COLOR_YELLOW, COLOR_BLACK, 16);
      LCD_ShowString(120, 100, "lx", COLOR_WHITE, COLOR_BLACK, 16);
    } else {
      LCD_ShowString(80, 100, "---", COLOR_RED, COLOR_BLACK, 16);
    }
    
    /* 显示 PM2.5 */
    LCD_ShowString(10, 120, "PM2.5:", COLOR_WHITE, COLOR_BLACK, 16);
    if (sensor_data.pm25_valid) {
      LCD_ShowFloat(80, 120, sensor_data.pm25, 1, COLOR_YELLOW, COLOR_BLACK, 16);
      LCD_ShowString(120, 120, "ug/m3", COLOR_WHITE, COLOR_BLACK, 16);
    } else {
      LCD_ShowString(80, 120, "---", COLOR_RED, COLOR_BLACK, 16);
    }
    
    WDT_TaskAlive(TASK_ID_LCD);
  }
}

/* UART 任务：通过蓝牙发送数据 */
void StartUartTask(void *argument) {
  SensorData_t sensor_data;
  char buffer[128];
  uint16_t len;
  
  for (;;) {
    /* 等待新数据事件 */
    osEventFlagsWait(sensorEventFlags, FLAG_NEW_DATA, osFlagsWaitAny, osWaitForever);
    sensor_data = g_sensor_data;
    
    /* 格式化数据字符串 */
    len = sprintf(buffer, "{\"temp\":%.2f,\"humid\":%.1f,\"press\":%.1f,\"light\":%.0f,\"pm25\":%.1f}\r\n",
      sensor_data.temperature,
      sensor_data.humidity,
      sensor_data.pressure,
      sensor_data.light,
      sensor_data.pm25);
    
    /* 通过 UART2 发送 (HC05) */
    HAL_UART_Transmit(&huart2, (uint8_t *)buffer, len, 100);
    
    /* 通过 UART1 发送 (调试) */
    HAL_UART_Transmit(&huart1, (uint8_t *)buffer, len, 100);
    
    WDT_TaskAlive(TASK_ID_UART);
  }
}

/* 风扇控制任务 - 温度过高自动触发风扇 (带迟滞状态机) */
void StartFanTask(void *argument) {
  SensorData_t sensor_data;
  FanState_t next_state;
  
  g_state_entry_time = osKernelGetTickCount();
  
  for (;;) {
    /* 等待新数据事件 */
    osEventFlagsWait(sensorEventFlags, FLAG_NEW_DATA, osFlagsWaitAny, osWaitForever);
    sensor_data = g_sensor_data;
    
    /* 只有在自动模式下才执行温度控制 */
    if (GetFanMode() == 0) {
      /* 温度阈值控制 - 带迟滞的状态机 */
      if (sensor_data.bme280_valid) {
        float temp = sensor_data.temp_filtered;
        uint32_t now = osKernelGetTickCount();
        uint32_t elapsed = now - g_state_entry_time;
        
        next_state = g_fan_state;
        
        switch (g_fan_state) {
          case FAN_IDLE:
            if (temp > 28.0f)  next_state = FAN_LOW;
            break;
          case FAN_LOW:
            if (temp > 32.0f)        next_state = FAN_MEDIUM;
            else if (temp < 26.0f && elapsed > FAN_HYSTERESIS_MS) next_state = FAN_IDLE;
            break;
          case FAN_MEDIUM:
            if (temp > 36.0f)        next_state = FAN_HIGH;
            else if (temp < 30.0f && elapsed > FAN_HYSTERESIS_MS) next_state = FAN_LOW;
            break;
          case FAN_HIGH:
            if (temp > 40.0f)        next_state = FAN_MAX;
            else if (temp < 34.0f && elapsed > FAN_HYSTERESIS_MS) next_state = FAN_MEDIUM;
            break;
          case FAN_MAX:
            if (temp < 38.0f && elapsed > FAN_HYSTERESIS_MS) next_state = FAN_HIGH;
            break;
        }
        
        if (next_state != g_fan_state) {
          g_fan_state = next_state;
          g_state_entry_time = now;
        }
        
        int32_t speed = fan_state_to_speed(g_fan_state);
        Motor_SetSpeedA(speed);
        SetManualFanSpeed(speed);
      }
    }
    WDT_TaskAlive(TASK_ID_FAN);
  }
}

/* 命令处理任务 */
void StartCmdTask(void *argument) {
  Command_t cmd;
  char response[256];
  uint16_t len;
  SystemParams_t sys_params;
  DiagReport_t diag_report;
  
  /* 初始化电机 */
  Motor_Init();
  
  /* 初始化 UART2 DMA 空闲中断接收 */
  UART2_DMA_Init();
  
  /* 加载持久化参数 */
  if (STORAGE_Load(&sys_params)) {
    SetFanMode(sys_params.fan_mode);
  }
  
  for (;;) {
    /* 等待命令 */
    if (osMessageQueueGet(cmdQueueHandle, &cmd, NULL, 1000) == osOK) {
      switch (cmd.type) {
        case CMD_SET_MOTOR:
          Motor_SetSpeedA(cmd.data.motor.speed);
          Motor_SetSpeedB(cmd.data.motor.speed);
          sys_params.fan_speed = cmd.data.motor.speed;
          break;
          
        case CMD_SET_FAN_MODE:
          SetFanMode(cmd.data.fan.mode);
          sys_params.fan_mode = cmd.data.fan.mode;
          if (cmd.data.fan.mode == 1) {
            Motor_SetSpeedA(0);
          }
          STORAGE_Save(&sys_params);
          break;
          
        case CMD_GET_VERSION:
          len = sprintf(response, "VERSION: %s\r\n", FW_STRING);
          HAL_UART_Transmit(&huart2, (uint8_t *)response, len, 100);
          HAL_UART_Transmit(&huart1, (uint8_t *)response, len, 100);
          break;
          
        case CMD_GET_STATUS: {
          len = sprintf(response, "STATUS: fan_mode=%d fan_speed=%d uptime=%lu\r\n",
            GetFanMode(), GetManualFanSpeed(), osKernelGetTickCount());
          HAL_UART_Transmit(&huart2, (uint8_t *)response, len, 100);
          HAL_UART_Transmit(&huart1, (uint8_t *)response, len, 100);

          WDT_PrintStatus(response, sizeof(response));
          HAL_UART_Transmit(&huart2, (uint8_t *)response, strlen(response), 100);
          break;
        }
        case CMD_GET_DATA: {
          len = sprintf(response, "DATA: sensor data is broadcast via event flags\r\n");
          HAL_UART_Transmit(&huart2, (uint8_t *)response, len, 100);
          HAL_UART_Transmit(&huart1, (uint8_t *)response, len, 100);
          break;
        }
        case CMD_DIAGNOSTIC: {
          DIAG_RunAll(&diag_report);
          DIAG_PrintReport(&diag_report, response, sizeof(response));
          HAL_UART_Transmit(&huart2, (uint8_t *)response, strlen(response), 100);
          HAL_UART_Transmit(&huart1, (uint8_t *)response, strlen(response), 100);
          break;
        }
        default:
          break;
      }
    }
    WDT_TaskAlive(TASK_ID_CMD);
  }
}

/* 系统监控任务：喂狗 + 检查任务健康 + 系统信息 */
void StartMonitorTask(void *argument) {

  for (;;) {
    WDT_Feed();
    WDT_CheckAllTasks();
    WDT_TaskAlive(TASK_ID_MONITOR);
    osDelay(2000);
  }
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

