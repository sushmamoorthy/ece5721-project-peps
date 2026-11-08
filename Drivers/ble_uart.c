/* Final Project Files
 * Filename: ble_uart.c
 * Author: Mrunalini Desai
 * Date: 11/04/2026
 *
 * Description: Implements functions for BLE UART communication.
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/uart.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "ble_uart.h"

void BT_Init(uint32_t systemClockHz)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);

    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB))
    {
        // not ready condition
    }

    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC))
    {
        // not ready condition
    }

    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART1))
    {
        // not ready condition
    }

    GPIOPinConfigure(GPIO_PB0_U1RX);
    GPIOPinConfigure(GPIO_PB1_U1TX);
    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    GPIOPinConfigure(GPIO_PC4_U1RTS);
    GPIOPinConfigure(GPIO_PC5_U1CTS);
    GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    UARTConfigSetExpClk(UART1_BASE, systemClockHz, 9600, 
                        UART_CONFIG_WLEN_8 |
                        UART_CONFIG_STOP_ONE |
                        UART_CONFIG_PAR_NONE);

    UARTFlowControlSet(UART1_BASE, 
                       UART_FLOWCONTROL_TX |
                       UART_FLOWCONTROL_RX);   
}

bool BT_RX_Available(void)
{
    return UARTCharsAvail(UART1_BASE);
}

uint8_t BT_RX_Read(void)
{
    return (uint8_t)UARTCharGet(UART1_BASE);
}

void BT_TX_Byte(uint8_t byte)
{
    UARTCharPut(UART1_BASE, byte);
}

void BT_TX_String(const char *text)
{
    while (*text != '\0')
    {
        BT_TX_Byte((uint8_t)*text);
        text++;
    }
}

bool BT_TX_TryByte(uint8_t byte)
{
    return UARTCharPutNonBlocking(UART1_BASE, byte);
}

bool BT_TX_Busy(void)
{
    return UARTBusy(UART1_BASE);
}
