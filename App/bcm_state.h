/**
 * @file bcm_state.h
 * @brief PEPS Body Control Module (BCM) state machine configuration and API.
 *
 * DEMO ONLY: set BCM_DEMO_MODE to 1 to simulate BLE using the LaunchPad
 * switches. Set it to 0 after your teammate's real BLE UART driver works.
 * App/main.c does NOT need to be edited for either setting.
 */
#ifndef BCM_STATE_H
#define BCM_STATE_H

#include <stdint.h>

/* Change this ONE setting to switch between simulated and real BLE. */
#define BCM_DEMO_MODE                 0U

/* Timeout and demonstration assumptions. */
#define BCM_DISCONNECT_TIMEOUT_MS    2000U
#define BCM_BUTTON_DEBOUNCE_MS       15U
#define BCM_DEMO_NEAR_CONFIRM_MS     300U
#define BCM_DEMO_NEAR_RSSI_DBM       (-55)
#define BCM_DEMO_FAR_RSSI_DBM        (-85)
#define BCM_DEMO_DISCONNECTED_DBM    (-100)
#define BCM_DEMO_THRESHOLD_DBM       (-70)

/* State values match the team's original project interface. */
typedef enum {
    STATE_LOCKED = 0,
    STATE_UNLOCKED
} SystemState_t;

/** Initialize locked state and the outputs (red LED, folded mirror). */
void BCM_Init(void);

/**
 * @brief Process BLE status, or switch inputs in demo mode.
 * @param nowMs Current application time in milliseconds.
 */
void BCM_ProcessState(uint32_t nowMs);

/** Called once per millisecond by the EXISTING SysTick_Handler in main.c. */
void BCM_OnTimerTick(void);

#if BCM_DEMO_MODE
/* Watch these symbols in Keil to diagnose switch inputs and state transitions. */
extern volatile uint32_t BCM_DemoSW1PressCount;
extern volatile uint32_t BCM_DemoSW2PressCount;
extern volatile int32_t BCM_DemoRSSI;
extern volatile uint32_t BCM_DemoRemainingTimeoutMs;
extern volatile uint32_t BCM_DemoState;  /* 0 = locked, 1 = unlocked */
#endif

#endif /* BCM_STATE_H */
