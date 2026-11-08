/**
 * @file pwm_servo.h
 * @brief SG90 servo PWM pin/register definitions and driver interface.
 *
 * TM4C123GH6PM: PB6 is M0PWM0 (PWM0 module, generator 0, output A).
 * PB6 can be reached on LaunchPad J2 pin 7.
 */
#ifndef PWM_SERVO_H
#define PWM_SERVO_H

#include <stdbool.h>
#include <stdint.h>

#ifndef PEPS_MMIO32
#define PEPS_MMIO32(address) (*((volatile uint32_t *)(uintptr_t)(address)))
#endif

/* Servo calibration: verify positions without attaching the linkage first. */
#define PEPS_SERVO_PWM_FREQUENCY_HZ   50UL
#define PEPS_SERVO_PWM_DIVIDER        64UL
#define PEPS_SERVO_PERIOD_US          20000UL
#define PEPS_SERVO_FOLDED_US          1100UL
#define PEPS_SERVO_UNFOLDED_US        1900UL

/* System Control registers. */
#define PEPS_SYSCTL_RCC_R             PEPS_MMIO32(0x400FE060UL)
#define PEPS_SYSCTL_RCGCPWM_R         PEPS_MMIO32(0x400FE640UL)
#define PEPS_SYSCTL_PRPWM_R           PEPS_MMIO32(0x400FEA40UL)
#define PEPS_SYSCTL_RCGCGPIO_R       PEPS_MMIO32(0x400FE608UL)
#define PEPS_SYSCTL_PRGPIO_R         PEPS_MMIO32(0x400FEA08UL)
#define PEPS_SYSCTL_RCC_USEPWMDIV     (1UL << 20)
#define PEPS_SYSCTL_RCC_PWMDIV_M      (7UL << 17)
#define PEPS_SYSCTL_RCC_PWMDIV_64     (5UL << 17)

/* GPIOB (APB) alternate-function setup for PB6 = M0PWM0. */
#define PEPS_GPIO_PORTB_DIR_R         PEPS_MMIO32(0x40005400UL)
#define PEPS_GPIO_PORTB_AFSEL_R       PEPS_MMIO32(0x40005420UL)
#define PEPS_GPIO_PORTB_DEN_R         PEPS_MMIO32(0x4000551CUL)
#define PEPS_GPIO_PORTB_AMSEL_R       PEPS_MMIO32(0x40005528UL)
#define PEPS_GPIO_PORTB_PCTL_R        PEPS_MMIO32(0x4000552CUL)
#define PEPS_SERVO_PB6_MASK          (1UL << 6)

/* PWM0 hardware, Generator 0 (PWM0_0), output A. */
#define PEPS_PWM0_ENABLE_R            PEPS_MMIO32(0x40028008UL)
#define PEPS_PWM0_0_CTL_R             PEPS_MMIO32(0x40028040UL)
#define PEPS_PWM0_0_LOAD_R            PEPS_MMIO32(0x40028050UL)
#define PEPS_PWM0_0_CMPA_R            PEPS_MMIO32(0x40028058UL)
#define PEPS_PWM0_0_GENA_R            PEPS_MMIO32(0x40028060UL)
#define PEPS_PWM0_0_GENA_HI_LOAD_LO_CMPAD  0x8CUL

/** Initialize PWM0 generator 0 at 50 Hz; start in folded position. */
void PWM_Servo_Init(void);

/** unfold=true selects unfolded pulse; unfold=false selects folded pulse. */
void PWM_SetMirrorPosition(bool unfold);

#endif /* PWM_SERVO_H */
