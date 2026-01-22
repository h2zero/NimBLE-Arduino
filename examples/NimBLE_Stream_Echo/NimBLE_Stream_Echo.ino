/**
 *  NimBLE_Stream_Echo Example:
 *
 *  A minimal example demonstrating NimBLEStreamServer.
 *  Echoes back any data received from BLE clients.
 *  
 *  This is the simplest way to use the NimBLE Stream interface,
 *  showing just the essential setup and read/write operations.
 *
 *  Created: November 2025
 *      Author: NimBLE-Arduino Contributors
 */

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEStream.h>

NimBLEStreamServer bleStream;

void setup() {
    Serial.begin(115200);
    Serial.println("NimBLE Stream Echo Server");

    // Initialize BLE
    NimBLEDevice::init("BLE-Echo");
    NimBLEDevice::createServer();

    // Initialize stream with default UUIDs, allow writes
    bleStream.init(NimBLEUUID(uint16_t(0xc0de)),  // Service UUID
                   NimBLEUUID(uint16_t(0xfeed)),  // Characteristic UUID
                   true,                           // canWrite
                   false);                         // secure
    bleStream.begin();

    // Start advertising
    NimBLEDevice::getAdvertising()->start();
    Serial.println("Ready! Connect with a BLE client and send data.");
}

void loop() {
    // Echo any received data back to the client
    if (bleStream.hasSubscriber() && bleStream.available()) {
        Serial.print("Echo: ");
        while (bleStream.available()) {
            char c = bleStream.read();
            Serial.write(c);
            bleStream.write(c);  // Echo back
        }
        Serial.println();
    }
    delay(10);
}
