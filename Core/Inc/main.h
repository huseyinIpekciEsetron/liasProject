/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

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
#define ETH_SCLK_Pin GPIO_PIN_2
#define ETH_SCLK_GPIO_Port GPIOE
#define ETH_INTn_Pin GPIO_PIN_3
#define ETH_INTn_GPIO_Port GPIOE
#define ETH_SCSn_Pin GPIO_PIN_4
#define ETH_SCSn_GPIO_Port GPIOE
#define ETH_MISO_Pin GPIO_PIN_5
#define ETH_MISO_GPIO_Port GPIOE
#define ETH_MOSI_Pin GPIO_PIN_6
#define ETH_MOSI_GPIO_Port GPIOE
#define ETH_PMODE0_Pin GPIO_PIN_8
#define ETH_PMODE0_GPIO_Port GPIOI
#define ETH_PMODE1_Pin GPIO_PIN_13
#define ETH_PMODE1_GPIO_Port GPIOC
#define ETH_PMODE2_Pin GPIO_PIN_9
#define ETH_PMODE2_GPIO_Port GPIOI
#define LCD_PWM_Pin GPIO_PIN_12
#define LCD_PWM_GPIO_Port GPIOD
#define LCD_ONOFF_Pin GPIO_PIN_1
#define LCD_ONOFF_GPIO_Port GPIOK
#define RS422_DE_Pin GPIO_PIN_9
#define RS422_DE_GPIO_Port GPIOC
#define RS422_RE_Pin GPIO_PIN_8
#define RS422_RE_GPIO_Port GPIOA
#define RS422_UART1_TX_Pin GPIO_PIN_9
#define RS422_UART1_TX_GPIO_Port GPIOA
#define RS422_UART1_RX_Pin GPIO_PIN_10
#define RS422_UART1_RX_GPIO_Port GPIOA
#define BUZZER_EN_Pin GPIO_PIN_1
#define BUZZER_EN_GPIO_Port GPIOI
#define BUZZER_MUTE_Pin GPIO_PIN_2
#define BUZZER_MUTE_GPIO_Port GPIOI
#define UART2_TX_Pin GPIO_PIN_5
#define UART2_TX_GPIO_Port GPIOD
#define UART2_RX_Pin GPIO_PIN_6
#define UART2_RX_GPIO_Port GPIOD
#define HC165_EN_Pin GPIO_PIN_11
#define HC165_EN_GPIO_Port GPIOG
#define HC165_MISO_Pin GPIO_PIN_12
#define HC165_MISO_GPIO_Port GPIOG
#define HC165_CLK_Pin GPIO_PIN_13
#define HC165_CLK_GPIO_Port GPIOG
#define HC165_MOSI_Pin GPIO_PIN_14
#define HC165_MOSI_GPIO_Port GPIOG
#define LED_DRV_EN_Pin GPIO_PIN_5
#define LED_DRV_EN_GPIO_Port GPIOB
#define LED_DRV_SCL_Pin GPIO_PIN_6
#define LED_DRV_SCL_GPIO_Port GPIOB
#define LED_DRV_SDA_Pin GPIO_PIN_7
#define LED_DRV_SDA_GPIO_Port GPIOB
#define ETH_RSTn_Pin GPIO_PIN_7
#define ETH_RSTn_GPIO_Port GPIOI

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
