# ECE 5721

## 1. Project Name and Group Members

**Project name:** BLE Proximity-Based Vehicle Access and Adaptive Body Control Module 
(App-Initiated Vehicle Access and Mirror Control)

**Team members:**

- Sushma Padubidri Shubhakara Moorthy
- Mrunalini Desai
- Subashree Anandhavel

This project implements a TM4C123GH6PM Adaptive Body Control Module classroom prototype. 
It combines a phone-connected BLE interface, onboard state machine, LaunchPad indicators,
servo mirror control, and a classic CAN status interface.

## 2. Hardware Used

The TM4C123GH6PM LaunchPad is the base controller and is excluded from the
parts count in the project report. Additional hardware includes:

| Component | Project role |
|---|---|
| Adafruit Bluefruit LE UART Friend | Bluetooth Low Energy UART interface and RSSI queries |
| Adafruit CAN Pal / TJA1051T/3 CAN transceiver | Connects TM4C123 CAN0 to differential CANH/CANL |
| USB-to-CAN analyzer with CAN FD support | Captures and verifies CAN frames; project uses classic CAN |
| SG90 micro servo motor | Simulates folding and unfolding a vehicle mirror |
| Breadboard and jumper wires | Peripheral interconnection and shared power/ground |
| BLE smartphone running Bluefruit Connect | Mobile digital-key interface |

The hardware layout uses a common ground and shares VBUS/5 V power across the
breadboard power rail where applicable. The SG90 should have regulated external
servo power if necessary, and the TJA1051T/3 must be operated at logic levels
compatible with the LaunchPad.

For wiring and system diagrams, see:

- [Project report](/doc/ECE5721_Project_BLEKey_AdaptiveBCM.docx)
- [Adaptive BCM system flow](/doc/AdaptiveBCM_System_FlowChart.png)
- [Hardware architecture](/doc/schematic/peps-ble-bcm-HW-Arch.drawio.pdf)

## 3. Environment and Build Configuration

- **IDE:** Keil uVision 5
- **Compiler:** ArmClang V6.24
- **Board support:** Keil TM4C_DFP 1.1.0
- **CMSIS:** ARM CMSIS 6.3.0
- **Peripheral library:** TivaWare/TivaWare C Series 2.1.4.178
- **Project include path:** `Tools\TivaWare\SW-TM4C\TivaWare_C_Series-2.1.4.178\`
- **Target:** TM4C123GH6PM / `Target_1`
- **Debug UART:** UART0 on PA0/PA1, 115200 baud, 8-N-1

Open `ece5721_peps.uvprojx` in Keil uVision and build `Target_1`. The project
includes the TM4C123 startup/system files and TivaWare driver libraries.
The firmware has a successful no-warning build recorded in `keil-build.log`.

## Demo videos

Final Demo Video of complete project:

Demo videos taken until Milestone 2:
- [LED and SERVO Demo Video] (https://drive.google.com/file/d/15v2vDEeWNCRvdJRXYTmpb0gX8tD--dBs/view)
- [BLE UART Milestone 2 demo video](https://drive.google.com/file/d/16wdreq3HgWIGX5PuflpKapD2RsStNHAp/view?usp=share_link)
- [CAN demo video] (https://drive.google.com/file/d/1zQXVMlC1f_SCCExTEJCU-SsuCUKhtQFx/view?usp=sharing)
                   (https://drive.google.com/file/d/1epMnrLED0H3RvSHkOQ4uojeEFwHxvPtR/view?usp=sharing)

## Implemented system flow

```
 real BLE status via Bluefruit service
                         |
                         v
                  App/bcm_state.c
                  BCM state machine
                         |
       +-----------------+-----------------+
       |                 |                 |
       v                 v                 v
 GPIO LEDs          SG90 servo        CAN0 driver
 PF1 red             PB6 PWM          PE4 RX / PE5 TX
 PF3 green                            |
                                      v
                              CAN transceiver
                                      |
                                      v
                               CAN bus 500 kbit/s
                                      |
                                      v
                             USB-to-CAN / Node B
