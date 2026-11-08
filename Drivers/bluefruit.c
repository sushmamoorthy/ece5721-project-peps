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

#define BLUEFRUIT_LINE_CAPACITY 64U
#define BLUEFRUIT_RESPONSE_TIMEOUT_MS 2000UL

#define RSSI_POLL_INTERVAL_MS 1000UL
#define RSSI_FRESHNESS_MS 2500UL

#define RSSI_NEAR_ENTER_DBM (-55)
#define RSSI_NEAR_EXIT_DBM (-65)

#define RSSI_NEAR_REQUIRED 3U
#define RSSI_FAR_REQUIRED 2U

#define BLUEFRUIT_TRACE_ENABLED 0

typedef enum
{
    BLUEFRUIT_DATA_IDLE,
    BLUEFRUIT_WAIT_COMMAND_MODE,
    BLUEFRUIT_WAIT_RSSI,
    BLUEFRUIT_WAIT_DATA_MODE,
    BLUEFRUIT_FAULT
} bluefruitState;

static bluefruitState serviceState = BLUEFRUIT_DATA_IDLE;
static uint32_t transactionStartedMs = 0;
static bool modeReported = false;

static char responseLine[BLUEFRUIT_LINE_CAPACITY];
static uint32_t responseLength = 0;
static bool discardingLine = false;

static bool rssiReported = false;
static int32_t candidateRSSI = 0;

static bool sampleValid = false;
static int32_t lastRSSI = 0;
static uint32_t lastSampleTimeMs = 0;
static const char *pendingTX = 0;

static uint32_t lastPollTimeMs = 0;
static uint32_t canditateSampleTimeMs = 0;

static bool proximityNear = false;
static uint32_t nearSampleCount = 0;
static uint32_t farSampleCount = 0;

static bool disconnectedReported = false;

static void Debug_TX_String(const char *text)
{
    while (*text != '\0')
    {
        UARTCharPut(UART0_BASE, *text);
        text++;
    }
}

static void Bluefruit_Trace(const char *text)
{
    #if BLUEFRUIT_TRACE_ENABLED
    Debug_TX_String(text);
    #else
    (void)text;
    #endif
}

static bool Bluefruit_TransactionActive(void)
{
    return serviceState == BLUEFRUIT_WAIT_COMMAND_MODE ||
           serviceState == BLUEFRUIT_WAIT_RSSI ||
           serviceState == BLUEFRUIT_WAIT_DATA_MODE;
}

static void Bluefruit_InvalidateProximity(void)
{
    sampleValid = false;
    nearSampleCount = 0;
    farSampleCount = 0;

    if (proximityNear)
    {
        Debug_TX_String("[PROX] INVALID; no unlock eligibility\r\n");
    }

    proximityNear = false;
}

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

static void Bluefruit_StartTX(const char *text)
{
    if (pendingTX != 0 || BT_TX_Busy())
    {
        Bluefruit_Fail("Previous UART transmission still pending");
        return;
    }

    pendingTX = text;
}

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

static void Bluefruit_RestoreDataMode(uint32_t nowMs)
{
    modeReported = false;
    transactionStartedMs = nowMs;
    serviceState = BLUEFRUIT_WAIT_DATA_MODE;

    Bluefruit_Trace("[AT] Restoring data mode\r\n");
    Bluefruit_StartTX("+++\n");
}

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

static void Bluefruit_ProcessLine(const char *text, uint32_t nowMs)
{
    if (serviceState == BLUEFRUIT_DATA_IDLE ||
        serviceState == BLUEFRUIT_FAULT ||
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

            if (!rssiReported &&
                Bluefruit_ParseRSSI(text, &candidateRSSI))
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

void Bluefruit_Monitor(uint32_t nowMs)
{
    uint8_t byte;

    if (Bluefruit_TransactionActive() &&
        (uint32_t)(nowMs - transactionStartedMs) >=
            BLUEFRUIT_RESPONSE_TIMEOUT_MS)
    {
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

    if (responseLength != 0U || discardingLine || BT_RX_Available() || pendingTX != 0 || BT_TX_Busy())
    {
        return;
    }

    if (!Bluefruit_RequestRSSI(nowMs))
    {
        Debug_TX_String("[ERR] Automatic RSSI request failed\r\n");
    }
}

bool Bluefruit_IsNear(uint32_t nowMs)
{
    return serviceState == BLUEFRUIT_DATA_IDLE &&
            sampleValid &&
            proximityNear &&
            (uint32_t)(nowMs - lastSampleTimeMs) < RSSI_FRESHNESS_MS;
}
