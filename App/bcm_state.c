/**
 * @file bcm_state.c
 * @brief PEPS Body Control Module (BCM) locked/unlocked state machine.
 *
 * The BCM does not measure radio signal strength itself. In real-BLE mode it
 * reads the Bluetooth UART driver's connection/proximity status. In demo mode
 * it simulates those inputs with onboard SW1 and SW2. The resulting state
 * drives the separate PWM servo, onboard LEDs, and CAN status interface.
 *
 * SysTick in the team's UNCHANGED App/main.c calls BCM_OnTimerTick() at 1 ms.
 */
#include "bcm_state.h"
#include <stdbool.h>

#include "../Drivers/bluefruit.h"
#include "../Drivers/can_driver.h"
#include "../Drivers/gpio_leds.h"
#include "../Drivers/pwm_servo.h"

static SystemState_t current_state = STATE_LOCKED;
static volatile uint32_t disconnect_timer_ms = 0U;

#if BCM_DEMO_MODE
/* Demo clock supplied by the existing 1 ms SysTick interrupt. */
static volatile uint32_t demo_time_ms = 0U;

/* Keil Watch variables: count switch events and show the active decision.
 * Globals are deliberately volatile so they remain visible while debugging. */
volatile uint32_t BCM_DemoSW1PressCount = 0U;
volatile uint32_t BCM_DemoSW2PressCount = 0U;
volatile int32_t BCM_DemoRSSI = BCM_DEMO_DISCONNECTED_DBM;
volatile uint32_t BCM_DemoRemainingTimeoutMs = 0U;
volatile uint32_t BCM_DemoState = (uint32_t)STATE_LOCKED;

typedef struct {
    bool candidate_pressed;  /* Instantaneous button level. */
    bool stable_pressed;     /* Level after the debounce interval. */
    uint32_t candidate_since_ms;
} BCM_DemoButton_t;

static BCM_DemoButton_t demo_sw1;
static BCM_DemoButton_t demo_sw2;
static bool demo_connected = false;
static bool demo_proximity_confirmed = false;
static int16_t demo_rssi_dbm = BCM_DEMO_DISCONNECTED_DBM;
static uint32_t demo_near_started_ms = 0U;

/**
 * @brief Report one event when the active-low button becomes stably pressed.
 * @param button Debounce state for one switch.
 * @param pressed Current button condition from GPIO (true = held down).
 * @param now Millisecond time read from SysTick-driven demo_time_ms.
 * @return true for a debounced press edge, false otherwise.
 */
static bool BCM_DemoButtonPressEvent(BCM_DemoButton_t *button,
                                      bool pressed, uint32_t now)
{
    if (pressed != button->candidate_pressed) {
        button->candidate_pressed = pressed;
        button->candidate_since_ms = now;
    }

    if ((button->stable_pressed != button->candidate_pressed) &&
        ((uint32_t)(now - button->candidate_since_ms) >= BCM_BUTTON_DEBOUNCE_MS)) {
        button->stable_pressed = button->candidate_pressed;
        return button->stable_pressed; /* Only the press edge creates an event. */
    }
    return false;
}

/**
 * @brief Convert SW1/SW2 presses into simulated Bluetooth input conditions.
 * @details SW1 toggles near/far. SW2 simulates an immediate disconnect.
 * 'Near' becomes qualified after BCM_DEMO_NEAR_CONFIRM_MS, simulating stable
 * multiple proximity measurements. No Bluetooth radio is used in this mode.
 */
static void BCM_DemoPollInputs(void)
{
    const uint32_t now = demo_time_ms;
    bool near_event;
    bool disconnect_event;

    near_event = BCM_DemoButtonPressEvent(&demo_sw1,
                                          GPIO_IsSW1Pressed(), now);
    disconnect_event = BCM_DemoButtonPressEvent(&demo_sw2,
                                                GPIO_IsSW2Pressed(), now);

    if (near_event) {
        BCM_DemoSW1PressCount++;
        if (demo_connected && (demo_rssi_dbm == BCM_DEMO_NEAR_RSSI_DBM)) {
            /* SW1 second press: still connected, phone moves out of range. */
            demo_rssi_dbm = BCM_DEMO_FAR_RSSI_DBM;
            demo_proximity_confirmed = false;
        } else {
            /* SW1 first press, or press after disconnect: reconnect near. */
            demo_connected = true;
            demo_rssi_dbm = BCM_DEMO_NEAR_RSSI_DBM;
            demo_near_started_ms = now;
            demo_proximity_confirmed = false;
        }
    }

    /* Disconnect takes priority if both switches are pressed together. */
    if (disconnect_event) {
        BCM_DemoSW2PressCount++;
        demo_connected = false;
        demo_rssi_dbm = BCM_DEMO_DISCONNECTED_DBM;
        demo_proximity_confirmed = false;
    }

    if (demo_connected &&
        (demo_rssi_dbm >= BCM_DEMO_THRESHOLD_DBM) &&
        ((uint32_t)(now - demo_near_started_ms) >= BCM_DEMO_NEAR_CONFIRM_MS)) {
        demo_proximity_confirmed = true;
    }
    BCM_DemoRSSI = (int32_t)demo_rssi_dbm;
}
#endif /* BCM_DEMO_MODE */

