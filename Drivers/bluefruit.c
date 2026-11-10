/* Final Project Files
 * Filename: bluefruit.c
 * Author: Mrunalini Desai
 * Date: 11/04/2026
 *
 * Description: Provides functions and definitions for BLUEFRUIT Module.
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "inc/hw_memmap.h"
#include "driverlib/uart.h"
#include "ble_uart.h"
#include "bluefruit.h"

/* Maximum response storage, including the terminating '\0' character. */
#define BLUEFRUIT_LINE_CAPACITY 64U

/* Maximum time allowed for one AT transaction phase. */
#define BLUEFRUIT_RESPONSE_TIMEOUT_MS 2000UL

/* Minimum delay after a completed poll before starting the next one. */
#define RSSI_POLL_INTERVAL_MS 1000UL

/* Maximum age of an RSSI observation before it becomes unusable. */
#define RSSI_FRESHNESS_MS 2500UL

/* A reading of -55 dBm or stronger counts toward entering NEAR. */
#define RSSI_NEAR_ENTER_DBM (-55)

/* While NEAR, a reading of -65 dBm or weaker counts toward FAR. */
#define RSSI_NEAR_EXIT_DBM (-65)

/* Consecutive qualifying samples required to enter NEAR. */
#define RSSI_NEAR_REQUIRED 3U

/* Consecutive qualifying samples required to leave NEAR for FAR. */
#define RSSI_FAR_REQUIRED 2U

/* Set to 1 to print additional AT transaction and response traces. */
#define BLUEFRUIT_TRACE_ENABLED 0U

/*
 * AT transaction phases. These describe communication with the module;
 * they are separate from the BCM's locked/unlocked states.
 */
typedef enum
{
    BLUEFRUIT_DATA_IDLE,                     //Ready to start a new transaction
    BLUEFRUIT_WAIT_COMMAND_MODE,             //waiting for command-mode confirmation
    BLUEFRUIT_WAIT_RSSI,                     //waiting for RSSI value and result
    BLUEFRUIT_WAIT_DATA_MODE,                //waiting for data-mode confirmation
    BLUEFRUIT_FAULT                          //fault latched;polling does not restart
} bluefruitState;


/* Current AT transaction phase. */
static bluefruitState serviceState = BLUEFRUIT_DATA_IDLE;

/* Start time of the current AT phase, used for its timeout. */
static uint32_t transactionStartedMs = 0;

/* True after the expected mode value is received, before its OK. */
static bool modeReported = false;


/* Buffer and current length for one line received from UART1. */
static char responseLine[BLUEFRUIT_LINE_CAPACITY];
static uint32_t responseLength = 0;

/* True while ignoring the remainder of an invalid or oversized line. */
static bool discardingLine = false;

/* Candidate RSSI received during the current transaction. */
static bool rssiReported = false;
static int32_t candidateRSSI = 0;


/* Last accepted RSSI and the time at which its value was received. */
static bool sampleValid = false;
static int32_t lastRSSI = 0;
static uint32_t lastSampleTimeMs = 0;

/* Next character to queue for UART1; null means no pending command. */
static const char *pendingTX = 0;


/* Poll scheduling time and timestamp of the current candidate RSSI. */
static uint32_t lastPollTimeMs = 0;
static uint32_t canditateSampleTimeMs = 0;


/* Near/far classifier state and consecutive qualifying sample counts. */
static bool proximityNear = false;
static uint32_t nearSampleCount = 0;
static uint32_t farSampleCount = 0;

/* Last confirmed proximity or connection status. */
static BluefruitStatus currentStatus = BLUEFRUIT_STATUS_UNKNOWN;


/* Prevents printing the same disconnected message on every poll. */
static bool disconnectedReported = false;

/* Private helpers follow the public main-loop and status functions below. */
static void Bluefruit_ProcessLine(const char *text, uint32_t nowMs);
static void Bluefruit_RestoreDataMode(uint32_t nowMs);
static bool Bluefruit_ParseRSSI(const char *text, int32_t *value);
static void Bluefruit_QualifySample(uint32_t nowMs);
static void Bluefruit_InvalidateProximity(void);
static bool Bluefruit_TransactionActive(void);
static void Bluefruit_StartTX(const char *text);
static void Bluefruit_ServiceTX(void);
static void Bluefruit_Fail(const char *reason);
static void Bluefruit_Trace(const char *text);
static void Debug_TX_String(const char *text);

/** @brief Service the transaction timeout, TX, and at most one RX byte.
 *  @param nowMs Current application time in milliseconds. */
