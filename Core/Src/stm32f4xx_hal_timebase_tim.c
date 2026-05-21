#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
  return HAL_OK;
}

uint32_t HAL_GetTick(void)
{
  if (osKernelGetState() >= osKernelRunning)
  {
    return osKernelGetTickCount();
  }
  return 0;
}

void HAL_Delay(uint32_t Delay)
{
  if (osKernelGetState() >= osKernelRunning)
  {
    uint32_t start = osKernelGetTickCount();
    while ((osKernelGetTickCount() - start) < Delay);
  }
  else
  {
    for (uint32_t i = 0; i < Delay * 10000; i++);
  }
}

void HAL_SuspendTick(void) {}
void HAL_ResumeTick(void) {}
