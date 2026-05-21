#ifndef __SENSOR_DATA_H
#define __SENSOR_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "filter.h"

/* 传感器数据结构体 */
typedef struct {
    float temperature;    /* 温度 (°C) */
    float humidity;       /* 湿度 (%RH) */
    float pressure;       /* 气压 (hPa) */
    float light;         /* 光照强度 (lx) */
    float pm25;          /* PM2.5 浓度 (μg/m³) */
    bool bme280_valid;   /* BME280 数据有效标志 */
    bool light_valid;    /* 光敏数据有效标志 */
    bool pm25_valid;     /* PM2.5 数据有效标志 */
    uint32_t timestamp;  /* 时间戳 */
    float temp_filtered; /* 滤波后的温度 */
    float humid_filtered;/* 滤波后的湿度 */
    float press_filtered;/* 滤波后的气压 */
    float light_filtered;/* 滤波后的光照 */
    float pm25_filtered; /* 滤波后的PM2.5 */
} SensorData_t;

/* 命令类型 */
typedef enum {
    CMD_NONE = 0,
    CMD_GET_DATA,         /* 获取传感器数据 */
    CMD_SET_MOTOR,        /* 设置电机速度 */
    CMD_GET_STATUS,       /* 获取系统状态 */
    CMD_SET_FAN_MODE,     /* 设置风扇模式 (自动/手动) */
    CMD_GET_VERSION,      /* 获取固件版本 */
    CMD_DIAGNOSTIC,       /* 系统自检诊断 */
    CMD_OTA_BEGIN,        /* 开始 OTA 升级 */
    CMD_OTA_DATA,         /* OTA 固件数据分片 */
    CMD_OTA_COMPLETE,     /* OTA 完成校验 */
    CMD_MAX
} CommandType_t;

/* 系统状态结构体 */
typedef struct {
    float cpu_temp;        /* CPU 温度 (模拟) */
    uint32_t uptime;       /* 运行时间 */
    uint8_t fan_mode;     /* 风扇模式: 0=自动, 1=手动 */
    int32_t fan_speed;     /* 当前风扇速度 */
    uint8_t version[16];  /* 版本号 */
} SystemStatus_t;

/* 命令结构体 */
typedef struct {
    CommandType_t type;
    union {
        struct {
            int32_t speed;  /* 电机速度 -100 到 100 */
        } motor;
        struct {
            uint8_t mode;   /* 风扇模式 */
        } fan;
        struct {
            uint8_t index; /* 数据索引 */
        } query;
        struct {
            uint32_t total_size;
            uint32_t crc32;
        } ota_begin;
        struct {
            uint32_t offset;
        } ota_data;
        struct {
            uint32_t crc32;
        } ota_complete;
        SystemStatus_t status;
        uint8_t raw[32];  /* 原始数据 */
    } data;
} Command_t;

#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_DATA_H */
