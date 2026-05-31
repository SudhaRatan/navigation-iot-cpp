#pragma once
#include "TFT_eSPI.h"

class QRCodeGenerator
{
private:
  TFT_eSprite *sprite;

public:
  bool qrPrinted = false;
  QRCodeGenerator() = default;
  QRCodeGenerator(TFT_eSprite &_sprite);
  void QRPrint();
};