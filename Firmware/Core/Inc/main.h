/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define GOAL_SWITCH_Pin GPIO_PIN_13
#define GOAL_SWITCH_GPIO_Port GPIOC
#define COMMODULE_IN0_Pin GPIO_PIN_14
#define COMMODULE_IN0_GPIO_Port GPIOC
#define COMMODULE_IN1_Pin GPIO_PIN_15
#define COMMODULE_IN1_GPIO_Port GPIOC
#define BAT_SENS_Pin GPIO_PIN_0
#define BAT_SENS_GPIO_Port GPIOC
#define DRB_CS_Pin GPIO_PIN_1
#define DRB_CS_GPIO_Port GPIOC
#define BALL1_Pin GPIO_PIN_4
#define BALL1_GPIO_Port GPIOA
#define BALL2_Pin GPIO_PIN_5
#define BALL2_GPIO_Port GPIOA
#define TIM13_CH1_DRB1_Pin GPIO_PIN_6
#define TIM13_CH1_DRB1_GPIO_Port GPIOA
#define TIM14_CH1_DRB2_Pin GPIO_PIN_7
#define TIM14_CH1_DRB2_GPIO_Port GPIOA
#define LED2_Pin GPIO_PIN_0
#define LED2_GPIO_Port GPIOB
#define SW3_LED_Pin GPIO_PIN_2
#define SW3_LED_GPIO_Port GPIOB
#define LED1_Pin GPIO_PIN_12
#define LED1_GPIO_Port GPIOB
#define SPKR_Pin GPIO_PIN_9
#define SPKR_GPIO_Port GPIOC
#define SW2_LED_Pin GPIO_PIN_11
#define SW2_LED_GPIO_Port GPIOA
#define KICK2_Pin GPIO_PIN_12
#define KICK2_GPIO_Port GPIOA
#define KICK1_Pin GPIO_PIN_15
#define KICK1_GPIO_Port GPIOA
#define SW2_Pin GPIO_PIN_8
#define SW2_GPIO_Port GPIOB
#define SW3_Pin GPIO_PIN_9
#define SW3_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
