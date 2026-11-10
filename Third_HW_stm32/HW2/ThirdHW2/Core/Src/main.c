/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "OLED.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ENC_DIV       4U
#define DUTY_MAX      100U

#define MOTOR_PORT    GPIOB
#define AIN1_PIN      GPIO_PIN_0
#define AIN2_PIN      GPIO_PIN_1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static int32_t  enc_raw = 0;
static int16_t  enc_last = 0;
static uint8_t  duty = 0;
static int32_t  last_shown_raw = -1;
static uint32_t last_ui = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Board_Led_Init(void);
static void Motor_Init(void);
static void Motor_SetDuty(uint8_t d);
static int16_t Encoder_ReadDelta(void);
static void Display_Update(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  MX_GPIO_Init();
  Board_Led_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();

  /* USER CODE BEGIN 2 */
  /* OLED first: it must be alive even if a timer later misbehaves. */
  OLED_Init();
  OLED_Clear();

  OLED_ShowString(1, 1, "MOTOR OPEN LOOP");
  OLED_ShowString(2, 1, "ENC:");
  OLED_ShowNum(2, 6, 0, 3);
  OLED_ShowString(3, 1, "DUTY:");
  OLED_ShowNum(3, 7, 0, 3);
  OLED_ShowString(3, 10, "%");
  OLED_ShowString(4, 1, "RAW:");
  OLED_ShowNum(4, 6, 0, 4);

  /* Motors and encoder after the display is initialized. */
  Motor_Init();

  (void)HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
  (void)HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);

  __HAL_TIM_SET_COUNTER(&htim3, 0);
  enc_last = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);

  last_shown_raw = 0;
  last_ui = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    int16_t delta;

    delta = Encoder_ReadDelta();
    if (delta != 0)
    {
      enc_raw += delta;

      if (enc_raw < 0)
      {
        enc_raw = 0;
      }
      if (enc_raw > (int32_t)(DUTY_MAX * ENC_DIV))
      {
        enc_raw = (int32_t)(DUTY_MAX * ENC_DIV);
      }

      duty = (uint8_t)(enc_raw / (int32_t)ENC_DIV);
      Motor_SetDuty(duty);
    }

    if ((enc_raw != last_shown_raw) &&
        ((HAL_GetTick() - last_ui) >= 50U))
    {
      last_ui = HAL_GetTick();
      last_shown_raw = enc_raw;
      Display_Update();
    }

    HAL_Delay(1);
    /* USER CODE END 3 */
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
static void Board_Led_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* Blue Pill PC13 LED: low level = on. */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
}

static void Motor_Init(void)
{
  /* Forward direction: AIN1 = 1, AIN2 = 0. PWM starts at 0%. */
  HAL_GPIO_WritePin(MOTOR_PORT, AIN1_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(MOTOR_PORT, AIN2_PIN, GPIO_PIN_RESET);
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
}

static void Motor_SetDuty(uint8_t d)
{
  if (d > DUTY_MAX)
  {
    d = (uint8_t)DUTY_MAX;
  }

  /* TIM2 ARR = 99, so CCR = 0..100 gives 0..100% duty. */
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, d);
}

static int16_t Encoder_ReadDelta(void)
{
  int16_t now;
  int16_t delta;

  now = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
  delta = (int16_t)(now - enc_last);
  enc_last = now;

  return delta;
}

static void Display_Update(void)
{
  OLED_ShowNum(2, 6, (uint32_t)(enc_raw / (int32_t)ENC_DIV), 3);
  OLED_ShowNum(3, 7, duty, 3);
  OLED_ShowNum(4, 6, (uint32_t)enc_raw, 4);
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  (void)file;
  (void)line;
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
