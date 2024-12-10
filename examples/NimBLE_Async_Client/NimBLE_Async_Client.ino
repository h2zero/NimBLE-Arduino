
/**
 *  NimBLE_Async_client Demo:
 *
 *  Demonstrates asynchronous client operations.
 *
 *  Created: on November 4, 2024
 *      Author: H2zero
 */

#include <NimBLEDevice.h>

static constexpr uint32_t scanTimeMs = 5 * 1000;

class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) {
        Serial.printf("Connected to: %s\n", pClient->getPeerAddress().toString().c_str());
    }

    void onDisconnect(NimBLEClient* pClient, int reason) {
        Serial.printf("%s Disconnected, reason = %d - Starting scan\n", pClient->getPeerAddress().toString().c_str(), reason);
        NimBLEDevice::getScan()->start(scanTimeMs);
    }
} clientCB;

class scanCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) {
        Serial.printf("Advertised Device found: %s\n", advertisedDevice->toString().c_str());
        if (advertisedDevice->haveName() && advertisedDevice->getName() == "NimBLE-Server") {
            Serial.printf("Found Our Device\n");

            /** Async connections can be made directly in the scan callbacks */
            auto pClient = NimBLEDevice::getDisconnectedClient();
            if (!pClient) {
                pClient = NimBLEDevice::createClient(advertisedDevice->getAddress());
                if (!pClient) {
                    Serial.printf("Failed to create client\n");
                    return;
                }
            }

            pClient->setClientCallbacks(&clientCB, false);
            if (!pClient->connect(true, true, false)) { // delete attributes, async connect, no MTU exchange
                NimBLEDevice::deleteClient(pClient);
                Serial.printf("Failed to connect\n");
                return;
            }
        }
    }

    void onScanEnd(const NimBLEScanResults&, int reason) {
        Serial.printf("Scan Ended\n");
        NimBLEDevice::getScan()->start(scanTimeMs);
    }
};

void setup() {
    Serial.begin(115200);
    Serial.printf("Starting NimBLE Async Client\n");
    NimBLEDevice::init("Async-Client");
    NimBLEDevice::setPower(3); /** +3db */

    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(new scanCallbacks());
    pScan->setInterval(45);
    pScan->setWindow(45);
    pScan->setActiveScan(true);
    pScan->start(scanTimeMs);
}

void loop() {
    delay(1000);
    auto pClients = NimBLEDevice::getConnectedClients();
    if (!pClients.size()) {
        return;
    }

    for (auto& pClient : pClients) {
        Serial.printf("%s\n", pClient->toString().c_str());
        NimBLEDevice::deleteClient(pClient);
    }

    NimBLEDevice::getScan()->start(scanTimeMs);
}
