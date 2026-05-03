#ifndef __BME280_H
#define __BME280_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "i2c.h"
#include <stdint.h>

/* BME280 I2C 地址 */
#define BME280_I2C_ADDR    0x76 << 1

/* BME280 寄存器地址 */
#define BME280_REG_CALIB00     0x88
#define BME280_REG_ID         0xD0
#define BME280_REG_RESET      0xE0
#define BME280_REG_CALIB26    0xE1
#define BME280_REG_CTRL_HUM   0xF2
#define BME280_REG_STATUS     0xF3
#define BME280_REG_CTRL_MEAS   0xF4
#define BME280_REG_CONFIG      0xF5
#define BME280_REG_PRESS_MSB   0xF7
#define BME280_REG_PRESS_LSB   0xF8
#define BME280_REG_PRESS_XLSB  0xF9
#define BME280_REG_TEMP_MSB    0xFA
#define BME280_REG_TEMP_LSB    0xFB
#define BME280_REG_TEMP_XLSB   0xFC
#define BME280_REG_HUM_MSB     0xFD
#define BME280_REG_HUM_LSB     0xFE

/* BME280 校准数据结构体 */
typedef struct {
    /* 温度校准值 */
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    
    /* 气压校准值 */
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
    
    /* 湿度校准值 */
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;
    
    int32_t t_fine;
} BME280_CalibData_t;

/* BME280 设备结构体 */
typedef struct {
    float temperature;
    float humidity;
    float pressure;
    bool initialized;
} BME280_t;

/* 函数声明 */
HAL_StatusTypeDef BME280_Init(void);
HAL_StatusTypeDef BME280_ReadData(float *temp, float *hum, float *press);
HAL_StatusTypeDef BME280_ReadTemperature(float *temp);
HAL_StatusTypeDef BME280_ReadHumidity(float *hum);
HAL_StatusTypeDef BME280_ReadPressure(float *press);

#ifdef __cplusplus
}
#endif

#endif /* __BME280_H */
