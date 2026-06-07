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

#define DELAY_INIT  250   /* 初始延时 500ms = 2Hz，方便仿真观察 */
#define DELAY_STEP  50   /* 每次按键减少 100ms */
#define DELAY_MIN   50   /* 最小延时 100ms = 10Hz */
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
*        接收事件标志控制流水灯
*        KEY1/KEY2 按下后至少亮半个周期才生效，防止LED脉冲波形
*        KEY3 暂停立即生效
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLEDTask */
void StartLEDTask(void *argument)
{
  /* USER CODE BEGIN StartLEDTask */
  uint32_t flags;
  uint32_t delay_ms = DELAY_INIT;
  uint32_t half_ms;         /* 半周期延时 */
  int8_t direction = 1;     /* 1=正向(LED1->LED2->LED3), -1=反向 */
  int8_t running = 1;       /* 1=运行, 0=暂停 */
  uint8_t step = 0;         /* 当前流水步骤 0~2 */

  /* 待生效标志：KEY1/KEY2 按下后先标记，等半周期后再真正应用 */
  uint8_t pend_dir   = 0;   /* 1=有方向切换待生效 */
  uint8_t pend_speed = 0;   /* 1=有速率变化待生效 */

  /* 流水灯 GPIO 引脚表 */
  const struct {
    GPIO_TypeDef *port;
    uint16_t pin;
  } leds[3] = {
    {LED1_GPIO_Port, LED1_Pin},  /* LED1: PB5 */
    {LED2_GPIO_Port, LED2_Pin},  /* LED2: PD6 */
    {LED3_GPIO_Port, LED3_Pin},  /* LED3: PD3 */
  };

  /* 上电先点亮第一个LED */
  HAL_GPIO_WritePin(leds[direction == 1 ? step : (2 - step)].port,
                    leds[direction == 1 ? step : (2 - step)].pin, GPIO_PIN_SET);

  /* Infinite loop */
  for(;;)
  {
    /* 等待事件标志：任意按键事件 或 当前延时超时 */
    flags = osThreadFlagsWait(EVT_KEY1 | EVT_KEY2 | EVT_KEY3,
                              osFlagsWaitAny,
                              pdMS_TO_TICKS(delay_ms));

    if (!(flags > 0x80000000U))  /* 收到有效按键事件 */
    {
      if (flags & EVT_KEY3)
        running = !running;

      if (flags & EVT_KEY1)
        pend_dir = 1;

      if (flags & EVT_KEY2)
        pend_speed = 1;
    }

    /* ---- 亮完前半周期 ---- */
    half_ms = delay_ms / 2;
    if (half_ms < 1) half_ms = 1;
    osDelay(half_ms);

    if (!running)
      continue;

    /* 半周期到，应用待生效的状态变化 */
    if (pend_dir)
    {
      direction = -direction;
      pend_dir = 0;
    }
    if (pend_speed)
    {
      delay_ms -= DELAY_STEP;
      if (delay_ms < DELAY_MIN)
        delay_ms = DELAY_INIT;
      pend_speed = 0;
    }

    /* ---- 亮完后半周期 ---- */
    half_ms = delay_ms / 2;
    if (half_ms < 1) half_ms = 1;
    osDelay(half_ms);

    /* ---- 当前灯灭，下一个灯立刻亮（无缝切换） ---- */
    if (direction == 1)
      HAL_GPIO_WritePin(leds[step].port, leds[step].pin, GPIO_PIN_RESET);
    else
      HAL_GPIO_WritePin(leds[2 - step].port, leds[2 - step].pin, GPIO_PIN_RESET);

    step = (step + 1) % 3;

    if (direction == 1)
      HAL_GPIO_WritePin(leds[step].port, leds[step].pin, GPIO_PIN_SET);
    else
      HAL_GPIO_WritePin(leds[2 - step].port, leds[2 - step].pin, GPIO_PIN_SET);
  }
  /* USER CODE END StartLEDTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
