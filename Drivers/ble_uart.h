/* Final Project Files
 * Filename: ble_uart.h
 * Author: Mrunalini Desai
 * Date: 11/04/2026
 *
 * Description: Provides functions and definitions for BLE UART communication.
 */

#ifndef BLE_UART_H
#define BLE_UART_H

#include <stdint.h>
#include <stdbool.h>

void BT_Init(uint32_t systemClockHz);
bool BT_RX_Available(void);
uint8_t BT_RX_Read(void);
void BT_TX_Byte(uint8_t byte);
void BT_TX_String(const char *text);
bool BT_TX_TryByte(uint8_t byte);
bool BT_TX_Busy(void);


#endif // BLE_UART_H
