
/** NimBLE_Server Demo:
 *
 *  Demonstrates many of the available features of the NimBLE client library.
 *  
 *  Created: on March 24 2020
 *      Author: H2zero
 * 
*/

#include <NimBLEDevice.h>

static NimBLERemoteService* pSvc = nullptr;
static NimBLERemoteCharacteristic* pChr = nullptr;
static NimBLERemoteDescriptor* pDsc = nullptr;

static NimBLEAdvertisedDevice* advDevice;

static NimBLEClient* pClient_a[3] = {nullptr};
static uint8_t cCount = 0;
static bool doConnect = false;


class ClientCallbacks : public NimBLEClientCallbacks {
	void onConnect(NimBLEClient* pClient) {
		Serial.println("Connected");
        //pClient->updateConnParams(24,48,0,800);
	};

	void onDisconnect(NimBLEClient* pClient) {
		Serial.println("Disconnected");
	};
	
	uint32_t onPassKeyRequest(){
		Serial.println("Client Passkey Request");
		return 123456;
	};

	bool onConfirmPIN(uint32_t pass_key){
		Serial.print("The passkey YES/NO number: ");
        Serial.println(pass_key);
		return true;
	};

	void onAuthenticationComplete(ble_gap_conn_desc* desc){
		if(!desc->sec_state.encrypted) {
            NimBLEDevice::getClientByID(desc->conn_handle)->disconnect();
            Serial.println("Encrypt connection failed - disconnecting");
            return;
        }
	};
};


class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
	void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
		Serial.print("Advertised Device found: ");
        Serial.println(advertisedDevice->toString().c_str());
		if(advertisedDevice->isAdvertisingService(NimBLEUUID("DEAD")))
		{
			Serial.println("Found Our Service");
			NimBLEDevice::getScan()->stop();

			advDevice = advertisedDevice;
			doConnect = true;
		}
	}
};


void scanEndedCB(NimBLEScanResults results){
	Serial.println("Scan Ended");
}


void notifyCB(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify){
    Serial.print((isNotify == true) ? "Notify" : "Indication");
    Serial.print(" from ");
	Serial.println(pBLERemoteCharacteristic->getUUID().toString().c_str());
	Serial.print("Value: ");
    Serial.println(std::string((char*)pData, length).c_str());
}


void setup (){
  Serial.begin(115200);
  Serial.println("Starting NimBLE Client");
  NimBLEDevice::init("");
  
//  BLEDevice::addIgnored(NimBLEAddress ("cc:b8:6c:d6:a8:82"));

//  BLEDevice::setPower(ESP_PWR_LVL_P9);
  	
  NimBLEScan* pScan = NimBLEDevice::getScan(); //create new scan
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  pScan->setInterval(1349);
  pScan->setWindow(449);
  pScan->setActiveScan(true);
  pScan->start(0, scanEndedCB);
}


void loop (){
	while(!doConnect){
		delay(10);
	}
	doConnect = false;
    
	pClient_a[cCount] = NimBLEDevice::createClient();
	NimBLEClient* pClient = pClient_a[cCount];
	cCount++;
	pClient->setClientCallbacks(new ClientCallbacks());
    pClient->setConnectionParams(6,6,0,400);
    pClient->setConnectTimeout(10);
	if (pClient->connect(advDevice)) {
        Serial.print("Connected to: ");
        Serial.println(pClient->getPeerAddress().toString().c_str());
        Serial.print("RSSI: ");
        Serial.println(pClient->getRssi());
        
		pSvc = pClient->getService("DEAD");
        if(pSvc) {
            pChr = pSvc->getCharacteristic("BEEF");
        }
		
		if(pChr != nullptr) {
            if(pChr->canRead()) {
                Serial.print(pChr->getUUID().toString().c_str());
                Serial.print(" Value: ");
                Serial.println(pChr->readValue().c_str());
            }
            
            if(pChr->canWrite()) {
                if(pChr->writeValue("Tasty")) {
                    Serial.print("Wrote new value to: ");
                    Serial.println(pChr->getUUID().toString().c_str());
                }
                else {
                    pClient->disconnect();
                    cCount--;
                    NimBLEDevice::deleteClient(pClient);
                    return;
                }
                
                if(pChr->canRead()) {
                    Serial.print("The value of: ");
                    Serial.print(pChr->getUUID().toString().c_str());
                    Serial.print(" is now: ");
                    Serial.println(pChr->readValue().c_str());
                }
            }
            
            if(pChr->canNotify()) {
                pChr->registerForNotify(notifyCB);
            }
            else if(pChr->canIndicate()) {
                pChr->registerForNotify(notifyCB, false);
            }    
        }
        
        else{
			Serial.println("DEAD service not found.");
		}
        
        pSvc = pClient->getService("BAAD");
        if(pSvc) {
            pChr = pSvc->getCharacteristic("F00D");
        }
        
        if(pChr != nullptr) {
            if(pChr->canRead()) {
                Serial.print(pChr->getUUID().toString().c_str());
                Serial.print(" Value: ");
                Serial.println(pChr->readValue().c_str());
            }
            
            pDsc = pChr->getDescriptor(NimBLEUUID("C01D"));
            if(pDsc != nullptr) {
                Serial.print(pDsc->getUUID().toString().c_str());
                Serial.print(" value: ");
                Serial.println(pDsc->readValue().c_str());
            }
            
            if(pChr->canWrite()) {
                if(pChr->writeValue("Take it back")) {
                    Serial.print("Wrote new value to: ");
                    Serial.println(pChr->getUUID().toString().c_str());
                }
                else {
                    pClient->disconnect();
                    cCount--;
                    NimBLEDevice::deleteClient(pClient);
                    return;
                }
                
                if(pChr->canRead()) {
                    Serial.print("The value of: ");
                    Serial.print(pChr->getUUID().toString().c_str());
                    Serial.print(" is now: ");
                    Serial.println(pChr->readValue().c_str());
                }
            }
            
            if(pChr->canNotify()) {
                pChr->registerForNotify(notifyCB);
            }
            else if(pChr->canIndicate()) {
                pChr->registerForNotify(notifyCB, false);
            }    
        }        
	
		else{
			Serial.println("BAAD service not found.");
		}
		
		if(cCount>2){
            cCount--;
			NimBLEDevice::deleteClient(pClient);
		}
	}
	else{
		cCount--;
		NimBLEDevice::deleteClient(pClient);
	}
    
    NimBLEDevice::getScan()->start(0,scanEndedCB);
}
