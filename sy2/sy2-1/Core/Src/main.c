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
#include "cmsis_os.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern osMessageQueueId_t Message_QueHandle;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* 消息结构：LED编号 + 频率(Hz) */
typedef struct {
    uint8_t led_num;    /* LED编号: 1, 2, 3 */
    float freq_hz;      /* 频率(Hz) */
} LED_Msg;

/* 串口接收相关 */
uint8_t rx_byte;           /* 单字节接收缓冲区 */
char rx_buf[16];           /* 数字缓冲区 */
uint8_t rx_index = 0;      /* 缓冲区索引 */
uint8_t rx_state = 0;      /* 接收状态：0=等待'L', 1=接收LED号, 2=等待'F', 3=接收频率 */
uint8_t led_num_temp = 0;  /* 临时存储LED编号 */

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

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);    // 上电初始一次接收
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
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

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
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
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  // 包结构: L<led>F<freq>\r\n 示例: L1F100\r\n 表示LED1频率100Hz
  if (huart->Instance == USART1)
  {
    if (rx_state == 0)
    {
      /* 等待接收 'L' (LED命令起始) */
      if (rx_byte == 'L')
      {
        rx_state = 1;
        rx_index = 0;
        memset(rx_buf, 0, sizeof(rx_buf));
      }
    }
    else if (rx_state == 1)
    {
      /* 接收LED编号 (1, 2, 3) */
      if (rx_byte >= '1' && rx_byte <= '3')
      {
        led_num_temp = rx_byte - '0';
        rx_state = 2;  /* 等待接收 'F' */
      }
      else
      {
        /* 无效字符，重置 */
        rx_state = 0;
      }
    }
    else if (rx_state == 2)
    {
      /* 等待接收 'F' */
      if (rx_byte == 'F')
      {
        rx_state = 3;
        rx_index = 0;
        memset(rx_buf, 0, sizeof(rx_buf));
      }
      else
      {
        /* 无效字符，重置 */
        rx_state = 0;
      }
    }
    else if (rx_state == 3)
    {
      /* 接收频率数字，直到收到回车或换行 */
      if (rx_byte == '\r' || rx_byte == '\n')
      {
        /* 解析频率 (Hz) */
        float freq_hz = atof(rx_buf);
        if (freq_hz >= 1.0f && freq_hz <= 1000.0f)
        {
          /* 发送消息到队列 */
          LED_Msg msg;
          msg.led_num = led_num_temp;
          msg.freq_hz = freq_hz;
          osMessageQueuePut(Message_QueHandle, &msg, 0, 0);

          char response[50];
          sprintf(response, "LED%d Freq: %.2f Hz\r\n", led_num_temp, freq_hz);
          HAL_UART_Transmit(&huart1, (uint8_t*)response, strlen(response), 100);
        }
        else
        {
          char msg[] = "Error: Freq must be 1~1000 Hz\r\n";
          HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
        }
        /* 重置状态 */
        rx_state = 0;
        rx_index = 0;
      }
      else if (rx_byte >= '0' && rx_byte <= '9' || rx_byte == '.')
      {
        /* 收到数字或小数点字符 */
        if (rx_index < 15)
        {
          rx_buf[rx_index++] = rx_byte;
        }
      }
      else
      {
        /* 无效字符，重置 */
        rx_state = 0;
        rx_index = 0;
      }
    }
    /* 继续接收下一字节 */
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
  }
}
/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
