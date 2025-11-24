/**
 *  NimBLE_Stream_Server Example:
 *
 *  Demonstrates using NimBLEStreamServer to create a BLE GATT server
 *  that behaves like a serial port using the Arduino Stream interface.
 *  
 *  This allows you to use familiar methods like print(), println(), 
 *  read(), and available() over BLE, similar to how you would use Serial.
 *
 *  Created: November 2025
 *      Author: NimBLE-Arduino Contributors
 */

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEStream.h>

// Create the stream server instance
NimBLEStreamServer bleStream;

// Service and Characteristic UUIDs for the stream
// Using custom UUIDs - you can change these as needed
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

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

    /** 
     * Create the BLE server and set callbacks 
     * Note: The stream will create its own service and characteristic
     */
    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);

    /**
     * Initialize the stream server with:
     * - Service UUID
     * - Characteristic UUID  
     * - canWrite: true (allows clients to write data to us)
     * - secure: false (no encryption required - set to true for secure connections)
     */
    if (!bleStream.init(NimBLEUUID(SERVICE_UUID), 
                        NimBLEUUID(CHARACTERISTIC_UUID),
                        true,   // canWrite - allow receiving data
                        false)) // secure
    {
        Serial.println("Failed to initialize BLE stream!");
        return;
    }

    /** Start the stream (begins internal buffers and tasks) */
    if (!bleStream.begin()) {
        Serial.println("Failed to start BLE stream!");
        return;
    }

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
    // Check if a client is subscribed (connected and listening)
    if (bleStream.hasSubscriber()) {
        
        // Send a message every 2 seconds using Stream methods
        static unsigned long lastSend = 0;
        if (millis() - lastSend > 2000) {
            lastSend = millis();
            
            // Using familiar Serial-like methods!
            bleStream.print("Hello from BLE Server! Time: ");
            bleStream.println(millis());
            
            // You can also use printf
            bleStream.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
            
            Serial.println("Sent data to client via BLE stream");
        }

        // Check if we received any data from the client
        if (bleStream.available()) {
            Serial.print("Received from client: ");
            
            // Read all available data (just like Serial.read())
            while (bleStream.available()) {
                char c = bleStream.read();
                Serial.write(c);  // Echo to Serial
                bleStream.write(c);  // Echo back to BLE client
            }
            Serial.println();
        }
    } else {
        // No subscriber, just wait
        delay(100);
    }
}
