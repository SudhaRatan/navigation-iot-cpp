#include <Wire.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <string>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
#include <SPI.h>
#include <MPU6500_Raw.h>
#include "ble.cpp"

// Screens
#include "headers/StateMachine.h"
// #include "navigation.cpp"
#include "headers/WelcomeScreen.h"
#include "headers/ConfigScreen.h"

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define SDA_PIN 8
#define SCL_PIN 9
// SPIClass hspi(HSPI);
// display
#define TFT_CS 5
#define TFT_DC 7
#define TFT_RST 10
#define TFT_MOSI 6
#define TFT_SCLK 4

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft); // framebuffer

Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);
MPU6500 mpu;

#pragma region GlobalStates
String mapBuffer = "";
String secBuffer = "";

bool receivingMap = false;
bool receivingSec = false;

uint8_t secBinary[20000]; // adjust if needed
int secBinaryLen = 0;

uint8_t mapBinary[20000]; // adjust if needed
int mapBinaryLen = 0;

const double scale = 0.4;

bool deviceConnected = false;

float ZOOM = 3;
float heading_degrees = 0.0;
unsigned long lastDrawTime = 0;
unsigned long currentMillis = millis();

#pragma endregion GlobalStates

#pragma region DependencyInjection

BLEService *pService = nullptr;
TFT_Display tftDisplay(sprite, mapBuffer, secBuffer, secBinaryLen, mapBinaryLen, deviceConnected, secBinary, mapBinary, ZOOM, heading_degrees);
MyServerCallbacks serverCallbacks(tftDisplay, deviceConnected);
StateMachine stateManager(deviceConnected, currentMillis);
WelcomeScreen welcomeScreen(sprite, stateManager);
ConfigScreen configScreen(sprite, deviceConnected, stateManager, &pService);
// ScreenNavigation navigation(tftDisplay, deviceConnected, currentMillis, receivingSec, receivingMap, secBinaryLen, mapBinaryLen, lastDrawTime);

State *states[4] = {&welcomeScreen, &configScreen, nullptr, nullptr};

#pragma endregion DependencyInjection

void setup()
{
  delay(1000);
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN); // ESP32 I2C pins
  Wire.setClock(400000);        // 400kHz Fast Mode (default is 100kHz)
  Serial.println("Starting");
  if (!mag.begin())
  {
    Serial.println("HMC5883 not detected");
    while (1)
      ;
  }

  mpu.setup(0x68);
  // mpu.calibrateAccelGyro();

  Serial.println("HMC5883 detected");

  delay(1000);

  Serial.println("Starting BLE work!");

  BLEDevice::init("MyESP32"); // set the device name
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(&serverCallbacks);
  pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setValue("Hello World!");
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  pCharacteristic->setCallbacks(new MyCallbacks(
      mapBuffer,
      secBuffer,
      receivingMap,
      receivingSec,
      secBinary,
      mapBinary,
      secBinaryLen,
      mapBinaryLen,
      tftDisplay,
      ZOOM));
  // navigation.init();
  Serial.println("Characteristic defined! Now you can read it in your phone!");

  tft.init();
  tft.setRotation(0);
  // tft.fillScreen(TFT_BLACK);

  // // SPRITE (buffer)
  // sprite.createSprite(240, 240);
  // sprite.fillSprite(TFT_BLACK);

  // sprite.setTextColor(TFT_WHITE);
  // sprite.setTextSize(2);
  // sprite.setCursor(40, 120);
  // sprite.println("CONNECT DEVICE");

  // sprite.pushSprite(0, 0);

  stateManager.init(states);
}

float smoothHeading = 0.0;
bool headingInitialized = false;

void loop()
{
  currentMillis = millis();
  stateManager.loop();
  // navigation.loop();
  
}