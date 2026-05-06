#include "motor.h"

/* 静态变量 */
static Motor_t motor_a;
static Motor_t motor_b;

/* 初始化电机 */
void Motor_Init(void) {
    /* 初始化控制引脚 */
    HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STBY_GPIO_Port, STBY_Pin, GPIO_PIN_SET);
    
    /* 启动 PWM */
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
    
    /* 初始化电机状态 */
    motor_a.speed = 0;
    motor_a.dir = MOTOR_STOP;
    motor_a.enabled = true;
    
    motor_b.speed = 0;
    motor_b.dir = MOTOR_STOP;
    motor_b.enabled = true;
}

/* 设置电机 A 速度 (-100 ~ 100) */
void Motor_SetSpeedA(int32_t speed) {
    if (!motor_a.enabled) return;
    
    /* 限制速度范围 */
    if (speed > 100) speed = 100;
    if (speed < -100) speed = -100;
    
    motor_a.speed = speed;
    
    /* 设置方向 */
    if (speed > 0) {
        HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, GPIO_PIN_RESET);
        motor_a.dir = MOTOR_FORWARD;
    } else if (speed < 0) {
        HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, GPIO_PIN_SET);
        motor_a.dir = MOTOR_BACKWARD;
        speed = -speed;
    } else {
        HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, GPIO_PIN_RESET);
        motor_a.dir = MOTOR_STOP;
    }
    
    /* 设置 PWM */
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, speed * 10);
}

/* 设置电机 B 速度 (-100 ~ 100) */
void Motor_SetSpeedB(int32_t speed) {
    if (!motor_b.enabled) return;
    
    /* 限制速度范围 */
    if (speed > 100) speed = 100;
    if (speed < -100) speed = -100;
    
    motor_b.speed = speed;
    
    /* 设置方向 */
    if (speed > 0) {
        HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, GPIO_PIN_RESET);
        motor_b.dir = MOTOR_FORWARD;
    } else if (speed < 0) {
        HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, GPIO_PIN_SET);
        motor_b.dir = MOTOR_BACKWARD;
        speed = -speed;
    } else {
        HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, GPIO_PIN_RESET);
        motor_b.dir = MOTOR_STOP;
    }
    
    /* 设置 PWM */
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, speed * 10);
}

/* 停止电机 A */
void Motor_StopA(void) {
    Motor_SetSpeedA(0);
}

/* 停止电机 B */
void Motor_StopB(void) {
    Motor_SetSpeedB(0);
}

/* 设置待机模式 */
void Motor_Standby(bool standby) {
    HAL_GPIO_WritePin(STBY_GPIO_Port, STBY_Pin, standby ? GPIO_PIN_RESET : GPIO_PIN_SET);
}
