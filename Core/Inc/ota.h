#pragma once

#include <stdint.h>
#include <stdbool.h>

/* F407 Flash 布局 */
#define OTA_START_ADDR    0x08080000
#define OTA_SIZE          0x00060000   /* 384KB (Sectors 4~6) */
#define OTA_CHUNK_SIZE    128

/* F4 扇区大小 (128KB) */
#define F4_SECTOR_SIZE    0x20000

#define OTA_STATUS_IDLE       0
#define OTA_STATUS_BUSY       1
#define OTA_STATUS_DONE       2
#define OTA_STATUS_ERROR      3

/* 初始化 OTA 接收 */
void OTA_Init(void);

/* 开始 OTA 接收：擦除 OTA 区 (3 个扇区) */
bool OTA_Begin(uint32_t total_size);

/* 写入一包固件数据 */
bool OTA_WriteChunk(uint32_t offset, const uint8_t *data, uint16_t len);

/* 完成 OTA：校验 CRC，标记就绪 */
bool OTA_Commit(uint32_t expected_crc);

/* 获取当前状态 */
int OTA_GetStatus(void);

/* 获取已写入字节数 */
uint32_t OTA_GetWrittenSize(void);
