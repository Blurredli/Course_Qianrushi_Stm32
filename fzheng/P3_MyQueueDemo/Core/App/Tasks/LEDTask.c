#include "cmsis_os2.h"
#include "main.h"
// #include "portable.h"
#include "FreeRTOS.h"
#include "Types/LEDType.h"
//
// Created by Mayn on 26-5-15.
//
void StartLEDTask(void *argument) {
    for (;;) {
        LEDMessage *message = NULL;
        osMessageQueueGet(LEDQueueHandle, &message, NULL, osWaitForever);
        switch (message->color) {
            case LED_COLOR_RED:
                HAL_GPIO_WritePin(RedLED_GPIO_Port, RedLED_Pin, message->state ? GPIO_PIN_SET : GPIO_PIN_RESET);
                break;
            case LED_COLOR_GREEN:
                HAL_GPIO_WritePin(GreenLED_GPIO_Port, GreenLED_Pin, message->state ? GPIO_PIN_SET : GPIO_PIN_RESET);
                break;
            case LED_COLOR_BLUE:
                HAL_GPIO_WritePin(BlueLED_GPIO_Port, BlueLED_Pin, message->state ? GPIO_PIN_SET : GPIO_PIN_RESET);
                break;
            default:
                break;
        }
        vPortFree(message);
        // osDelay(10); // 由事件驱动，不需要延时
    }
}