/**
 *  NimBLE_Stream_Client Example:
 *
 *  Demonstrates using NimBLEStreamClient to connect to a BLE GATT server
 *  and communicate using the Stream-like interface.
 *
 *  This example connects to the NimBLE_Stream_Server example.
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <NimBLEDevice.h>

// Service and Characteristic UUIDs (must match the server)
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

// Create the stream client instance
NimBLEStreamClient bleStream;

struct RxOverflowStats {
    uint32_t droppedOld{0};
    uint32_t droppedNew{0};
};

RxOverflowStats g_rxOverflowStats;
uint32_t        scanTime = 5000; // Scan duration in milliseconds

NimBLEStream::RxOverflowAction onRxOverflow(const uint8_t* data, size_t len, void* userArg) {
    auto* stats = static_cast<RxOverflowStats*>(userArg);
    if (stats) {
        stats->droppedOld++;
    }

    // For status/telemetry streams, prioritize newest packets.
    (void)data;
    (void)len;
    return NimBLEStream::DROP_OLDER_DATA;
}

static uint64_t millis() {
    return esp_timer_get_time() / 1000ULL;
}

// Connection state variables
static bool                          doConnect     = false;
static bool                          connected     = false;
static const NimBLEAdvertisedDevice* pServerDevice = nullptr;
static NimBLEClient*                 pClient       = nullptr;

/** Scan callbacks to find the server */
class ScanCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
        printf("Advertised Device: %s\n", advertisedDevice->toString().c_str());

        // Check if this device advertises our service.
        if (advertisedDevice->isAdvertisingService(NimBLEUUID(SERVICE_UUID))) {
            printf("Found our stream server!\n");
            pServerDevice = advertisedDevice;
            NimBLEDevice::getScan()->stop();
            doConnect = true;
        }
    }

    void onScanEnd(const NimBLEScanResults& results, int reason) override {
        (void)results;
        (void)reason;
        printf("Scan ended\n");
        if (!doConnect && !connected) {
            printf("Server not found, restarting scan...\n");
            NimBLEDevice::getScan()->start(scanTime, false, true);
        }
    }
} scanCallbacks;

/** Client callbacks for connection/disconnection events */
class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) override {
        printf("Connected to server\n");
        // Update connection parameters for better throughput.
        pClient->updateConnParams(12, 24, 0, 200);
    }

    void onDisconnect(NimBLEClient* pClient, int reason) override {
        (void)pClient;
        printf("Disconnected from server, reason: %d\n", reason);
        connected = false;
        bleStream.end();

        // Restart scanning.
        printf("Restarting scan...\n");
        NimBLEDevice::getScan()->start(scanTime, false, true);
    }
} clientCallbacks;

/** Connect to the BLE Server and set up the stream */
bool connectToServer() {
    printf("Connecting to: %s\n", pServerDevice->getAddress().toString().c_str());

    // Create or reuse a client.
    pClient = NimBLEDevice::getClientByPeerAddress(pServerDevice->getAddress());
    if (!pClient) {
        pClient = NimBLEDevice::createClient();
        if (!pClient) {
            printf("Failed to create client\n");
            return false;
        }
        pClient->setClientCallbacks(&clientCallbacks, false);
        pClient->setConnectionParams(12, 24, 0, 200);
        pClient->setConnectTimeout(5000);
    }

    // Connect to the remote BLE Server.
    if (!pClient->connect(pServerDevice)) {
        printf("Failed to connect to server\n");
        return false;
    }

    printf("Connected! Discovering services...\n");

    // Get the service and characteristic.
    NimBLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
    if (!pRemoteService) {
        printf("Failed to find our service UUID\n");
        pClient->disconnect();
        return false;
    }
    printf("Found the stream service\n");

    NimBLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
    if (!pRemoteCharacteristic) {
        printf("Failed to find our characteristic UUID\n");
        pClient->disconnect();
        return false;
    }
    printf("Found the stream characteristic\n");

    // subscribeNotify=true means notifications are stored in the RX buffer.
    if (!bleStream.begin(pRemoteCharacteristic, true)) {
        printf("Failed to initialize BLE stream!\n");
        pClient->disconnect();
        return false;
    }

    bleStream.setRxOverflowCallback(onRxOverflow, &g_rxOverflowStats);

    printf("BLE Stream initialized successfully!\n");
    connected = true;
    return true;
}

extern "C" void app_main(void) {
    printf("Starting NimBLE Stream Client\n");

    /** Initialize NimBLE */
    NimBLEDevice::init("NimBLE-StreamClient");

    // Create the BLE scan instance and set callbacks.
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(&scanCallbacks, false);
    pScan->setActiveScan(true);

    // Start scanning for the server.
    printf("Scanning for BLE Stream Server...\n");
    pScan->start(scanTime, false, true);

    uint32_t lastDroppedOld = 0;
    uint32_t lastDroppedNew = 0;
    uint64_t lastSend       = 0;

    for (;;) {
        if (g_rxOverflowStats.droppedOld != lastDroppedOld || g_rxOverflowStats.droppedNew != lastDroppedNew) {
            lastDroppedOld = g_rxOverflowStats.droppedOld;
            lastDroppedNew = g_rxOverflowStats.droppedNew;
            printf("RX overflow handled (drop-old=%" PRIu32 ", drop-new=%" PRIu32 ")\n", lastDroppedOld, lastDroppedNew);
        }

        // If we found a server, try to connect.
        if (doConnect) {
            doConnect = false;
            if (connectToServer()) {
                printf("Stream ready for communication!\n");
            } else {
                printf("Failed to connect to server, restarting scan...\n");
                pServerDevice = nullptr;
                NimBLEDevice::getScan()->start(scanTime, false, true);
            }
        }

        // If connected, demonstrate stream communication.
        if (connected && bleStream) {
            if (bleStream.available()) {
                printf("Received from server: ");
                while (bleStream.available()) {
                    char c = bleStream.read();
                    putchar(c);
                }
                printf("\n");
            }

            uint64_t now = millis();
            if (now - lastSend > 5000) {
                lastSend = now;
                bleStream.printf("Hello from client! Uptime: %" PRIu64 " seconds\n", now / 1000);
                printf("Sent data to server via BLE stream\n");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}