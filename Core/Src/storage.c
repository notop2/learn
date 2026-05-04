#include "storage.h"
#include "main.h"
#include <string.h>

#define FLASH_PARAM_ADDR    0x0807F800

static bool g_loaded = false;
static SystemParams_t g_params;

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
    params->crc = STORAGE_CalcCRC((uint8_t *)params, sizeof(SystemParams_t) - 2);
}

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
    g_loaded = true;
    return true;
}

bool STORAGE_Save(const SystemParams_t *params)
{
    uint32_t erase_addr = FLASH_PARAM_ADDR;
    uint32_t page_error = 0;

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase = {
        .TypeErase = FLASH_TYPEERASE_PAGES,
        .PageAddress = erase_addr,
        .NbPages = 1
    };

    if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK) {
        HAL_FLASH_Lock();
        return false;
    }

    SystemParams_t temp = *params;
    temp.save_count++;
    temp.crc = STORAGE_CalcCRC((uint8_t *)&temp, sizeof(SystemParams_t) - 2);

    uint32_t *data = (uint32_t *)&temp;
    for (uint32_t i = 0; i < sizeof(SystemParams_t) / 4; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, erase_addr + i * 4, data[i]) != HAL_OK) {
            HAL_FLASH_Lock();
            return false;
        }
    }

    HAL_FLASH_Lock();
    g_loaded = true;
    return true;
}
