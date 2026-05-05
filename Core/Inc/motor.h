#ifndef __MOTOR_H
#define __MOTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "tim.h"
#include "gpio.h"
#include <stdint.h>
#include <stdbool.h>

/* 电机控制引脚定义 (BIN1/BIN2 使用 PB0/PB1 避免与 USART2 冲突) */
#define AIN1_Pin      GPIO_PIN_0
#define AIN1_GPIO_Port GPIOA
#define AIN2_Pin      GPIO_PIN_1
#define AIN2_GPIO_Port GPIOA
#define BIN1_Pin      GPIO_PIN_0
#define BIN1_GPIO_Port GPIOB
#define BIN2_Pin      GPIO_PIN_1
#define BIN2_GPIO_Port GPIOB
#define STBY_Pin      GPIO_PIN_5
#define STBY_GPIO_Port GPIOA

/* 电机方向 */
typedef enum {
    MOTOR_STOP = 0,
    MOTOR_FORWARD,
    MOTOR_BACKWARD
} MotorDir_t;

/* 电机结构体 */
typedef struct {
    int32_t speed;      /* 速度 -100 到 100 */
    MotorDir_t dir;     /* 方向 */
    bool enabled;       /* 使能状态 */
} Motor_t;

/* 函数声明 */
void Motor_Init(void);
void Motor_SetSpeedA(int32_t speed);
void Motor_SetSpeedB(int32_t speed);
void Motor_StopA(void);
void Motor_StopB(void);
void Motor_Standby(bool standby);

#ifdef __cplusplus
}
#endif

#endif /* __MOTOR_H */