void Bluefruit_Monitor(uint32_t nowMs)
{
    uint8_t byte;

    if (Bluefruit_TransactionActive() &&
        (uint32_t)(nowMs - transactionStartedMs) >= BLUEFRUIT_RESPONSE_TIMEOUT_MS)
    {
        /* Capture the UART state before reporting the timeout. */
        bool txPending = (pendingTX != 0);
        bool txHasCharacters = txPending && (*pendingTX != '\0');
        bool txBusy = BT_TX_Busy();
        bool rxAvailable = BT_RX_Available();

        if (serviceState == BLUEFRUIT_WAIT_COMMAND_MODE)
        {
            Debug_TX_String("[DIAG] Phase: enter command mode\r\n");
        }
        else if (serviceState == BLUEFRUIT_WAIT_RSSI)
        {
            Debug_TX_String("[DIAG] Phase: RSSI query\r\n");
        }
        else
        {
            Debug_TX_String("[DIAG] Phase: restore data mode\r\n");
        }

        Debug_TX_String(txPending ? "[DIAG] TX operation pending: yes\r\n"
                                  : "[DIAG] TX operation pending: no\r\n");

        Debug_TX_String(txHasCharacters ? "[DIAG] Characters left to queue: yes\r\n"
                                        : "[DIAG] Characters left to queue: no\r\n");

        Debug_TX_String(txBusy ? "[DIAG] UART transmitting/busy: yes\r\n"
                               : "[DIAG] UART transmitting/busy: no\r\n");

        Debug_TX_String(rxAvailable ? "[DIAG] RX byte waiting: yes\r\n"
                                    : "[DIAG] RX byte waiting: no\r\n");

        /* Show whether a partial response or mode indication was received. */
        Debug_TX_String(responseLength != 0U ? "[DIAG] Partial RX line: yes\r\n"
                                             : "[DIAG] Partial RX line: no\r\n");

        Debug_TX_String(modeReported ? "[DIAG] Mode value received: yes\r\n"
                                     : "[DIAG] Mode value received: no\r\n");

        Debug_TX_String(discardingLine ? "[DIAG] Discarding RX line: yes\r\n"
                                       : "[DIAG] Discarding RX line: no\r\n");

        Bluefruit_Fail("AT transmission/response timed out");
    }

    Bluefruit_ServiceTX();

    if (!BT_RX_Available())
    {
        return;
    }

    byte = BT_RX_Read();

    if (byte == '\r')
    {
        return;
    }

    if (byte == '\n')
    {
        if (!discardingLine && responseLength > 0U)
        {
            responseLine[responseLength] = '\0';
            responseLength = 0;
            Bluefruit_ProcessLine(responseLine, nowMs);
        }

        responseLength = 0;
        discardingLine = false;
        return;
    }

    if (discardingLine)
    {
        return;
    }

    if (byte < 0x20U || byte > 0x7EU)
    {
        responseLength = 0;
        discardingLine = true;
        Debug_TX_String("[ERR] Invalid response byte\r\n");

        if (Bluefruit_TransactionActive())
        {
            Bluefruit_Fail("Invalid byte during AT transaction");
        }

        return;
    }

    if (responseLength >= BLUEFRUIT_LINE_CAPACITY - 1U)
    {
        responseLength = 0;
        discardingLine = true;
        Debug_TX_String("[ERR] Response line too long\r\n");

        if (Bluefruit_TransactionActive())
        {
            Bluefruit_Fail("Response overflow during AT transaction");
        }

        return;
    }

    responseLine[responseLength] = (char)byte;
    responseLength++;
}

/** @brief Expire stale samples and schedule RSSI requests when idle.
 *  @param nowMs Current application time in milliseconds. */
void Bluefruit_Poll(uint32_t nowMs)
{
    if (sampleValid && (uint32_t)(nowMs - lastSampleTimeMs) >= RSSI_FRESHNESS_MS)
    {
        Debug_TX_String("[RSSI] Sample expired\r\n");
        Bluefruit_InvalidateProximity();
    }

    if (serviceState != BLUEFRUIT_DATA_IDLE)
    {
        return;
    }

    if ((uint32_t)(nowMs - lastPollTimeMs) < RSSI_POLL_INTERVAL_MS)
    {
        return;
    }

    if (responseLength != 0U || discardingLine || BT_RX_Available() || pendingTX != 0 ||
        BT_TX_Busy())
    {
        return;
    }

    if (!Bluefruit_RequestRSSI(nowMs))
    {
        Debug_TX_String("[ERR] Automatic RSSI request failed\r\n");
    }
}

