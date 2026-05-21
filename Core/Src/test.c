#include <stdint.h>

/* 利用 DWT 数据观察点与跟踪单元的周期计数器实现精确延时 */
/* 依赖 STM32F103 内核中的 DWT_CYCCNT，精度为 1/72 µs */

#define DWT_CTRL       (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT     (*(volatile uint32_t *)0xE0001004)
#define DWT_CTRL_EN    (1UL << 0)
#define CPU_FREQ_HZ    72000000UL

static void dwt_init(void)
{
    DWT_CTRL |= DWT_CTRL_EN;          /* 使能 CYCCNT 计数器 */
}

static void delay_us(uint32_t us)
{
    uint32_t start = DWT_CYCCNT;
    uint32_t ticks = us * (CPU_FREQ_HZ / 1000000UL);  /* us 对应的时钟周期数 */
    while ((DWT_CYCCNT - start) < ticks);
}

static void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++) {
        delay_us(1000);
    }
}

#define buffmax 10
typedef struct test
{
    uint8_t head;
    uint8_t tail;
    uint8_t buff[buffmax];

    /* data */
} testbuff;

uint8_t buff_in(testbuff *a, uint8_t value)
{
    if((a->head+1)%buffmax==a->tail)
        return  0 ; //缓冲区满了，写入失败
    a->head=(a->head+1)%buffmax;
    a->buff[a->head]=value;
    return 1;
}

uint8_t buff_out(testbuff *a,uint8_t *value)
{
    if(a->head==a->tail)
        return 0; //缓冲区为空
    *value= a->buff[a->tail];
    a->tail=(a->tail+1)%buffmax;
    return 1;
}

/* ========== I2C 软件模拟 ========== */

#define I2C_SCL_L()     do { scl = 0; } while(0)
#define I2C_SCL_H()     do { scl = 1; } while(0)
#define I2C_SDA_L()     do { sda = 0; } while(0)
#define I2C_SDA_H()     do { sda = 1; } while(0)
#define I2C_SDA_READ()  sda

static int scl, sda;

static void i2c_delay(void)
{
    delay_us(5);
}

static void i2c_start(void)
{
    I2C_SDA_H();
    I2C_SCL_H();
    i2c_delay();

    I2C_SDA_L();
    i2c_delay();
    
    I2C_SCL_L();
    i2c_delay();
}

static void i2c_stop(void)
{
    I2C_SCL_L();                     /* 确保 SCL 低电平，避免误产生 START */
    I2C_SDA_L();                     /* SDA 拉低 */
    i2c_delay();
    I2C_SCL_H();                     /* 先释放 SCL */
    i2c_delay();
    I2C_SDA_H();                     /* SCL 高时 SDA 低→高 = STOP 条件 */
    i2c_delay();
}

static int i2c_write_byte(uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        if (data & 0x80)
            I2C_SDA_H();
        else
            I2C_SDA_L();
        data <<= 1;
        I2C_SCL_H();
        i2c_delay();
        I2C_SCL_L();
        i2c_delay();
    }

    I2C_SDA_H();
    I2C_SCL_H();
    i2c_delay();
    int ack = I2C_SDA_READ();
    I2C_SCL_L();
    i2c_delay();

    return ack ? -1 : 0;
}

static uint8_t i2c_read_byte(int send_ack)
{
    uint8_t data = 0;

    I2C_SDA_H();
    for (int i = 0; i < 8; i++) {
        data <<= 1;
        I2C_SCL_H();
        i2c_delay();
        if (I2C_SDA_READ())
            data |= 1;
        I2C_SCL_L();
        i2c_delay();
    }

    if (send_ack)
        I2C_SDA_L();
    else
        I2C_SDA_H();
    I2C_SCL_H();
    i2c_delay();
    I2C_SCL_L();
    i2c_delay();

    return data;
}

int i2c_write(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    i2c_start();
    if (i2c_write_byte(dev_addr << 1) < 0) {
        i2c_stop();
        return -1;
    }
    if (i2c_write_byte(reg) < 0) {
        i2c_stop();
        return -1;
    }
    for (uint16_t i = 0; i < len; i++) {
        if (i2c_write_byte(data[i]) < 0) {
            i2c_stop();
            return -1;
        }
    }
    i2c_stop();
    return 0;
}

int i2c_read(uint8_t dev_addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    i2c_start();
    if (i2c_write_byte(dev_addr << 1) < 0) {
        i2c_stop();
        return -1;
    }
    if (i2c_write_byte(reg) < 0) {
        i2c_stop();
        return -1;
    }

    i2c_start();
    if (i2c_write_byte((dev_addr << 1) | 1) < 0) {
        i2c_stop();
        return -1;
    }
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = i2c_read_byte(i < len - 1);
    }
    i2c_stop();
    return 0;
}

/* ========== SPI 软件模拟 (Mode 0: CPOL=0, CPHA=0) ========== */

#define SPI_CS_L()      do { cs  = 0; } while(0)
#define SPI_CS_H()      do { cs  = 1; } while(0)
#define SPI_SCLK_L()    do { sclk = 0; } while(0)
#define SPI_SCLK_H()    do { sclk = 1; } while(0)
#define SPI_MOSI_L()    do { mosi = 0; } while(0)
#define SPI_MOSI_H()    do { mosi = 1; } while(0)
#define SPI_MISO_READ() miso

static int cs, sclk, mosi, miso;

static void spi_delay(void)
{
    delay_us(1);
}

uint8_t spi_write_read(uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        if (data & 0x80)
            SPI_MOSI_H();
        else
            SPI_MOSI_L();
        data <<= 1;

        SPI_SCLK_H();
        spi_delay();

        data |= SPI_MISO_READ();

        SPI_SCLK_L();
        spi_delay();
    }

    return data;
}

void spi_write(uint8_t *data, uint16_t len)
{
    SPI_CS_L();
    for (uint16_t i = 0; i < len; i++) {
        spi_write_read(data[i]);
    }
    SPI_CS_H();
}

void spi_read(uint8_t *buf, uint16_t len)
{
    SPI_CS_L();
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = spi_write_read(0xFF);
    }
    SPI_CS_H();
}
