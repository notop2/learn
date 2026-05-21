#include "storage.h"
#include "main.h"
#include <string.h>

#define FLASH_PARAM_ADDR    0x080E0000

/* 缓存上次写入的参数，数据未变时不重复擦写以保护 Flash 寿命 */
static SystemParams_t g_last_saved;
static bool g_cache_valid = false;

/* CRC16 (Modbus) 校验 */
uint16_t STORAGE_CalcCRC(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/* 使用默认值填充参数结构体 */
void STORAGE_SetDefaults(SystemParams_t *params)
{
    memset(params, 0, sizeof(SystemParams_t));
    params->magic = STORAGE_MAGIC;
    params->version = STORAGE_VERSION;
    params->fan_mode = 0;
    params->fan_speed = 0;
    params->temp_offset = 0.0f;
    params->humid_offset = 0.0f;
    params->light_threshold = 1000;
    params->save_count = 0;
    
    uint8_t *p = (uint8_t *)params;
    params->crc = 0;
    params->crc = STORAGE_CalcCRC(p, sizeof(SystemParams_t));
}

/* 从 Flash 加载参数，校验失败则回退到默认值 */
bool STORAGE_Load(SystemParams_t *params)
{
    uint32_t *src = (uint32_t *)FLASH_PARAM_ADDR;

    if (src[0] != STORAGE_MAGIC) {
        STORAGE_SetDefaults(params);
        return false;
    }

    memcpy(params, (void *)FLASH_PARAM_ADDR, sizeof(SystemParams_t));

    uint16_t saved_crc = params->crc;
    params->crc = 0;
    uint16_t calc_crc = STORAGE_CalcCRC((uint8_t *)params, sizeof(SystemParams_t));

    if (saved_crc != calc_crc) {
        STORAGE_SetDefaults(params);
        return false;
    }

    params->crc = saved_crc;

    g_last_saved = *params;
    g_cache_valid = true;

    return true;
}

/* 保存参数到 Flash，数据未变时跳过写入以减少磨损 */
bool STORAGE_Save(const SystemParams_t *params)
{
    if (g_cache_valid && memcmp(params, &g_last_saved, sizeof(SystemParams_t)) == 0) {
        return true;
    }

    uint32_t erase_addr = FLASH_PARAM_ADDR;
    uint32_t page_error = 0;

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase = {
        .TypeErase = FLASH_TYPEERASE_SECTORS,
        .Sector = 7,
        .NbSectors = 1,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3
    };

    if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK) {
        HAL_FLASH_Lock();
        return false;
    }

    SystemParams_t temp = *params;
    temp.save_count++;
    temp.crc = 0;
    temp.crc = STORAGE_CalcCRC((uint8_t *)&temp, sizeof(SystemParams_t));

    uint32_t *data = (uint32_t *)&temp;
    for (uint32_t i = 0; i < sizeof(SystemParams_t) / 4; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, erase_addr + i * 4, data[i]) != HAL_OK) {
            HAL_FLASH_Lock();
            return false;
        }
    }

    HAL_FLASH_Lock();

    g_last_saved = temp;
    g_cache_valid = true;

    return true;
}
