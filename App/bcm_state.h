#ifndef BCM_STATE_H
#define BCM_STATE_H

#include <stdint.h>

/* BCM operating states. */
typedef enum {
    STATE_LOCKED = 0,
    STATE_UNLOCKED
} SystemState_t;

void BCM_Init(void);
void BCM_ProcessState(void);

/* Called every 1 ms by SysTick */
void BCM_OnTimerTick(void);

#endif
