#ifndef __WATCHDOG_H
#define __WATCHDOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define TASK_WATCHDOG_TIMEOUT_MS  5000

typedef enum {
    TASK_ID_BME280 = 0,
    TASK_ID_LIGHT,
    TASK_ID_PM25,
    TASK_ID_DISTRIBUTOR,
    TASK_ID_LCD,
    TASK_ID_UART,
    TASK_ID_CMD,
    TASK_ID_FAN,
    TASK_ID_MONITOR,
    TASK_ID_COUNT
} TaskId_t;

typedef struct {
    TaskId_t id;
    const char *name;
    uint32_t last_alive_tick;
    uint32_t max_expected_ms;
    uint32_t deadline_misses;
    bool is_critical;
} TaskWatchdog_t;

void WDT_Init(void);
void WDT_Feed(void);
void WDT_RegisterTask(TaskId_t id, const char *name, uint32_t timeout_ms, bool critical);
void WDT_TaskAlive(TaskId_t id);
void WDT_CheckAllTasks(void);
uint32_t WDT_GetTaskMisses(TaskId_t id);
bool WDT_IsTaskHealthy(TaskId_t id);
void WDT_PrintStatus(char *buf, uint16_t size);
uint8_t WDT_GetAliveMask(void);
uint8_t WDT_GetResetCount(void);

#ifdef __cplusplus
}
#endif

#endif
