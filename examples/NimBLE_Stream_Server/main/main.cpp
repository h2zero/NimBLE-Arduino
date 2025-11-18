/**
 *  NimBLE_Stream_Server Example:
 *
 *  Demonstrates using NimBLEStreamServer to create a BLE GATT server
 *  that behaves like a serial port using the Stream-like interface.
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <NimBLEDevice.h>

// Create the stream server instance
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

    // Keep the newest bytes for command/stream style traffic.
    (void)data;
    (void)len;
    return NimBLEStream::DROP_OLDER_DATA;
}

static uint64_t millis() {
    return esp_timer_get_time() / 1000ULL;
}

// Service and Characteristic UUIDs for the stream.
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

/** Server callbacks to handle connection/disconnection events */
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        printf("Client connected: %s\n", connInfo.getAddress().toString().c_str());
        // Optionally update connection parameters for better throughput.
        pServer->updateConnParams(connInfo.getConnHandle(), 12, 24, 0, 200);
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        (void)pServer;
        (void)connInfo;
        printf("Client disconnected - reason: %d, restarting advertising\n", reason);
        NimBLEDevice::startAdvertising();
    }

    void onMTUChange(uint16_t MTU, NimBLEConnInfo& connInfo) override {
        printf("MTU updated: %u for connection ID: %u\n", MTU, connInfo.getConnHandle());
    }
} serverCallbacks;

extern "C" void app_main(void) {
    printf("Starting NimBLE Stream Server\n");

    /** Initialize NimBLE and set the device name */
    NimBLEDevice::init("NimBLE-Stream");

    /**
     * Create the BLE server and set callbacks.
     * Note: The stream will create its own service and characteristic.
     */
    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);

    /**
     * Initialize the stream server with:
     * - Service UUID
     * - Characteristic UUID
     * - txBufSize: 1024 bytes for outgoing data (notifications)
     * - rxBufSize: 1024 bytes for incoming data (writes)
     * - secure: false (no encryption required - set true for secure connections)
     */
    if (!bleStream.begin(NimBLEUUID(SERVICE_UUID),
                         NimBLEUUID(CHARACTERISTIC_UUID),
                         1024,
                         1024,
                         false)) {
        printf("Failed to initialize BLE stream!\n");
        return;
    }

    bleStream.setRxOverflowCallback(onRxOverflow, &g_rxOverflowStats);

    // Make the stream service discoverable.
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setName("NimBLE-Stream");
    pAdvertising->enableScanResponse(true);
    pAdvertising->start();

    printf("BLE Stream Server ready!\n");
    printf("Waiting for client connection...\n");

    uint32_t lastDroppedOld = 0;
    uint32_t lastDroppedNew = 0;
    uint64_t lastSend       = 0;

    for (;;) {
        if (g_rxOverflowStats.droppedOld != lastDroppedOld || g_rxOverflowStats.droppedNew != lastDroppedNew) {
            lastDroppedOld = g_rxOverflowStats.droppedOld;
            lastDroppedNew = g_rxOverflowStats.droppedNew;
            printf("RX overflow handled (drop-old=%" PRIu32 ", drop-new=%" PRIu32 ")\n", lastDroppedOld, lastDroppedNew);
        }

        if (bleStream.ready()) {
            uint64_t now = millis();
            if (now - lastSend > 2000) {
                lastSend = now;
                bleStream.printf("Hello from server! Uptime: %" PRIu64 " seconds\n", now / 1000);
                bleStream.printf("Free heap: %" PRIu32 " bytes\n", esp_get_free_heap_size());
                printf("Sent data to client via BLE stream\n");
            }

            if (bleStream.available()) {
                printf("Received from client: ");
                while (bleStream.available()) {
                    char c = bleStream.read();
                    putchar(c);
                    bleStream.write(c); // Echo back to BLE client.
                }
                printf("\n");
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}