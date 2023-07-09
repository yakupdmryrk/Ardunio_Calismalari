//#include <BluetoothSerial.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <math.h>
#include <BLECharacteristic.h>
#include <Arduino.h>

//BLE Server
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool cihazBaglanti = false;
bool eskiCihazBaglanti = false;
float temp;
float temp1;
float temp3;
float onceTemp3 = 0.0;
unsigned long gecenZaman=0;
unsigned long onceGecenZaman=0;
unsigned long okumaBasladi=0;
unsigned long okumaBitti=0;
int xFirstRaw = 0;
int yFirstRaw = 0;
int zFirstRaw = 0;
int i = 0;
float voltageRef = 5;
float accelerationScale = voltageRef / 1023.0;  // Ölçek faktörü hesaplama
float accelerationX;
float accelerationY;
float accelerationZ;
float acceleration3;
float onceAcceleration3 = 0.0;
static bool ilkOkuma = true;
static bool kalibrasyon = false;
static bool basiAlindi = false;
String receivedValue;
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "1ef6b806-899a-402c-a7db-f78b6c36259b"
#define X_PIN 34
#define Y_PIN 35
#define Z_PIN 36
#define pressurePin 39

float xRaw = analogRead(X_PIN);
float yRaw = analogRead(Y_PIN);
float zRaw = analogRead(Z_PIN);
float flexValue = analogRead(pressurePin); // Basınç sensöründen okunan değer
float onceFlexValue = 0;
float geciciFlexValue = 0;
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    cihazBaglanti = true;
  };
  void onDisconnect(BLEServer* pServer)
  {
    cihazBaglanti = false;
  }
};
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    ilkOkuma = true;
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      for (int i = 0; i < value.length(); i++) {
        Serial.print(value[i]);
        }
      }
      Serial.println();
      if(value=="#Basi#"){
      onRead(pCharacteristic);
    }
  }
  void onRead(BLECharacteristic* pCharacteristic){
    okumaBitti=micros();
    gecenZaman = okumaBitti - okumaBasladi;
    if(ilkOkuma&&(flexValue==0)){
      xRaw = analogRead(X_PIN);
      yRaw = analogRead(Y_PIN);
      zRaw = analogRead(Z_PIN);
      xFirstRaw = xRaw;
      yFirstRaw = yRaw;
      zFirstRaw = zRaw;
    }
    flexValue = analogRead(pressurePin);
    if(kalibrasyon&&!ilkOkuma&&flexValue>0||(kalibrasyon&&!ilkOkuma&&flexValue==0))
    { 
      if(flexValue!=0)
      {
      Serial.println(onceFlexValue);
      Serial.println(flexValue);
      }

      xRaw = analogRead(X_PIN);
      yRaw = analogRead(Y_PIN);
      zRaw = analogRead(Z_PIN);
      accelerationX = ((xRaw-xFirstRaw) * accelerationScale);
      accelerationY = ((yRaw-yFirstRaw) * accelerationScale);
      accelerationZ = ((zRaw-zFirstRaw) * accelerationScale);
      acceleration3 = sqrt(pow(accelerationX, 2) + pow(accelerationY, 2) + pow(accelerationZ, 2));
      temp3 = acceleration3*(pow(gecenZaman, 2));
      
      if(onceFlexValue>flexValue)
      {
      Serial.print("ivme: ");
      Serial.print(onceAcceleration3);
      Serial.println(" m/s^2");   
      Serial.print("Basinc: ");
      Serial.print(onceFlexValue);
      Serial.println("%");
      Serial.println(onceGecenZaman);
      Serial.println(onceTemp3);
      temp = onceAcceleration3;
      temp1 = onceFlexValue;
      if(cihazBaglanti){
        char gonderilecek[24];
        dtostrf(temp,6,2,gonderilecek);
        strcat(gonderilecek, "/");
        dtostrf(temp1, 6, 2, gonderilecek + strlen(gonderilecek));
        pCharacteristic->setValue(gonderilecek);
        pCharacteristic->notify();
        Serial.print("Gonderilen: ");
        Serial.println(gonderilecek);
      }
      }
    }
    onceTemp3 = temp3;
    onceGecenZaman = gecenZaman;
    onceFlexValue = flexValue;
    onceAcceleration3 = acceleration3;
    okumaBasladi=okumaBitti;
    ilkOkuma = false; 
    kalibrasyon = true; 
    return;
  }
};
void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  okumaBasladi=micros();
  BLEDevice::init("ESP32");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                      );
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  BLEDevice::startAdvertising();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();
  Serial.println("Bilgi aktarmak için istemcinin bağlanması bekleniyor");
}
void loop() {
  if (!cihazBaglanti && eskiCihazBaglanti){
  pServer->startAdvertising();
  Serial.println("Yayın başlatıldı");
  eskiCihazBaglanti = cihazBaglanti;
  }
  if (cihazBaglanti && !eskiCihazBaglanti){
    eskiCihazBaglanti = cihazBaglanti;
  }
}