```

`App/main.c` initializes the LED, servo, CAN, and BLE/Bluefruit services,
then runs the Bluefruit polling and BCM processing in a super-loop.
`SysTick_Handler()` calls `BCM_OnTimerTick()` once per millisecond. UART0 is
configured at 115200 baud for diagnostic output.

## BCM behavior

The BCM has two states:

| State | Mirror | Red LED (PF1) | Green LED (PF3) | CAN payload |
|---|---|---:|---:|---|
| Locked | Folded | On | Off | `00 00` |
| Unlocked | Unfolded | Off | On | `01 01` |

The default state is locked. The vehicle unlocks when a valid near result is
confirmed. An explicit disconnect locks immediately. When the status changes
from near to far, the state remains unlocked for a 2-second walk-away timeout
before it locks.

In demo mode:

- `SW1` (PF4) toggles the simulated phone between near and far.
- Near must remain stable for 300 ms before the unlock decision.
- `SW2` (PF0) simulates immediate disconnect.
- Both inputs use active-low GPIO and 15 ms debouncing.

Set `BCM_DEMO_MODE` to `0U` to use the implemented Bluefruit RSSI classifier
instead. Live BLE UART/RSSI work has its own UART1 diagnostic and polling
implementation, but `Drivers/ble_uart.c` remains incomplete and does not yet
implement the UART1 interrupt or RX ring parser.

## CAN interface

CAN0 is configured by `Drivers/can_driver.c` as follows:

| Item | Configuration |
|---|---|
| CAN controller | CAN0 |
| Bit rate | 500 kbit/s |
| Frame format | Classic CAN, standard 11-bit identifier |
| Frame ID | `0x200` |
| Message object | 1 |
| DLC | 2 bytes |
| CAN RX pin | PE4 / `GPIO_PE4_CAN0RX` |
| CAN TX pin | PE5 / `GPIO_PE5_CAN0TX` |

The PE4/PE5 pins connect to a CAN transceiver, not directly to the
differential CANH/CANL bus.

### Status frame

```text
CAN ID: 0x200
DLC:    2

Byte 0: lock state
  0 = locked
  1 = unlocked

Byte 1: mirror state
  0 = folded
  1 = unfolded
```

The BCM sends `0x200#00 00` during locked initialization and on a transition
to locked. It sends `0x200#01 01` on a transition to unlocked. Frames are sent
on state changes and at initialization; the driver does not currently transmit
the frame periodically.

### CAN wiring

```text
TM4C123 LaunchPad                 CAN transceiver
PE4 (CAN0RX) ------------------> RX
PE5 (CAN0TX) ------------------> TX
GND          ------------------> GND
3.3 V        ------------------> VCC only if transceiver ratings require 3.3 V

Transceiver CANH/H ---------------- CANH
Transceiver CANL/L ---------------- CANL
Terminate both ends of the CAN bus with 120 ohm resistors.
```

Verify transceiver logic-voltage compatibility and bus termination before
connecting two nodes.

## Hardware configuration

- `UART0`: PA0 RX / PA1 TX, 115200 baud, 8-N-1, debug terminal.
- `UART1`: PB0 RX / PB1 TX, 9600 baud, 8-N-1, Bluefruit HM-10/Adafruit
  interface; PC4 RTS and PC5 CTS provide hardware flow control.
- `CAN0`: PE4 RX / PE5 TX, 500 kbit/s.
- `PWM0`: PB6 / M0PWM0, 50 Hz mirror servo output.
- Servo folded pulse: 1100 us.
- Servo unfolded pulse: 1900 us.
- Servo power: use an external regulated 5 V supply as appropriate for the
  SG90 and share ground with the LaunchPad.

Calibrate the servo pulse widths in `Drivers/pwm_servo.h` before attaching
mechanical linkage.

## Source files

- `App/main.c` initializes hardware and runs the application super-loop.
- `App/bcm_state.c/.h` implement the locked/unlocked state machine, demo
  switches, 2-second timeout, LEDs, servo, and CAN outputs.
