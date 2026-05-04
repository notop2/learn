#include "diagnostic.h"
#include "main.h"
#include "bme280.h"
#include "lcd.h"
#include "motor.h"
#include "adc.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static void diag_set(DiagReport_t *r, int idx, const char *name, DiagResult_t res, const char *fmt, ...)
{
    r->items[idx].name = name;
    r->items[idx].result = res;
    va_list args;
    va_start(args, fmt);
    vsnprintf(r->items[idx].detail, sizeof(r->items[idx].detail), fmt, args);
    va_end(args);
}

bool DIAG_TestI2C(void)
{
    HAL_StatusTypeDef st;
    uint8_t id = 0;
    st = HAL_I2C_Mem_Read(&hi2c1, 0x76 << 1, 0xD0, I2C_MEMADD_SIZE_8BIT, &id, 1, 100);
    return (st == HAL_OK && id == 0x60);
}

bool DIAG_TestADC(void)
{
    HAL_StatusTypeDef st;
    HAL_ADC_Start(&hadc1);
    st = HAL_ADC_PollForConversion(&hadc1, 50);
    HAL_ADC_Stop(&hadc1);
    return (st == HAL_OK);
}

bool DIAG_TestLCD(void)
{
    LCD_Init();
    LCD_Clear(COLOR_RED);
    HAL_Delay(100);
    LCD_Clear(COLOR_GREEN);
    HAL_Delay(100);
    LCD_Clear(COLOR_BLUE);
    HAL_Delay(100);
    LCD_Clear(COLOR_BLACK);
    return true;
}

bool DIAG_TestUART(void)
{
    const char test[] = "UART_TEST_OK\r\n";
    HAL_StatusTypeDef s1 = HAL_UART_Transmit(&huart1, (uint8_t *)test, strlen(test), 100);
    HAL_StatusTypeDef s2 = HAL_UART_Transmit(&huart2, (uint8_t *)test, strlen(test), 100);
    return (s1 == HAL_OK && s2 == HAL_OK);
}

bool DIAG_TestMotor(void)
{
    Motor_Init();
    Motor_SetSpeedA(50);
    HAL_Delay(200);
    Motor_StopA();
    Motor_SetSpeedB(50);
    HAL_Delay(200);
    Motor_StopB();
    return true;
}

bool DIAG_TestFSMC(void)
{
    volatile uint16_t *test_addr = (uint16_t *)0x60000000;
    *test_addr = 0xAA55;
    uint16_t val = *test_addr;
    return (val == 0xAA55);
}

bool DIAG_TestSensor(void)
{
    float t, h, p;
    HAL_StatusTypeDef st = BME280_ReadData(&t, &h, &p);
    return (st == HAL_OK);
}

bool DIAG_TestFlash(void)
{
    uint32_t *flash = (uint32_t *)0x0807F800;
    return (flash[0] == 0xFFFFFFFF || flash[0] == 0x454E5253);
}

void DIAG_RunAll(DiagReport_t *report)
{
    memset(report, 0, sizeof(DiagReport_t));
    report->timestamp = HAL_GetTick();

    uint32_t start = HAL_GetTick();

    diag_set(report, 0, "I2C_BME280",
        DIAG_TestI2C() ? DIAG_PASS : DIAG_FAIL,
        DIAG_TestI2C() ? "id=0x60" : "no device");

    diag_set(report, 1, "ADC",
        DIAG_TestADC() ? DIAG_PASS : DIAG_FAIL,
        DIAG_TestADC() ? "conversion ok" : "timeout");

    diag_set(report, 2, "LCD",
        DIAG_TestLCD() ? DIAG_PASS : DIAG_FAIL,
        DIAG_TestLCD() ? "color test done" : "fsmc error");

    diag_set(report, 3, "UART",
        DIAG_TestUART() ? DIAG_PASS : DIAG_FAIL,
        DIAG_TestUART() ? "tx ok" : "tx failed");

    diag_set(report, 4, "MOTOR",
        DIAG_TestMotor() ? DIAG_PASS : DIAG_FAIL,
        DIAG_TestMotor() ? "pwm ok" : "pwm error");

    diag_set(report, 5, "FSMC",
        DIAG_TestFSMC() ? DIAG_PASS : DIAG_FAIL,
        DIAG_TestFSMC() ? "rw ok" : "readback fail");

    diag_set(report, 6, "SENSOR",
        DIAG_TestSensor() ? DIAG_PASS : DIAG_FAIL,
        DIAG_TestSensor() ? "data ok" : "read fail");

    diag_set(report, 7, "FLASH",
        DIAG_TestFlash() ? DIAG_PASS : DIAG_FAIL,
        DIAG_TestFlash() ? "param area ok" : "unexpected");

    report->total_ms = HAL_GetTick() - start;
}

void DIAG_PrintReport(const DiagReport_t *report, char *buf, uint16_t size)
{
    uint16_t pos = 0;
    pos += snprintf(buf + pos, size - pos, "=== DIAG REPORT [%lu ms] ===\r\n",
        (unsigned long)report->total_ms);

    for (int i = 0; i < DIAG_TEST_COUNT; i++) {
        if (report->items[i].name == NULL) break;
        const char *status = "PASS";
        if (report->items[i].result == DIAG_FAIL) status = "FAIL";
        else if (report->items[i].result == DIAG_WARN) status = "WARN";
        else if (report->items[i].result == DIAG_SKIP) status = "SKIP";
        pos += snprintf(buf + pos, size - pos, "  [%s] %s: %s\r\n",
            status, report->items[i].name, report->items[i].detail);
    }
    pos += snprintf(buf + pos, size - pos, "=== END ===\r\n");
}
