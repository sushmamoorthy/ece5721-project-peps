/**
 * @file gpio_leds.h
 * @brief Register definitions and APIs for LaunchPad LEDs and demo buttons.
 *
 * TM4C123GH6PM GPIOF APB address space (default GPIOHBCTL setting).
 * PF1 = red LED, PF2 = blue LED (unused/off), PF3 = green LED.
 * PF4 = SW1, PF0 = SW2 (both active LOW, demo input only).
 */
#ifndef GPIO_LEDS_H
#define GPIO_LEDS_H

#include <stdbool.h>
#include <stdint.h>

#ifndef PEPS_MMIO32
#define PEPS_MMIO32(address) (*((volatile uint32_t *)(uintptr_t)(address)))
#endif

/* System Control: Run Mode Clock Gating Control / Peripheral Ready GPIO. */
#define PEPS_SYSCTL_RCGCGPIO_R       PEPS_MMIO32(0x400FE608UL)
#define PEPS_SYSCTL_PRGPIO_R         PEPS_MMIO32(0x400FEA08UL)
#define PEPS_GPIOF_CLOCK_BIT         (1UL << 5)

/* GPIO Port F register names follow the TM4C123GH6PM datasheet. */
#define PEPS_GPIO_PORTF_DATA_R       PEPS_MMIO32(0x400253FCUL)
#define PEPS_GPIO_PORTF_DIR_R        PEPS_MMIO32(0x40025400UL)
#define PEPS_GPIO_PORTF_AFSEL_R      PEPS_MMIO32(0x40025420UL)
#define PEPS_GPIO_PORTF_PUR_R        PEPS_MMIO32(0x40025510UL)
#define PEPS_GPIO_PORTF_DEN_R        PEPS_MMIO32(0x4002551CUL)
#define PEPS_GPIO_PORTF_LOCK_R       PEPS_MMIO32(0x40025520UL)
#define PEPS_GPIO_PORTF_CR_R         PEPS_MMIO32(0x40025524UL)
#define PEPS_GPIO_PORTF_AMSEL_R      PEPS_MMIO32(0x40025528UL)
#define PEPS_GPIO_PORTF_PCTL_R       PEPS_MMIO32(0x4002552CUL)

#define PEPS_LED_RED_MASK            (1UL << 1)  /* PF1 */
#define PEPS_LED_BLUE_MASK           (1UL << 2)  /* PF2 */
#define PEPS_LED_GREEN_MASK          (1UL << 3)  /* PF3 */
#define PEPS_LED_ALL_MASK            (PEPS_LED_RED_MASK | PEPS_LED_BLUE_MASK | PEPS_LED_GREEN_MASK)
#define PEPS_SW1_MASK                (1UL << 4)  /* PF4 */
#define PEPS_SW2_MASK                (1UL << 0)  /* PF0, protected pin */
#define PEPS_GPIO_UNLOCK_KEY         0x4C4F434BUL

/** Configure PF1, PF2 and PF3 as onboard RGB LED outputs. */
void GPIO_LEDs_Init(void);

/** Set locked (red) / unlocked (green) indications. */
void GPIO_SetLEDs(bool red_on, bool green_on);

/** Configure the two onboard active-low switches; demo mode only. */
void GPIO_DemoButtons_Init(void);

/** Return true while the indicated onboard switch is physically pressed. */
bool GPIO_IsSW1Pressed(void);
bool GPIO_IsSW2Pressed(void);

#endif /* GPIO_LEDS_H */
