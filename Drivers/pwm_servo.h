#ifndef PWM_SERVO_H
#define PWM_SERVO_H

#include <stdbool.h>

void PWM_Servo_Init(void);

/* unfold=true requests the mirror-open position; false requests folded. */
void PWM_SetMirrorPosition(bool unfold);

#endif
