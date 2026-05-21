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

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "queue.h"
#include "sensor_data.h"
#include "sensor_raw_msg.h"
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
#include "mavlink_protocol.h"
#include "ota.h"
#include <string.h>

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* ========================================================= */
/*  共享内存池 — 消费者零拷贝读取                               */
/* ========================================================= */
typedef enum {
    SENSOR_TYPE_TEMP = 0,
    SENSOR_TYPE_HUMID,
    SENSOR_TYPE_PRESS,
    SENSOR_TYPE_LIGHT,
    SENSOR_TYPE_PM25,
    SENSOR_TYPE_COUNT
} SensorType_t;

typedef struct {
    float value;
    uint32_t timestamp;
    bool valid;
} SensorSlot_t;

static volatile SensorSlot_t g_sensor_pool[SENSOR_TYPE_COUNT];

/* 每个消费者独立的事件标志 */
static osEventFlagsId_t lcdEventFlags;
static osEventFlagsId_t uartEventFlags;
static osEventFlagsId_t fanEventFlags;
#define FLAG_NEW_DATA  0x0001U

/* 消息队列 */
osMessageQueueId_t cmdQueueHandle;
static osMessageQueueId_t sensorRawQueueHandle;

/* ADC 互斥锁 — 保护 ADC1 (LightTask + PM25Task) */
static osMutexId_t adcMutexHandle;

/* 任务句柄 */
osThreadId_t bme280TaskHandle;
osThreadId_t lightTaskHandle;
osThreadId_t pm25TaskHandle;
osThreadId_t distributorTaskHandle;
osThreadId_t lcdTaskHandle;
osThreadId_t uartTaskHandle;
osThreadId_t cmdTaskHandle;
osThreadId_t fanTaskHandle;
osThreadId_t monitorTaskHandle;