/** @brief Start an RSSI transaction if the service and receive path are idle.
 *  @param nowMs Current application time in milliseconds.
 *  @return true if started; the result arrives asynchronously. */
bool Bluefruit_RequestRSSI(uint32_t nowMs)
{
    if (serviceState != BLUEFRUIT_DATA_IDLE)
    {
        Debug_TX_String("[ERR] Service not in data idle\r\n");
        return false;
    }

    if (responseLength != 0U || discardingLine || BT_RX_Available())
    {
        Debug_TX_String("[ERR] RX not idle; finish incoming line first\r\n");
        return false;
    }

    lastPollTimeMs = nowMs;
    rssiReported = false;
    candidateRSSI = 0;
    modeReported = false;

    transactionStartedMs = nowMs;
    serviceState = BLUEFRUIT_WAIT_COMMAND_MODE;

    Bluefruit_Trace("[AT] Entering command mode\r\n");
    Bluefruit_StartTX("+++\n");

    return serviceState != BLUEFRUIT_FAULT;
}

/**
 * @brief Return the latest usable Bluefruit proximity status.
 *
 * @param nowMs Current application time in milliseconds.
 * @return The confirmed status, UNKNOWN if it is unavailable or stale,
 *         or FAULT if the Bluefruit service has failed.
 *
 * A normal transaction retains the previous status while it is fresh.
 */
BluefruitStatus Bluefruit_GetStatus(uint32_t nowMs)
{
    if (serviceState == BLUEFRUIT_FAULT)
    {
        return BLUEFRUIT_STATUS_FAULT;
    }

    if (currentStatus == BLUEFRUIT_STATUS_UNKNOWN)
    {
        return BLUEFRUIT_STATUS_UNKNOWN;
    }

    if ((uint32_t)(nowMs - lastSampleTimeMs) >= RSSI_FRESHNESS_MS)
    {
        return BLUEFRUIT_STATUS_UNKNOWN;
    }

    if (currentStatus == BLUEFRUIT_STATUS_DISCONNECTED)
    {
        return BLUEFRUIT_STATUS_DISCONNECTED;
    }

    if (!sampleValid)
    {
        return BLUEFRUIT_STATUS_UNKNOWN;
    }

    return currentStatus;
}

/**
 * @brief Check whether the last valid RSSI samples qualified as near.
 *
 * @param nowMs Current time in milliseconds.
 * @return true when the qualified near result is fresh and the service
 *         has not faulted; otherwise false.
 *
 * A normal AT transaction does not immediately clear a fresh result.
 */
bool Bluefruit_IsNear(uint32_t nowMs)
{
    return serviceState != BLUEFRUIT_FAULT && sampleValid && proximityNear &&
           (uint32_t)(nowMs - lastSampleTimeMs) < RSSI_FRESHNESS_MS;
}

/** @brief Copy the last sample marked valid and its reception time.
 *  @param rssiDbm Output for the RSSI in dBm.
 *  @param sampleTimeMs Output for the sample timestamp.
 *  @return true when the pointers and sample are valid; age is not checked. */
bool Bluefruit_GetRSSI(int32_t *rssiDbm, uint32_t *sampleTimeMs)
{
    if (rssiDbm == 0 || sampleTimeMs == 0)
    {
        Debug_TX_String("[ERR] Missing RSSI output pointer\r\n");
        return false;
    }

    if (!sampleValid)
    {
        return false;
    }

    *rssiDbm = lastRSSI;
    *sampleTimeMs = lastSampleTimeMs;
    return true;
}

/** @brief Interpret a complete line according to the current transaction phase.
 *  @param text Null-terminated response line.
 *  @param nowMs Current application time in milliseconds. */
