/**
 * @file gpio_leds.c
 * @brief Drive onboard red/green LEDs and read demo input switches.
 */
#include "gpio_leds.h"

/**
 * @brief Configure PF1 (red), PF2 (blue off), and PF3 (green) as GPIO outputs.
 * @details SYSCTL_RCGCGPIO enables the GPIOF clock. GPIOF_AFSEL/PCTL select
 * ordinary GPIO, GPIOF_DIR selects outputs, and GPIOF_DEN enables digital I/O.
 * No external LEDs or resistors are necessary for this LaunchPad.
 */
void GPIO_LEDs_Init(void)
{
    PEPS_SYSCTL_RCGCGPIO_R |= PEPS_GPIOF_CLOCK_BIT;
    while ((PEPS_SYSCTL_PRGPIO_R & PEPS_GPIOF_CLOCK_BIT) == 0UL) { }

    PEPS_GPIO_PORTF_AFSEL_R &= ~PEPS_LED_ALL_MASK;
    PEPS_GPIO_PORTF_AMSEL_R &= ~PEPS_LED_ALL_MASK;
    PEPS_GPIO_PORTF_PCTL_R &= ~((0xFUL << 4) | (0xFUL << 8) | (0xFUL << 12));
    PEPS_GPIO_PORTF_DIR_R |= PEPS_LED_ALL_MASK;
    PEPS_GPIO_PORTF_DEN_R |= PEPS_LED_ALL_MASK;

    GPIO_SetLEDs(true, false); /* Safe default: red/locked; blue off. */
}

/**
 * @brief Set the visible BCM status on the onboard LED.
 * @param red_on true lights PF1 red; false turns it off.
 * @param green_on true lights PF3 green; false turns it off.
 * @details Only the RGB LED bits are modified; switch/input pins are preserved.
 */
void GPIO_SetLEDs(bool red_on, bool green_on)
{
    uint32_t outputs = 0UL;
    if (red_on)   { outputs |= PEPS_LED_RED_MASK; }
    if (green_on) { outputs |= PEPS_LED_GREEN_MASK; }

    PEPS_GPIO_PORTF_DATA_R =
        (PEPS_GPIO_PORTF_DATA_R & ~PEPS_LED_ALL_MASK) | outputs;
}

/**
 * @brief Configure SW1/PF4 and SW2/PF0 as active-low demo inputs.
 * @details PF0 is NMI-protected on reset, so GPIOF_LOCK must be unlocked and
 * GPIOF_CR bit 0 committed before setting ordinary input/pull-up functions.
 */
void GPIO_DemoButtons_Init(void)
{
    const uint32_t buttons = PEPS_SW1_MASK | PEPS_SW2_MASK;

    PEPS_SYSCTL_RCGCGPIO_R |= PEPS_GPIOF_CLOCK_BIT;
    while ((PEPS_SYSCTL_PRGPIO_R & PEPS_GPIOF_CLOCK_BIT) == 0UL) { }

    PEPS_GPIO_PORTF_LOCK_R = PEPS_GPIO_UNLOCK_KEY;
    PEPS_GPIO_PORTF_CR_R |= PEPS_SW2_MASK;
    PEPS_GPIO_PORTF_AFSEL_R &= ~buttons;
    PEPS_GPIO_PORTF_AMSEL_R &= ~buttons;
    PEPS_GPIO_PORTF_PCTL_R &= ~((0xFUL << 0) | (0xFUL << 16));
    PEPS_GPIO_PORTF_DIR_R &= ~buttons;
    PEPS_GPIO_PORTF_PUR_R |= buttons;
    PEPS_GPIO_PORTF_DEN_R |= buttons;
    PEPS_GPIO_PORTF_LOCK_R = 0UL; /* Optional: re-lock PF0 configuration. */
}

/** @brief SW1 (PF4) reads 0 when pressed because the pull-up is enabled. */
bool GPIO_IsSW1Pressed(void)
{
    return (PEPS_GPIO_PORTF_DATA_R & PEPS_SW1_MASK) == 0UL;
}

/** @brief SW2 (PF0) reads 0 when pressed because the pull-up is enabled. */
bool GPIO_IsSW2Pressed(void)
{
    return (PEPS_GPIO_PORTF_DATA_R & PEPS_SW2_MASK) == 0UL;
}
