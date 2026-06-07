/**
  * @file    main.c
  * @brief   FreeRTOS message queue demo
  *          UART receive task parses commands from queue,
  *          3 LED tasks blink at commanded frequency.
  *          Command format: "LED1:500\n" (LED1 blink at 500ms)
  */
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Private variables */
UART_HandleTypeDef huart1;

static TaskHandle_t hTaskUartRx;
static TaskHandle_t hTaskLed1;
static TaskHandle_t hTaskLed2;
static TaskHandle_t hTaskLed3;
static QueueHandle_t hQueueCmd;

/* LED blink periods (ms), default 1000 */
static volatile uint32_t led_period[3] = {1000, 1000, 1000};

/* UART receive buffer */
static uint8_t rx_byte;
static char    rx_buf[64];
static uint8_t rx_idx = 0;

/* LED pin table */
typedef struct { GPIO_TypeDef *port; uint16_t pin; } led_t;
static const led_t leds[3] = {
  { GPIOB, GPIO_PIN_5 },   /* LED1 */
  { GPIOD, GPIO_PIN_6 },   /* LED2 */
  { GPIOD, GPIO_PIN_3 },   /* LED3 */
};

/* Function prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);

/*-----------------------------------------------------------*/
/* UART receive interrupt callback */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    if (rx_byte == '\n' || rx_byte == '\r')
    {
      if (rx_idx > 0)
      {
        rx_buf[rx_idx] = '\0';
        /* Send complete command string to queue (copy, not pointer) */
        char msg[64];
        strncpy(msg, rx_buf, sizeof(msg) - 1);
        msg[sizeof(msg) - 1] = '\0';
        xQueueSendFromISR(hQueueCmd, msg, NULL);
        rx_idx = 0;
      }
    }
    else
    {
      if (rx_idx < sizeof(rx_buf) - 1)
        rx_buf[rx_idx++] = rx_byte;
    }
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
  }
}

/*-----------------------------------------------------------*/
/* Task: UART command receive & parse */
static void TaskUartRx(void *param)
{
  char cmd[64];
  (void)param;

  /* Startup message */
  const char *hello = "Ready. Usage: LED1:<ms> LED2:<ms> LED3:<ms>\r\n";
  HAL_UART_Transmit(&huart1, (uint8_t *)hello, strlen(hello), HAL_MAX_DELAY);

  for (;;)
  {
    if (xQueueReceive(hQueueCmd, cmd, portMAX_DELAY) == pdTRUE)
    {
      /* Parse "LEDx:period" */
      int led_idx = -1;
      uint32_t period = 0;

      if (strncmp(cmd, "LED1:", 5) == 0)      { led_idx = 0; period = atoi(&cmd[5]); }
      else if (strncmp(cmd, "LED2:", 5) == 0)  { led_idx = 1; period = atoi(&cmd[5]); }
      else if (strncmp(cmd, "LED3:", 5) == 0)  { led_idx = 2; period = atoi(&cmd[5]); }

      if (led_idx >= 0 && period >= 50 && period <= 10000)
      {
        led_period[led_idx] = period;
        char reply[48];
        int len = sprintf(reply, "LED%d period = %lu ms\r\n", led_idx + 1, (unsigned long)period);
        HAL_UART_Transmit(&huart1, (uint8_t *)reply, len, HAL_MAX_DELAY);
      }
      else
      {
        const char *err = "ERR: use LED1:<ms> (50~10000)\r\n";
        HAL_UART_Transmit(&huart1, (uint8_t *)err, strlen(err), HAL_MAX_DELAY);
      }
    }
  }
}

/*-----------------------------------------------------------*/
/* LED task template: blink one LED at its commanded period */
static void TaskLed(void *param)
{
  int idx = (int)param;
  TickType_t last = xTaskGetTickCount();

  for (;;)
  {
    HAL_GPIO_WritePin(leds[idx].port, leds[idx].pin, GPIO_PIN_SET);
    vTaskDelayUntil(&last, pdMS_TO_TICKS(led_period[idx] / 2));
    HAL_GPIO_WritePin(leds[idx].port, leds[idx].pin, GPIO_PIN_RESET);
    vTaskDelayUntil(&last, pdMS_TO_TICKS(led_period[idx] / 2));
  }
}

/*-----------------------------------------------------------*/
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();

  /* Create message queue (8 items, each 64 bytes) */
  hQueueCmd = xQueueCreate(8, 64);

  /* Create tasks */
  xTaskCreate(TaskUartRx, "UartRx", 256, NULL, 3, &hTaskUartRx);
  xTaskCreate(TaskLed,    "Led1",   128, (void *)0, 2, &hTaskLed1);
  xTaskCreate(TaskLed,    "Led2",   128, (void *)1, 2, &hTaskLed2);
  xTaskCreate(TaskLed,    "Led3",   128, (void *)2, 2, &hTaskLed3);

  /* Start UART receive */
  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

  /* Start scheduler — does not return */
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
}

/*-----------------------------------------------------------*/
/* FreeRTOS hooks */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  (void)xTask;
  (void)pcTaskName;
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
