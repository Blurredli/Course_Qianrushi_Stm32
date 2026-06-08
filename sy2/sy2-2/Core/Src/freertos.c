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

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* 按键事件标志位定义，用于 osThreadFlagsSet / osThreadFlagsWait */
#define EVT_KEY1    (1U << 0)  /* KEY1: 切换流水方向 */
#define EVT_KEY2    (1U << 1)  /* KEY2: 增加流水速率 */
#define EVT_KEY3    (1U << 2)  /* KEY3: 暂停/恢复 */

#define DELAY_INIT  100  /* 初始延时 100ms = 10Hz */
#define DELAY_STEP  30   /* 每次按键减少 30ms 不下溢出*/
#define DELAY_MIN   20   /* 最小延时 20ms */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

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

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartKeyTask(void *argument);
void StartLEDTask(void *argument);

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
  KeyTaskHandle = osThreadNew(StartKeyTask, NULL, &KeyTask_attributes);
  LEDTaskHandle = osThreadNew(StartLEDTask, NULL, &LEDTask_attributes);

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
*        轮询 KEY1/KEY2/KEY3，按下时通过 osThreadFlagsSet 通知 LEDTask
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
    /* 读取按键状态（低电平有效，未按下时为高） */
    key1_cur = HAL_GPIO_ReadPin(Key1_GPIO_Port, Key1_Pin);
    key2_cur = HAL_GPIO_ReadPin(Key2_GPIO_Port, Key2_Pin);
    key3_cur = HAL_GPIO_ReadPin(Key3_GPIO_Port, Key3_Pin);

    /* KEY1 下降沿检测：从高变低说明按下 */
    if (key1_prev == 1 && key1_cur == 0)
      osThreadFlagsSet(LEDTaskHandle, EVT_KEY1);

    /* KEY2 下降沿检测 */
    if (key2_prev == 1 && key2_cur == 0)
      osThreadFlagsSet(LEDTaskHandle, EVT_KEY2);

    /* KEY3 下降沿检测 */
    if (key3_prev == 1 && key3_cur == 0)
      osThreadFlagsSet(LEDTaskHandle, EVT_KEY3);

    /* 保存当前按键状态，供下一轮比较 */
    key1_prev = key1_cur;
    key2_prev = key2_cur;
    key3_prev = key3_cur;

    osDelay(20);  /* 20ms 消抖周期 */
  }
  /* USER CODE END StartKeyTask */
}

/* USER CODE BEGIN Header_StartLEDTask */
/**
* @brief Function implementing the LEDTask thread.
*        接收事件标志控制流水灯：方向/速率/暂停
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLEDTask */
void StartLEDTask(void *argument)
{
  /* USER CODE BEGIN StartLEDTask */
  uint32_t flags;
  uint32_t delay_ms = DELAY_INIT;
  int8_t direction = 1;   /* 1=正向(LED1->LED2->LED3), -1=反向 */
  int8_t running = 1;     /* 1=运行, 0=暂停 */
  uint8_t step = 0;       /* 当前流水步骤 0~2 */
  int8_t current_led = -1; /* 当前点亮的 LED 索引，-1 表示初始状态 */

  /* 流水灯 GPIO 引脚表 */
  const struct {
    GPIO_TypeDef *port;
    uint16_t pin;
  } leds[3] = {
    {LED1_GPIO_Port, LED1_Pin},  /* LED1: PB5 */
    {LED2_GPIO_Port, LED2_Pin},  /* LED2: PD6 */
    {LED3_GPIO_Port, LED3_Pin},  /* LED3: PD3 */
  };

  /* Infinite loop */
  for(;;)
  {
    /* 等待事件标志：任意事件（EVT_KEY1|EVT_KEY2|EVT_KEY3）
       或超时（延时时间到）后返回。osFlagsWaitAny 等价于 xTaskNotifyWait
       的"等待任意通知"，osWaitForever 对应 portMAX_DELAY
       超时值为当前流水延时，超时返回0说明无按键，继续步进 */
    flags = osThreadFlagsWait(EVT_KEY1 | EVT_KEY2 | EVT_KEY3,
                              osFlagsWaitAny,
                              pdMS_TO_TICKS(delay_ms));

    if (flags > 0x80000000U)  /* 错误或超时，flags 最高位为1 */
    {
      /* 超时：无按键按下，继续下一步LED切换 */
    }
    else
    {
      /* 收到事件标志，处理对应按键 */
      if (flags & EVT_KEY1)  /* 切换方向 */
      {
        direction = -direction;
      }
      if (flags & EVT_KEY2)  /* 增加速率 */
      {
        delay_ms -= DELAY_STEP;
        if (delay_ms < DELAY_MIN)
          delay_ms = DELAY_INIT;  /* 超过阈值回到初始速率 */
      }
      if (flags & EVT_KEY3)  /* 暂停/恢复 */
      {
        running = !running;
      }
    }

    /* 暂停时跳过LED操作，但继续循环等待通知 */
    if (!running)
      continue;

    /* 计算本次要点亮的 LED 编号 */
    int8_t next_led;
    if (direction == 1)
      next_led = step;
    else
      next_led = 2 - step;

    /* 如果目标 LED 和当前亮着的相同，就无需操作 */
    if (next_led != current_led) {
      /* 先关闭上一个 LED */
      if (current_led >= 0)
        HAL_GPIO_WritePin(leds[current_led].port, leds[current_led].pin, GPIO_PIN_RESET);

      /* 再打开本次要点亮的 LED */
      HAL_GPIO_WritePin(leds[next_led].port, leds[next_led].pin, GPIO_PIN_SET);

      current_led = next_led;
    }

    /* 步进到下一个LED */
    step = (step + 1) % 3;
  }
  /* USER CODE END StartLEDTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
