/**
 * @file bcm_state.c
 * @brief Implements the PEPS body control module state machine.
 *
 * BLE messages are processed in the main loop, where connection and proximity
 * status determine whether the system remains locked or becomes unlocked.
 * SysTick calls BCM_OnTimerTick() to count down the walk-away timeout.
 */

#include "bcm_state.h"

#include <stdbool.h>

#include "../Drivers/ble_uart.h"
#include "../Drivers/can_driver.h"
#include "../Drivers/gpio_leds.h"
#include "../Drivers/pwm_servo.h"

/* Delay before locking after proximity is lost, in milliseconds. */
#define DISCONNECT_TIMEOUT_MS 2000U

/* Current BCM operating state. */
static SystemState_t current_state = STATE_LOCKED;

/* Remaining walk-away timeout, decremented by the SysTick handler. */
static volatile uint32_t disconnect_timer_ms = 0U;

/**
 * @brief Apply outputs corresponding to the requested lock state.
 * @param unlocked true to indicate unlocked; false to indicate locked.
 */
static void BCM_ApplyStateOutputs(bool unlocked)
{
    /* Keep all physical outputs and the network report in sync. */
    GPIO_SetLEDs(!unlocked, unlocked);
    PWM_SetMirrorPosition(unlocked);
    CAN_SendBCMStatus((uint8_t)!unlocked, (uint8_t)unlocked);
}

/**
 * @brief Change the BCM state and update outputs when the state changes.
 * @param new_state State to enter.
 */
static void BCM_SetState(SystemState_t new_state)
{
    if (new_state == current_state) {
        return;
    }

    current_state = new_state;
    BCM_ApplyStateOutputs(new_state == STATE_UNLOCKED);
}

/**
 * @brief Lock the BCM and cancel the walk-away timeout.
 */
static void BCM_Lock(void)
{
    disconnect_timer_ms = 0U;
    BCM_SetState(STATE_LOCKED);
}

/**
 * @brief Unlock after the BLE connection and proximity conditions are met.
 */
static void BCM_TryUnlock(void)
{
    disconnect_timer_ms = DISCONNECT_TIMEOUT_MS;
    BCM_SetState(STATE_UNLOCKED);
}

/**
 * @brief Initialize the state machine and establish locked outputs.
 *
 * The LEDs, mirror, and CAN status are set to their locked baseline.
 */
void BCM_Init(void)
{
    current_state = STATE_LOCKED;
    disconnect_timer_ms = 0U;

    /* Establish the locked baseline once at startup. */
    GPIO_SetLEDs(true, false);
    PWM_SetMirrorPosition(false);
    CAN_SendBCMStatus(0U, 0U);
}

/**
 * @brief Decrement the walk-away timeout by one millisecond when active.
 *
 * Called from SysTick every millisecond. 
 */
void BCM_OnTimerTick(void)
{
    if (disconnect_timer_ms > 0U) {
        disconnect_timer_ms--;
    }
}

/**
 * @brief Process pending BLE input and advance the BCM state machine.
 *
 * In the unlocked state, an explicit BLE disconnect locks immediately.
 * If BLE remains connected but proximity is lost, the BCM locks after
 * DISCONNECT_TIMEOUT_MS unless proximity returns and refreshes the timer.
 */
void BCM_ProcessState(void)
{
    /*
     * The ISR only buffers UART bytes. Parse them here, then make decisions
     * outside interrupt context.
     */
    BLE_UART_ProcessRx();

    switch (current_state) {
    case STATE_LOCKED:
        if (BLE_IsConnected() && BLE_IsProximityDetected()) {
            BCM_TryUnlock();
        }
        break;

    case STATE_UNLOCKED:
        if (!BLE_IsConnected()) {
            /* An explicit app disconnect locks immediately. */
            BCM_Lock();
        } else if (BLE_IsProximityDetected()) {
            /* Near RSSI refreshes the 2-second no-activity timer. */
            disconnect_timer_ms = DISCONNECT_TIMEOUT_MS;
        } else if (disconnect_timer_ms == 0U) {
            /* Stable far RSSI or missing RSSI eventually triggers walk-away lock. */
            BCM_Lock();
        }
        break;

    default:
        BCM_Lock();
        break;
    }
}
