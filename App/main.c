//*****************************************************************************
//
// main.c - BLE diagnostic application derived from the blinky example.
//
// Copyright (c) 2012-2020 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
//
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
//
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
//
// This is part of revision 2.2.0.295 of the EK-TM4C123GXL Firmware Package.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include <string.h>


#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "Drivers/ble_uart.h"
#include "inc/hw_memmap.h"
#include "Drivers/bluefruit.h"
#include "Drivers/gpio_leds.h"


//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line) {
  while (1)
    ;
}
#endif

#define LED_TIMEOUT_MS 5000UL

//*****************************************************************************
//
// Blink the on-board LED.
//
//*****************************************************************************
//
static bool ledIsOn = false;
static volatile uint32_t sysMSec = 0;
static uint32_t ledOnStartedMs = 0;
//
void SysTick_Handler(void) { sysMSec++; }
//
// helper
void BT_ProcessCommand(const char *command) {
  if (strcmp(command, "LED ON") == 0) {
    if (!ledIsOn) {
      ledOnStartedMs = sysMSec;
      GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);
      ledIsOn = true;
    }
    BT_TX_String("OK LED ON\r\n");

  } else if (strcmp(command, "LED OFF") == 0) {
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);
    ledIsOn = false;
    BT_TX_String("OK LED OFF\r\n");

  } else if (strcmp(command, "STATUS") == 0) {
    if (ledIsOn) {
      BT_TX_String("STATUS LED ON\r\n");
    } else {
      BT_TX_String("STATUS LED OFF\r\n");
    }
  } else {
    BT_TX_String("ERR COMMAND\r\n");
  }
}

// Timeout helper
void LED_CheckTimeout(void) {
  if (ledIsOn && (uint32_t)(sysMSec - ledOnStartedMs) >= LED_TIMEOUT_MS) {
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);
    ledIsOn = false;
    BT_TX_String("EVNT LED OFF TIMEOUT\r\n");
  }
}

int main(void) {
  // volatile uint32_t ui32Loop;
  volatile uint32_t systemClockHz;
  unsigned char receivedChar;

  // Config system clock
  SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                 SYSCTL_OSC_MAIN);
  //
  systemClockHz = SysCtlClockGet();
  //
  BT_Init(systemClockHz);

  // Initialize
  // Enable the GPIO port that is used for the on-board LED.
  //
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  //
  // Check if the peripheral access is enabled.
  //
  while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)) {
  }

  while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0)) {
  }
  //
  // Enable the GPIO pin for the LED (PF1).  Set the direction as output, and
  //
  DoorLED_Init();

  // Select PA0's PA1'sUART function
  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  //
  // Configure UART0
  //
  UARTConfigSetExpClk(UART0_BASE, systemClockHz, 115200,
                      UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                          UART_CONFIG_PAR_NONE);
  //
  // System Tick Handler
  //
  SysTickPeriodSet(systemClockHz / 1000);
  SysTickIntEnable();
  SysTickEnable();
  IntMasterEnable();

  //
  // Loop forever.
  //
  while (1) 
  {
    
    Bluefruit_Monitor(sysMSec);
    Bluefruit_Poll(sysMSec);
    DoorLED_Update(sysMSec);

    if (UARTCharsAvail(UART0_BASE))
    {
      receivedChar = UARTCharGet(UART0_BASE);

      if (receivedChar != '\r' && receivedChar != '\n')
      {
        const char *message = "[INFO] Automatic RSSI polling; PC command disabled\r\n";

        while (*message != '\0')
        {
          UARTCharPut(UART0_BASE, *message++);
        }
      }
    }

  }
}
