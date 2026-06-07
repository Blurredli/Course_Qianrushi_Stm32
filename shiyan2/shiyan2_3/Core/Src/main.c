/**
  * @file    main.c
  * @brief   FreeRTOS key + UART status display
  *          Task 1: poll KEY1(PC5)/KEY2(PC2)/KEY3(PC3), send notification
  *          Task 2: receive notification, control LED mode
  *          Task 3: UART send, display current status
  *            KEY1: toggle direction   KEY2: speed up   KEY3: pause/resume
  */
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>

/* Private variables */
UART_HandleTypeDef huart1;

static TaskHandle_t hTaskKeyPoll;
static TaskHandle_t hTaskLedCtrl;
static TaskHandle_t hTaskUartTx;

/* LED pin table */
typedef struct { GPIO_TypeDef *port; uint16_t pin; } led_t;
static const led_t leds[3] = {
  { GPIOB, GPIO_PIN_5 },
  { GPIOD, GPIO_PIN_6 },
  { GPIOD, GPIO_PIN_3 },
};

/* Key pin table */
typedef struct { GPIO_TypeDef *port; uint16_t pin; } key_t;
static const key_t keys[3] = {
  { GPIOC, GPIO_PIN_5 },
  { GPIOC, GPIO_PIN_2 },
  { GPIOC, GPIO_PIN_3 },
};

/* Notification bits */
#define NOTIFY_KEY1  (1 << 0)
#define NOTIFY_KEY2  (1 << 1)
#define NOTIFY_KEY3  (1 << 2)

/* LED control state (shared with UART task) */
#define SPEED_LEVELS 5
static const uint32_t speed_ms[SPEED_LEVELS] = {1000, 600, 400, 250, 150};
static volatile uint8_t speed_idx  = 0;
static volatile uint8_t direction  = 0;   /* 0=forward, 1=reverse */
static volatile uint8_t paused     = 0;
static volatile uint8_t state_changed = 1; /* trigger initial display */

/* Function prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);

/*-----------------------------------------------------------*/
static void UART_SendStr(const char *s)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)s, strlen(s), HAL_MAX_DELAY);
}

/*-----------------------------------------------------------*/
static void LED_Set(uint8_t idx)
{
  for (int i = 0; i < 3; i++)
    HAL_GPIO_WritePin(leds[i].port, leds[i].pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(leds[idx].port, leds[idx].pin, GPIO_PIN_SET);
}

/*-----------------------------------------------------------*/
/* Task 1: Poll keys, send notification */
static void TaskKeyPoll(void *param)
{
  (void)param;
  uint8_t last[3] = {1, 1, 1};

  for (;;)
  {
    for (int i = 0; i < 3; i++)
    {
      uint8_t now = HAL_GPIO_ReadPin(keys[i].port, keys[i].pin);
      if (last[i] == 1 && now == 0)
      {
        vTaskDelay(pdMS_TO_TICKS(20));
        now = HAL_GPIO_ReadPin(keys[i].port, keys[i].pin);
        if (now == 0)
        {
          xTaskNotify(hTaskLedCtrl, (1 << i), eSetBits);
          /* Wait for release */
          while (HAL_GPIO_ReadPin(keys[i].port, keys[i].pin) == 0)
            vTaskDelay(pdMS_TO_TICKS(10));
        }
      }
      last[i] = now;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

/*-----------------------------------------------------------*/
static void LED_AdvanceLocal(int8_t *idx, uint8_t dir)
{
  if (dir == 0) { (*idx)++; if (*idx >= 3) *idx = 0; }
  else          { (*idx)--; if (*idx < 0)  *idx = 2; }
}

/*-----------------------------------------------------------*/
/* Task 2: Receive notification, control LEDs */
static void TaskLedCtrl(void *param)
{
  (void)param;
  uint32_t notify_val;
  int8_t led_index = 0;

  for (;;)
  {
    if (paused)
    {
      for (int i = 0; i < 3; i++)
        HAL_GPIO_WritePin(leds[i].port, leds[i].pin, GPIO_PIN_RESET);
      xTaskNotifyWait(0, 0xFFFFFFFF, &notify_val, portMAX_DELAY);
      if (notify_val & NOTIFY_KEY1) { direction ^= 1; state_changed = 1; }
      if (notify_val & NOTIFY_KEY2) { speed_idx++; if (speed_idx >= SPEED_LEVELS) speed_idx = 0; state_changed = 1; }
      if (notify_val & NOTIFY_KEY3) { paused ^= 1; state_changed = 1; }
      continue;
    }

    LED_Set((uint8_t)led_index);

    BaseType_t ret = xTaskNotifyWait(0, 0xFFFFFFFF, &notify_val,
                                      pdMS_TO_TICKS(speed_ms[speed_idx]));

    if (ret == pdTRUE)
    {
      if (notify_val & NOTIFY_KEY1) { direction ^= 1; state_changed = 1; }
      if (notify_val & NOTIFY_KEY2) { speed_idx++; if (speed_idx >= SPEED_LEVELS) speed_idx = 0; state_changed = 1; }
      if (notify_val & NOTIFY_KEY3) { paused ^= 1; state_changed = 1; continue; }
      LED_AdvanceLocal(&led_index, direction);
    }
    else
    {
      LED_AdvanceLocal(&led_index, direction);
    }
  }
}

/*-----------------------------------------------------------*/
/* Task 3: UART send status on state change */
static void TaskUartTx(void *param)
{
  (void)param;

  for (;;)
  {
    if (state_changed)
    {
      state_changed = 0;

      if (paused)
      {
        UART_SendStr("PUA\r\n");
      }
      else
      {
        uint8_t dir = direction;
        uint8_t spd = speed_idx;

        if (dir == 0 && spd == 0)
          UART_SendStr("ZHENG.....\r\n");
        else if (dir == 1 && spd == 0)
          UART_SendStr("FAN.....\r\n");
        else if (dir == 0 && spd > 0)
          UART_SendStr("ZHENGTO.....\r\n");
        else
          UART_SendStr("FANTO.....\r\n");
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

/*-----------------------------------------------------------*/
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();

  xTaskCreate(TaskKeyPoll, "KeyPoll", 128, NULL, 3, &hTaskKeyPoll);
  xTaskCreate(TaskLedCtrl, "LedCtrl", 256, NULL, 2, &hTaskLedCtrl);
  xTaskCreate(TaskUartTx,  "UartTx",  256, NULL, 1, &hTaskUartTx);

  vTaskStartScheduler();
  while (1) {}
}

/*-----------------------------------------------------------*/
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&huart1);

  HAL_NVIC_SetPriority(USART1_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6 | GPIO_PIN_3, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_2 | GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/*-----------------------------------------------------------*/
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  (void)xTask; (void)pcTaskName;
  while (1) {}
}

void vApplicationMallocFailedHook(void)
{
  while (1) {}
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}
