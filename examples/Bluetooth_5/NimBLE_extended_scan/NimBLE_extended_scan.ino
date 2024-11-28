/**
 *  NimBLE Extended Scanner Demo:
 *
 *  Demonstrates the Bluetooth 5.x scanning capabilities of the NimBLE library.
 *
 *  Created: on November 28, 2024
 *      Author: H2zero
 *
*/

#include <Arduino.h>
#include <NimBLEDevice.h>

static uint32_t scanTime = 10 * 1000; // In milliseconds, 0 = scan forever
static NimBLEScan::Phy scanPhy = NimBLEScan::Phy::SCAN_ALL;

// Define a class to handle the callbacks when advertisements are received
class scanCallbacks: public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) {
        Serial.printf("Advertised Device found: %s\n PHY1: %d\n PHY2: %d\n", advertisedDevice->toString().c_str(),
               advertisedDevice->getPrimaryPhy(), advertisedDevice->getSecondaryPhy());
    }

    // Callback to process the results of the completed scan or restart it
    void onScanEnd(const NimBLEScanResults& scanResults, int reason) {
        Serial.printf("Scan Ended, reason: %d; found %d devices\n", reason, scanResults.getCount());

        // Try Different PHY's
        switch (scanPhy) {
            case NimBLEScan::Phy::SCAN_ALL:
                Serial.prinln("Scanning only 1M PHY");
                scanPhy = NimBLEScan::Phy::SCAN_1M;
                break;
            case NimBLEScan::Phy::SCAN_1M:
                Serial.println("Scanning only CODED PHY");
                scanPhy = NimBLEScan::Phy::SCAN_CODED;
                break;
            case NimBLEScan::Phy::SCAN_CODED:
                Serial.println("Scanning all PHY's");
                scanPhy = NimBLEScan::Phy::SCAN_ALL;
                break;
        }

        NimBLEScan* pScan = NimBLEDevice::getScan();
        pScan->setPhy(scanPhy);
        pScan->start(scanTime);
    }
} scanCb;

void setup() {
    Serial.begin(115200);
    Serial.printf("Starting Extended Scanner\n");

    // Initialize NimBLE, no device name specified as we are not advertising
    NimBLEDevice::init("");
    NimBLEScan* pScan = NimBLEDevice::getScan();

    // Set the callbacks that the scanner will call on events.
    pScan->setScanCallbacks(&scanCb);

    // Use active scanning to obtain scan response data from advertisers
    pScan->setActiveScan(true);

    // Set the initial PHY's to scan on, default is SCAN_ALL
    pScan->setPhy(scanPhy);

    // Start scanning for scanTime, 0 = forever
    pScan->start(scanTime);
    Serial.println("Scanning for peripherals");
}

void loop() {
    delay(2000);
}
