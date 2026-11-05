#ifndef CAN_DRIVER_H
#define CAN_DRIVER_H

#include <stdint.h>

void CAN_Driver_Init(void);

/* Draft payload: 0=locked/folded and 1=unlocked/unfolded. */
void CAN_SendBCMStatus(uint8_t lock_state, uint8_t mirror_state);

#endif
