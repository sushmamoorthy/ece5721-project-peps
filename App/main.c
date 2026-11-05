/*
 * PEPS Adaptive BCM - main entry point (draft version)
 *
 * SysTick keeps the 1 ms timeout counter. The main super-loop parses BLE
 * input and runs the FSM; UART interrupts only place received bytes in a buffer.
 */

#include <TM4C123.h>

#include "bcm_state.h"
#include "../Drivers/ble_uart.h"
#include "../Drivers/can_driver.h"
#include "../Drivers/gpio_leds.h"
#include "../Drivers/pwm_servo.h"

void SysTick_Handler(void)
{
    BCM_OnTimerTick();
}

int main(void)
{
    /* TODO: confirm the selected clock configuration in the Keil device pack. */
    SystemCoreClockUpdate();

    /* TODO: configure the 1 ms SysTick reload for the confirmed clock. */
    (void)SysTick_Config(SystemCoreClock / 1000U);

    /* Initialize hardware drivers before the FSM applies its locked outputs. */
    GPIO_LEDs_Init();
    PWM_Servo_Init();
    CAN_Driver_Init();
    BLE_UART_Init();

    BCM_Init();

    while (1) {
        BCM_ProcessState();
    }
}