static void Bluefruit_ProcessLine(const char *text, uint32_t nowMs)
{
    if (serviceState == BLUEFRUIT_DATA_IDLE || serviceState == BLUEFRUIT_FAULT ||
        BLUEFRUIT_TRACE_ENABLED)
    {
        Debug_TX_String("[LINE] ");
        Debug_TX_String(text);
        Debug_TX_String("\r\n");
    }

    switch (serviceState)
    {
    case BLUEFRUIT_WAIT_COMMAND_MODE:
        if (strcmp(text, "+++") == 0)
        {
            return;
        }

        if (strcmp(text, "1") == 0 && !modeReported)
        {
            modeReported = true;
            return;
        }

        if (strcmp(text, "OK") == 0 && modeReported)
        {
            modeReported = false;
            rssiReported = false;
            transactionStartedMs = nowMs;
            serviceState = BLUEFRUIT_WAIT_RSSI;

            Bluefruit_Trace("[AT] Command mode verified\r\n");
            Bluefruit_Trace("[AT] Querying RSSI\r\n");
            Bluefruit_StartTX("AT+BLEGETRSSI\n");
            return;
        }

        Bluefruit_Fail("Unexpected command-mode response");
        return;

    case BLUEFRUIT_WAIT_RSSI:
        if (strcmp(text, "AT+BLEGETRSSI") == 0)
        {
            return;
        }

        if (strcmp(text, "ERROR") == 0)
        {
            rssiReported = false;
            Bluefruit_InvalidateProximity();
            Debug_TX_String("[ERR] RSSI query returned ERROR\r\n");
            Bluefruit_RestoreDataMode(nowMs);
            return;
        }

        if (!rssiReported && Bluefruit_ParseRSSI(text, &candidateRSSI))
        {
            canditateSampleTimeMs = nowMs;
            rssiReported = true;

            if (candidateRSSI == 0)
            {
                Bluefruit_InvalidateProximity();
            }

            return;
        }

        if (strcmp(text, "OK") == 0 && rssiReported)
        {
            Bluefruit_RestoreDataMode(nowMs);
            return;
        }

        Bluefruit_Fail("Malformed RSSI response");
        return;

    case BLUEFRUIT_WAIT_DATA_MODE:
        if (strcmp(text, "+++") == 0)
        {
            return;
        }

        if (strcmp(text, "0") == 0 && !modeReported)
        {
            modeReported = true;
            return;
        }

        if (strcmp(text, "OK") == 0 && modeReported)
        {
            serviceState = BLUEFRUIT_DATA_IDLE;
            modeReported = false;

            Bluefruit_Trace("[AT] Data mode verified\r\n");

            lastPollTimeMs = nowMs;

            if (rssiReported && candidateRSSI < 0)
            {
                if (currentStatus == BLUEFRUIT_STATUS_DISCONNECTED)
                {
                    currentStatus = BLUEFRUIT_STATUS_UNKNOWN;
                }
                disconnectedReported = false;

                lastRSSI = candidateRSSI;
                lastSampleTimeMs = canditateSampleTimeMs;
                sampleValid = true;

                if ((uint32_t)(nowMs - lastSampleTimeMs) >= RSSI_FRESHNESS_MS)
                {
                    Debug_TX_String("[RSSI] Sample already stale\r\n");
                    Bluefruit_InvalidateProximity();
                }
                else
                {
                    Bluefruit_Trace("[RSSI] Sample valid\r\n");
                    Bluefruit_QualifySample(nowMs);
                }
            }
            else
            {
                Bluefruit_InvalidateProximity();

                if (rssiReported && candidateRSSI == 0)
                {
                    lastSampleTimeMs = canditateSampleTimeMs;
                    currentStatus = BLUEFRUIT_STATUS_DISCONNECTED;

                    if (!disconnectedReported)
                    {
                        Debug_TX_String("[RSSI] Disconnected; sample invalid\r\n");
                        disconnectedReported = true;
                    }
                }
                else
                {
                    Debug_TX_String("[RSSI] Query failed; sample invalid\r\n");
                }
            }

            rssiReported = false;
            return;
        }

        Bluefruit_Fail("Unexpected data-mode response");
        return;

    case BLUEFRUIT_DATA_IDLE:
    case BLUEFRUIT_FAULT:
        return;
    }
}

/** @brief Request data mode and start its response timeout.
 *  @param nowMs Current application time in milliseconds. */
static void Bluefruit_RestoreDataMode(uint32_t nowMs)
{
    modeReported = false;
    transactionStartedMs = nowMs;
    serviceState = BLUEFRUIT_WAIT_DATA_MODE;

    Bluefruit_Trace("[AT] Restoring data mode\r\n");
    Bluefruit_StartTX("+++\n");
}

/** @brief Parse zero or a negative RSSI in the range -1 to -127.
 *  @param text Null-terminated response line.
 *  @param value Output for the parsed value.
 *  @return true if the line was accepted. */
static bool Bluefruit_ParseRSSI(const char *text, int32_t *value)
{
    uint32_t magnitude = 0;
    uint32_t digits = 0;

    if (strcmp(text, "0") == 0)
    {
        *value = 0;
        return true;
    }

    if (*text != '-')
    {
        return false;
    }

    text++;

    while (*text != '\0')
    {
        if (*text < '0' || *text > '9' || digits >= 3U)
        {
            return false;
        }

        magnitude = magnitude * 10U + (uint32_t)(*text - '0');
        digits++;
        text++;
    }

    if (digits == 0U || magnitude == 0U || magnitude > 127U)
    {
        return false;
    }

    *value = -(int32_t)magnitude;
    return true;
}

