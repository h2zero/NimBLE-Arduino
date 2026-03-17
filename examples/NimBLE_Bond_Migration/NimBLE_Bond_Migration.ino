/**
 * NimBLE_Bond_Migration Demo:
 *
 * This example demonstrates running one-time bond-store migration before
 * NimBLE initialization. Set NIMBLE_BOND_MIGRATION_MODE to select direction:
 * 0 = off, 1 = migrate to current, 2 = migrate to v1.
 */

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEBondMigration.h>

namespace NimBLE_Bond_Migration = NimBLEBondMigration;

#ifndef NIMBLE_BOND_MIGRATION_MODE
#define NIMBLE_BOND_MIGRATION_MODE 0
#endif

#if (NIMBLE_BOND_MIGRATION_MODE < 0) || (NIMBLE_BOND_MIGRATION_MODE > 2)
#error "NIMBLE_BOND_MIGRATION_MODE must be 0 (off), 1 (to current), or 2 (to v1)"
#endif

void setup() {
    Serial.begin(115200);
    Serial.println("Starting NimBLE_Bond_Migration Demo");

#if NIMBLE_BOND_MIGRATION_MODE == 1
    {
        esp_err_t rc = NimBLE_Bond_Migration::migrateBondStoreToCurrent();
        Serial.printf("NimBLE_Bond_Migration to current rc=%d\n", static_cast<int>(rc));
    }
#elif NIMBLE_BOND_MIGRATION_MODE == 2
    {
        esp_err_t rc = NimBLE_Bond_Migration::migrateBondStoreToV1();
        Serial.printf("NimBLE_Bond_Migration to v1 rc=%d\n", static_cast<int>(rc));
        if (rc == ESP_OK) {
            Serial.println("NimBLE_Bond_Migration completed to v1, upload v1 firmware to continue");
            while (true) {
                delay(1000);
            }
        } else {
            Serial.println("NimBLE_Bond_Migration to v1 failed");
        }
    }
#endif

    NimBLEDevice::init("NimBLE");
    NimBLEDevice::setPower(3); /** +3db */

    NimBLEDevice::setSecurityAuth(true, true, false); /** bonding, MITM, don't need BLE secure connections as we are using passkey pairing */
    NimBLEDevice::setSecurityPasskey(123456);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY); /** Display only passkey */
    NimBLEServer*         pServer                  = NimBLEDevice::createServer();
    NimBLEService*        pService                 = pServer->createService("ABCD");
    NimBLECharacteristic* pNonSecureCharacteristic = pService->createCharacteristic("1234", NIMBLE_PROPERTY::READ);
    NimBLECharacteristic* pSecureCharacteristic =
        pService->createCharacteristic("1235",
                                       NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN);

    pNonSecureCharacteristic->setValue("Hello Non Secure BLE");
    pSecureCharacteristic->setValue("Hello Secure BLE");

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID("ABCD");
    pAdvertising->start();
}

void loop() {}
