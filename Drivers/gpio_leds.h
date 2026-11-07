#ifndef GPIO_LEDS_H
#define GPIO_LEDS_H

#include <stdbool.h>

void GPIO_LEDs_Init(void);

/* Specify LED outputs explicitly: red=true turns red on, etc. */
void GPIO_SetLEDs(bool red_on, bool green_on);

#endif
