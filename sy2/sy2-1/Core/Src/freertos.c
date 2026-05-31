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
/* USER CODE BEGIN Variables */
/* 消息结构：LED编号 + 频率(Hz) */
typedef struct {
    uint8_t led_num;    /* LED编号: 1, 2, 3 */
    float freq_hz;      /* 频率(Hz) */
} LED_Msg;

/* LED频率变量 */
float led1_freq = 1.0f;
float led2_freq = 1.0f;
float led3_freq = 1.0f;

/* 外部变量声明（在main.c中定义） */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for UartTask */
osThreadId_t UartTaskHandle;
const osThreadAttr_t UartTask_attributes = {
  .name = "UartTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for LED1Task */
osThreadId_t LED1TaskHandle;
const osThreadAttr_t LED1Task_attributes = {
  .name = "LED1Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for LED2Task */
osThreadId_t LED2TaskHandle;
const osThreadAttr_t LED2Task_attributes = {
  .name = "LED2Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for LED3Task */
osThreadId_t LED3TaskHandle;
const osThreadAttr_t LED3Task_attributes = {
  .name = "LED3Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for Message_Que */
osMessageQueueId_t Message_QueHandle;
const osMessageQueueAttr_t Message_Que_attributes = {
  .name = "Message_Que"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartUartTask(void *argument);
void StartLED1Task(void *argument);
void StartLED2Task(void *argument);
void StartLED3Task(void *argument);

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

  /* Create the queue(s) */
  /* creation of Message_Que - 使用LED_Msg结构体作为消息 */
  Message_QueHandle = osMessageQueueNew (10, sizeof(LED_Msg), &Message_Que_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of UartTask */
  UartTaskHandle = osThreadNew(StartUartTask, NULL, &UartTask_attributes);

  /* creation of LED1Task */
  LED1TaskHandle = osThreadNew(StartLED1Task, NULL, &LED1Task_attributes);

  /* creation of LED2Task */
  LED2TaskHandle = osThreadNew(StartLED2Task, NULL, &LED2Task_attributes);

  /* creation of LED3Task */
  LED3TaskHandle = osThreadNew(StartLED3Task, NULL, &LED3Task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartUartTask */
/**
* @brief Function implementing the UartTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartUartTask */
void StartUartTask(void *argument)
{
  /* USER CODE BEGIN StartUartTask */
  LED_Msg received_msg;
  osStatus_t status;

  /* Infinite loop */
  for(;;)
  {
    /* 从消息队列接收消息 */
    status = osMessageQueueGet(Message_QueHandle, &received_msg, NULL, osWaitForever);

    if (status == osOK)
    {
      /* 根据LED编号更新对应的频率 */
      switch(received_msg.led_num)
      {
        case 1:
          led1_freq = received_msg.freq_hz;
          break;
        case 2:
          led2_freq = received_msg.freq_hz;
          break;
        case 3:
          led3_freq = received_msg.freq_hz;
          break;
      }
    }
    osDelay(10);
  }
  /* USER CODE END StartUartTask */
}

/* USER CODE BEGIN Header_StartLED1Task */
/**
* @brief Function implementing the LED1Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLED1Task */
void StartLED1Task(void *argument)
{
  /* USER CODE BEGIN StartLED1Task */
  uint32_t delay_ms;

  /* Infinite loop */
  for(;;)
  {
    /* 计算延时时间 */
    delay_ms = (uint32_t)(1000.0f / led1_freq);

    /* LED1闪烁 */
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
    osDelay(delay_ms);
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    osDelay(delay_ms);
  }
  /* USER CODE END StartLED1Task */
}

/* USER CODE BEGIN Header_StartLED2Task */
/**
* @brief Function implementing the LED2Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLED2Task */
void StartLED2Task(void *argument)
{
  /* USER CODE BEGIN StartLED2Task */
  uint32_t delay_ms;

  /* Infinite loop */
  for(;;)
  {
    /* 计算延时时间 */
    delay_ms = (uint32_t)(1000.0f / led2_freq);

    /* LED2闪烁 */
    HAL_GPIO_WritePin(GPIOD, LED2_Pin, GPIO_PIN_SET);
    osDelay(delay_ms);
    HAL_GPIO_WritePin(GPIOD, LED2_Pin, GPIO_PIN_RESET);
    osDelay(delay_ms);
  }
  /* USER CODE END StartLED2Task */
}

/* USER CODE BEGIN Header_StartLED3Task */
/**
* @brief Function implementing the LED3Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLED3Task */
void StartLED3Task(void *argument)
{
  /* USER CODE BEGIN StartLED3Task */
  uint32_t delay_ms;

  /* Infinite loop */
  for(;;)
  {
    /* 计算延时时间 */
    delay_ms = (uint32_t)(1000.0f / led3_freq);

    /* LED3闪烁 */
    HAL_GPIO_WritePin(GPIOD, LED3_Pin, GPIO_PIN_SET);
    osDelay(delay_ms);
    HAL_GPIO_WritePin(GPIOD, LED3_Pin, GPIO_PIN_RESET);
    osDelay(delay_ms);
  }
  /* USER CODE END StartLED3Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

