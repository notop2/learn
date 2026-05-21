#ifndef __DMA_H__
#define __DMA_H__

#include "main.h"

extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;

void MX_DMA_Init(void);

#endif
