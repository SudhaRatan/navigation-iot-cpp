#include <Wire.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <string>
#include "mbedtls/base64.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
#include <TFT_eSPI.h>
#include <SPI.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define SDA_PIN 8
#define SCL_PIN 9

#define TFT_CS 5
#define TFT_DC 7
#define TFT_RST 10
#define TFT_MOSI 6
#define TFT_SCLK 4

// Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft); // framebuffer

Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);

String mapBuffer = "";
String secBuffer = "";

bool receivingMap = false;
bool receivingSec = false;

uint8_t secBinary[30000]; // adjust if needed
int secBinaryLen = 0;

uint8_t mapBinary[30000]; // adjust if needed
int mapBinaryLen = 0;

class MyServerCallbacks : public BLEServerCallbacks
{

  void onConnect(BLEServer *pServer)
  {
    sprite.fillScreen(TFT_BLACK);
    sprite.setTextColor(TFT_WHITE);
    sprite.setTextSize(2);
    sprite.setCursor(40, 120);
    sprite.println("CONNECTED");
    sprite.pushSprite(0, 0);
  }

  void onDisconnect(BLEServer *pServer)
  {

    mapBuffer = "";
    secBuffer = "";

    secBinaryLen = 0;
    mapBinaryLen = 0;

    sprite.fillScreen(TFT_BLACK);
    sprite.setTextColor(TFT_WHITE);
    sprite.setTextSize(2);
    sprite.setCursor(40, 120);
    sprite.println("CONNECT DEVICE");
    sprite.pushSprite(0, 0);
    BLEDevice::startAdvertising(); // restart advertising
  }
};

// --- COHEN-SUTHERLAND CLIPPING ALGORITHM ---
const int INSIDE = 0; // 0000
const int LEFT = 1;   // 0001
const int RIGHT = 2;  // 0010
const int BOTTOM = 4; // 0100
const int TOP = 8;    // 1000

int computeOutCode(int16_t x, int16_t y)
{
  int code = INSIDE;

  if (x < 0)
    code |= LEFT;
  else if (x > 239)
    code |= RIGHT;

  if (y < 0)
    code |= TOP;
  else if (y > 239)
    code |= BOTTOM;

  return code;
}

bool clipLine(int16_t &x0, int16_t &y0, int16_t &x1, int16_t &y1)
{
  int outcode0 = computeOutCode(x0, y0);
  int outcode1 = computeOutCode(x1, y1);
  bool accept = false;

  while (true)
  {
    if (!(outcode0 | outcode1))
    {
      accept = true;
      break;
    }
    else if (outcode0 & outcode1)
    {
      break;
    }
    else
    {
      int outcodeOut = outcode0 ? outcode0 : outcode1;
      int16_t x = 0, y = 0;

      // The (int32_t) cast stays *only* here to protect the multiplication
      if (outcodeOut & TOP)
      {
        x = x0 + (int32_t)(x1 - x0) * (0 - y0) / (y1 - y0 != 0 ? y1 - y0 : 1);
        y = 0;
      }
      else if (outcodeOut & BOTTOM)
      {
        x = x0 + (int32_t)(x1 - x0) * (239 - y0) / (y1 - y0 != 0 ? y1 - y0 : 1);
        y = 239;
      }
      else if (outcodeOut & RIGHT)
      {
        y = y0 + (int32_t)(y1 - y0) * (239 - x0) / (x1 - x0 != 0 ? x1 - x0 : 1);
        x = 239;
      }
      else if (outcodeOut & LEFT)
      {
        y = y0 + (int32_t)(y1 - y0) * (0 - x0) / (x1 - x0 != 0 ? x1 - x0 : 1);
        x = 0;
      }

      if (outcodeOut == outcode0)
      {
        x0 = x;
        y0 = y;
        outcode0 = computeOutCode(x0, y0);
      }
      else
      {
        x1 = x;
        y1 = y;
        outcode1 = computeOutCode(x1, y1);
      }
    }
  }
  return accept;
}

// THIS FIXES THE THICK LINE OLED WRAP BUG
void drawSafeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
  if (clipLine(x0, y0, x1, y1))
  {
    sprite.drawLine(x0, y0, x1, y1, color);
  }
}

float heading_degrees = 0.0;
unsigned long lastDrawTime = 0;

// --- CAMERA ANIMATION GLOBALS ---
double targetRiderX = 0;
double targetRiderY = 0;
double currentRiderX = 0.0;
double currentRiderY = 0.0;

// --- GPS TO PIXEL CONVERSION GLOBALS ---
bool originSet = false;
double originLat = 0.0;
double originLon = 0.0;

