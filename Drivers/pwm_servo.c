/**
 * @file pwm_servo.c
 * @brief Control an SG90 mirror servo from PB6 / M0PWM0 at 50 Hz.
 *
 * Power the SG90 from a suitable external regulated 5 V supply, and share
 * supply GND with the LaunchPad. Do not connect servo power to a GPIO pin.
 */
#include <TM4C123.h>  /* Keil device pack: SystemCoreClock. */
#include "pwm_servo.h"

static uint32_t servo_period_ticks = 0UL;
static bool servo_initialized = false;

/**
 * @brief Convert servo high-pulse width into PWM0 generator 0 comparator A.
 * @param pulse_us High-pulse length, in microseconds.
 * @return PWM0_0_CMPA value for down-counting PWM operation.
 */
static uint32_t PWM_CompareFromPulse(uint32_t pulse_us)
{
    uint32_t high_ticks;

    high_ticks = (servo_period_ticks * pulse_us) / PEPS_SERVO_PERIOD_US;
    if (high_ticks < 2UL) { high_ticks = 2UL; }
    if (high_ticks > (servo_period_ticks - 2UL)) {
        high_ticks = servo_period_ticks - 2UL;
    }
    return (servo_period_ticks - 1UL) - high_ticks;
}

/**
 * @brief Enable PWM0 generator 0 output A on PB6 and establish folded position.
 * @details SYSCTL_RCGCPWM/RCGCGPIO enable clocks; GPIOB_AFSEL/PCTL select
 * M0PWM0; SYSCTL_RCC sets PWM divider /64; PWM0_0_LOAD specifies 20 ms;
 * PWM0_0_GENA drives high on LOAD and low on CMPA down-count match.
 * Main calls SystemCoreClockUpdate() before this initialization.
 */
void PWM_Servo_Init(void)
{
    uint32_t pwm_clock_hz;

    servo_initialized = false;
    PEPS_SYSCTL_RCGCGPIO_R |= (1UL << 1); /* Clock to GPIO Port B. */
    PEPS_SYSCTL_RCGCPWM_R  |= (1UL << 0); /* Clock to PWM0. */
    while ((PEPS_SYSCTL_PRGPIO_R & (1UL << 1)) == 0UL) { }
    while ((PEPS_SYSCTL_PRPWM_R  & (1UL << 0)) == 0UL) { }

    /* RCC: USEPWMDIV = 1, PWMDIV = 5 => PWM clock = system clock / 64. */
    PEPS_SYSCTL_RCC_R =
        (PEPS_SYSCTL_RCC_R & ~PEPS_SYSCTL_RCC_PWMDIV_M) |
        PEPS_SYSCTL_RCC_USEPWMDIV | PEPS_SYSCTL_RCC_PWMDIV_64;

    PEPS_GPIO_PORTB_AFSEL_R |= PEPS_SERVO_PB6_MASK;
    PEPS_GPIO_PORTB_AMSEL_R &= ~PEPS_SERVO_PB6_MASK;
    PEPS_GPIO_PORTB_PCTL_R =
        (PEPS_GPIO_PORTB_PCTL_R & ~(0xFUL << 24)) | (0x4UL << 24);
    PEPS_GPIO_PORTB_DIR_R |= PEPS_SERVO_PB6_MASK;
    PEPS_GPIO_PORTB_DEN_R |= PEPS_SERVO_PB6_MASK;

    pwm_clock_hz = SystemCoreClock / PEPS_SERVO_PWM_DIVIDER;
    servo_period_ticks = pwm_clock_hz / PEPS_SERVO_PWM_FREQUENCY_HZ;

    /* The generator uses a 16-bit LOAD register (up to 65536 ticks). */
    if ((servo_period_ticks < 100UL) || (servo_period_ticks > 65536UL)) {
        return; /* Check system clock and Keil target configuration. */
    }

    PEPS_PWM0_ENABLE_R &= ~(1UL << 0); /* Disable PWM output 0 while setting up. */
    PEPS_PWM0_0_CTL_R = 0UL;           /* Generator stopped, count-down mode. */
    PEPS_PWM0_0_GENA_R = PEPS_PWM0_0_GENA_HI_LOAD_LO_CMPAD;
    PEPS_PWM0_0_LOAD_R = servo_period_ticks - 1UL;
    PEPS_PWM0_0_CMPA_R = PWM_CompareFromPulse(PEPS_SERVO_FOLDED_US);

    servo_initialized = true;
    PEPS_PWM0_0_CTL_R |= (1UL << 0); /* Start generator 0. */
    PEPS_PWM0_ENABLE_R |= (1UL << 0); /* Enable PB6 / M0PWM0 output. */
}

/**
 * @brief Select folded or unfolded position, without stopping the PWM.
 * @param unfold true = unfolded, false = folded.
 * @details Calibrate pulse widths in pwm_servo.h before attaching linkage.
 */
void PWM_SetMirrorPosition(bool unfold)
{
    uint32_t pulse_us;
    if (!servo_initialized) { return; }

    pulse_us = unfold ? PEPS_SERVO_UNFOLDED_US : PEPS_SERVO_FOLDED_US;
    PEPS_PWM0_0_CMPA_R = PWM_CompareFromPulse(pulse_us);
}
