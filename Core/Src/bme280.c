#include "bme280.h"
#include <string.h>

/* 静态变量 */
static BME280_CalibData_t calib_data;
static BME280_t bme280_device;

/* 本地函数声明 */
static HAL_StatusTypeDef BME280_ReadCalibData(void);
static HAL_StatusTypeDef BME280_WriteReg(uint8_t reg, uint8_t data);
static HAL_StatusTypeDef BME280_ReadReg(uint8_t reg, uint8_t *data, uint16_t len);

/* 初始化 BME280 */
HAL_StatusTypeDef BME280_Init(void) {
    HAL_StatusTypeDef status;
    uint8_t id;
    
    /* 读取芯片 ID */
    status = BME280_ReadReg(BME280_REG_ID, &id, 1);
    if (status != HAL_OK) return status;
    
    /* 读取校准数据 */
    status = BME280_ReadCalibData();
    if (status != HAL_OK) return status;
    
    /* 配置 BME280 */
    /* 湿度控制寄存器 */
    status = BME280_WriteReg(BME280_REG_CTRL_HUM, 0x01); /* 1x oversampling */
    if (status != HAL_OK) return status;
    
    /* 温度和气压控制寄存器 */
    status = BME280_WriteReg(BME280_REG_CTRL_MEAS, 0x27); /* temp x1, press x1, normal mode */
    if (status != HAL_OK) return status;
    
    /* 配置寄存器 */
    status = BME280_WriteReg(BME280_REG_CONFIG, 0xA0); /* 1000ms standby, filter x1 */
    if (status != HAL_OK) return status;
    
    bme280_device.initialized = true;
    
    return HAL_OK;
}