/**
 * @brief Apply locked/unlocked actuator, indicator and CAN output values.
 * @param unlocked true = green/unfolded; false = red/folded.
 * @details CAN convention matches can_driver.h: 0=locked/folded,
 * 1=unlocked/unfolded. Note: CAN driver may still be a placeholder.
 */
static void BCM_ApplyStateOutputs(bool unlocked)
{
    GPIO_SetLEDs(!unlocked, unlocked);
    PWM_SetMirrorPosition(unlocked);
    CAN_SendBCMStatus((uint8_t)unlocked, (uint8_t)unlocked);
}

/** @brief Change state only if it differs from the previous state. */
static void BCM_SetState(SystemState_t next_state)
{
    if (next_state == current_state) { return; }
    current_state = next_state;
#if BCM_DEMO_MODE
    BCM_DemoState = (uint32_t)current_state;
#endif
    BCM_ApplyStateOutputs(next_state == STATE_UNLOCKED);
}

/** @brief Return to locked state and clear any pending walk-away timeout. */
static void BCM_Lock(void)
{
    disconnect_timer_ms = 0U;
    BCM_SetState(STATE_LOCKED);
}

/** @brief Unlock the BCM when BLE is connected and stable near is detected. */
static void BCM_TryUnlock(void)
{
    disconnect_timer_ms = BCM_DISCONNECT_TIMEOUT_MS;
    BCM_SetState(STATE_UNLOCKED);
}

/**
 * @brief Initialize state, demo switches (if enabled), and locked outputs.
 * @details Called by the existing main() after GPIO_LEDs_Init() and
 * PWM_Servo_Init(). On startup mirror is folded and LED is red.
 */
void BCM_Init(void)
{
    current_state = STATE_LOCKED;
    disconnect_timer_ms = 0U;

#if BCM_DEMO_MODE
    GPIO_DemoButtons_Init();
    demo_time_ms = 0U;
    BCM_DemoSW1PressCount = 0U;
    BCM_DemoSW2PressCount = 0U;
    BCM_DemoRSSI = BCM_DEMO_DISCONNECTED_DBM;
    BCM_DemoRemainingTimeoutMs = 0U;
    BCM_DemoState = (uint32_t)STATE_LOCKED;
    demo_connected = false;
    demo_proximity_confirmed = false;
    demo_rssi_dbm = BCM_DEMO_DISCONNECTED_DBM;
    demo_near_started_ms = 0U;

    demo_sw1.candidate_pressed = GPIO_IsSW1Pressed();
    demo_sw1.stable_pressed = demo_sw1.candidate_pressed;
    demo_sw1.candidate_since_ms = 0U;
    demo_sw2.candidate_pressed = GPIO_IsSW2Pressed();
    demo_sw2.stable_pressed = demo_sw2.candidate_pressed;
    demo_sw2.candidate_since_ms = 0U;
#endif

    GPIO_SetLEDs(true, false);
    PWM_SetMirrorPosition(false);
    CAN_SendBCMStatus(0U, 0U);
}

/**
 * @brief SysTick callback: one elapsed millisecond.
 * @details Keeps the walk-away countdown independent of main-loop speed.
 */
void BCM_OnTimerTick(void)
{
    if (disconnect_timer_ms > 0U) {
        disconnect_timer_ms--;
    }
#if BCM_DEMO_MODE
    BCM_DemoRemainingTimeoutMs = disconnect_timer_ms;
#endif
#if BCM_DEMO_MODE
    demo_time_ms++;
#endif
}

/**
 * @brief Process connection/proximity input and run BCM state machine.
 * @details Real mode: BLE_UART_ProcessRx() reads actual UART events.
 * Demo mode: SW1 toggles near/far; SW2 disconnects. A close connection
 * unlocks; walking away locks after two seconds; disconnect locks as soon
 * as the switch event passes its 30 ms debounce interval.
 */
void BCM_ProcessState(uint32_t nowMs)
{
    bool connected;
    bool near;

#if BCM_DEMO_MODE
    (void)nowMs;
    BCM_DemoPollInputs();
    connected = demo_connected;
    near = connected && demo_proximity_confirmed;
#else
    BluefruitStatus status = Bluefruit_GetStatus(nowMs);

    /*
     * Only confirmed NEAR or FAR counts as a usable connection.
     * Unknown, disconnected, stale, and fault all lock safely.
     */
    connected = (status == BLUEFRUIT_STATUS_NEAR ||
                 status == BLUEFRUIT_STATUS_FAR);
    near = (status == BLUEFRUIT_STATUS_NEAR);
#endif

    switch (current_state) {
    case STATE_LOCKED:
        if (connected && near) {
            BCM_TryUnlock();
        }
        break;

    case STATE_UNLOCKED:
        if (!connected) {
            BCM_Lock(); /* Explicit disconnect: no 2-second wait. */
        } else if (near) {
            disconnect_timer_ms = BCM_DISCONNECT_TIMEOUT_MS;
        } else if (disconnect_timer_ms == 0U) {
            BCM_Lock(); /* Out of range for 2 seconds. */
        }
        break;

    default:
        BCM_Lock();
        break;
    }
}
