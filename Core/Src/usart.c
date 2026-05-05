/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "usart.h"
#include "cmsis_os.h"
#include "sensor_data.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* USER CODE BEGIN 0 */
#define UART2_RX_BUFFER_SIZE  128
/* USER CODE END 0 */

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}
/* USART2 init function */

void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */
  /* 调用 UART2 DMA 空闲中断接收初始化 */
  /* USER CODE END USART2_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspInit 0 */

  /* USER CODE END USART2_MspInit 0 */
    /* USART2 clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART2 DMA Init */
    /* USART2_RX Init */
    hdma_usart2_rx.Instance = DMA1_Channel6;
    hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_rx.Init.Mode = DMA_NORMAL;
    hdma_usart2_rx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart2_rx);

    /* USART2_TX Init */
    hdma_usart2_tx.Instance = DMA1_Channel7;
    hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_tx.Init.Mode = DMA_NORMAL;
    hdma_usart2_tx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart2_tx);

    /* USART2 interrupt Init */
    HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspInit 1 */

  /* USER CODE END USART2_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspDeInit 0 */

  /* USER CODE END USART2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART2_CLK_DISABLE();

    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);

    /* USART2 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspDeInit 1 */

  /* USER CODE END USART2_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* 外部变量声明 */
extern osMessageQueueId_t cmdQueueHandle;
extern UART_HandleTypeDef huart2;
extern osThreadId_t fanTaskHandle;

/* 全局变量 */
static uint8_t rx_buffer[UART2_RX_BUFFER_SIZE];
static uint16_t rx_len = 0;
static uint8_t g_fan_mode = 0;  /* 0=自动, 1=手动 */
static int32_t g_manual_fan_speed = 0;

/* 获取风扇模式 */
uint8_t GetFanMode(void) {
  return g_fan_mode;
}

/* 获取手动风扇速度 */
int32_t GetManualFanSpeed(void) {
  return g_manual_fan_speed;
}

/* 设置风扇模式 */
void SetFanMode(uint8_t mode) {
  g_fan_mode = mode;
}

/* 设置手动风扇速度 */
void SetManualFanSpeed(int32_t speed) {
  g_manual_fan_speed = speed;
}

/* 发送响应字符串 */
static void SendResponse(const char *response) {
  HAL_UART_Transmit(&huart2, (uint8_t *)response, strlen(response), 100);
}

/* 解析并处理命令 */
static void ParseCommand(uint8_t *data, uint16_t len) {
  /* 确保字符串以 null 结尾 */
  if (len >= UART2_RX_BUFFER_SIZE) {
    len = UART2_RX_BUFFER_SIZE - 1;
  }
  data[len] = '\0';
  
  Command_t cmd;
  char response[256];
  
  /* 解析 GET /sensor */
  if (strncmp((char *)data, "GET /sensor", 11) == 0) {
    cmd.type = CMD_GET_DATA;
    osMessageQueuePut(cmdQueueHandle, &cmd, 0, 0);
    SendResponse("OK: GET /sensor queued\r\n");
    return;
  }
  
  /* 解析 GET /status */
  if (strncmp((char *)data, "GET /status", 11) == 0) {
    cmd.type = CMD_GET_STATUS;
    osMessageQueuePut(cmdQueueHandle, &cmd, 0, 0);
    SendResponse("OK: GET /status queued\r\n");
    return;
  }
  
  /* 解析 GET /version */
  if (strncmp((char *)data, "GET /version", 12) == 0) {
    cmd.type = CMD_GET_VERSION;
    osMessageQueuePut(cmdQueueHandle, &cmd, 0, 0);
    SendResponse("OK: GET /version queued\r\n");
    return;
  }
  
  /* 解析 GET /diag */
  if (strncmp((char *)data, "GET /diag", 9) == 0) {
    cmd.type = CMD_DIAGNOSTIC;
    osMessageQueuePut(cmdQueueHandle, &cmd, 0, 0);
    SendResponse("OK: GET /diag queued\r\n");
    return;
  }
  
  /* 解析 SET /motor?speed=X */
  if (strncmp((char *)data, "SET /motor", 10) == 0) {
    char *speed_str = strstr((char *)data, "speed=");
    if (speed_str) {
      speed_str += 6;
      int32_t speed = atoi(speed_str);
      
      cmd.type = CMD_SET_MOTOR;
      cmd.data.motor.speed = speed;
      osMessageQueuePut(cmdQueueHandle, &cmd, 0, 0);
      
      sprintf(response, "OK: motor speed set to %ld\r\n", speed);
      SendResponse(response);
      return;
    }
  }
  
  /* 解析 SET /fan?mode=auto 或 manual */
  if (strncmp((char *)data, "SET /fan", 8) == 0) {
    char *mode_str = strstr((char *)data, "mode=");
    if (mode_str) {
      mode_str += 5;
      
      if (strncmp(mode_str, "auto", 4) == 0) {
        cmd.type = CMD_SET_FAN_MODE;
        cmd.data.fan.mode = 0;  /* 自动模式 */
        osMessageQueuePut(cmdQueueHandle, &cmd, 0, 0);
        SendResponse("OK: fan mode set to auto\r\n");
        return;
      } else if (strncmp(mode_str, "manual", 6) == 0) {
        cmd.type = CMD_SET_FAN_MODE;
        cmd.data.fan.mode = 1;  /* 手动模式 */
        osMessageQueuePut(cmdQueueHandle, &cmd, 0, 0);
        SendResponse("OK: fan mode set to manual\r\n");
        return;
      }
    }
  }
  
  /* 未知命令 */
  SendResponse("ERROR: unknown command\r\n");
}

/* HAL 库回调函数：UART 事件回调 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
  if (huart == &huart2) {
    /* 处理接收到的数据 */
    rx_len = Size;
    
    /* 解析命令 */
    ParseCommand(rx_buffer, rx_len);
    
    /* 重新开启接收 */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_buffer, UART2_RX_BUFFER_SIZE);
  }
}

/* 初始化 UART2 DMA 接收 */
void UART2_DMA_Init(void) {
  /* 使用 HAL 库的空闲中断接收函数 */
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_buffer, UART2_RX_BUFFER_SIZE);
}

/* USER CODE END 1 */
