/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define NOTIF_KEY1    1   /* 切换流水方向 */
#define NOTIF_KEY2    2   /* 增加流水速率 */
#define NOTIF_KEY3    3   /* 暂停/恢复 */

#define DELAY_INIT    1000  /* 初始延时 1000ms = 1Hz */
#define DELAY_STEP    200   /* 每次按键减少 200ms */
#define DELAY_MIN     100   /* 最小延时 100ms = 10Hz */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
/* 共享状态：LEDTask 更新，UartTask 读取 */
volatile int8_t   g_direction = 1;     /* 1=正向, -1=反向 */
volatile int8_t   g_running = 1;       /* 1=运行, 0=暂停 */
volatile uint32_t g_delay_ms = DELAY_INIT; /* 当前延时 */
/* USER CODE END Variables */
/* Definitions for KeyTask */
osThreadId_t KeyTaskHandle;
const osThreadAttr_t KeyTask_attributes = {
  .name = "KeyTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for LEDTask */
osThreadId_t LEDTaskHandle;
const osThreadAttr_t LEDTask_attributes = {
  .name = "LEDTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for UartTask */
osThreadId_t UartTaskHandle;
const osThreadAttr_t UartTask_attributes = {
  .name = "UartTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartKeyTask(void *argument);
void StartLEDTask(void *argument);
void StartUartTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of KeyTask */
  KeyTaskHandle = osThreadNew(StartKeyTask, NULL, &KeyTask_attributes);

  /* creation of LEDTask */
  LEDTaskHandle = osThreadNew(StartLEDTask, NULL, &LEDTask_attributes);

  /* creation of UartTask */
  UartTaskHandle = osThreadNew(StartUartTask, NULL, &UartTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartKeyTask */
/**
* @brief Function implementing the KeyTask thread.
*        轮询 KEY1/KEY2/KEY3，按下时通过任务通知发送给 LEDTask
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartKeyTask */
void StartKeyTask(void *argument)
{
  /* USER CODE BEGIN StartKeyTask */
  uint8_t key1_prev = 1, key2_prev = 1, key3_prev = 1;
  uint8_t key1_cur, key2_cur, key3_cur;

  /* Infinite loop */
  for(;;)
  {
    key1_cur = HAL_GPIO_ReadPin(Key1_GPIO_Port, Key1_Pin);
    key2_cur = HAL_GPIO_ReadPin(Key2_GPIO_Port, Key2_Pin);
    key3_cur = HAL_GPIO_ReadPin(Key3_GPIO_Port, Key3_Pin);

    if (key1_prev == 1 && key1_cur == 0)
      xTaskNotify(LEDTaskHandle, NOTIF_KEY1, eSetValueWithOverwrite);

    if (key2_prev == 1 && key2_cur == 0)
      xTaskNotify(LEDTaskHandle, NOTIF_KEY2, eSetValueWithOverwrite);

    if (key3_prev == 1 && key3_cur == 0)
      xTaskNotify(LEDTaskHandle, NOTIF_KEY3, eSetValueWithOverwrite);

    key1_prev = key1_cur;
    key2_prev = key2_cur;
    key3_prev = key3_cur;

    osDelay(20);
  }
  /* USER CODE END StartKeyTask */
}

/* USER CODE BEGIN Header_StartLEDTask */
/**
* @brief Function implementing the LEDTask thread.
*        接收任务通知控制流水灯，状态变化时通知 UartTask
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLEDTask */
void StartLEDTask(void *argument)
{
  /* USER CODE BEGIN StartLEDTask */
  uint32_t notify_val;
  uint8_t step = 0;

  const struct {
    GPIO_TypeDef *port;
    uint16_t pin;
  } leds[3] = {
    {LED1_GPIO_Port, LED1_Pin},
    {LED2_GPIO_Port, LED2_Pin},
    {LED3_GPIO_Port, LED3_Pin},
  };

  /* Infinite loop */
  for(;;)
  {
    if (xTaskNotifyWait(0, 0xFFFFFFFF, &notify_val, pdMS_TO_TICKS(g_delay_ms)) == pdTRUE)
    {
      switch (notify_val)
      {
        case NOTIF_KEY1:
          g_direction = -g_direction;
          xTaskNotify(UartTaskHandle, 0, eNoAction);
          break;

        case NOTIF_KEY2:
        {
          uint32_t old_delay = g_delay_ms;
          g_delay_ms -= DELAY_STEP;
          if (g_delay_ms < DELAY_MIN)
            g_delay_ms = DELAY_INIT;
          /* 速率变化时通知 UartTask（排除初始恢复的情况） */
          if (old_delay != g_delay_ms)
            xTaskNotify(UartTaskHandle, 0, eNoAction);
          break;
        }

        case NOTIF_KEY3:
          g_running = !g_running;
          xTaskNotify(UartTaskHandle, 0, eNoAction);
          break;
      }
    }

    if (!g_running)
      continue;

    /* 关闭所有LED */
    for (int i = 0; i < 3; i++)
      HAL_GPIO_WritePin(leds[i].port, leds[i].pin, GPIO_PIN_RESET);

    /* 点亮当前步骤的LED */
    if (g_direction == 1)
      HAL_GPIO_WritePin(leds[step].port, leds[step].pin, GPIO_PIN_SET);
    else
      HAL_GPIO_WritePin(leds[2 - step].port, leds[2 - step].pin, GPIO_PIN_SET);

    step = (step + 1) % 3;
  }
  /* USER CODE END StartLEDTask */
}

/* USER CODE BEGIN Header_StartUartTask */
/**
* @brief Function implementing the UartTask thread.
*        收到通知后通过串口发送当前流水灯状态
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartUartTask */
void StartUartTask(void *argument)
{
  /* USER CODE BEGIN StartUartTask */
  /* Infinite loop */
  for(;;)
  {
    /* 等待 LEDTask 的状态变化通知 */
    xTaskNotifyWait(0, 0xFFFFFFFF, NULL, portMAX_DELAY);

    if (!g_running)
    {
      /* 运行已暂停 */
      char msg[] = "Running paused\r\n";
      HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
    }
    else if (g_direction == 1 && g_delay_ms >= DELAY_INIT)
    {
      /* 正在正向运行中 */
      char msg[] = "Forward running.....\r\n";
      HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
    }
    else if (g_direction == -1 && g_delay_ms >= DELAY_INIT)
    {
      /* 正在反向运行中 */
      char msg[] = "Reverse running.....\r\n";
      HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
    }
    else if (g_direction == 1 && g_delay_ms < DELAY_INIT)
    {
      /* 正向加速中 */
      char msg[] = "Forward accelerating.....\r\n";
      HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
    }
    else if (g_direction == -1 && g_delay_ms < DELAY_INIT)
    {
      /* 反向加速中 */
      char msg[] = "Reverse accelerating.....\r\n";
      HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
    }
  }
  /* USER CODE END StartUartTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
