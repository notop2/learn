#include "watchdog.h"
#include "main.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>

#ifndef IWDG_KEY_RELOAD
#define IWDG_KEY_RELOAD   0xAAAA
#endif
#ifndef IWDG_KEY_ENABLE
#define IWDG_KEY_ENABLE   0xCCCC
#endif
#define IWDG_KEY_UNLOCK   0x5555

static TaskWatchdog_t g_task_wd[TASK_ID_COUNT];
static bool g_initialized = false;

void WDT_Init(void)
{
    IWDG->KR = IWDG_KEY_UNLOCK;
    IWDG->PR = 5;
    IWDG->RLR = 1249;
    IWDG->KR = IWDG_KEY_ENABLE;
    IWDG->KR = IWDG_KEY_RELOAD;

    memset(g_task_wd, 0, sizeof(g_task_wd));
    g_initialized = true;
}

void WDT_Feed(void)
{
    IWDG->KR = IWDG_KEY_RELOAD;
}

void WDT_RegisterTask(TaskId_t id, const char *name, uint32_t timeout_ms, bool critical)
{
    if (id >= TASK_ID_COUNT) return;
    g_task_wd[id].id = id;
    g_task_wd[id].name = name;
    g_task_wd[id].max_expected_ms = timeout_ms;
    g_task_wd[id].is_critical = critical;
    g_task_wd[id].last_alive_tick = osKernelGetTickCount();
    g_task_wd[id].deadline_misses = 0;
}

void WDT_TaskAlive(TaskId_t id)
{
    if (id >= TASK_ID_COUNT) return;
    g_task_wd[id].last_alive_tick = osKernelGetTickCount();
}

void WDT_CheckAllTasks(void)
{
    if (!g_initialized) return;

    uint32_t now = osKernelGetTickCount();

    for (int i = 0; i < TASK_ID_COUNT; i++) {
        if (g_task_wd[i].name == NULL) continue;

        uint32_t elapsed = now - g_task_wd[i].last_alive_tick;

        if (elapsed > g_task_wd[i].max_expected_ms) {
            g_task_wd[i].deadline_misses++;

            if (g_task_wd[i].deadline_misses >= 3 && g_task_wd[i].is_critical) {
                while (1) { __disable_irq(); }
            }
        } else {
            g_task_wd[i].deadline_misses = 0;
        }
    }
}

uint32_t WDT_GetTaskMisses(TaskId_t id)
{
    if (id >= TASK_ID_COUNT) return 0;
    return g_task_wd[id].deadline_misses;
}

bool WDT_IsTaskHealthy(TaskId_t id)
{
    if (id >= TASK_ID_COUNT) return false;
    return (osKernelGetTickCount() - g_task_wd[id].last_alive_tick) < g_task_wd[id].max_expected_ms;
}

void WDT_PrintStatus(char *buf, uint16_t size)
{
    uint16_t pos = 0;
    uint32_t now = osKernelGetTickCount();

    for (int i = 0; i < TASK_ID_COUNT; i++) {
        if (g_task_wd[i].name == NULL) continue;
        uint32_t elapsed = now - g_task_wd[i].last_alive_tick;
        pos += snprintf(buf + pos, size - pos, "[%s]%s alive=%lu miss=%lu\r\n",
            (elapsed < g_task_wd[i].max_expected_ms) ? "OK" : "WARN",
            g_task_wd[i].name,
            (unsigned long)elapsed,
            (unsigned long)g_task_wd[i].deadline_misses);
    }
}

uint8_t WDT_GetAliveMask(void)
{
    uint8_t mask = 0;
    for (int i = 0; i < TASK_ID_COUNT && i < 8; i++) {
        if (g_task_wd[i].name != NULL && WDT_IsTaskHealthy((TaskId_t)i)) {
            mask |= (1 << i);
        }
    }
    return mask;
}

uint8_t WDT_GetResetCount(void)
{
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
        __HAL_RCC_CLEAR_RESET_FLAGS();
        return 1;
    }
    __HAL_RCC_CLEAR_RESET_FLAGS();
    return 0;
}
