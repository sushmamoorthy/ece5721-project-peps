# ECE 5721 PEPS Project

This is the draft code structure for our TM4C123 PEPS project. The code is
split into the main program, the state machine, and separate hardware drivers.
The driver functions are still placeholders and will be filled in as we test
each part.

## Demo video

[BLE UART Milestone 2 demo video](https://drive.google.com/file/d/16wdreq3HgWIGX5PuflpKapD2RsStNHAp/view?usp=share_link)

## Planned flow

 [Phone app]
      | BLE connection and RSSI
      v
 [BLE module]
      | UART
      v
 [ble_uart.c]
      | UART interrupt puts bytes in buffer
      | BLE_UART_ProcessRx() parses messages in the main loop
      | BLE_IsConnected() / BLE_IsProximityDetected()
      v
 [bcm_state.c]
      | checks proximity and the 2-second timer
      |
      +----------+-----------+
      |          |           |
      v          v           v
 [pwm_servo.c] [gpio_leds.c] [can_driver.c]
  mirror        red/green      sends lock/mirror status
                                  |
                         +--------+--------+
                         |                 |
                         v                 v
                  USB-to-CAN tool     Optional Node B


SysTick calls `BCM_OnTimerTick()` every millisecond. The main loop calls
`BCM_ProcessState()`. The UART interrupt only buffers received data; the main
loop parses it and lets the state machine decide what to do.

When the phone is near, the planned behavior is to unlock, unfold the mirror,
turn on the green LED, and send a CAN update. On disconnect, walk-away, or
timeout, it should lock, fold the mirror, turn on the red LED, and send an
updated CAN message.

## Files

- `App/main.c` starts the drivers and runs the main loop.
- `App/bcm_state.c/.h` handle locked/unlocked behavior and the timeout.
- `Drivers/ble_uart.c/.h` handle BLE UART input and RSSI information.
- `Drivers/pwm_servo.c/.h` control the mirror servo.
- `Drivers/gpio_leds.c/.h` control the red and green LEDs.
- `Drivers/can_driver.c/.h` send the state over CAN.
- The baseline unlock decision uses BLE connection and proximity; AES
  authentication is not included.

The diagram shows the planned flow; UART parsing, RSSI handling, and hardware
drivers still need to be implemented and tested. CAN can go to a USB-to-CAN
tool or, in the two-node setup, another LaunchPad. The project diagram shows
UART1 for BLE, UART0 for debugging, and CAN0 on PE4/PE5; verify the pin setup
before wiring.
