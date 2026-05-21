#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include <stdint.h>
#include <stdbool.h>

/* F407 Flash 布局 */
#define BOOTLOADER_SIZE        0x20000   /* 128KB (Sector 0) */
#define APP_START_ADDR         0x08020000
#define APP_SIZE               0x60000   /* 384KB (Sectors 1~3) */
#define OTA_START_ADDR         0x08080000
#define OTA_SIZE               0x60000   /* 384KB (Sectors 4~6) */
#define STORAGE_SECTOR_ADDR    0x080E0000

/* STM32F4 Flash 寄存器 (F4 的 FLASH 基地址与 F1 不同) */
#define FLASH_BASE_F4          0x40023C00
#define FLASH_ACR_F4           0x00
#define FLASH_KEYR_F4          0x04
#define FLASH_SR_F4            0x0C
#define FLASH_CR_F4            0x10

#define F4_FLASH_CR_PG         (1 << 0)
#define F4_FLASH_CR_SER        (1 << 1)
#define F4_FLASH_CR_SNB_POS    3
#define F4_FLASH_CR_PSIZE_32   (2 << 8)
#define F4_FLASH_CR_STRT       (1 << 16)
#define F4_FLASH_CR_LOCK       (1 << 31)
#define F4_FLASH_SR_BSY        (1 << 16)
#define F4_FLASH_SR_EOP        (1 << 0)

#define FLASH_KEY1             0x45670123
#define FLASH_KEY2             0xCDEF89AB

/* 将 Flash 地址转换为 F4 扇区号 (0~7) */
#define ADDR_TO_SECTOR(addr)   ((addr - 0x08000000) / 0x20000)

#define OTA_MAGIC              0xA5A5A5A5

void SystemClock_Init(void);
bool IsOTAFirmwareReady(void);
bool CopyOTAToApp(void);
void ClearOTAFirmwareFlag(void);
void JumpToApp(uint32_t app_addr);

#endif
