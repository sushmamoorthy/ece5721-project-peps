#include "pwm_servo.h"

void PWM_Servo_Init(void)
{
    /*
     * TODO: configure the selected TM4C123 PWM generator and output pin.
     * SG90 uses a roughly <XX> Hz?? control period; confirm the timer clock/divider.
     * Start folded, then calibrate safe pulse widths for the actual linkage.
     *
     * Power the servo from its suitable external supply(in our case VBUS) and 
     * share ground with the MCU board GND. While integration, if multiple components are used,
     * ensure they all share the same ground reference.
     * 
     */
}

void PWM_SetMirrorPosition(bool unfold)
{
    if (unfold) {
        /* TODO: write the calibrated unfolded-position PWM compare value. */
    } else {
        /* TODO: write the calibrated folded-position PWM compare value. */
    }
}
