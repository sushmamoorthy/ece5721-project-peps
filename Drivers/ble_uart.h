#ifndef BLE_UART_H
#define BLE_UART_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initialize UART1 for the Bluefruit module.
 * @param systemClockHz Actual system clock frequency in hertz.
 */
void BLE_UART_Init(uint32_t systemClockHz);

/*
 * Call in the main loop to parse bytes collected by the UART interrupt.
 * The Bluefruit response format still needs to be selected and implemented.
 */
void BLE_UART_ProcessRx(void);

bool BLE_IsProximityDetected(void);
int16_t BLE_GetLatestRSSI(void);
bool BLE_IsConnected(void);

/* Called by the UART ISR after reading one byte from the hardware UART. */
void BLE_UART_OnByteReceived(uint8_t rx_char);

/**
 * @brief Check whether UART1 has a byte waiting to be read.
 * @return true if a byte is available; otherwise false.
 */
 bool BT_RX_Available(void);
 
 /**
 * @brief Read one byte from UART1.
 * @pre Call only when BT_RX_Available() returns true.
 * @return The received byte.
 */
 uint8_t BT_RX_Read(void);
 
 /**
 * @brief Try to place one byte in the UART1 transmit FIFO without waiting.
 * @param byte Byte to send to the Bluefruit module.
 * @return true if the byte was accepted; false if the FIFO is full.
 */
 bool BT_TX_TryByte(uint8_t byte);
 
 /**
 * @brief Check whether UART1 is still transmitting data.
 * @return true while transmission is active or pending.
 */
bool BT_TX_Busy(void);

#endif
