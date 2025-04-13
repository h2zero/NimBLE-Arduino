/**
 *  NimBLE Extended Server Demo:
 *
 *  Demonstrates the Bluetooth 5.x extended advertising capabilities.
 *
 *  This demo will advertise a long data string on the CODED and 1M Phy's and
 *  starts a server allowing connection over either PHY's. It will advertise for
 *  5 seconds then sleep for 20 seconds, if a client connects it will sleep once
 *  it has disconnected then repeats.
 *
 *  Created: on April 2 2022
 *      Author: H2zero
 */

#include <Arduino.h>
#include <NimBLEDevice.h>
#if !CONFIG_BT_NIMBLE_EXT_ADV
# error Must enable extended advertising, see nimconfig.h file.
#endif

#ifdef ESP_PLATFORM
# include "esp_sleep.h"
#endif

#define SERVICE_UUID        "ABCD"
#define CHARACTERISTIC_UUID "1234"

/** Time in milliseconds to advertise */
static uint32_t advTime = 5000;

/** Time to sleep between advertisements */
static uint32_t sleepSeconds = 20;

/** Primary PHY used for advertising, can be one of BLE_HCI_LE_PHY_1M or BLE_HCI_LE_PHY_CODED */
static uint8_t primaryPhy = BLE_HCI_LE_PHY_CODED;

/**
 *  Secondary PHY used for advertising and connecting,
 *  can be one of BLE_HCI_LE_PHY_1M, BLE_HCI_LE_PHY_2M or BLE_HCI_LE_PHY_CODED
 */
static uint8_t secondaryPhy = BLE_HCI_LE_PHY_1M;

/** Handler class for server events */
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        Serial.printf("Client connected:: %s\n", connInfo.getAddress().toString().c_str());
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        Serial.printf("Client disconnected - sleeping for %" PRIu32 " seconds\n", sleepSeconds);
#ifdef ESP_PLATFORM
        esp_deep_sleep_start();
#else
        systemRestart(); // nRF platforms restart then sleep via delay in setup.
#endif
    }
} serverCallbacks;

/** Callback class to handle advertising events */
class AdvertisingCallbacks : public NimBLEExtAdvertisingCallbacks {
    void onStopped(NimBLEExtAdvertising* pAdv, int reason, uint8_t instId) override {
        /* Check the reason advertising stopped, don't sleep if client is connecting */
        Serial.printf("Advertising instance %u stopped\n", instId);
        switch (reason) {
            case 0:
                Serial.printf("Client connecting\n");
                return;
            case BLE_HS_ETIMEOUT:
                Serial.printf("Time expired - sleeping for %" PRIu32 " seconds\n", sleepSeconds);
                break;
            default:
                break;
        }

#ifdef ESP_PLATFORM
        esp_deep_sleep_start();
#else
        delay(sleepSeconds * 1000); // system ON sleep mode for nRF platforms to simulate the esp deep sleep with timer wakeup
        systemRestart();            // nRF platforms restart then sleep via delay in setup.
#endif
    }
} advertisingCallbacks;

void setup() {
    Serial.begin(115200);

    /** Initialize NimBLE and set the device name */
    NimBLEDevice::init("Extended advertiser");

    /** Create the server and add the services/characteristics/descriptors */
    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);

    NimBLEService*        pService = pServer->createService(SERVICE_UUID);
    NimBLECharacteristic* pCharacteristic =
        pService->createCharacteristic(CHARACTERISTIC_UUID,
                                       NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);

    pCharacteristic->setValue("Hello World");

    /** Start the service */
    pService->start();

    /**
     *  Create an extended advertisement with the instance ID 0 and set the PHY's.
     *  Multiple instances can be added as long as the instance ID is incremented.
     */
    NimBLEExtAdvertisement extAdv(primaryPhy, secondaryPhy);

    /** Set the advertisement as connectable */
    extAdv.setConnectable(true);

    /** As per Bluetooth specification, extended advertising cannot be both scannable and connectable */
    extAdv.setScannable(false); // The default is false, set here for demonstration.

    /** Extended advertising allows for 251 bytes (minus header bytes ~20) in a single advertisement or up to 1650 if chained */
    extAdv.setServiceData(NimBLEUUID(SERVICE_UUID),
                          std::string("Extended Advertising Demo.\r\n"
                                      "Extended advertising allows for "
                                      "251 bytes of data in a single advertisement,\r\n"
                                      "or up to 1650 bytes with chaining.\r\n"
                                      "This example message is 226 bytes long "
                                      "and is using CODED_PHY for long range."));

    extAdv.setCompleteServices16({NimBLEUUID(SERVICE_UUID)});
    extAdv.setName("Extended advertiser");

    /** When extended advertising is enabled `NimBLEDevice::getAdvertising` returns a pointer to `NimBLEExtAdvertising */
    NimBLEExtAdvertising* pAdvertising = NimBLEDevice::getAdvertising();

    /** Set the callbacks for advertising events */
    pAdvertising->setCallbacks(&advertisingCallbacks);

    /**
     *  NimBLEExtAdvertising::setInstanceData takes the instance ID and
     *  a reference to a `NimBLEExtAdvertisement` object. This sets the data
     *  that will be advertised for this instance ID, returns true if successful.
     *
     *  Note: It is safe to create the advertisement as a local variable if setInstanceData
     *  is called before exiting the code block as the data will be copied.
     */
    if (pAdvertising->setInstanceData(0, extAdv)) {
        /**
         *  NimBLEExtAdvertising::start takes the advertisement instance ID to start
         *  and a duration in milliseconds or a max number of advertisements to send (or both).
         */
        if (pAdvertising->start(0, advTime)) {
            Serial.printf("Started advertising\n");
        } else {
            Serial.printf("Failed to start advertising\n");
        }
    } else {
        Serial.printf("Failed to register advertisement data\n");
    }

#ifdef ESP_PLATFORM
    esp_sleep_enable_timer_wakeup(sleepSeconds * 1000000);
#endif
}

void loop() {}
