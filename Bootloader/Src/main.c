#include "bootloader.h"

/* F4 寄存器定义 */
#define FLASH_ACR   (*(volatile uint32_t *)(FLASH_BASE_F4 + FLASH_ACR_F4))
#define FLASH_KEYR  (*(volatile uint32_t *)(FLASH_BASE_F4 + FLASH_KEYR_F4))
#define FLASH_SR    (*(volatile uint32_t *)(FLASH_BASE_F4 + FLASH_SR_F4))
#define FLASH_CR    (*(volatile uint32_t *)(FLASH_BASE_F4 + FLASH_CR_F4))

/* RCC 寄存器 */
#define RCC_BASE    0x40023800
#define RCC_CR      (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_PLLCFGR (*(volatile uint32_t *)(RCC_BASE + 0x04))
#define RCC_CFGR    (*(volatile uint32_t *)(RCC_BASE + 0x08))

#define RCC_CR_HSEON     (1 << 16)
#define RCC_CR_HSERDY    (1 << 17)
#define RCC_CR_PLLON     (1 << 24)
#define RCC_CR_PLLRDY    (1 << 25)

/* PLL config: HSE 8MHz → /8(M) × 336(N) /2(P) = 168MHz */
#define PLL_M  8
#define PLL_N  336
#define PLL_P  0   /* 0→2, 1→4, 2→6, 3→8 → 选0即div2 */

/* 等待 Flash 操作完成 (F4: BSY 在 bit 16) */
static void flash_wait_busy(void)
{
    while (FLASH_SR & F4_FLASH_SR_BSY);
}

/* 解锁 */
static void flash_unlock(void)
{
    FLASH_KEYR = FLASH_KEY1;
    FLASH_KEYR = FLASH_KEY2;
}

/* 锁定 */
static void flash_lock(void)
{
    FLASH_CR |= F4_FLASH_CR_LOCK;
}

/* 擦除一个扇区 (F4 按扇区擦除, 128KB) */
static void flash_erase_sector(uint8_t sector)
{
    flash_wait_busy();
    FLASH_CR |= F4_FLASH_CR_SER;
    FLASH_CR &= ~(0x1F << F4_FLASH_CR_SNB_POS);
    FLASH_CR |= (sector & 0x1F) << F4_FLASH_CR_SNB_POS;
    FLASH_CR |= F4_FLASH_CR_STRT;
    flash_wait_busy();
    FLASH_CR &= ~F4_FLASH_CR_SER;
    FLASH_SR |= F4_FLASH_SR_EOP;
}

static uint8_t is_valid_reset_vector(uint32_t addr)
{
    uint32_t sp = *(volatile uint32_t *)addr;
    return (sp >= 0x20000000 && sp < 0x20030000);
}

/* 系统时钟: HSE → PLL → 168MHz */
void SystemClock_Init(void)
{
    uint32_t timeout;

    /* 打开 HSE */
    RCC_CR |= RCC_CR_HSEON;
    timeout = 0;
    while (!(RCC_CR & RCC_CR_HSERDY)) {
        timeout++;
        if (timeout > 0x100000) break;
    }

    /* Flash 等待周期: 168MHz → 5 WS */
    FLASH_ACR = 5;

    /* 配置 PLL: 8MHz / 8 × 336 / 2 = 168MHz */
    RCC_PLLCFGR = (PLL_M << 0) | (PLL_N << 6) | (PLL_P << 16) |
                  (1 << 22) |          /* PLLSRC = HSE */
                  0;                    /* PLLQ = default */

    /* 打开 PLL */
    RCC_CR |= RCC_CR_PLLON;
    timeout = 0;
    while (!(RCC_CR & RCC_CR_PLLRDY)) {
        timeout++;
        if (timeout > 0x100000) break;
    }

    /* APB1 = /4 → 42MHz, APB2 = /2 → 84MHz */
    RCC_CFGR |= (5 << 10);   /* APB1 = /4 */
    RCC_CFGR |= (4 << 13);   /* APB2 = /2 */

    /* 切换 SYSCLK 到 PLL */
    RCC_CFGR |= 0x02;
    while ((RCC_CFGR & 0x0C) != 0x08);
}

bool IsOTAFirmwareReady(void)
{
    uint32_t magic = *(volatile uint32_t *)(OTA_START_ADDR);
    if (magic != OTA_MAGIC) return false;
    if (!is_valid_reset_vector(OTA_START_ADDR + 4)) return false;
    return true;
}

void ClearOTAFirmwareFlag(void)
{
    flash_unlock();
    flash_erase_sector(ADDR_TO_SECTOR(OTA_START_ADDR));
    flash_lock();
}

bool CopyOTAToApp(void)
{
    const uint32_t *src = (const uint32_t *)OTA_START_ADDR;
    volatile uint32_t *dst = (volatile uint32_t *)APP_START_ADDR;
    uint32_t words = APP_SIZE / 4;

    flash_unlock();

    /* 擦除 App 区 (3 个扇区: Sector 1~3) */
    for (int sec = 1; sec <= 3; sec++) {
        flash_erase_sector(sec);
    }

    /* 写入模式 + 32 位 */
    FLASH_CR |= F4_FLASH_CR_PG | F4_FLASH_CR_PSIZE_32;

    for (uint32_t i = 0; i < words; i++) {
        flash_wait_busy();
        dst[i] = src[i];
        __DMB();
        if (dst[i] != src[i]) {
            FLASH_CR &= ~F4_FLASH_CR_PG;
            flash_lock();
            return false;
        }
    }

    flash_wait_busy();
    FLASH_CR &= ~F4_FLASH_CR_PG;
    flash_lock();
    return true;
}

void JumpToApp(uint32_t app_addr)
{
    uint32_t msp = *(volatile uint32_t *)app_addr;
    uint32_t reset_vector = *(volatile uint32_t *)(app_addr + 4);

    __asm volatile("cpsid i");

    /* 关 SysTick */
    *(volatile uint32_t *)0xE000E010 = 0;

    /* 清 NVIC 中断 */
    for (volatile int i = 0; i < 8; i++) {
        *(volatile uint32_t *)(0xE000E180 + i * 4) = 0xFFFFFFFF;
        *(volatile uint32_t *)(0xE000E280 + i * 4) = 0xFFFFFFFF;
    }

    __asm volatile("msr msp, %0" : : "r"(msp));
    *(volatile uint32_t *)0xE000ED08 = app_addr;

    __asm volatile("cpsie i");

    void (*app_entry)(void) = (void (*)(void))reset_vector;
    app_entry();
}

int main(void)
{
    SystemClock_Init();

    if (IsOTAFirmwareReady()) {
        if (CopyOTAToApp()) {
            ClearOTAFirmwareFlag();
        }
    }

    if (is_valid_reset_vector(APP_START_ADDR)) {
        JumpToApp(APP_START_ADDR);
    }

    while (1);
}
