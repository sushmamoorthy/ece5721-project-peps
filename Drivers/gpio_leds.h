/* Final Project Files
 * Filename: gpio_leds.h
 * Author: Mrunalini Desai
 * Date: 11/04/2026
 *
 * Description: Provides functions and definitions for DOOR LED Module.
 *
 */
#ifndef GPIO_LEDS_H
#define GPIO_LEDS_H

#include <stdbool.h>
#include <stdint.h>
#include "App/bcm_state.h"

void DoorLED_Init(void);
bool DoorLED_SetState(doorState state, uint32_t nowMs);
void DoorLED_Update(uint32_t nowMs);

#endif // GPIO_LEDS_H
