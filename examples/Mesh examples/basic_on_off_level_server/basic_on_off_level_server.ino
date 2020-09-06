#include "NimBLEDevice.h"
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

// LED pins
#define LEDR 16
#define LEDG 17
#define LEDB 18

static uint8_t onOffVal = 0; 
static int16_t levelVal = 0; 

class onOffSrvModelCallbacks : public NimBLEMeshModelCallbacks {
  void setOnOff(uint8_t val) {
    Serial.printf("on/off set val %d\n", val);
    onOffVal = val;
    digitalWrite(LEDG, !onOffVal);
  }

  uint8_t getOnOff() {
    Serial.printf("on/off get val %d\n", onOffVal);
    return onOffVal;
  }
};

class levelSrvModelCallbacks : public NimBLEMeshModelCallbacks {
  void setLevel(int16_t val) {
    Serial.printf("Level set val %d\n", val);
    levelVal = val;
  }

  int16_t getLevel() {
    Serial.printf("Level get val %d\n", levelVal);
    return levelVal;
  }
};


void setup() {
  Serial.begin(115200);
  pinMode(LEDG,OUTPUT);
  digitalWrite(LEDG, HIGH);
  
  NimBLEDevice::init("");
  NimBLEMeshNode *pMesh = NimBLEDevice::createMeshNode(NimBLEUUID(SERVICE_UUID),0);
  NimBLEMeshElement* pElem = pMesh->getElement();
  pElem->createModel(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, new onOffSrvModelCallbacks());
  //pElem = pMesh->createElement();
  pElem->createModel(BT_MESH_MODEL_ID_GEN_LEVEL_SRV, new levelSrvModelCallbacks());
  pMesh->start();
  Serial.println("Mesh Started");
}

void loop() {
  delay(1000);
}