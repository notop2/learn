#include "ota.h"
#include "stm32f4xx_hal.h"
#include <string.h>

/* F4 扇区号: 以 0x08000000 为基址, 每 128KB 一个扇区 */
#define ADDR_TO_SECTOR(addr)   ((addr - 0x08000000) / F4_SECTOR_SIZE)
#define OTA_SECTOR_FIRST       ADDR_TO_SECTOR(OTA_START_ADDR)   /* = 4 */
#define OTA_SECTOR_COUNT       3                                 /* Sectors 4,5,6 */

#define OTA_MAGIC_ADDR         OTA_START_ADDR

static volatile int s_ota_status = OTA_STATUS_IDLE;
static volatile uint32_t s_written_size = 0;

/* CRC32 软件计算 (与 Linux 端对齐) */
static uint32_t crc32_sw(const uint8_t *buf, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    static const uint32_t table[16] = {
        0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC,
        0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
        0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
        0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C
    };
    for (uint32_t i = 0; i < len; i++) {
        crc ^= buf[i];
        crc = table[crc & 0x0F] ^ (crc >> 4);
        crc = table[crc & 0x0F] ^ (crc >> 4);
    }
    return ~crc;
}

void OTA_Init(void)
{
    s_ota_status = OTA_STATUS_IDLE;
    s_written_size = 0;
}

bool OTA_Begin(uint32_t total_size)
{
    if (total_size == 0 || total_size > OTA_SIZE) {
        return false;
    }

    s_written_size = 0;
    s_ota_status = OTA_STATUS_BUSY;

    HAL_FLASH_Unlock();

    /* F4: 按扇区擦除 */
    FLASH_EraseInitTypeDef erase = {
        .TypeErase = FLASH_TYPEERASE_SECTORS,
        .Sector = OTA_SECTOR_FIRST,
        .NbSectors = OTA_SECTOR_COUNT,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3
    };
    uint32_t sect_err = 0;
    HAL_StatusTypeDef hal_status = HAL_FLASHEx_Erase(&erase, &sect_err);

    HAL_FLASH_Lock();

    if (hal_status != HAL_OK || sect_err != 0xFFFFFFFF) {
        s_ota_status = OTA_STATUS_ERROR;
        return false;
    }

    s_ota_status = OTA_STATUS_BUSY;
    return true;
}

bool OTA_WriteChunk(uint32_t offset, const uint8_t *data, uint16_t len)
{
    if (s_ota_status != OTA_STATUS_BUSY) return false;
    if (offset + len > OTA_SIZE) return false;
    if (len == 0) return true;

    HAL_FLASH_Unlock();

    /* F4: 32 位写入 */
    for (uint16_t i = 0; i + 3 < len; i += 4) {
        uint32_t word;
        memcpy(&word, &data[i], 4);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                OTA_START_ADDR + offset + i, word) != HAL_OK) {
            HAL_FLASH_Lock();
            s_ota_status = OTA_STATUS_ERROR;
            return false;
        }
    }

    /* 处理末尾不足 4 字节的部分 (半字写入) */
    uint16_t remaining = len % 4;
    if (remaining) {
        uint16_t half = 0;
        memcpy(&half, &data[len - remaining], remaining);
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
            OTA_START_ADDR + offset + len - remaining, half);
    }

    HAL_FLASH_Lock();
    s_written_size = offset + len;
    return true;
}

bool OTA_Commit(uint32_t expected_crc)
{
    if (s_ota_status != OTA_STATUS_BUSY) return false;

    uint32_t actual_crc = crc32_sw((const uint8_t *)OTA_START_ADDR, s_written_size);
    if (actual_crc != expected_crc) {
        s_ota_status = OTA_STATUS_ERROR;
        return false;
    }

    /* F4 支持 32 位写入, 直接写魔数 */
    HAL_FLASH_Unlock();
    HAL_StatusTypeDef ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
        OTA_MAGIC_ADDR, 0xA5A5A5A5);
    HAL_FLASH_Lock();

    if (ret != HAL_OK) {
        s_ota_status = OTA_STATUS_ERROR;
        return false;
    }

    s_ota_status = OTA_STATUS_DONE;
    return true;
}

int OTA_GetStatus(void)
{
    return s_ota_status;
}

uint32_t OTA_GetWrittenSize(void)
{
    return s_written_size;
}
