/**
 * @file gpio_leds.c
 * @brief Implements GPIO control for the red and green status LEDs.
 * 
 */

#include "gpio_leds.h"

/**
 * @brief Initialize the GPIO pins used for the red and green status LEDs.
 *
 * TODO: Configure the selected pins as digital outputs and confirm their
 * active level for the onboard RGB LED.
 */
void GPIO_LEDs_Init(void)
{
    /*
     * TODO: configure the selected red/green GPIO pins as digital outputs.
     * Since we are using onboard RGB LED, confirm its pins and active level.
     */
}

/**
 * @brief Set the red and green status LED outputs.
 *
 * @param red_on    true to turn the red LED on; false to turn it off.
 * @param green_on  true to turn the green LED on; false to turn it off.
 *
 * The locked indication is red on/green off. The unlocked indication is
 * red off/green on.
 */
void GPIO_SetLEDs(bool red_on, bool green_on)
{
    /*
     * TODO: write the two output pins.
     * Locked: call GPIO_SetLEDs(true, false).
     * Unlocked: call GPIO_SetLEDs(false, true).
     */
    (void)red_on;
    (void)green_on;
}
