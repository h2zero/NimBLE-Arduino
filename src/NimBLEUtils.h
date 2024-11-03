/*
 * NimBLEUtils.h
 *
 *  Created: on Jan 25 2020
 *      Author H2zero
 *
 */

#ifndef COMPONENTS_NIMBLEUTILS_H_
#define COMPONENTS_NIMBLEUTILS_H_

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED)

# include <freertos/FreeRTOS.h>
# include <freertos/task.h>

#include <string>

class NimBLEAddress;

struct BleTaskData {
    void*        pATT;
    TaskHandle_t task;
    int          rc;
    void*        buf;
};

struct ble_gap_event;

/**
 * @brief A BLE Utility class with methods for debugging and general purpose use.
 */
class NimBLEUtils {
public:
    static void                 dumpGapEvent(ble_gap_event *event, void *arg);
    static const char*          gapEventToString(uint8_t eventType);
    static char*                buildHexData(uint8_t* target, const uint8_t* source, uint8_t length);
    static const char*          advTypeToString(uint8_t advType);
    static const char*          returnCodeToString(int rc);
    static NimBLEAddress        generateAddr(bool nrpa);
};

#endif // CONFIG_BT_ENABLED
#endif // COMPONENTS_NIMBLEUTILS_H_
