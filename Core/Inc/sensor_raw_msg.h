#pragma once

#include <stdint.h>
#include <stdbool.h>

/* 传感器原始数据类型枚举 */
typedef enum {
    RAW_TEMP = 0,
    RAW_HUMID,
    RAW_PRESS,
    RAW_LIGHT,
    RAW_PM25,
    RAW_TYPE_COUNT
} SensorRawType_t;

/* 生产者→解析任务的原始数据消息 */
typedef struct {
    SensorRawType_t type;
    float value;
    bool valid;
} SensorRawMsg_t;