/** @brief Apply near and far thresholds to the latest fresh sample.
 *  @param nowMs Current application time in milliseconds. */
static void Bluefruit_QualifySample(uint32_t nowMs)
{
    if (!sampleValid || (uint32_t)(nowMs - lastSampleTimeMs) >= RSSI_FRESHNESS_MS)
    {
        Bluefruit_InvalidateProximity();
        return;
    }

    if (!proximityNear)
    {
        farSampleCount = 0;

        if (lastRSSI >= RSSI_NEAR_ENTER_DBM)
        {
            if (nearSampleCount < RSSI_NEAR_REQUIRED)
            {
                nearSampleCount++;
            }

            if (nearSampleCount >= RSSI_NEAR_REQUIRED)
            {
                proximityNear = true;
                currentStatus = BLUEFRUIT_STATUS_NEAR;
                nearSampleCount = 0;
                Debug_TX_String("[PROX] Near qualified\r\n");
            }
        }
        else
        {
            nearSampleCount = 0;
        }
    }
    else
    {
        nearSampleCount = 0;

        if (lastRSSI <= RSSI_NEAR_EXIT_DBM)
        {
            if (farSampleCount < RSSI_FAR_REQUIRED)
            {
                farSampleCount++;
            }

            if (farSampleCount >= RSSI_FAR_REQUIRED)
            {
                proximityNear = false;
                currentStatus = BLUEFRUIT_STATUS_FAR;
                farSampleCount = 0;
                Debug_TX_String("[PROX] FAR qualified\r\n");
            }
        }
        else
        {
            farSampleCount = 0;
        }
    }
}

/** @brief Invalidate the sample and clear proximity qualification. */
static void Bluefruit_InvalidateProximity(void)
{
    sampleValid = false;
    currentStatus = BLUEFRUIT_STATUS_UNKNOWN;
    nearSampleCount = 0;
    farSampleCount = 0;

    if (proximityNear)
    {
        Debug_TX_String("[PROX] INVALID; no unlock eligibility\r\n");
    }

    proximityNear = false;
}

/** @brief Check whether an AT transaction phase is active.
 *  @return true when waiting for a mode or RSSI response. */
static bool Bluefruit_TransactionActive(void)
{
    return serviceState == BLUEFRUIT_WAIT_COMMAND_MODE || serviceState == BLUEFRUIT_WAIT_RSSI ||
           serviceState == BLUEFRUIT_WAIT_DATA_MODE;
}

/** @brief Schedule a command for transmission over UART1.
 *  @param text Null-terminated command that remains valid until sent. */
static void Bluefruit_StartTX(const char *text)
{
    if (pendingTX != 0 || BT_TX_Busy())
    {
        Bluefruit_Fail("Previous UART transmission still pending");
        return;
    }

    pendingTX = text;
}

/** @brief Queue at most one command byte, or wait for UART1 to finish. */
static void Bluefruit_ServiceTX(void)
{
    if (pendingTX == 0)
    {
        return;
    }

    if (*pendingTX == '\0')
    {
        if (!BT_TX_Busy())
        {
            pendingTX = 0;
        }

        return;
    }

    if (BT_TX_TryByte((uint8_t)*pendingTX))
    {
        pendingTX++;
    }
}

/** @brief Latch a fault and invalidate proximity; polling stops.
 *  @param reason Null-terminated diagnostic message. */
static void Bluefruit_Fail(const char *reason)
{
    serviceState = BLUEFRUIT_FAULT;
    pendingTX = 0;
    Bluefruit_InvalidateProximity();
    rssiReported = false;
    modeReported = false;

    Debug_TX_String("[ERR] ");
    Debug_TX_String(reason);
    Debug_TX_String("; reset Bluefruit and MCU\r\n");
}

/** @brief Print a message when transaction tracing is enabled.
 *  @param text Null-terminated diagnostic message. */
static void Bluefruit_Trace(const char *text)
{
#if BLUEFRUIT_TRACE_ENABLED
    Debug_TX_String(text);
#else
    (void)text;
#endif
}

/** @brief Send diagnostic text over UART0; output may block.
 *  @param text Null-terminated message. */
static void Debug_TX_String(const char *text)
{
    while (*text != '\0')
    {
        UARTCharPut(UART0_BASE, *text);
        text++;
    }
}
