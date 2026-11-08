/* Final Project Files
 * Filename: gpio_leds.c
 * Author: Mrunalini Desai
 * Date: 11/04/2026
 *
 * Description: Provides functions and definitions for GPIO LED Module.
 *
 */

#include <stdbool.h>
#include <stdint.h>

#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "gpio_leds.h"

#define DOOR_LED_PINS (GPIO_PIN_1 | GPIO_PIN_3)
#define BLINK_HALF_PERIOD_MS 250UL

static doorState displayedState = DOOR_LOCK;
static uint32_t pendingStartedMs = 0;

void DoorLED_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF))
    {
    }

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, DOOR_LED_PINS);

    displayedState = DOOR_LOCK;
    pendingStartedMs = 0;
    GPIOPinWrite(GPIO_PORTF_BASE, DOOR_LED_PINS, GPIO_PIN_1);
}

bool DoorLED_SetState(doorState state, uint32_t nowMs)
{
    switch (state)
    {
    case DOOR_LOCK:
    case DOOR_PENDING_CONFIRMATION:
    case DOOR_UNLOCK:
        break;

    default:
        return false;
    }

    if (state != displayedState)
    {
        displayedState = state;

        if (state == DOOR_PENDING_CONFIRMATION)
        {
            pendingStartedMs = nowMs;
        }
    }

    DoorLED_Update(nowMs);
    return true;
}

void DoorLED_Update(uint32_t nowMs)
{
    uint8_t pins = GPIO_PIN_1;

    switch (displayedState)
    {
        case DOOR_LOCK:
            pins = GPIO_PIN_1;
            break;

        case DOOR_PENDING_CONFIRMATION:
            pins = (((uint32_t)(nowMs - pendingStartedMs) / BLINK_HALF_PERIOD_MS) % 2U == 0U)
                    ? GPIO_PIN_1
                    : 0;
                    
            break; 

        case DOOR_UNLOCK:
            pins = GPIO_PIN_3; 
            break;
    }

    GPIOPinWrite(GPIO_PORTF_BASE, DOOR_LED_PINS, pins);
}
