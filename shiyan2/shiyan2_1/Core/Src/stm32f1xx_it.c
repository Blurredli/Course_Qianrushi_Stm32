/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_it.h"
#include "FreeRTOS.h"
#include "task.h"

/* FreeRTOS port function — declared here to suppress implicit declaration warning */
extern void xPortSysTickHandler(void);

/* External variables --------------------------------------------------------*/
extern UART_HandleTypeDef huart1;

/* Cortex-M3 Exception Handlers */
void NMI_Handler(void)           { while (1); }
void HardFault_Handler(void)     { while (1); }
void MemManage_Handler(void)     { while (1); }
void BusFault_Handler(void)      { while (1); }
void UsageFault_Handler(void)    { while (1); }
/* SVC_Handler and PendSV_Handler are provided by FreeRTOS port (FreeRTOSConfig.h) */
void DebugMon_Handler(void)      {}

/**
  * @brief SysTick — drives both HAL tick and FreeRTOS scheduler
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
#if (INCLUDE_xTaskGetSchedulerState == 1)
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    xPortSysTickHandler();
#endif
}

/* Peripheral Interrupt Handlers */
void USART1_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart1);
}
