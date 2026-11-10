/* Final Project Files
 * Filename: bluefruit.h
 * Author: Mrunalini Desai
 * Date: 11/04/2026
 *
 * Description: Provides the Bluefruit protocol monitoring interface.
 */

#ifndef BLUEFRUIT_H
#define BLUEFRUIT_H

#include <stdbool.h>
#include <stdint.h>

/* Proximity and communication status consumed by the BCM. */
typedef enum
{
    BLUEFRUIT_STATUS_UNKNOWN,
    BLUEFRUIT_STATUS_NEAR,
    BLUEFRUIT_STATUS_FAR,
    BLUEFRUIT_STATUS_DISCONNECTED,
    BLUEFRUIT_STATUS_FAULT
} BluefruitStatus;

/**
 * @brief Service TX, RX, and the active transaction's timeout.
 * @param nowMs Current application time in milliseconds.
 * @details Call repeatedly from the main loop, before Bluefruit_Poll().
 */
void Bluefruit_Monitor(uint32_t nowMs);

/**
 * @brief Expire old samples and schedule an RSSI transaction when idle.
 * @param nowMs Current application time in milliseconds.
 */
void Bluefruit_Poll(uint32_t nowMs);

/**
 * @brief Start an RSSI transaction if the service and receive path are idle.
 * @param nowMs Current application time in milliseconds.
 * @return true if started; completion requires calls to Bluefruit_Monitor().
 */
bool Bluefruit_RequestRSSI(uint32_t nowMs);

/**
 * @brief Return the current usable proximity status.
 * @param nowMs Current millisecond time.
 * @return Near, far, disconnected, unknown, or fault.
 */
BluefruitStatus Bluefruit_GetStatus(uint32_t nowMs);

/**
 * @brief Check whether the qualified near result is fresh and not faulted.
 * @param nowMs Current application time in milliseconds.
 * @return true only for a valid, fresh, qualified near result.
 */
bool Bluefruit_IsNear(uint32_t nowMs);

/**
 * @brief Copy the last sample marked valid and its reception time.
 * @param rssiDbm Output location for RSSI in dBm.
 * @param sampleTimeMs Output location for the sample timestamp.
 * @return true if the pointers are valid and a sample is marked valid.
 * @details This function does not independently check sample age.
 */
bool Bluefruit_GetRSSI(int32_t *rssiDbm, uint32_t *sampleTimeMs);

#endif // BLUEFRUIT_H
