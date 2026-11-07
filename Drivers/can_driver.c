#include "can_driver.h"

void CAN_Driver_Init(void)
{
    /*
     * TODO after CAN is covered in class:
     * - initialize the TM4C123 CAN controller and message object
     * - configure the CAN pins shown in the project system diagram (PE4/PE5)
     * - confirm the bit rate and frame format with the CAN receiver/analyzer
     * - connect CAN TX/RX to the SN65HVD230 logic-side pins
     *
     * The SN65HVD230 is a transceiver, not the CAN controller.
     * The receiver is either a USB-to-CAN analyzer or optional Node B.
     */
}

void CAN_SendBCMStatus(uint8_t lock_state, uint8_t mirror_state)
{
    uint8_t msg_data[2];

    msg_data[0] = lock_state;   /* Draft convention: 0=locked, 1=unlocked. */
    msg_data[1] = mirror_state; /* Draft convention: 0=folded, 1=unfolded. */

    /*
     * TODO: place msg_data in a CAN transmit message object and request send.
     * Come up with a suitable CAN identifiers during implementation.
     */
    (void)msg_data;
}