/* 任务属性 */
const osThreadAttr_t bme280Task_attributes = {
  .name = "bme280Task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
const osThreadAttr_t lightTask_attributes = {
  .name = "lightTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
const osThreadAttr_t pm25Task_attributes = {
  .name = "pm25Task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
const osThreadAttr_t distributorTask_attributes = {
  .name = "distributorTask",
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

/* USER CODE END Variables */

void StartBME280Task(void *argument);
void StartLightTask(void *argument);
void StartPM25Task(void *argument);
void StartDistributorTask(void *argument);
void StartLcdTask(void *argument);
void StartUartTask(void *argument);
void StartCmdTask(void *argument);
void StartFanTask(void *argument);
void StartMonitorTask(void *argument);

void MX_FREERTOS_Init(void);

void MX_FREERTOS_Init(void) {
  WDT_Init();

  WDT_RegisterTask(TASK_ID_BME280, "bme280", 1500, true);
  WDT_RegisterTask(TASK_ID_LIGHT, "light", 2000, false);
  WDT_RegisterTask(TASK_ID_PM25, "pm25", 2000, false);
  WDT_RegisterTask(TASK_ID_DISTRIBUTOR, "distributor", 2000, false);
  WDT_RegisterTask(TASK_ID_LCD, "lcd", 1500, false);
  WDT_RegisterTask(TASK_ID_UART, "uart", 1500, false);
  WDT_RegisterTask(TASK_ID_CMD, "cmd", 1500, true);
  WDT_RegisterTask(TASK_ID_FAN, "fan", 2000, false);
  WDT_RegisterTask(TASK_ID_MONITOR, "monitor", 10000, true);

  /* USER CODE BEGIN RTOS_MUTEX */
  adcMutexHandle = osMutexNew(NULL);
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_QUEUES */
  cmdQueueHandle = osMessageQueueNew(5, sizeof(Command_t), NULL);
  sensorRawQueueHandle = osMessageQueueNew(10, sizeof(SensorRawMsg_t), NULL);
  /* USER CODE END RTOS_QUEUES */

  bme280TaskHandle = osThreadNew(StartBME280Task, NULL, &bme280Task_attributes);
  lightTaskHandle = osThreadNew(StartLightTask, NULL, &lightTask_attributes);
  pm25TaskHandle = osThreadNew(StartPM25Task, NULL, &pm25Task_attributes);
  distributorTaskHandle = osThreadNew(StartDistributorTask, NULL, &distributorTask_attributes);
  lcdTaskHandle = osThreadNew(StartLcdTask, NULL, &lcdTask_attributes);
  uartTaskHandle = osThreadNew(StartUartTask, NULL, &uartTask_attributes);
  cmdTaskHandle = osThreadNew(StartCmdTask, NULL, &cmdTask_attributes);
  fanTaskHandle = osThreadNew(StartFanTask, NULL, &fanTask_attributes);
  monitorTaskHandle = osThreadNew(StartMonitorTask, NULL, &monitorTask_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  lcdEventFlags = osEventFlagsNew(NULL);
  uartEventFlags = osEventFlagsNew(NULL);
  fanEventFlags = osEventFlagsNew(NULL);
  /* USER CODE END RTOS_EVENTS */
}

/* ========================================================= */
/*  工具函数                                                  */
/* ========================================================= */
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

static uint32_t adc_sample_oversample(uint32_t channel, int samples)
{
    uint32_t sum = 0;
    ADC_SelectChannel(channel);
    HAL_ADC_Start(&hadc1);
    for (int i = 0; i < samples; i++) {
        HAL_ADC_PollForConversion(&hadc1, 100);
        sum += HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);
    return sum;
}

/* ========================================================= */
/*  生产者: BME280 — 纯采集，不滤波、不写池                    */
/* ========================================================= */

void StartBME280Task(void *argument) {
  SensorRawMsg_t msg;

  BME280_Init();

  for (;;) {
    float temp, humid, press;

    msg.valid = (BME280_ReadData(&temp, &humid, &press) == HAL_OK) ? true : false;

    if (msg.valid) {
      msg.type  = RAW_TEMP;  msg.value = temp;  osMessageQueuePut(sensorRawQueueHandle, &msg, 0, 0);
      msg.type  = RAW_HUMID; msg.value = humid; osMessageQueuePut(sensorRawQueueHandle, &msg, 0, 0);
      msg.type  = RAW_PRESS; msg.value = press; osMessageQueuePut(sensorRawQueueHandle, &msg, 0, 0);
    } else {
      msg.type  = RAW_TEMP;  msg.value = 0; osMessageQueuePut(sensorRawQueueHandle, &msg, 0, 0);
      msg.type  = RAW_HUMID; msg.value = 0; osMessageQueuePut(sensorRawQueueHandle, &msg, 0, 0);
      msg.type  = RAW_PRESS; msg.value = 0; osMessageQueuePut(sensorRawQueueHandle, &msg, 0, 0);
    }

    WDT_TaskAlive(TASK_ID_BME280);
    osDelay(500);
  }
}

/* ========================================================= */
/*  生产者: 光敏 — 纯采集，不滤波、不写池                      */
/* ========================================================= */

void StartLightTask(void *argument) {
  SensorRawMsg_t msg = { .type = RAW_LIGHT };

  for (;;) {
    osMutexAcquire(adcMutexHandle, osWaitForever);
    uint32_t sum = adc_sample_oversample(ADC_CHANNEL_10, 4);
    osMutexRelease(adcMutexHandle);

    if (sum > 0) {
      float voltage = ADC_ToVoltage(sum / 4);
      msg.value = ADC_ToLux(voltage);
      msg.valid = true;
    } else {
      msg.value = 0;
      msg.valid = false;
    }
    osMessageQueuePut(sensorRawQueueHandle, &msg, 0, 0);

    WDT_TaskAlive(TASK_ID_LIGHT);
    osDelay(500);
  }
}

/* ========================================================= */
/*  生产者: PM2.5 — 纯采集，不滤波、不写池                     */
/* ========================================================= */

void StartPM25Task(void *argument) {
  SensorRawMsg_t msg = { .type = RAW_PM25 };

  for (;;) {
    osMutexAcquire(adcMutexHandle, osWaitForever);
    uint32_t sum = adc_sample_oversample(ADC_CHANNEL_12, 4);
    osMutexRelease(adcMutexHandle);

    if (sum > 0) {
      float voltage = ADC_ToVoltage(sum / 4);
      msg.value = ADC_ToPM25(voltage);
      msg.valid = true;
    } else {
      msg.value = 0;
      msg.valid = false;
    }
    osMessageQueuePut(sensorRawQueueHandle, &msg, 0, 0);

    WDT_TaskAlive(TASK_ID_PM25);
    osDelay(500);
  }
}

/* ========================================================= */
/*  解析任务: 接收原始数据 → 滤波 → 写池 → 发标志             */
/* ========================================================= */

static MovingAvgFilter_t g_temp_filter;
static MovingAvgFilter_t g_humid_filter;
static MovingAvgFilter_t g_press_filter;
static MovingAvgFilter_t g_light_filter;
static MovingAvgFilter_t g_pm25_filter;

void StartDistributorTask(void *argument) {
  SensorRawMsg_t msg;

  MovingAvg_Init(&g_temp_filter,  25.0f);
  MovingAvg_Init(&g_humid_filter, 50.0f);
  MovingAvg_Init(&g_press_filter, 1013.0f);
  MovingAvg_Init(&g_light_filter, ADC_ToLux(ADC_ToVoltage(2048)));
  MovingAvg_Init(&g_pm25_filter,  ADC_ToPM25(ADC_ToVoltage(2048)));

  for (;;) {
    osMessageQueueGet(sensorRawQueueHandle, &msg, NULL, osWaitForever);

    uint32_t now = osKernelGetTickCount();
    float filtered = msg.value;

    switch (msg.type) {
      case RAW_TEMP:
        filtered = MovingAvg_Update(&g_temp_filter, msg.value);
        g_sensor_pool[SENSOR_TYPE_TEMP]  = (SensorSlot_t){ filtered, now, msg.valid };
        osEventFlagsSet(lcdEventFlags,  FLAG_NEW_DATA);
        osEventFlagsSet(uartEventFlags, FLAG_NEW_DATA);
        osEventFlagsSet(fanEventFlags,  FLAG_NEW_DATA);
        break;
      case RAW_HUMID:
        filtered = MovingAvg_Update(&g_humid_filter, msg.value);
        g_sensor_pool[SENSOR_TYPE_HUMID] = (SensorSlot_t){ filtered, now, msg.valid };
        break;
      case RAW_PRESS:
        filtered = MovingAvg_Update(&g_press_filter, msg.value);
        g_sensor_pool[SENSOR_TYPE_PRESS] = (SensorSlot_t){ filtered, now, msg.valid };
        break;
      case RAW_LIGHT:
        filtered = MovingAvg_Update(&g_light_filter, msg.value);
        g_sensor_pool[SENSOR_TYPE_LIGHT] = (SensorSlot_t){ filtered, now, msg.valid };
        osEventFlagsSet(lcdEventFlags,  FLAG_NEW_DATA);
        osEventFlagsSet(uartEventFlags, FLAG_NEW_DATA);
        break;
      case RAW_PM25:
        filtered = MovingAvg_Update(&g_pm25_filter, msg.value);
        g_sensor_pool[SENSOR_TYPE_PM25] = (SensorSlot_t){ filtered, now, msg.valid };
        osEventFlagsSet(lcdEventFlags,  FLAG_NEW_DATA);
        osEventFlagsSet(uartEventFlags, FLAG_NEW_DATA);
        break;
      default:
        break;
    }

    WDT_TaskAlive(TASK_ID_DISTRIBUTOR);
  }
}

/* ========================================================= */
/*  消费者: LCD 显示                                          */
/* ========================================================= */

void StartLcdTask(void *argument) {
  LCD_Init();
  LCD_Clear(COLOR_BLACK);
  LCD_ShowString(10, 10, "Sensor Monitor", COLOR_WHITE, COLOR_BLACK, 16);
  LCD_DrawLine(10, 28, 310, 28, COLOR_GRAY);

  for (;;) {
    osEventFlagsWait(lcdEventFlags, FLAG_NEW_DATA, osFlagsWaitAny, osWaitForever);

    SensorSlot_t t = g_sensor_pool[SENSOR_TYPE_TEMP];
    SensorSlot_t h = g_sensor_pool[SENSOR_TYPE_HUMID];
    SensorSlot_t p = g_sensor_pool[SENSOR_TYPE_PRESS];
    SensorSlot_t l = g_sensor_pool[SENSOR_TYPE_LIGHT];
    SensorSlot_t m = g_sensor_pool[SENSOR_TYPE_PM25];

    LCD_ShowString(10, 40, "Temp:", COLOR_WHITE, COLOR_BLACK, 16);
    if (t.valid) { LCD_ShowFloat(80, 40, t.value, 2, COLOR_GREEN, COLOR_BLACK, 16); LCD_ShowString(180, 40, "C", COLOR_WHITE, COLOR_BLACK, 16); }
    else         { LCD_ShowString(80, 40, "---", COLOR_RED, COLOR_BLACK, 16); }

    LCD_ShowString(10, 60, "Humid:", COLOR_WHITE, COLOR_BLACK, 16);
    if (h.valid) { LCD_ShowFloat(80, 60, h.value, 1, COLOR_GREEN, COLOR_BLACK, 16); LCD_ShowString(140, 60, "%", COLOR_WHITE, COLOR_BLACK, 16); }
    else         { LCD_ShowString(80, 60, "---", COLOR_RED, COLOR_BLACK, 16); }

    LCD_ShowString(10, 80, "Press:", COLOR_WHITE, COLOR_BLACK, 16);
    if (p.valid) { LCD_ShowFloat(80, 80, p.value, 1, COLOR_GREEN, COLOR_BLACK, 16); LCD_ShowString(180, 80, "hPa", COLOR_WHITE, COLOR_BLACK, 16); }
    else         { LCD_ShowString(80, 80, "---", COLOR_RED, COLOR_BLACK, 16); }

    LCD_ShowString(10, 100, "Light:", COLOR_WHITE, COLOR_BLACK, 16);
    if (l.valid) { LCD_ShowNumber(80, 100, (uint16_t)l.value, COLOR_YELLOW, COLOR_BLACK, 16); LCD_ShowString(120, 100, "lx", COLOR_WHITE, COLOR_BLACK, 16); }
    else         { LCD_ShowString(80, 100, "---", COLOR_RED, COLOR_BLACK, 16); }

    LCD_ShowString(10, 120, "PM2.5:", COLOR_WHITE, COLOR_BLACK, 16);
    if (m.valid) { LCD_ShowFloat(80, 120, m.value, 1, COLOR_YELLOW, COLOR_BLACK, 16); LCD_ShowString(120, 120, "ug/m3", COLOR_WHITE, COLOR_BLACK, 16); }
    else         { LCD_ShowString(80, 120, "---", COLOR_RED, COLOR_BLACK, 16); }

    WDT_TaskAlive(TASK_ID_LCD);
  }
}

/* ========================================================= */
/*  消费者: UART / MAVLink                                    */
/* ========================================================= */

void StartUartTask(void *argument) {
  uint32_t hb_counter = 0;

  for (;;) {
    osEventFlagsWait(uartEventFlags, FLAG_NEW_DATA, osFlagsWaitAny, osWaitForever);

    SensorSlot_t t = g_sensor_pool[SENSOR_TYPE_TEMP];
    SensorSlot_t h = g_sensor_pool[SENSOR_TYPE_HUMID];
    SensorSlot_t p = g_sensor_pool[SENSOR_TYPE_PRESS];
    SensorSlot_t l = g_sensor_pool[SENSOR_TYPE_LIGHT];
    SensorSlot_t m = g_sensor_pool[SENSOR_TYPE_PM25];
    uint32_t ts = osKernelGetTickCount();

    MAVLink_SendSensorData(&huart2, t.value, h.value, p.value, l.value, m.value, ts, t.valid, l.valid, m.valid);
    MAVLink_SendSensorData(&huart1, t.value, h.value, p.value, l.value, m.value, ts, t.valid, l.valid, m.valid);

    hb_counter++;
    if ((hb_counter % 10) == 0) {
      MAVLink_SendHeartbeat(&huart2);
      MAVLink_SendHeartbeat(&huart1);
    }

    WDT_TaskAlive(TASK_ID_UART);
  }
}

/* ========================================================= */
/*  消费者: 风扇控制（只关心温度）                              */
/* ========================================================= */

typedef enum {
    FAN_IDLE = 0, FAN_LOW, FAN_MEDIUM, FAN_HIGH, FAN_MAX
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

void StartFanTask(void *argument) {
  g_state_entry_time = osKernelGetTickCount();

  for (;;) {
    osEventFlagsWait(fanEventFlags, FLAG_NEW_DATA, osFlagsWaitAny, osWaitForever);

    if (GetFanMode() != 0) { WDT_TaskAlive(TASK_ID_FAN); continue; }

    SensorSlot_t t = g_sensor_pool[SENSOR_TYPE_TEMP];
    if (!t.valid) { WDT_TaskAlive(TASK_ID_FAN); continue; }

    float temp = t.value;
    uint32_t now = osKernelGetTickCount();
    uint32_t elapsed = now - g_state_entry_time;
    FanState_t next_state = g_fan_state;

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
    Motor_SetSpeedA(fan_state_to_speed(g_fan_state));

    WDT_TaskAlive(TASK_ID_FAN);
  }
}

/* MAVLink 命令回调 — ISR 上下文，仅入队不做耗时操作 */
static uint8_t s_ota_chunk_buf[128];
static uint16_t s_ota_chunk_len;
static void on_mavlink_cmd(const mavlink_message_t *msg)
{
  Command_t cmd;
  memset(&cmd, 0, sizeof(cmd));

  switch (msg->msgid) {
    case MAVLINK_MSG_ID_CMD_MOTOR: {
      mavlink_cmd_motor_t motor_cmd;
      mavlink_msg_cmd_motor_decode(msg, &motor_cmd);
      cmd.type = CMD_SET_MOTOR;
      cmd.data.motor.speed = motor_cmd.speed;
      osMessageQueuePut(cmdQueueHandle, &cmd, 0, 0);
      break;
    }
    case MAVLINK_MSG_ID_CMD_FAN: {
      mavlink_cmd_fan_t fan_cmd;
      mavlink_msg_cmd_fan_decode(msg, &fan_cmd);
      cmd.type = CMD_SET_FAN_MODE;
      cmd.data.fan.mode = fan_cmd.mode;
      osMessageQueuePut(cmdQueueHandle, &cmd, 0, 0);
      break;
    }
    case MAVLINK_MSG_ID_CMD_GET_DATA:
      cmd.type = CMD_GET_DATA;
      osMessageQueuePut(cmdQueueHandle, &cmd, 0, 0);
      break;
    case MAVLINK_MSG_ID_CMD_GET_STATUS:
      cmd.type = CMD_GET_STATUS;
      osMessageQueuePut(cmdQueueHandle, &cmd, 0, 0);
      break;
    case MAVLINK_MSG_ID_CMD_GET_VERSION:
      cmd.type = CMD_GET_VERSION;
      osMessageQueuePut(cmdQueueHandle, &cmd, 0, 0);
      break;
    case MAVLINK_MSG_ID_OTA_BEGIN: {
      mavlink_ota_begin_t ota;
      mavlink_msg_ota_begin_decode(msg, &ota);
      cmd.type = CMD_OTA_BEGIN;
      cmd.data.ota_begin.total_size = ota.total_size;
      cmd.data.ota_begin.crc32 = ota.expected_crc;
      osMessageQueuePut(cmdQueueHandle, &cmd, 0, 0);
      break;
    }
    case MAVLINK_MSG_ID_OTA_DATA: {
      mavlink_ota_data_t ota;
      mavlink_msg_ota_data_decode(msg, &ota);
      memcpy(s_ota_chunk_buf, ota.data, OTA_DATA_CHUNK_SIZE);
      s_ota_chunk_len = msg->len > 4 ? msg->len - 4 : 0;
      cmd.type = CMD_OTA_DATA;
      cmd.data.ota_data.offset = ota.offset;
      osMessageQueuePut(cmdQueueHandle, &cmd, 0, 0);
      break;
    }
    case MAVLINK_MSG_ID_OTA_COMPLETE: {
      mavlink_ota_complete_t ota;
      mavlink_msg_ota_complete_decode(msg, &ota);
      cmd.type = CMD_OTA_COMPLETE;
      cmd.data.ota_complete.crc32 = ota.crc32;
      osMessageQueuePut(cmdQueueHandle, &cmd, 0, 0);
      break;
    }
    default:
      break;
  }
}

/* 命令处理任务 */
void StartCmdTask(void *argument) {
  Command_t cmd;
  SystemParams_t sys_params;
  DiagReport_t diag_report;

  Motor_Init();
  OTA_Init();

  MAVLink_Init();
  MAVLink_RegisterCmdCallback(on_mavlink_cmd);

  HC05_SoftInit();
  UART2_DMA_Init();

  if (STORAGE_Load(&sys_params)) {
    SetFanMode(sys_params.fan_mode);
  }

  for (;;) {
    if (osMessageQueueGet(cmdQueueHandle, &cmd, NULL, 1000) == osOK) {
      switch (cmd.type) {
        case CMD_SET_MOTOR:
          SetFanMode(1);
          Motor_SetSpeedA(cmd.data.motor.speed);
          Motor_SetSpeedB(cmd.data.motor.speed);
          sys_params.fan_speed = cmd.data.motor.speed;
          MAVLink_SendAck(&huart2, cmd.data.motor.speed);
          break;
        case CMD_SET_FAN_MODE:
          SetFanMode(cmd.data.fan.mode);
          sys_params.fan_mode = cmd.data.fan.mode;
          if (cmd.data.fan.mode == 1) Motor_SetSpeedA(0);
          STORAGE_Save(&sys_params);
          MAVLink_SendAck(&huart2, cmd.data.fan.mode);
          break;
        case CMD_GET_VERSION:
          MAVLink_SendVersionReport(&huart2, FW_STRING);
          MAVLink_SendVersionReport(&huart1, FW_STRING);
          break;
        case CMD_GET_STATUS: {
          uint8_t task_count = 9;
          uint8_t alive_mask = WDT_GetAliveMask();
          uint8_t reset_count = WDT_GetResetCount();
          MAVLink_SendStatusReport(&huart2,
            osKernelGetTickCount(), GetManualFanSpeed(),
            GetFanMode(), task_count, alive_mask, reset_count);
          MAVLink_SendStatusReport(&huart1,
            osKernelGetTickCount(), GetManualFanSpeed(),
            GetFanMode(), task_count, alive_mask, reset_count);
          break;
        }
        case CMD_GET_DATA:
          MAVLink_SendAck(&huart2, 1);
          break;
        case CMD_DIAGNOSTIC: {
          DIAG_RunAll(&diag_report);
          MAVLink_SendDiagReport(&huart2, (const uint8_t *)&diag_report);
          MAVLink_SendDiagReport(&huart1, (const uint8_t *)&diag_report);
          break;
        }
        case CMD_OTA_BEGIN: {
          bool ok = OTA_Begin(cmd.data.ota_begin.total_size);
          MAVLink_SendOTAACK(&huart2, ok ? OTA_ACK_OK : OTA_ACK_ERROR, OTA_GetWrittenSize());
          break;
        }
        case CMD_OTA_DATA: {
          bool ok = OTA_WriteChunk(cmd.data.ota_data.offset, s_ota_chunk_buf, s_ota_chunk_len);
          MAVLink_SendOTAACK(&huart2, ok ? OTA_ACK_OK : OTA_ACK_ERROR, OTA_GetWrittenSize());
          break;
        }
        case CMD_OTA_COMPLETE: {
          bool ok = OTA_Commit(cmd.data.ota_complete.crc32);
          MAVLink_SendOTAACK(&huart2, ok ? OTA_ACK_OK : OTA_ACK_ERROR, OTA_GetWrittenSize());
          if (ok) { osDelay(200); NVIC_SystemReset(); }
          break;
        }
        default:
          break;
      }
    }
    WDT_TaskAlive(TASK_ID_CMD);
  }
}

/* 系统监控任务 */
void StartMonitorTask(void *argument) {
  for (;;) {
    osDelay(2000);
    WDT_Feed();
    WDT_CheckAllTasks();
    WDT_TaskAlive(TASK_ID_MONITOR);
  }
}
