#include <BLEDevice.h>
#include <BLEServer.h>
#include "display.cpp"
#include "mbedtls/base64.h"

class MyServerCallbacks : public BLEServerCallbacks
{
private:
  bool *deviceConnected;
  TFT_Display *display;

public:
  MyServerCallbacks(
    TFT_Display &_display,
    bool &_deviceConnected) :
    display(&_display),
    deviceConnected(&_deviceConnected)
  {
  }
  void onConnect(BLEServer *pServer)
  {
    *deviceConnected = true;
    // display->showConnected();
  }

  void onDisconnect(BLEServer *pServer)
  {
    *deviceConnected = false;
    // display->stopNavigation();
    // display->sprite->fillScreen(TFT_BLACK);
    // display->sprite->setTextColor(TFT_WHITE);
    // display->sprite->setTextSize(2);
    // display->sprite->setCursor(40, 120);
    // display->sprite->println("CONNECT DEVICE");
    // display->sprite->pushSprite(0, 0);
    BLEDevice::startAdvertising(); // restart advertising
  }
};

class MyCallbacks : public BLECharacteristicCallbacks
{

private:
  String *mapBuffer;
  String *secBuffer;
  bool *receivingMap;
  bool *receivingSec;
  uint8_t (*secBinary)[20000]; // adjust if needed
  uint8_t (*mapBinary)[20000]; // adjust if needed
  int *secBinaryLen = 0;
  int *mapBinaryLen = 0;
  TFT_Display *tftDisplay; // ← added
  float *ZOOM;

public:
  MyCallbacks(
      String &_mapBuffer,
      String &_secBuffer,
      bool &_receivingMap,
      bool &_receivingSec,
      uint8_t (&_secBinary)[20000],
      uint8_t (&_mapBinary)[20000],
      int &_secBinaryLen,
      int &_mapBinaryLen,
      TFT_Display &_tftDisplay,
      float &_ZOOM) : mapBuffer(&_mapBuffer),
                      secBuffer(&_secBuffer),
                      receivingMap(&_receivingMap),
                      receivingSec(&_receivingSec),
                      secBinary(&_secBinary),
                      mapBinary(&_mapBinary),
                      secBinaryLen(&_secBinaryLen),
                      mapBinaryLen(&_mapBinaryLen),
                      tftDisplay(&_tftDisplay),
                      ZOOM(&_ZOOM) {}

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
      *mapBuffer = "";
      *receivingMap = true;
      return;
    }

    if (msg == "MAP_END")
    {
      Serial.println("MAP END");
      *receivingMap = false;

      Serial.print("MAP SIZE: ");
      Serial.println(mapBuffer->length());
      Serial.print("\nFull MAP: ");
      Serial.println(*mapBuffer);

      size_t outLen = 0;

      mbedtls_base64_decode(
          *mapBinary,
          sizeof(*mapBinary),
          &outLen,
          (const unsigned char *)mapBuffer->c_str(),
          mapBuffer->length());

      *mapBinaryLen = outLen;

      Serial.print("DECODED BYTES: ");
      Serial.println(*mapBinaryLen);
      return;
    }

    if (msg.startsWith("MAP_DATA|") && *receivingMap)
    {
      *mapBuffer += msg.substring(9);
      Serial.println("MAP CHUNK");
      return;
    }

    /* ===== SECONDARY STREAM ===== */

    if (msg == "SEC_START")
    {
      Serial.println("SEC START");
      *secBuffer = "";
      *receivingSec = true;
      tftDisplay->originSet = false;
      tftDisplay->targetRiderX = 0.0;
      tftDisplay->targetRiderY = 0.0;
      tftDisplay->currentRiderX = 0.0;
      tftDisplay->currentRiderY = 0.0;
      return;
    }

    if (msg == "SEC_END")
    {
      Serial.println("SEC END");
      *receivingSec = false;

      Serial.print("SEC SIZE: ");
      Serial.println(secBuffer->length());
      Serial.print("\nFull SEC: ");
      Serial.println(*secBuffer);

      size_t outLen = 0;

      mbedtls_base64_decode(
          *secBinary,
          sizeof(*secBinary),
          &outLen,
          (const unsigned char *)secBuffer->c_str(),
          secBuffer->length());

      *secBinaryLen = outLen;

      Serial.print("DECODED BYTES: ");
      Serial.println(*secBinaryLen);
      return;
    }

    if (msg.startsWith("SEC_DATA|") && *receivingSec)
    {
      *secBuffer += msg.substring(9);
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
        if (!tftDisplay->originSet)
        {
          tftDisplay->originLat = liveLat;
          tftDisplay->originLon = liveLon;
          tftDisplay->originSet = true;
          Serial.println("MAP ORIGIN SET!");
        }

        // 3. DO THE MATH ON THE ESP32
        // Make sure this scale matches the "0.4" you use in React Native

        double metersPerDegLat = 111320.0;
        double metersPerDegLon = 111320.0 * cos(tftDisplay->originLat * PI / 180.0);

        // 4. Calculate distance in meters from the origin
        double dx = (liveLon - tftDisplay->originLon) * metersPerDegLon;
        double dy = (liveLat - tftDisplay->originLat) * metersPerDegLat;

        // 5. Convert meters to OLED Pixels and update the Lerp target
        tftDisplay->targetRiderX = dx * (*ZOOM);
        tftDisplay->targetRiderY = -dy * (*ZOOM); // Negative because OLED Y-axis goes down

        Serial.print("NEW PIXEL TARGET: ");
        Serial.print(tftDisplay->targetRiderX, 4);
        Serial.print(", ");
        Serial.println(tftDisplay->targetRiderY, 4);
      }
      return;
    }
  }
};
