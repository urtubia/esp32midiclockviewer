#include <NeoPixelBus.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// Depends on the NeoPixelBus 
// https://github.com/Makuna/NeoPixelBus
// and Espressif esp32
// https://github.com/espressif/arduino-esp32

const uint16_t PixelCount = 32; 
const uint8_t PixelPin = 15;

#define colorSaturation 128

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

HslColor hslRed(red);
HslColor hslGreen(green);
HslColor hslBlue(blue);
HslColor hslWhite(white);
HslColor hslBlack(black);

void setupNeoPixels(){
  strip.Begin();
  strip.Show();
}

#define SERVICE_UUID        "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
#define CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class NeoPixelAnimation {
  public:
    NeoPixelAnimation() : _lightOn(false), _cleared(false)
    {}
    void turnOn() {
      _lightOn = true;
      _timeToTurnOff = millis() + 100;
      _cleared = false;
    }
    void loop() {
      if (_lightOn) {
        if (millis() > _timeToTurnOff) {
          _lightOn = false;
        }
        strip.SetPixelColor(0, red);
        strip.SetPixelColor(1, green);
        strip.SetPixelColor(2, blue);
        strip.SetPixelColor(3, white);
        strip.Show();
      }else{
        if(!_cleared){
          strip.ClearTo(black);
          strip.Show();
          _cleared = true;
        }
      }
      
    }
  private:
    bool _lightOn;
    unsigned long _timeToTurnOff;
    bool _cleared;
};


NeoPixelAnimation anim;

class MyCharacteristicCallbackHandler: public BLECharacteristicCallbacks {
  virtual void onRead(BLECharacteristic *pCharacteristic) {
  }
  virtual void onWrite(BLECharacteristic *pCharacteristic) {
    std::string val = pCharacteristic->getValue();
    int size = val.size();
    if(size == 3){
      if(val.at(2) == 0xf8){ // ppqn message
        _ppqnCounter = (_ppqnCounter + 1) % 24;
        if(_ppqnCounter == 0){
          _beatCounter = (_beatCounter + 1) % 4;
          Serial.println(_beatCounter);
          anim.turnOn();
        }
      }else if(val.at(2) == 0xfa){
        _ppqnCounter = -1;
        _beatCounter = -1;
      }
      else if(val.at(2) == 0xfc){
        _ppqnCounter = -1;
        _beatCounter = -1;
      }
    }
  }

  int _ppqnCounter;
  int _beatCounter;
};

void setupBLEServer() {
  BLEDevice::init("ESP32 MIDI Example");
    
  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID));

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
    BLEUUID(CHARACTERISTIC_UUID),
    BLECharacteristic::PROPERTY_READ   |
    BLECharacteristic::PROPERTY_WRITE  |
    BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_WRITE_NR
  );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  pCharacteristic->setCallbacks(new MyCharacteristicCallbackHandler());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(pService->getUUID());
  pAdvertising->start();
}


void setup() {
  Serial.begin(115200);
  setupBLEServer();
  setupNeoPixels();
  Serial.println("Hello world, all setup");
}

void loop() {
 anim.loop();
}
