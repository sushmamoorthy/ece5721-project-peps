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

bool Bluefruit_RequestRSSI(uint32_t nowMs);
void Bluefruit_Monitor(uint32_t nowMs);
bool Bluefruit_GetRSSI(int32_t *rssiDbm, uint32_t *sampleTimeMs);
void Bluefruit_Poll(uint32_t nowMs);
bool Bluefruit_IsNear(uint32_t nowMs);

#endif // BLUEFRUIT_H
