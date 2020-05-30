/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
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
#include "stm32f2xx_hal.h"

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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define USBovr_Pin GPIO_PIN_6
#define USBovr_GPIO_Port GPIOE
#define DIpu_Pin GPIO_PIN_14
#define DIpu_GPIO_Port GPIOD
#define CLKpu_Pin GPIO_PIN_15
#define CLKpu_GPIO_Port GPIOD
#define ResWF_Pin GPIO_PIN_8
#define ResWF_GPIO_Port GPIOC
#define Vbus_Pin GPIO_PIN_9
#define Vbus_GPIO_Port GPIOA
#define USBpw_Pin GPIO_PIN_10
#define USBpw_GPIO_Port GPIOA
#define nOnpWF_Pin GPIO_PIN_0
#define nOnpWF_GPIO_Port GPIOD
#define nRSTind_Pin GPIO_PIN_8
#define nRSTind_GPIO_Port GPIOB
#define onLIGT_Pin GPIO_PIN_9
#define onLIGT_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
