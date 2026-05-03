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

/* USER CODE END Variables */
/* 消息队列定义 */
osMessageQueueId_t dataQueueHandle;
osMessageQueueId_t cmdQueueHandle;

/* 任务定义 */
osThreadId_t sensorTaskHandle;
osThreadId_t lcdTaskHandle;
osThreadId_t uartTaskHandle;
osThreadId_t cmdTaskHandle;
osThreadId_t fanTaskHandle;

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

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartSensorTask(void *argument);
void StartLcdTask(void *argument);
void StartUartTask(void *argument);
void StartCmdTask(void *argument);
void StartFanTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

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
  dataQueueHandle = osMessageQueueNew(10, sizeof(SensorData_t), NULL);
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

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* 传感器任务：读取所有传感器数据 */
void StartSensorTask(void *argument) {
  SensorData_t sensor_data;
  
  /* 初始化传感器 */
  BME280_Init();
  
  for (;;) {
    /* 读取 BME280 */
    if (BME280_ReadData(&sensor_data.temperature, &sensor_data.humidity, &sensor_data.pressure) == HAL_OK) {
      sensor_data.bme280_valid = true;
    } else {
      sensor_data.bme280_valid = false;
    }
    
    /* 读取 ADC (光敏和 PM2.5) */
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
      sensor_data.light = HAL_ADC_GetValue(&hadc1);
      sensor_data.light_valid = true;
    } else {
      sensor_data.light_valid = false;
    }
    
    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
      sensor_data.pm25 = HAL_ADC_GetValue(&hadc1);
      sensor_data.pm25_valid = true;
    } else {
      sensor_data.pm25_valid = false;
    }
    
    /* 添加时间戳 */
    sensor_data.timestamp = osKernelGetTickCount();
    
    /* 发送数据到队列 */
    osMessageQueuePut(dataQueueHandle, &sensor_data, 0, 0);
    
    osDelay(500); /* 500ms 采样一次 */
  }
}

/* LCD 显示任务 */
void StartLcdTask(void *argument) {
  SensorData_t sensor_data;
  char buffer[64];
  
  LCD_Init();
  LCD_Clear(COLOR_BLACK);
  
  LCD_ShowString(10, 10, "Sensor Monitor", COLOR_WHITE, COLOR_BLACK, 16);
  LCD_DrawLine(10, 28, 310, 28, COLOR_GRAY);
  
  for (;;) {
    /* 接收传感器数据 */
    if (osMessageQueueGet(dataQueueHandle, &sensor_data, NULL, 100) == osOK) {
      /* 显示温度 */
      LCD_ShowString(10, 40, "Temp:", COLOR_WHITE, COLOR_BLACK, 16);
      if (sensor_data.bme280_valid) {
        LCD_ShowFloat(80, 40, sensor_data.temperature, 2, COLOR_GREEN, COLOR_BLACK, 16);
        LCD_ShowString(180, 40, "C", COLOR_WHITE, COLOR_BLACK, 16);
      } else {
        LCD_ShowString(80, 40, "---", COLOR_RED, COLOR_BLACK, 16);
      }
      
      /* 显示湿度 */
      LCD_ShowString(10, 60, "Humid:", COLOR_WHITE, COLOR_BLACK, 16);
      if (sensor_data.bme280_valid) {
        LCD_ShowFloat(80, 60, sensor_data.humidity, 1, COLOR_GREEN, COLOR_BLACK, 16);
        LCD_ShowString(140, 60, "%", COLOR_WHITE, COLOR_BLACK, 16);
      } else {
        LCD_ShowString(80, 60, "---", COLOR_RED, COLOR_BLACK, 16);
      }
      
      /* 显示气压 */
      LCD_ShowString(10, 80, "Press:", COLOR_WHITE, COLOR_BLACK, 16);
      if (sensor_data.bme280_valid) {
        LCD_ShowFloat(80, 80, sensor_data.pressure, 1, COLOR_GREEN, COLOR_BLACK, 16);
        LCD_ShowString(180, 80, "hPa", COLOR_WHITE, COLOR_BLACK, 16);
      } else {
        LCD_ShowString(80, 80, "---", COLOR_RED, COLOR_BLACK, 16);
      }
      
      /* 显示光敏 */
      LCD_ShowString(10, 100, "Light:", COLOR_WHITE, COLOR_BLACK, 16);
      if (sensor_data.light_valid) {
        LCD_ShowNumber(80, 100, sensor_data.light, COLOR_YELLOW, COLOR_BLACK, 16);
      } else {
        LCD_ShowString(80, 100, "---", COLOR_RED, COLOR_BLACK, 16);
      }
      
      /* 显示 PM2.5 */
      LCD_ShowString(10, 120, "PM2.5:", COLOR_WHITE, COLOR_BLACK, 16);
      if (sensor_data.pm25_valid) {
        LCD_ShowNumber(80, 120, sensor_data.pm25, COLOR_YELLOW, COLOR_BLACK, 16);
      } else {
        LCD_ShowString(80, 120, "---", COLOR_RED, COLOR_BLACK, 16);
      }
    }
  }
}

