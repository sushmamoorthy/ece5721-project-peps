/*
 * PEPS Adaptive BCM - main entry point (draft version)
 *
 * SysTick keeps the 1 ms timeout counter. The main super-loop parses BLE
 * input and runs the FSM; UART interrupts only place received bytes in a buffer.
 */

#include <TM4C123.h>

#include <stdbool.h>
#include <stdint.h>

#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"

#include "bcm_state.h"
#include "../Drivers/ble_uart.h"
#include "../Drivers/can_driver.h"
#include "../Drivers/gpio_leds.h"
#include "../Drivers/pwm_servo.h"
#include "../Drivers/bluefruit.h"

/**
 * @brief Initialize UART0 for diagnostic output to the PC.
 *
 * @details
 * Uses PA0/PA1 and the LaunchPad's onboard USB debug connection.
 * The terminal settings are 115200 baud, 8 data bits, no parity,
 * 1 stop bit, and no terminal flow control.
 */
 static void Debug_UART_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)) { }
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0)) { }

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIOA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTConfigSetExpClk(UART0_BASE, SystemCoreClock, 115200,
                        UART_CONFIG_WLEN_8 |
                        UART_CONFIG_STOP_ONE |
                        UART_CONFIG_PAR_NONE);
}

 /* Elapsed milliseconds, incremented by the SysTick interrupt. */
static volatile uint32_t systemTimeMs = 0U;

/**
 * @brief Update the application time and the BCM timer every millisecond.
 */
	void SysTick_Handler(void)
	{
		systemTimeMs++;  
		BCM_OnTimerTick();
	}

	int main(void)
	{
			/* TODO: confirm the selected clock configuration in the Keil device pack. */
			SystemCoreClockUpdate();
		
			Debug_UART_Init();

			/* TODO: configure the 1 ms SysTick reload for the confirmed clock. */
			(void)SysTick_Config(SystemCoreClock / 1000U);

			/* Initialize hardware drivers before the FSM applies its locked outputs. */
			GPIO_LEDs_Init();
			PWM_Servo_Init();
			CAN_Driver_Init();
			BLE_UART_Init(SystemCoreClock);

			BCM_Init();

			while (1) 
				{
					 /* Use one time snapshot for both Bluefruit service calls. */
						uint32_t nowMs = systemTimeMs;

						Bluefruit_Monitor(nowMs);
						Bluefruit_Poll(nowMs);

						/* Demo mode still uses SW1 and SW2 for BCM decisions. */
						BCM_ProcessState(nowMs);
			}
	}
