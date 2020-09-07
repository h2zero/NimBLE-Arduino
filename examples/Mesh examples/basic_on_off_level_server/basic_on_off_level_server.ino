#include "NimBLEDevice.h"
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

// LED pins
#define LEDR 16
#define LEDG 17
#define LEDB 18

static uint8_t onOffVal = 0; 
static int16_t levelVal = 0; 

class onOffSrvModelCallbacks : public NimBLEMeshModelCallbacks {
  void setOnOff(NimBLEMeshModel *pModel, uint8_t val) {
    Serial.printf("on/off set val %d, transition time: %dms\n", val, pModel->getTransTime());
    onOffVal = val;
    digitalWrite(LEDG, !onOffVal);
    pModel->publish();
  }

  uint8_t getOnOff(NimBLEMeshModel *pModel) {
    Serial.printf("on/off get val %d\n", onOffVal);
    return onOffVal;
  }
};

class levelSrvModelCallbacks : public NimBLEMeshModelCallbacks {
  void setLevel(NimBLEMeshModel *pModel, int16_t val) {
    Serial.printf("Level set val %d, transition time: %dms\n", val, pModel->getTransTime());
    levelVal = val;
    pModel->publish();
  }

  int16_t getLevel(NimBLEMeshModel *pModel) {
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
  NimBLEMeshModel* pModel = pElem->createModel(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, new onOffSrvModelCallbacks());
  pModel->setValue(onOffVal);
  //pElem = pMesh->createElement();
  pModel = pElem->createModel(BT_MESH_MODEL_ID_GEN_LEVEL_SRV, new levelSrvModelCallbacks());
  pModel->setValue(levelVal);
  pMesh->start();
  Serial.println("Mesh Started");
}

void loop() {
  delay(1000);
}