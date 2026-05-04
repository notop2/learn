#ifndef __DIAGNOSTIC_H
#define __DIAGNOSTIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    DIAG_PASS = 0,
    DIAG_WARN,
    DIAG_FAIL,
    DIAG_SKIP
} DiagResult_t;

typedef struct {
    const char *name;
    DiagResult_t result;
    char detail[64];
} DiagItem_t;

#define DIAG_TEST_COUNT  8

typedef struct {
    DiagItem_t items[DIAG_TEST_COUNT];
    uint32_t timestamp;
    uint32_t total_ms;
} DiagReport_t;

void DIAG_RunAll(DiagReport_t *report);
void DIAG_PrintReport(const DiagReport_t *report, char *buf, uint16_t size);

bool DIAG_TestI2C(void);
bool DIAG_TestADC(void);
bool DIAG_TestLCD(void);
bool DIAG_TestUART(void);
bool DIAG_TestMotor(void);
bool DIAG_TestFSMC(void);
bool DIAG_TestSensor(void);
bool DIAG_TestFlash(void);

#ifdef __cplusplus
}
#endif

#endif
