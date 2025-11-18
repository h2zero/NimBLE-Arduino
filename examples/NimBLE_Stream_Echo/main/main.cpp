/**
 *  NimBLE_Stream_Echo Example:
 *
 *  A minimal example demonstrating NimBLEStreamServer.
 *  Echoes back any data received from BLE clients.
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <NimBLEDevice.h>

NimBLEStreamServer bleStream;

struct RxOverflowStats {
    uint32_t droppedOld{0};
    uint32_t droppedNew{0};
};

RxOverflowStats g_rxOverflowStats;

NimBLEStream::RxOverflowAction onRxOverflow(const uint8_t* data, size_t len, void* userArg) {
    auto* stats = static_cast<RxOverflowStats*>(userArg);
    if (stats) {
        stats->droppedOld++;
    }

    // Echo mode prefers the latest incoming bytes.
    (void)data;
    (void)len;
    return NimBLEStream::DROP_OLDER_DATA;
}

extern "C" void app_main(void) {
    printf("NimBLE Stream Echo Server\n");

    // Initialize BLE.
    NimBLEDevice::init("BLE-Echo");
    auto pServer = NimBLEDevice::createServer();
    pServer->advertiseOnDisconnect(true); // Keep advertising after disconnects.

    if (!bleStream.begin(NimBLEUUID(uint16_t(0xc0de)),
                         NimBLEUUID(uint16_t(0xfeed)),
                         1024,
                         1024,
                         false)) {
        printf("Failed to initialize BLE stream\n");
        return;
    }

    bleStream.setRxOverflowCallback(onRxOverflow, &g_rxOverflowStats);

    // Start advertising.
    NimBLEDevice::getAdvertising()->start();
    printf("Ready! Connect with a BLE client and send data.\n");

    uint32_t lastDroppedOld = 0;
    uint32_t lastDroppedNew = 0;

    for (;;) {
        if (g_rxOverflowStats.droppedOld != lastDroppedOld || g_rxOverflowStats.droppedNew != lastDroppedNew) {
            lastDroppedOld = g_rxOverflowStats.droppedOld;
            lastDroppedNew = g_rxOverflowStats.droppedNew;
            printf("RX overflow handled (drop-old=%" PRIu32 ", drop-new=%" PRIu32 ")\n", lastDroppedOld, lastDroppedNew);
        }

        // Echo any received data back to the client.
        if (bleStream.ready() && bleStream.available()) {
            printf("Echo: ");
            while (bleStream.available()) {
                char c = bleStream.read();
                putchar(c);
                bleStream.write(c);
            }
            printf("\n");
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}