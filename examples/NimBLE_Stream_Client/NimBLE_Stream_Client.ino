/**
 *  NimBLE_Stream_Client Example:
 *
 *  Demonstrates using NimBLEStreamClient to connect to a BLE GATT server
 *  and communicate using the Arduino Stream interface.
 *  
 *  This allows you to use familiar methods like print(), println(), 
 *  read(), and available() over BLE, similar to how you would use Serial.
 *
 *  This example connects to the NimBLE_Stream_Server example.
 *
 *  Created: November 2025
 *      Author: NimBLE-Arduino Contributors
 */

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEStream.h>

// Service and Characteristic UUIDs (must match the server)
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

// Create the stream client instance
NimBLEStreamClient bleStream;

// Connection state variables
static bool                          doConnect     = false;
static bool                          connected     = false;
static const NimBLEAdvertisedDevice* pServerDevice = nullptr;
static NimBLEClient*                 pClient       = nullptr;

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
            NimBLEDevice::getScan()->start(5, false, true);
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
        bleStream.deinit();
        
        // Restart scanning
        Serial.println("Restarting scan...");
        NimBLEDevice::getScan()->start(5, false, true);
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

    // Get the service and characteristic
    NimBLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
    if (!pRemoteService) {
        Serial.println("Failed to find our service UUID");
        pClient->disconnect();
        return false;
    }
    Serial.println("Found the stream service");

    NimBLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
    if (!pRemoteCharacteristic) {
        Serial.println("Failed to find our characteristic UUID");
        pClient->disconnect();
        return false;
    }
    Serial.println("Found the stream characteristic");

    /**
     * Initialize the stream client with the remote characteristic
     * subscribeNotify=true means we'll receive notifications in the RX buffer
     */
    if (!bleStream.init(pRemoteCharacteristic, true)) {
        Serial.println("Failed to initialize BLE stream!");
        pClient->disconnect();
        return false;
    }

    /** Start the stream (begins internal buffers and tasks) */
    if (!bleStream.begin()) {
        Serial.println("Failed to start BLE stream!");
        bleStream.deinit();
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
    pScan->setInterval(100);
    pScan->setWindow(99);
    pScan->setActiveScan(true);

    /** Start scanning for the server */
    Serial.println("Scanning for BLE Stream Server...");
    pScan->start(5, false, true);
}

void loop() {
    // If we found a server, try to connect
    if (doConnect) {
        doConnect = false;
        if (connectToServer()) {
            Serial.println("Stream ready for communication!");
        } else {
            Serial.println("Failed to connect to server");
            // Scan will restart automatically
        }
    }

    // If we're connected, demonstrate the stream interface
    if (connected && bleStream) {
        
        // Check if we received any data from the server
        if (bleStream.available()) {
            Serial.print("Received from server: ");
            
            // Read all available data (just like Serial.read())
            while (bleStream.available()) {
                char c = bleStream.read();
                Serial.write(c);
            }
            Serial.println();
        }

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

        // You can also read from Serial and send over BLE
        if (Serial.available()) {
            Serial.println("Reading from Serial and sending via BLE:");
            while (Serial.available()) {
                char c = Serial.read();
                Serial.write(c);  // Echo locally
                bleStream.write(c);  // Send via BLE
            }
            Serial.println();
        }
    }

    delay(10);
}