void drawSecondaryRoads()
{
  sprite.fillRect(0, 0, 240, 240, TFT_BLACK);
  bool first = true;
  const int CENTER_X = 120;
  const int CENTER_Y = 120;

  // Smooth camera
  currentRiderX += (targetRiderX - currentRiderX) * 0.1;
  currentRiderY += (targetRiderY - currentRiderY) * 0.1;

  float heading_radians = -heading_degrees * (PI / 180.0);
  float s = sin(heading_radians);
  float c = cos(heading_radians);

  // =========================
  // SECONDARY ROADS
  // =========================
  if (secBinaryLen >= 5)
  {
    int16_t prevX = 0;
    int16_t prevY = 0;

    for (int i = 0; i <= secBinaryLen - 5; i += 5)
    {
      if ((i % 200) == 0)
        delay(0);
      uint8_t cmd = secBinary[i];

      int16_t mapX = secBinary[i + 1] | (secBinary[i + 2] << 8);
      int16_t mapY = secBinary[i + 3] | (secBinary[i + 4] << 8);

      float relX = mapX - currentRiderX;
      float relY = mapY - currentRiderY;

      int16_t rotX = round((relX * c) - (relY * s));
      int16_t rotY = round((relX * s) + (relY * c));

      int16_t screenX = CENTER_X + rotX;
      int16_t screenY = CENTER_Y + rotY;

      if (cmd == 1 && !first)
      {
        drawSafeLine(prevX, prevY, screenX, screenY, TFT_DARKGREY);
      }

      prevX = screenX;
      prevY = screenY;
      first = false;
    }
  }

  // =========================
  // MAIN ROUTE (THICK)
  // =========================
  if (mapBinaryLen >= 5)
  {
    int16_t prevX = 0;
    int16_t prevY = 0;

    for (int i = 0; i <= mapBinaryLen - 5; i += 5)
    {
      if ((i % 200) == 0)
        delay(0);
      uint8_t cmd = mapBinary[i];

      int16_t mapX = mapBinary[i + 1] | (mapBinary[i + 2] << 8);
      int16_t mapY = mapBinary[i + 3] | (mapBinary[i + 4] << 8);

      float relX = mapX - currentRiderX;
      float relY = mapY - currentRiderY;

      int16_t rotX = round((relX * c) - (relY * s));
      int16_t rotY = round((relX * s) + (relY * c));

      int16_t screenX = CENTER_X + rotX;
      int16_t screenY = CENTER_Y + rotY;

      if (cmd == 1)
      {
        drawSafeLine(prevX, prevY, screenX, screenY, TFT_WHITE);
        drawSafeLine(prevX + 1, prevY, screenX + 1, screenY, TFT_WHITE);
        drawSafeLine(prevX - 1, prevY, screenX - 1, screenY, TFT_WHITE);
        drawSafeLine(prevX, prevY + 1, screenX, screenY + 1, TFT_WHITE);
        drawSafeLine(prevX, prevY - 1, screenX, screenY - 1, TFT_WHITE);
      }

      prevX = screenX;
      prevY = screenY;
    }
  }

  // =========================
  // RIDER ARROW
  // =========================
  sprite.fillTriangle(
      CENTER_X, CENTER_Y - 10,
      CENTER_X - 6, CENTER_Y + 8,
      CENTER_X + 6, CENTER_Y + 8,
      TFT_WHITE);

  // push once per frame (NO flicker)
  sprite.pushSprite(0, 0);
}

inline bool dimPixel(int i)
{
  return (i % 2) == 0; // simple dithering
}

class MyCallbacks : public BLECharacteristicCallbacks
{

