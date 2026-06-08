//
// Created by Mayn on 26-5-15.
//

#ifndef LEDTYPE_H
#define LEDTYPE_H

typedef enum {
    LED_COLOR_RED = 0,
    LED_COLOR_GREEN = 1,
    LED_COLOR_BLUE = 2,
} LEDColor;

typedef enum {
    LED_STATE_OFF = 0,
    LED_STATE_ON = 1,
} LEDState;

typedef struct {
    LEDColor color;
    LEDState state;
} LEDMessage;

#endif //LEDTYPE_H
