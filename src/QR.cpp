#include <qrcode.h>
#include <Wire.h>
#include <HWCDC.h>
#include <TFT_eSPI.h>

const char SERVICE_UUID[] = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
const char CHARACTERISTIC_UUID[] = "beb5483e-36e1-4688-b7f5-ea07361b26a8";

class QRCodeGenerator
{
  private:
  TFT_eSprite *sprite;
public:
  QRCodeGenerator(TFT_eSprite &_sprite) : sprite(&_sprite)
  {
  }
  void QRPrint()
  {
    // Start time
    uint32_t dt = millis();

    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(6)];
    String qrText = String(SERVICE_UUID) + " " + String(CHARACTERISTIC_UUID);
    qrcode_initText(&qrcode, qrcodeData, 6, ECC_LOW, qrText.c_str());

    // Scale each module to this many pixels
    const float moduleSize = 4;

    // Center the QR code on the 240x240 display
    float totalSize = qrcode.size * moduleSize;
    float offsetX = (240 - totalSize) / 2;
    float offsetY = (240 - totalSize) / 2;

    sprite->fillScreen(TFT_WHITE);

    for (uint8_t y = 0; y < qrcode.size; y++)
    {
      for (uint8_t x = 0; x < qrcode.size; x++)
      {
        uint16_t color = qrcode_getModule(&qrcode, x, y) ? TFT_BLACK : TFT_WHITE;
        sprite->fillRect(
            offsetX + x * moduleSize,
            offsetY + y * moduleSize,
            moduleSize, moduleSize,
            color);
      }
    }

    sprite->pushSprite(0, 0);
  }
};