  void onWrite(BLECharacteristic *pCharacteristic)
  {

    std::string value = pCharacteristic->getValue();
    if (value.empty())
      return;

    String msg = String(value.c_str());

    /* ===== MAP STREAM ===== */

    if (msg == "MAP_START")
    {
      Serial.println("MAP START");
      mapBuffer = "";
      receivingMap = true;
      return;
    }

    if (msg == "MAP_END")
    {
      Serial.println("MAP END");
      receivingMap = false;

      Serial.print("MAP SIZE: ");
      Serial.println(mapBuffer.length());
      Serial.print("\nFull MAP: ");
      Serial.println(mapBuffer);

      size_t outLen = 0;

      mbedtls_base64_decode(
          mapBinary,
          sizeof(mapBinary),
          &outLen,
          (const unsigned char *)mapBuffer.c_str(),
          mapBuffer.length());

      mapBinaryLen = outLen;

      Serial.print("DECODED BYTES: ");
      Serial.println(mapBinaryLen);
      return;
    }

    if (msg.startsWith("MAP_DATA|") && receivingMap)
    {
      mapBuffer += msg.substring(9);
      Serial.println("MAP CHUNK");
      return;
    }

    /* ===== SECONDARY STREAM ===== */

    if (msg == "SEC_START")
    {
      Serial.println("SEC START");
      secBuffer = "";
      receivingSec = true;
      originSet = false;
      targetRiderX = 0.0;
      targetRiderY = 0.0;
      currentRiderX = 0.0;
      currentRiderY = 0.0;
      return;
    }

    if (msg == "SEC_END")
    {
      Serial.println("SEC END");
      receivingSec = false;

      Serial.print("SEC SIZE: ");
      Serial.println(secBuffer.length());
      Serial.print("\nFull SEC: ");
      Serial.println(secBuffer);

      size_t outLen = 0;

      mbedtls_base64_decode(
          secBinary,
          sizeof(secBinary),
          &outLen,
          (const unsigned char *)secBuffer.c_str(),
          secBuffer.length());

      secBinaryLen = outLen;

      Serial.print("DECODED BYTES: ");
      Serial.println(secBinaryLen);
      return;
    }

    if (msg.startsWith("SEC_DATA|") && receivingSec)
    {
      secBuffer += msg.substring(9);
      Serial.println("SEC CHUNK");
      return;
    }

    /* ===== POSITION STREAM ===== */
    if (msg.startsWith("POS|"))
    {
      int splitIndex = msg.indexOf(',');
      if (splitIndex > 0)
      {

        // 1. Extract the raw LIVE Lat and Lon from the phone
        double liveLat = strtod(msg.substring(4, splitIndex).c_str(), NULL);
        double liveLon = strtod(msg.substring(splitIndex + 1).c_str(), NULL);

        // 2. Set the Map Origin (Runs only once on the first GPS fix)
        // If your map center isn't exactly where you are standing,
        // you will need to send these via a separate BLE command instead!
        if (!originSet)
        {
          originLat = liveLat;
          originLon = liveLon;
          originSet = true;
          Serial.println("MAP ORIGIN SET!");
        }

        // 3. DO THE MATH ON THE ESP32
        // Make sure this scale matches the "0.4" you use in React Native
        double scale = 0.4;
        double metersPerDegLat = 111320.0;
        double metersPerDegLon = 111320.0 * cos(originLat * PI / 180.0);

        // 4. Calculate distance in meters from the origin
        double dx = (liveLon - originLon) * metersPerDegLon;
        double dy = (liveLat - originLat) * metersPerDegLat;

        // 5. Convert meters to OLED Pixels and update the Lerp target
        targetRiderX = dx * scale;
        targetRiderY = -dy * scale; // Negative because OLED Y-axis goes down

        Serial.print("NEW PIXEL TARGET: ");
        Serial.print(targetRiderX, 4);
        Serial.print(", ");
        Serial.println(targetRiderY, 4);
      }
      return;
    }
  }
};

void setup()
{

  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN); // ESP32 I2C pins
  Wire.setClock(400000);        // 400kHz Fast Mode (default is 100kHz)

  delay(4000);
  if (!mag.begin())
  {
    Serial.println("HMC5883 not detected");
    while (1)
      ;
  }

  Serial.println("HMC5883 detected");

  delay(1000);

  Serial.println("Starting BLE work!");

  BLEDevice::init("MyESP32"); // set the device name
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
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
  pCharacteristic->setCallbacks(new MyCallbacks());
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

  // // 2. Draw the frame (Limit to ~30 FPS so we don't choke the I2C bus)
  if ((!receivingSec && secBinaryLen >= 5) || (!receivingMap && mapBinaryLen >= 5))
  {
    unsigned long currentMillis = millis();

    if (currentMillis - lastDrawTime >= 33)
    {
      lastDrawTime = currentMillis;
      sensors_event_t event;
      mag.getEvent(&event);
      float heading = atan2(event.magnetic.y, event.magnetic.x);

      if (heading < 0)
        heading += 2 * PI;

      float headingDegrees = heading * 180 / M_PI;
      float newHeading = headingDegrees;

      // --- INIT ---
      if (!headingInitialized)
      {
        smoothHeading = newHeading;
        headingInitialized = true;
      }

      // --- HANDLE WRAP (critical) ---
      float delta = newHeading - smoothHeading;

      if (delta > 180)
        delta -= 360;
      if (delta < -180)
        delta += 360;

      // --- SMOOTHING (low-pass filter) ---
      smoothHeading += delta * 0.25; // 🔥 tune this

      // keep in 0–360
      if (smoothHeading < 0)
        smoothHeading += 360;
      if (smoothHeading >= 360)
        smoothHeading -= 360;

      heading_degrees = smoothHeading;

      // Trigger the OLED render frame with the new heading_degrees
      drawSecondaryRoads();
    }
  }
  // drawSecondaryRoads();
  delay(1);
}