/* 读取所有数据 */
HAL_StatusTypeDef BME280_ReadData(float *temp, float *hum, float *press) {
    HAL_StatusTypeDef status;
    uint8_t data[8];
    int32_t adc_T, adc_P, adc_H;
    int32_t var1, var2, t, p, h;
    int64_t var1_64, var2_64, p_64;
    
    if (!bme280_device.initialized) {
        return HAL_ERROR;
    }
    
    /* 读取数据 */
    status = BME280_ReadReg(BME280_REG_PRESS_MSB, data, 8);
    if (status != HAL_OK) return status;
    
    /* 转换数据 */
    adc_P = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    adc_T = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);
    adc_H = (data[6] << 8) | data[7];
    
    /* 补偿温度 */
    var1 = ((((adc_T >> 3) - ((int32_t)calib_data.dig_T1 << 1)) * ((int32_t)calib_data.dig_T2) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib_data.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib_data.dig_T1))) >> 12);
    var2 = (var2 * ((int32_t)calib_data.dig_T3) >> 14;
    calib_data.t_fine = var1 + var2;
    t = (calib_data.t_fine * 5 + 128) >> 8;
    
    /* 补偿气压 */
    var1_64 = ((int64_t)calib_data.t_fine) - 128000;
    var2_64 = var1_64 * var1_64 * (int64_t)calib_data.dig_P6;
    var2_64 = var2_64 + ((var1_64 * (int64_t)calib_data.dig_P5) << 17);
    var2_64 = var2_64 + (((int64_t)calib_data.dig_P4) << 35);
    var1_64 = ((var1_64 * var1_64 * (int64_t)calib_data.dig_P3) >> 8) + ((var1_64 * (int64_t)calib_data.dig_P2) << 12;
    var1_64 = (((((int64_t)1) << 47) + var1_64)) * ((int64_t)calib_data.dig_P1) >> 33;
    if (var1_64 == 0) {
        p = 0;
    } else {
        p_64 = ((int64_t)1048576 - adc_P;
        p_64 = (((p_64 << 31) - var2_64) * 3125 / var1_64;
        var1_64 = (((int64_t)calib_data.dig_P9) * (p_64 >> 13) * (p_64 >> 13)) >> 25;
        var2_64 = (((int64_t)calib_data.dig_P8) * p_64) >> 19;
        p_64 = ((p_64 + var1_64 + var2_64) >> 8) + (((int64_t)calib_data.dig_P7) << 4;
        p = (int32_t)(p_64);
    }
    
    /* 补偿湿度 */
    h = (calib_data.t_fine - ((int32_t)76800));
    h = ((((adc_H << 14) - (((int32_t)calib_data.dig_H4) << 20) - (((int32_t)calib_data.dig_H5 * h) + ((int32_t)16384) >> 15;
    h = (((((h << 15) + 12288)) * ((int32_t)calib_data.dig_H2 + 32768) >> 18;
    h = h - ((((h >> 15) * (h >> 15)) >> 7) * ((int32_t)calib_data.dig_H6)) >> 4;
    h = (h < 0 ? 0 : h);
    h = h > 419430400 ? 419430400 : h;
    h = h >> 12;
    
    *temp = t / 100.0f;
    *hum = h / 1024.0f;
    *press = p / 25600.0f;
    
    return HAL_OK;
}

/* 读取温度 */
HAL_StatusTypeDef BME280_ReadTemperature(float *temp) {
    float hum, press;
    return BME280_ReadData(temp, &hum, &press);
}

/* 读取湿度 */
HAL_StatusTypeDef BME280_ReadHumidity(float *hum) {
    float temp, press;
    return BME280_ReadData(&temp, hum, &press);
}

/* 读取气压 */
HAL_StatusTypeDef BME280_ReadPressure(float *press) {
    float temp, hum;
    return BME280_ReadData(&temp, &hum, press);
}

/* 读取校准数据 */
static HAL_StatusTypeDef BME280_ReadCalibData(void) {
    HAL_StatusTypeDef status;
    uint8_t data[32];
    
    /* 读取第一部分校准数据 */
    status = BME280_ReadReg(BME280_REG_CALIB00, data, 26);
    if (status != HAL_OK) return status;
    
    /* 解析温度和气压校准数据 */
    calib_data.dig_T1 = (uint16_t)((data[1] << 8) | data[0]);
    calib_data.dig_T2 = (int16_t)((data[3] << 8) | data[2]);
    calib_data.dig_T3 = (int16_t)((data[5] << 8) | data[4]);
    calib_data.dig_P1 = (uint16_t)((data[7] << 8) | data[6]);
    calib_data.dig_P2 = (int16_t)((data[9] << 8) | data[8]);
    calib_data.dig_P3 = (int16_t)((data[11] << 8) | data[10]);
    calib_data.dig_P4 = (int16_t)((data[13] << 8) | data[12]);
    calib_data.dig_P5 = (int16_t)((data[15] << 8) | data[14]);
    calib_data.dig_P6 = (int16_t)((data[17] << 8) | data[16]);
    calib_data.dig_P7 = (int16_t)((data[19] << 8) | data[18]);
    calib_data.dig_P8 = (int16_t)((data[21] << 8) | data[20]);
    calib_data.dig_P9 = (int16_t)((data[23] << 8) | data[22]);
    calib_data.dig_H1 = data[25];
    
    /* 读取第二部分校准数据 */
    status = BME280_ReadReg(BME280_REG_CALIB26, data, 7);
    if (status != HAL_OK) return status;
    
    /* 解析湿度校准数据 */
    calib_data.dig_H2 = (int16_t)((data[1] << 8) | data[0]);
    calib_data.dig_H3 = data[2];
    calib_data.dig_H4 = (int16_t)((data[3] << 4) | (data[4] & 0x0F));
    calib_data.dig_H5 = (int16_t)((data[5] << 4) | ((data[4] >> 4));
    calib_data.dig_H6 = (int8_t)data[6];
    
    return HAL_OK;
}

/* 写寄存器 */
static HAL_StatusTypeDef BME280_WriteReg(uint8_t reg, uint8_t data) {
    return HAL_I2C_Mem_Write(&hi2c1, BME280_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, HAL_MAX_DELAY);
}

/* 读寄存器 */
static HAL_StatusTypeDef BME280_ReadReg(uint8_t reg, uint8_t *data, uint16_t len) {
    return HAL_I2C_Mem_Read(&hi2c1, BME280_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, len, HAL_MAX_DELAY);
}
