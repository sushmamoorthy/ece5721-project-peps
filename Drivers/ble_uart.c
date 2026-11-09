#include "ble_uart.h"

#include "inc/hw_memmap.h"

#include "driverlib/uart.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"

/*
 * UART ISR writes at rx_head; the main loop consumes at rx_tail.
 * The ISR only moves bytes. It does not parse commands or change BCM outputs.
 */
static int16_t current_rssi = -100;
static bool device_connected = false;
static bool proximity_detected = false;

static void BLE_ProcessRxChar(uint8_t rx_char)
{
    /*
     * TODO: assemble the Bluefruit UART response from buffered characters.
     * When a complete response is recognized:
     *   - set device_connected on connect/disconnect events
     *   - parse RSSI and call BLE_UpdateRSSI()
     *   - report malformed/unknown responses through the debug UART if needed
     */
    (void)rx_char;
}

static void BLE_UpdateRSSI(int16_t rssi_dbm)
{
    current_rssi = rssi_dbm;

    /* Note: Require multiple reads so that a single noisy RSSI sample won't affect the proximity
     detection. */
  
}

/**
 * @brief Initialize UART1 for communication with the Bluefruit module.
 *
 * @details
 * Uses PB0/PB1 for UART receive/transmit and PC4/PC5 for RTS/CTS
 * hardware flow control. The UART runs at 9600 baud, 8 data bits,
 * no parity, and 1 stop bit.
 *
 * systemClockHz must reflect the actual clock before this is called.
 */
void BLE_UART_Init(uint32_t systemClockHz)
{
    /* Enable the peripherals used by UART1 and its pins. */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);

    /* Wait until each peripheral is ready to be configured. */
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB)) { }
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC)) { }
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART1)) { }

    /* PB0 receives from Bluefruit TX; PB1 transmits to Bluefruit RX. */
    GPIOPinConfigure(GPIO_PB0_U1RX);
    GPIOPinConfigure(GPIO_PB1_U1TX);
    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    /* PC4 and PC5 provide UART1 RTS and CTS flow control. */
    GPIOPinConfigure(GPIO_PC4_U1RTS);
    GPIOPinConfigure(GPIO_PC5_U1CTS);
    GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    UARTConfigSetExpClk(UART1_BASE, systemClockHz, 9600,
                        UART_CONFIG_WLEN_8 |
                        UART_CONFIG_STOP_ONE |
                        UART_CONFIG_PAR_NONE);

    UARTFlowControlSet(UART1_BASE,
                       UART_FLOWCONTROL_TX |
                       UART_FLOWCONTROL_RX);
}

void BLE_UART_OnByteReceived(uint8_t rx_char)
{
    /* Drop new data rather than overwrite unread bytes if the buffer is full. */
	(void)rx_char;
}

void BLE_UART_ProcessRx(void)
{
    /* TODO: Drain the RX ring buffer and pass each byte to BLE_ProcessRxChar() for parsing. */
}

void UART1_Handler(void)
{
    /*
     * TODO: read/clear UART1 interrupt status using the selected device header.
     * For each received byte, call BLE_UART_OnByteReceived(rx_char).
     */
}

bool BLE_IsProximityDetected(void)
{
    return device_connected && proximity_detected;
}

int16_t BLE_GetLatestRSSI(void)
{
    return current_rssi;
}

bool BLE_IsConnected(void)
{
    return device_connected;
}

/*
 * TODO: implement UART1_Handler using the selected Keil device header/vector.
 * It should clear the hardware interrupt and pass each received byte to
 * BLE_UART_OnByteReceived(); parsing remains in BLE_UART_ProcessRx().
 */

/**
 * @brief Check whether a received byte is waiting in UART1.
 * @return true if the Bluefruit module has sent a byte.
 */
bool BT_RX_Available(void)
{
    return UARTCharsAvail(UART1_BASE);
}

/**
 * @brief Read one received byte from UART1.
 * @pre BT_RX_Available() must return true before this function is called.
 * @return The received byte.
 */
uint8_t BT_RX_Read(void)
{
    return (uint8_t)UARTCharGet(UART1_BASE);
}

/**
 * @brief Try to send one byte through UART1 without waiting for FIFO space.
 * @param byte Byte to transmit to the Bluefruit module.
 * @return true if UART1 accepted the byte; false if its FIFO was full.
 */
bool BT_TX_TryByte(uint8_t byte)
{
    return UARTCharPutNonBlocking(UART1_BASE, byte);
}

/**
 * @brief Check whether UART1 still has an active transmission.
 * @return true while data is being transmitted or remains pending.
 */
bool BT_TX_Busy(void)
{
    return UARTBusy(UART1_BASE);
}



