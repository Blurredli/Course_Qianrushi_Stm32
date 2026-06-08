#include "cmsis_os2.h"
#include "main.h"
#include "FreeRTOS.h" // 如果不按顺序导入可能报错"portable.h"
#include "Types/LEDType.h"

#define IS_KEY_PRESSED() (HAL_GPIO_ReadPin(Key1_GPIO_Port, Key1_Pin) == GPIO_PIN_RESET)
// 按键检测间隔
#define KEY_CHECK_INTERVAL 10
// 按键消抖时间
#define KEY_DEBOUNCE_TIME 30
// 按键消抖次数 (30 / 10 = 3次)
#define KEY_DEBOUNCE_COUNT (KEY_DEBOUNCE_TIME / KEY_CHECK_INTERVAL)

uint8_t isKey1Clicked() {
    static uint8_t count = 0;
    static uint8_t pressed = 0;

    if (IS_KEY_PRESSED() && pressed == 0) {
        count++;
        if (count >= KEY_DEBOUNCE_COUNT && IS_KEY_PRESSED()) {
            pressed = 1;
            return 1;
        }
    }
    if (!IS_KEY_PRESSED()) {
        pressed = 0;
        count = 0;
    }
    return 0;
}


void StartKeyTask(void *argument) {
    LEDState state = LED_STATE_OFF;
    for (;;) {
        if (isKey1Clicked()) {
            // 按键1被点击
            state = !state;
            LEDMessage *message = pvPortMalloc(sizeof(LEDMessage));  // 动态分配内存
            message->color = LED_COLOR_RED;
            message->state = state;
            osMessageQueuePut(LEDQueueHandle, &message, 0, osWaitForever); // 传入指针的指针
        }
            osDelay(10);
    }
}
