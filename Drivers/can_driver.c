#include <stdbool.h>
#include <stdint.h>

#include "inc/hw_memmap.h"
#include "driverlib/can.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"

#include <TM4C123.h>

#include "can_driver.h"

extern void SystemCoreClockUpdate(void);

#define CAN_BCM_STATUS_ID       0x200U
#define CAN_BCM_STATUS_OBJECT   1U
#define CAN_BCM_STATUS_LENGTH   2U
#define CAN_BIT_RATE            500000U

/**
 * @brief Initialize CAN0 on PE4/PE5.
 *
 * @details Configures classic CAN at 500 kbit/s using the active system
 * clock. PE4 is CAN0RX and PE5 is CAN0TX; connect these logic-level pins to
 * the CAN transceiver, not directly to the differential CAN bus. This pin
 * mapping must match the project wiring.
 */
void CAN_Driver_Init(void)
{
    /* Keep CAN timing aligned with the clock configured by the CMSIS startup. */
    SystemCoreClockUpdate();

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);

    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)) { }
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_CAN0)) { }

    GPIOPinConfigure(GPIO_PE4_CAN0RX);
    GPIOPinConfigure(GPIO_PE5_CAN0TX);
    GPIOPinTypeCAN(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    CANInit(CAN0_BASE);
    CANBitRateSet(CAN0_BASE, SystemCoreClock, CAN_BIT_RATE);
    CANEnable(CAN0_BASE);
}

/**
 * @brief Transmit the current lock and mirror state on the CAN bus.
 *
 * @details Sends a standard 11-bit frame with ID 0x200 and two data bytes:
 * byte 0 is lock state, byte 1 is mirror state; each uses 0 for locked/folded
 * and 1 for unlocked/unfolded.
 */
void CAN_SendBCMStatus(uint8_t lock_state, uint8_t mirror_state)
{
    uint8_t msg_data[CAN_BCM_STATUS_LENGTH];
    tCANMsgObject tx_object;

    msg_data[0] = lock_state;
    msg_data[1] = mirror_state;

    tx_object.ui32MsgID = CAN_BCM_STATUS_ID;
    tx_object.ui32MsgIDMask = 0U;
    tx_object.ui32Flags = 0U;
    tx_object.ui32MsgLen = CAN_BCM_STATUS_LENGTH;
    tx_object.pui8MsgData = msg_data;

    CANMessageSet(CAN0_BASE, CAN_BCM_STATUS_OBJECT, &tx_object,
                  MSG_OBJ_TYPE_TX);
}
