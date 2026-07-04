#pragma once
#include <TFT_eSPI.h>

class TFT_Display{
private:
  bool *deviceConnected;
  String *mapBuffer;
  String *secBuffer;
  int *secBinaryLen = 0;
  int *mapBinaryLen = 0;
  uint8_t (*secBinary)[20000];
  uint8_t (*mapBinary)[20000];
  float *ZOOM;
  float *heading_degrees;
public:
  TFT_eSprite *sprite;
  TFT_Display(
      TFT_eSprite &_sprite,
      String &_mapBuffer,
      String &_secBuffer,
      int &_secBinaryLen,
      int &_mapBinaryLen,
      bool &_deviceConnected,
      uint8_t (&_secBinary)[20000],
      uint8_t (&_mapBinary)[20000],
      float &_ZOOM,
      float &_heading_degrees);

  void stopNavigation();
  void showConnected();
  int computeOutCode(int16_t x, int16_t y);

    // --- COHEN-SUTHERLAND CLIPPING ALGORITHM ---
  const int INSIDE = 0; // 0000
  const int LEFT = 1;   // 0001
  const int RIGHT = 2;  // 0010
  const int BOTTOM = 4; // 0100
  const int TOP = 8;    // 1000
  
  bool clipLine(int16_t &x0, int16_t &y0, int16_t &x1, int16_t &y1);
  void drawSafeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

    // --- CAMERA ANIMATION GLOBALS ---
  double targetRiderX = 0;
  double targetRiderY = 0;
  double currentRiderX = 0.0;
  double currentRiderY = 0.0;

  // --- GPS TO PIXEL CONVERSION GLOBALS ---
  bool originSet = false;
  double originLat = 0.0;
  double originLon = 0.0;

  inline void drawSegmentQuad(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                            uint16_t color, float halfW,
                            int16_t &lx0, int16_t &ly0, int16_t &rx0, int16_t &ry0,
                            int16_t &lx1, int16_t &ly1, int16_t &rx1, int16_t &ry1);

  void drawPolyFilled(int16_t *xs, int16_t *ys, int n, uint16_t color, float halfW);
  void drawPolyHollow(int16_t *xs, int16_t *ys, int n, uint16_t fillColor, uint16_t borderColor, float halfW);

  const int CX = 120, CY = 120;
  const float MAIN_HALF = 8.0f;   // 8px total main route
  const float SEC_HALF = 8.0f;    // 3px total secondary (hollow)
  const int CAMERA_OFFSET_Y = 35; // positive = move camera up (rider goes down)

  void drawSecondaryRoads();

};