/**
 *  NimBLE_Stream_Server Example:
 *
 *  Demonstrates using NimBLEStreamServer to create a BLE GATT server
 *  that behaves like a serial port using the Arduino Stream interface.
 *
 *  This allows you to use familiar methods like print(), println(),
 *  read(), and available() over BLE, similar to how you would use Serial.
 *
 *  Uses the Nordic UART Service (NUS) UUIDs with separate TX and RX characteristics
 *  for compatibility with NUS terminal apps (e.g. nRF UART, Serial Bluetooth Terminal).
 *
 *  Created: November 2025
 *      Author: NimBLE-Arduino Contributors
 */

#include <Arduino.h>
#include <NimBLEDevice.h>

// Nordic UART Service (NUS) UUIDs
#define SERVICE_UUID  "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define TX_CHAR_UUID  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // Server TX: notify (server -> client)
#define RX_CHAR_UUID  "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // Server RX: write  (client -> server)

/**
 * Two stream server instances:
 * - bleStreamTx sends notifications to the client (server -> client).
 *   Backed by the TX characteristic (NOTIFY-only); TX buffer is enabled, RX buffer disabled.
 * - bleStreamRx receives data written by the client (client -> server).
 *   Backed by the RX characteristic (WRITE-only); RX buffer is enabled, TX buffer disabled.
 *
 * NimBLEStreamServer::begin(pChr) automatically enables only the directions supported
 * by the characteristic's properties, so no special configuration is needed.
 */
NimBLEStreamServer bleStreamTx;
NimBLEStreamServer bleStreamRx;

struct RxOverflowStats {
    uint32_t droppedOld{0};
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

/** Server callbacks to handle connection/disconnection events */
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        Serial.printf("Client connected: %s\n", connInfo.getAddress().toString().c_str());
        // Optionally update connection parameters for better throughput
        pServer->updateConnParams(connInfo.getConnHandle(), 12, 24, 0, 200);
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        Serial.printf("Client disconnected - reason: %d, restarting advertising\n", reason);
        NimBLEDevice::startAdvertising();
    }

    void onMTUChange(uint16_t MTU, NimBLEConnInfo& connInfo) override {
        Serial.printf("MTU updated: %u for connection ID: %u\n", MTU, connInfo.getConnHandle());
    }
} serverCallbacks;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting NimBLE Stream Server");

    /** Initialize NimBLE and set the device name */
    NimBLEDevice::init("NimBLE-Stream");

    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);

    /**
     * Create the NUS service with two characteristics:
     * - TX (6E400003): NOTIFY -- server sends data to the client
     * - RX (6E400002): WRITE  -- client sends data to the server
     */
    NimBLEService*        pSvc    = pServer->createService(SERVICE_UUID);
    NimBLECharacteristic* pTxChar = pSvc->createCharacteristic(TX_CHAR_UUID, NIMBLE_PROPERTY::NOTIFY);
    NimBLECharacteristic* pRxChar = pSvc->createCharacteristic(RX_CHAR_UUID,
                                                               NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);

    /**
     * Pass each characteristic to its own NimBLEStreamServer instance.
     * begin() checks the characteristic properties and enables only the supported
     * direction: pTxChar (NOTIFY-only) enables the TX buffer; pRxChar (WRITE-only)
     * enables the RX buffer.
     */
    if (!bleStreamTx.begin(pTxChar) || !bleStreamRx.begin(pRxChar)) {
        Serial.println("Failed to initialize BLE stream!");
        return;
    }

    bleStreamRx.setRxOverflowCallback(onRxOverflow, &g_rxOverflowStats);

    /**
     * Create advertising instance and add service UUID
     * This makes the stream service discoverable
     */
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setName("NimBLE-Stream");
    pAdvertising->enableScanResponse(true);
    pAdvertising->start();

    Serial.println("BLE Stream Server ready!");
    Serial.println("Waiting for client connection...");
    Serial.println("Once connected, you can send data from the client.");
}

void loop() {
    static uint32_t lastDroppedOld = 0;
    if (g_rxOverflowStats.droppedOld != lastDroppedOld) {
        lastDroppedOld = g_rxOverflowStats.droppedOld;
        Serial.printf("RX overflow: %lu packets dropped\n", lastDroppedOld);
    }

    // bleStreamTx.ready() is true when a client has subscribed to the TX characteristic
    if (bleStreamTx.ready()) {
        // Send a message every 2 seconds using Stream methods
        static unsigned long lastSend = 0;
        if (millis() - lastSend > 2000) {
            lastSend = millis();

            // Using familiar Serial-like methods!
            bleStreamTx.print("Hello from BLE Server! Time: ");
            bleStreamTx.println(millis());

            // You can also use printf
            bleStreamTx.printf("Free heap: %d bytes\n", ESP.getFreeHeap());

            Serial.println("Sent data to client via BLE stream");
        }

        // Check if we received any data written by the client on the RX characteristic
        if (bleStreamRx.available()) {
            Serial.print("Received from client: ");

            // Read all available data (just like Serial.read())
            while (bleStreamRx.available()) {
                char c = bleStreamRx.read();
                Serial.write(c);       // Echo to Serial
                bleStreamTx.write(c);  // Echo back to BLE client via TX notification
            }
            Serial.println();
        }
    } else {
        // No subscriber, just wait
        delay(100);
    }
}