- `Drivers/can_driver.c/.h` initialize CAN0 and transmit BCM status frames.
- `Drivers/bluefruit.c/.h` implement Bluefruit AT/RSSI polling and near/far
  classification.
- `Drivers/ble_uart.c/.h` provide UART1 and transport helpers.
- `Drivers/pwm_servo.c/.h` control the PB6 mirror servo PWM.
- `Drivers/gpio_leds.c/.h` control the onboard LEDs and demo switches.

## 4. References and Intellectual Contributions

### Project references

1. Texas Instruments, *TM4C123GH6PM Microcontroller Data Sheet*, SPMS376E.
   Used for TM4C123 peripheral, GPIO, PWM, UART, and CAN configuration.
2. Texas Instruments, *TivaWare C API*.
   Used for UART, CAN, GPIO, pin-configuration, system-control, and CMSIS
   project integration. The local copy is in
   [doc/datasheets/TivaWare C API.pdf](/doc/datasheets/TivaWare%20C%20API.pdf).
3. Adafruit,
   [CAN Pal - CAN Bus Transceiver - TJA1051T/3](https://learn.adafruit.com/adafruit-can-pal).
   Used to understand the CAN transceiver and differential bus interface.
4. Adafruit,
   [Bluefruit LE UART Friend](https://learn.adafruit.com/introducing-the-adafruit-bluefruit-le-uart-friend/downloads).
   Used for UART BLE module operation and RSSI/AT-command integration.
5. J. W. Valvano, *Embedded Systems: Real-Time Interfacing to ARM
   Cortex-M Microcontrollers* and associated Tiva/TM4C materials.
   Used as general guidance for TM4C register access and embedded real-time
   interfaces.
6. MicrocontrollersLab,
   [UART communication on the TM4C123 Tiva C LaunchPad](http://microcontrollerslab.com/uart-communication-tm4c123-tiva-c-launchpad/)
   and
   [SG90 servo motor interfacing](https://microcontrollerslab.com/servo-motor-interfacing-with-tm4c123-launchpad-keil/#google_vignette).
   Used for general UART and 50 Hz servo concepts.
7. S. Lee, Y. Cho, and H. Kim, "In-Vehicle Localization and Proximity
   Tracking for BLE-Enabled PEPS Access Systems," *IEEE Access / ResearchGate*,
   2024.
   Used as conceptual reference for BLE RSSI-based vehicle proximity.

The repository also includes these local datasheets:

- [TM4C123GH6PM data sheet](/doc/datasheets/MCU_TM4C123GH6PM_datasheet.pdf)
- [DSD TECH HM-10 data sheet](/doc/datasheets/DSD%20TECH%20HM-10%20datasheet.pdf)
- [Bluefruit LE UART Friend data sheet](/doc/datasheets/adafruit-bluefruit-le-uart-friend-datasheet.pdf)

The CAN analyzer Amazon listing and PEPS article were supporting references;
no code was copied from those sources.

### Generative-AI use

Generative AI through GitHub Copilot/Copilot SDK was used to reorganize this
README and refine wording from the project report and source code. It did not
generate or replace the embedded firmware. This README update was based on:

- the project report in [ECE5721_Project_BLEKey_AdaptiveBCM.docx](/doc/ECE5721_Project_BLEKey_AdaptiveBCM.docx),
- the source files in `App/` and `Drivers/`, and
- the hardware datasheets already stored in `doc/datasheets/`.

Our intellectual contribution is the modular integration
design and project-specific implementation: the BCM state machine and 2-second
timeout, demo switch debouncing, lock/mirror/LED/CAN state mapping, CAN frame
format, SG90 PWM setup, Bluefruit polling/classification, and the hardware
pin/interface decisions reflected in [App/bcm_state.c](/App/bcm_state.c),
[Drivers/can_driver.c](/Drivers/can_driver.c),
[Drivers/pwm_servo.c](/Drivers/pwm_servo.c),
[Drivers/bluefruit.c](/Drivers/bluefruit.c), and
[App/main.c](/App/main.c). No third-party GitHub project code is currently
used in these firmware files.
