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

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define SDA_PIN 8
#define SCL_PIN 9

// display
#define TFT_CS 5
#define TFT_DC 7
#define TFT_RST 10
#define TFT_MOSI 6
#define TFT_SCLK 4

// Buttons
#define BTN1 0
#define BTN2 1
#define BTN3 2
#define BTN4 3

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
unsigned long lastDrawTimeBtn1 = 0;

#pragma endregion GlobalStates

#pragma region DependencyInjection

TFT_Display tftDisplay(sprite, mapBuffer, secBuffer, secBinaryLen, mapBinaryLen, deviceConnected, secBinary, mapBinary, ZOOM, heading_degrees);
MyServerCallbacks serverCallbacks(tftDisplay, deviceConnected);

#pragma endregion DependencyInjection

void setup()
{

  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN); // ESP32 I2C pins
  Wire.setClock(400000);        // 400kHz Fast Mode (default is 100kHz)

  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);
  pinMode(BTN4, INPUT_PULLUP);

  delay(3000);
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
  BLEService *pService = pServer->createService(SERVICE_UUID);
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
      ZOOM
  ));
  Serial.println("Characteristic defined! Now you can read it in your phone!");

  delay(10);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  // SPRITE (buffer)
  sprite.createSprite(240, 240);
  sprite.fillSprite(TFT_BLACK);

  sprite.setTextColor(TFT_WHITE);
  sprite.setTextSize(2);
  sprite.setCursor(40, 120);
  sprite.println("CONNECT DEVICE");

  sprite.pushSprite(0, 0);
}

float smoothHeading = 0.0;
bool headingInitialized = false;

void loop()
{
  unsigned long currentMillis = millis();

  if (digitalRead(BTN1) == LOW)
  {
    if (currentMillis - lastDrawTimeBtn1 >= 300 && deviceConnected)
    {
      tftDisplay.stopNavigation();
      tftDisplay.showConnected();
      lastDrawTimeBtn1 = currentMillis;
    }
  }

  if ((!receivingSec && secBinaryLen >= 5) || (!receivingMap && mapBinaryLen >= 5))
  {
    if (currentMillis - lastDrawTime >= 33)
    {
      lastDrawTime = currentMillis;

      // Tilt-compensated heading
      // mpu.update_accel_gyro();
      sensors_event_t magEvent;
      mag.getEvent(&magEvent);

      float ax = mpu.getAccX(), ay = mpu.getAccY(), az = mpu.getAccZ();
      float roll = atan2f(ay, az);
      float pitch = atan2f(-ax, sqrtf(ay * ay + az * az));

      float mx = magEvent.magnetic.x;
      float my = magEvent.magnetic.y;
      float mz = magEvent.magnetic.z;

      float Xh = mx * cosf(pitch) + mz * sinf(pitch);
      float Yh = mx * sinf(roll) * sinf(pitch) + my * cosf(roll) - mz * sinf(roll) * cosf(pitch);

      float heading = atan2f(-Yh, Xh);
      if (heading < 0)
        heading += 2 * PI;
      float newHeading = heading * 180.0f / M_PI;

      if (!headingInitialized)
      {
        smoothHeading = newHeading;
        headingInitialized = true;
      }
      float delta = newHeading - smoothHeading;
      if (delta > 180)
        delta -= 360;
      if (delta < -180)
        delta += 360;
      smoothHeading += delta * 0.25f;
      if (smoothHeading < 0)
        smoothHeading += 360;
      if (smoothHeading >= 360)
        smoothHeading -= 360;
      heading_degrees = smoothHeading;

      tftDisplay.drawSecondaryRoads();
    }
  }
  delay(1);
}