/* UART 任务：通过蓝牙发送数据 */
void StartUartTask(void *argument) {
  SensorData_t sensor_data;
  char buffer[128];
  uint16_t len;
  
  for (;;) {
    /* 接收传感器数据 */
    if (osMessageQueueGet(dataQueueHandle, &sensor_data, NULL, 1000) == osOK) {
      /* 格式化数据字符串 */
      len = sprintf(buffer, "{\"temp\":%.2f,\"humid\":%.1f,\"press\":%.1f,\"light\":%d,\"pm25\":%d}\r\n",
        sensor_data.temperature,
        sensor_data.humidity,
        sensor_data.pressure,
        sensor_data.light,
        sensor_data.pm25);
      
      /* 通过 UART2 发送 (HC05) */
      HAL_UART_Transmit(&huart2, (uint8_t *)buffer, len, 100);
      
      /* 通过 UART1 发送 (调试) */
      HAL_UART_Transmit(&huart1, (uint8_t *)buffer, len, 100);
    }
  }
}

/* 风扇控制任务 - 温度过高自动触发风扇 */
void StartFanTask(void *argument) {
  SensorData_t sensor_data;
  int32_t fan_speed = 0;
  
  for (;;) {
    /* 从队列接收传感器数据 */
    if (osMessageQueueGet(dataQueueHandle, &sensor_data, NULL, 100) == osOK) {
      /* 只有在自动模式下才执行温度控制 */
      if (GetFanMode() == 0) {
        /* 温度阈值控制风扇 */
        if (sensor_data.bme280_valid) {
          if (sensor_data.temperature > 35.0f) {
            fan_speed = 100;  /* 温度 > 35°C, 全速 */
          } else if (sensor_data.temperature > 30.0f) {
            fan_speed = 50;   /* 温度 30-35°C, 半速 */
          } else if (sensor_data.temperature > 25.0f) {
            fan_speed = 25;   /* 温度 25-30°C, 低速 */
          } else {
            fan_speed = 0;    /* 温度 < 25°C, 关闭 */
          }
          
          /* 设置风扇速度 (电机A连接风扇) */
          Motor_SetSpeedA(fan_speed);
          SetManualFanSpeed(fan_speed);
        }
      }
    }
  }
}

/* 命令处理任务 */
void StartCmdTask(void *argument) {
  Command_t cmd;
  char response[128];
  uint16_t len;
  
  /* 初始化电机 */
  Motor_Init();
  
  /* 初始化 UART2 DMA 空闲中断接收 */
  UART2_DMA_Init();
  
  for (;;) {
    /* 等待命令 */
    if (osMessageQueueGet(cmdQueueHandle, &cmd, NULL, 1000) == osOK) {
      switch (cmd.type) {
        case CMD_SET_MOTOR:
          Motor_SetSpeedA(cmd.data.motor.speed);
          Motor_SetSpeedB(cmd.data.motor.speed);
          break;
          
        case CMD_SET_FAN_MODE:
          SetFanMode(cmd.data.fan.mode);
          if (cmd.data.fan.mode == 1) {
            Motor_SetSpeedA(0);
          }
          break;
          
        case CMD_GET_VERSION:
          len = sprintf(response, "VERSION: STM32-F103VET6-FreeRTOS v1.0.0\r\n");
          HAL_UART_Transmit(&huart2, (uint8_t *)response, len, 100);
          HAL_UART_Transmit(&huart1, (uint8_t *)response, len, 100);
          break;
          
        case CMD_GET_STATUS:
          len = sprintf(response, "STATUS: fan_mode=%d fan_speed=%d uptime=%lu\r\n",
            GetFanMode(), GetManualFanSpeed(), osKernelGetTickCount());
          HAL_UART_Transmit(&huart2, (uint8_t *)response, len, 100);
          HAL_UART_Transmit(&huart1, (uint8_t *)response, len, 100);
          break;
          
        case CMD_GET_DATA:
          len = sprintf(response, "DATA: use dataQueue to get sensor data\r\n");
          HAL_UART_Transmit(&huart2, (uint8_t *)response, len, 100);
          HAL_UART_Transmit(&huart1, (uint8_t *)response, len, 100);
          break;
          
        default:
          break;
      }
    }
  }
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

