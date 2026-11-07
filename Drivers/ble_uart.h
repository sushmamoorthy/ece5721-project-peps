#ifndef BLE_UART_H
#define BLE_UART_H

#include <stdbool.h>
#include <stdint.h>

void BLE_UART_Init(void);

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

#endif
