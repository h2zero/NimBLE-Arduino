/**
 *  NimBLE_Stream_Client Example:
 *
 *  Demonstrates using NimBLEStreamClient to connect to a BLE GATT server
 *  and communicate using the Arduino Stream interface.
 *
 *  This allows you to use familiar methods like print(), println(),
 *  and write() over BLE, similar to how you would use Serial.
 *
 *  This example connects to the NimBLE_Stream_Server example using the Nordic UART
 *  Service (NUS) with separate TX and RX characteristics.
 *
 *  Created: November 2025
 *      Author: NimBLE-Arduino Contributors
 */

#include <Arduino.h>
#include <NimBLEDevice.h>

// Nordic UART Service (NUS) UUIDs (must match the server)
#define SERVICE_UUID  "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define TX_CHAR_UUID  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // Server TX: client subscribes here
#define RX_CHAR_UUID  "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // Server RX: client writes here

/**
 * Stream for sending data to the server.
 * Attached to the server's RX characteristic (6E400002, WRITE_NR).
 * Data received from the server arrives via the onServerNotify() callback below.
 */
NimBLEStreamClient bleStream;

uint32_t scanTime = 5000; // Scan duration in milliseconds

// Connection state variables
static bool                          doConnect     = false;
static bool                          connected     = false;
static const NimBLEAdvertisedDevice* pServerDevice = nullptr;
static NimBLEClient*                 pClient       = nullptr;

/** Callback invoked when the server sends a notification on its TX characteristic (6E400003) */
void onServerNotify(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t len, bool isNotify) {
    Serial.print("Received from server: ");
    Serial.write(pData, len);
    Serial.println();
}

/** Scan callbacks to find the server */
class ScanCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
        Serial.printf("Advertised Device: %s\n", advertisedDevice->toString().c_str());

        // Check if this device advertises our service
        if (advertisedDevice->isAdvertisingService(NimBLEUUID(SERVICE_UUID))) {
            Serial.println("Found our stream server!");
            pServerDevice = advertisedDevice;
            NimBLEDevice::getScan()->stop();
            doConnect = true;
        }
    }

    void onScanEnd(const NimBLEScanResults& results, int reason) override {
        Serial.println("Scan ended");
        if (!doConnect && !connected) {
            Serial.println("Server not found, restarting scan...");
            NimBLEDevice::getScan()->start(scanTime, false, true);
        }
    }
} scanCallbacks;

/** Client callbacks for connection/disconnection events */
class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) override {
        Serial.println("Connected to server");
        // Update connection parameters for better throughput
        pClient->updateConnParams(12, 24, 0, 200);
    }

    void onDisconnect(NimBLEClient* pClient, int reason) override {
        Serial.printf("Disconnected from server, reason: %d\n", reason);
        connected = false;
        bleStream.end();

        // Restart scanning
        Serial.println("Restarting scan...");
        NimBLEDevice::getScan()->start(scanTime, false, true);
    }
} clientCallbacks;

/** Connect to the BLE Server and set up the stream */
bool connectToServer() {
    Serial.printf("Connecting to: %s\n", pServerDevice->getAddress().toString().c_str());

    // Create or reuse a client
    pClient = NimBLEDevice::getClientByPeerAddress(pServerDevice->getAddress());
    if (!pClient) {
        pClient = NimBLEDevice::createClient();
        if (!pClient) {
            Serial.println("Failed to create client");
            return false;
        }
        pClient->setClientCallbacks(&clientCallbacks, false);
        pClient->setConnectionParams(12, 24, 0, 200);
        pClient->setConnectTimeout(5000);
    }

    // Connect to the remote BLE Server
    if (!pClient->connect(pServerDevice)) {
        Serial.println("Failed to connect to server");
        return false;
    }

    Serial.println("Connected! Discovering services...");

    // Get the service
    NimBLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
    if (!pRemoteService) {
        Serial.println("Failed to find our service UUID");
        pClient->disconnect();
        return false;
    }
    Serial.println("Found the stream service");

    // Get the server's RX characteristic -- client writes here (our TX path)
    NimBLERemoteCharacteristic* pRxChar = pRemoteService->getCharacteristic(RX_CHAR_UUID);
    if (!pRxChar) {
        Serial.println("Failed to find RX characteristic");
        pClient->disconnect();
        return false;
    }
    Serial.println("Found the RX characteristic");

    // Get the server's TX characteristic -- client subscribes here (our RX path)
    NimBLERemoteCharacteristic* pTxChar = pRemoteService->getCharacteristic(TX_CHAR_UUID);
    if (!pTxChar) {
        Serial.println("Failed to find TX characteristic");
        pClient->disconnect();
        return false;
    }
    Serial.println("Found the TX characteristic");

    /**
     * Initialize the stream with the server's RX characteristic for writing (our TX).
     * The RX characteristic (6E400002) supports WRITE but not NOTIFY, so subscribeNotify
     * must be false. Notifications are handled separately on the TX characteristic below.
     */
    if (!bleStream.begin(pRxChar, false)) {
        Serial.println("Failed to initialize BLE stream!");
        pClient->disconnect();
        return false;
    }

    /**
     * Subscribe to the server's TX characteristic (6E400003) to receive notifications.
     * Received data is handled directly in the onServerNotify callback.
     */
    if (!pTxChar->subscribe(true, onServerNotify)) {
        Serial.println("Failed to subscribe to server TX characteristic");
        bleStream.end();
        pClient->disconnect();
        return false;
    }

    Serial.println("BLE Stream initialized successfully!");
    connected = true;
    return true;
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting NimBLE Stream Client");

    /** Initialize NimBLE */
    NimBLEDevice::init("NimBLE-StreamClient");

    /**
     * Create the BLE scan instance and set callbacks
     * Configure scan parameters
     */
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(&scanCallbacks, false);
    pScan->setActiveScan(true);

    /** Start scanning for the server */
    Serial.println("Scanning for BLE Stream Server...");
    pScan->start(scanTime, false, true);
}

void loop() {
    // If we found a server, try to connect
    if (doConnect) {
        doConnect = false;
        if (connectToServer()) {
            Serial.println("Stream ready for communication!");
        } else {
            Serial.println("Failed to connect to server, restarting scan...");
            pServerDevice = nullptr;
            NimBLEDevice::getScan()->start(scanTime, false, true);
        }
    }

    // If we're connected, use the stream to send data
    if (connected && bleStream) {
        // Send a message every 5 seconds using Stream methods
        static unsigned long lastSend = 0;
        if (millis() - lastSend > 5000) {
            lastSend = millis();

            // Using familiar Serial-like methods!
            bleStream.print("Hello from client! Uptime: ");
            bleStream.print(millis() / 1000);
            bleStream.println(" seconds");

            Serial.println("Sent data to server via BLE stream");
        }

        // Read from Serial and send over BLE
        if (Serial.available()) {
            Serial.println("Reading from Serial and sending via BLE:");
            while (Serial.available()) {
                char c = Serial.read();
                Serial.write(c);    // Echo locally
                bleStream.write(c); // Send via BLE
            }
            Serial.println();
        }
    }

    delay(10);
}
