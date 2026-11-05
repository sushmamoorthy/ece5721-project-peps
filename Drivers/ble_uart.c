#include "ble_uart.h"

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

void BLE_UART_Init(void)
{
    /*
     * TODO: configure UART1 and Port B for the project diagram's PB0/PB1.
     * Confirm the LaunchPad header mapping and module TX/RX connections.
     * Select the actual module baud rate; 
     * Enable RX/receive-timeout interrupts after the receive buffer is ready.
     */
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
