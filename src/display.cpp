#pragma once
#include <TFT_eSPI.h>

class TFT_Display
{
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
      float &_heading_degrees) : sprite(&_sprite),
                                 mapBuffer(&_mapBuffer),
                                 secBuffer(&_secBuffer),
                                 secBinaryLen(&_secBinaryLen),
                                 mapBinaryLen(&_mapBinaryLen),
                                 deviceConnected(&_deviceConnected),
                                 secBinary(&_secBinary),
                                 mapBinary(&_mapBinary),
                                 ZOOM(&_ZOOM),
                                 heading_degrees(&_heading_degrees)
  {
  }

  void stopNavigation()
  {
    *mapBuffer = "";
    *secBuffer = "";

    *secBinaryLen = 0;
    *mapBinaryLen = 0;
  }

  void showConnected()
  {
    if (*deviceConnected)
    {
      sprite->setTextFont(1);
      sprite->fillScreen(TFT_BLACK);
      sprite->setTextColor(TFT_WHITE);
      sprite->setTextSize(2);
      sprite->setCursor(40, 120);
      sprite->println("CONNECTED");
      sprite->pushSprite(0, 0);
      
    }
  }

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

  void drawSafeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
  {
    if (clipLine(x0, y0, x1, y1))
    {
      sprite->drawLine(x0, y0, x1, y1, color);
    }
  }

  // --- CAMERA ANIMATION GLOBALS ---
  double targetRiderX = 0;
  double targetRiderY = 0;
  double currentRiderX = 0.0;
  double currentRiderY = 0.0;

  // --- GPS TO PIXEL CONVERSION GLOBALS ---
  bool originSet = false;
  double originLat = 0.0;
  double originLon = 0.0;

  // ── Draw a filled quad between two segments (no loops, 2 triangles only) ──
  inline void drawSegmentQuad(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                              uint16_t color, float halfW,
                              int16_t &lx0, int16_t &ly0, int16_t &rx0, int16_t &ry0,
                              int16_t &lx1, int16_t &ly1, int16_t &rx1, int16_t &ry1)
  {
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.5f)
    {
      lx0 = rx0 = lx1 = rx1 = x0;
      ly0 = ry0 = ly1 = ry1 = y0;
      return;
    }
    float nx = -dy / len * halfW, ny = dx / len * halfW;
    lx0 = x0 + nx;
    ly0 = y0 + ny;
    rx0 = x0 - nx;
    ry0 = y0 - ny;
    lx1 = x1 + nx;
    ly1 = y1 + ny;
    rx1 = x1 - nx;
    ry1 = y1 - ny;
    sprite->fillTriangle(lx0, ly0, rx0, ry0, lx1, ly1, color);
    sprite->fillTriangle(rx0, ry0, lx1, ly1, rx1, ry1, color);
  }

  // ── Filled polyline: quad per segment + join triangle between segments ──
  void drawPolyFilled(int16_t *xs, int16_t *ys, int n, uint16_t color, float halfW)
  {
    if (n < 2)
      return;
    int16_t pLx0, pLy0, pRx0, pRy0, pLx1, pLy1, pRx1, pRy1;
    drawSegmentQuad(xs[0], ys[0], xs[1], ys[1], color, halfW,
                    pLx0, pLy0, pRx0, pRy0, pLx1, pLy1, pRx1, pRy1);
    for (int i = 1; i < n - 1; i++)
    {
      int16_t cLx0, cLy0, cRx0, cRy0, cLx1, cLy1, cRx1, cRy1;
      drawSegmentQuad(xs[i], ys[i], xs[i + 1], ys[i + 1], color, halfW,
                      cLx0, cLy0, cRx0, cRy0, cLx1, cLy1, cRx1, cRy1);
      // seal the join gap with 2 triangles
      sprite->fillTriangle(pLx1, pLy1, pRx1, pRy1, cLx0, cLy0, color);
      sprite->fillTriangle(pRx1, pRy1, cLx0, cLy0, cRx0, cRy0, color);
      pLx0 = cLx0;
      pLy0 = cLy0;
      pRx0 = cRx0;
      pRy0 = cRy0;
      pLx1 = cLx1;
      pLy1 = cLy1;
      pRx1 = cRx1;
      pRy1 = cRy1;
    }
  }

  // ── Hollow polyline: 2 clipped lines per segment, no circles ──
  // Replace drawPolyHollow with this:
  void drawPolyHollow(int16_t *xs, int16_t *ys, int n, uint16_t fillColor, uint16_t borderColor, float halfW)
  {
    if (n < 2)
      return;

    // 1. Draw filled dark body first (covers intersections cleanly)
    for (int i = 0; i < n - 1; i++)
    {
      float dx = xs[i + 1] - xs[i], dy = ys[i + 1] - ys[i];
      float len = sqrtf(dx * dx + dy * dy);
      if (len < 0.5f)
        continue;
      float nx = -dy / len * halfW, ny = dx / len * halfW;
      int16_t lx0 = xs[i] + nx, ly0 = ys[i] + ny;
      int16_t rx0 = xs[i] - nx, ry0 = ys[i] - ny;
      int16_t lx1 = xs[i + 1] + nx, ly1 = ys[i + 1] + ny;
      int16_t rx1 = xs[i + 1] - nx, ry1 = ys[i + 1] - ny;
      sprite->fillTriangle(lx0, ly0, rx0, ry0, lx1, ly1, fillColor);
      sprite->fillTriangle(rx0, ry0, lx1, ly1, rx1, ry1, fillColor);
      // join fill
      if (i > 0)
      {
        sprite->fillTriangle(lx0, ly0, rx0, ry0, xs[i], ys[i], fillColor);
      }
    }

    // 2. Draw outer border lines on top
    float borderHalf = halfW + 1.5f; // border extends slightly beyond fill
    for (int i = 0; i < n - 1; i++)
    {
      float dx = xs[i + 1] - xs[i], dy = ys[i + 1] - ys[i];
      float len = sqrtf(dx * dx + dy * dy);
      if (len < 0.5f)
        continue;
      float nx = -dy / len * borderHalf, ny = dx / len * borderHalf;
      int16_t a0, b0, a1, b1;
      a0 = xs[i] + nx;
      b0 = ys[i] + ny;
      a1 = xs[i + 1] + nx;
      b1 = ys[i + 1] + ny;
      if (clipLine(a0, b0, a1, b1))
        sprite->drawLine(a0, b0, a1, b1, borderColor);
      a0 = xs[i] - nx;
      b0 = ys[i] - ny;
      a1 = xs[i + 1] - nx;
      b1 = ys[i + 1] - ny;
      if (clipLine(a0, b0, a1, b1))
        sprite->drawLine(a0, b0, a1, b1, borderColor);
    }
  }

  const int CX = 120, CY = 120;
  const float MAIN_HALF = 8.0f;   // 8px total main route
  const float SEC_HALF = 8.0f;    // 3px total secondary (hollow)
  const int CAMERA_OFFSET_Y = 35; // positive = move camera up (rider goes down)

  void drawSecondaryRoads()
  {
    // ── Shared screen-space buffers — no allocation per frame ──
    int16_t _scrX[1024], _scrY[1024];

    sprite->fillRect(0, 0, 240, 240, TFT_BLACK);

    currentRiderX += (targetRiderX - currentRiderX) * 0.15;
    currentRiderY += (targetRiderY - currentRiderY) * 0.15;

    float hr = -(*heading_degrees) * (PI / 180.0f);
    float sinH = sinf(hr), cosH = cosf(hr);
    float offX = currentRiderX, offY = currentRiderY;

// ── Inline transform macro ──
// sx = CX + (relX*cosH - relY*sinH)
// sy = CY + (relX*sinH + relY*cosH)
#define TO_SCREEN(mapX, mapY, sx, sy)      \
  {                                        \
    float _rx = ((mapX) - offX) * (*ZOOM); \
    float _ry = ((mapY) - offY) * (*ZOOM); \
    sx = CX + _rx * cosH - _ry * sinH;     \
    sy = CY + _rx * sinH + _ry * cosH;     \
  }

    // ══════════════════════════════════════════
    // 1. SECONDARY ROADS
    // ══════════════════════════════════════════
    if (*secBinaryLen >= 5)
    {
      const float widthTable[] = {0, 1.0f, 1.5f, 2.0f, 3.0f, 4.5f, 6.0f};

      // PASS 1 — borders first
      int16_t prevX = 0, prevY = 0;
      float curHW = 2.0f;
      bool first = true;

      for (int i = 0; i <= *secBinaryLen - 5; i += 5)
      {
        uint8_t cmd = (*secBinary)[i];
        int16_t mapX = (*secBinary)[i + 1] | ((*secBinary)[i + 2] << 8);
        int16_t mapY = (*secBinary)[i + 3] | ((*secBinary)[i + 4] << 8);
        int16_t sx, sy;
        TO_SCREEN(mapX, mapY, sx, sy);
        if (cmd != 255)
        {
          first = true;
          curHW = (cmd <= 6) ? widthTable[cmd] : 2.0f;
        }
        else if (!first)
        {
          float dx = sx - prevX, dy = sy - prevY;
          float len = sqrtf(dx * dx + dy * dy);
          if (len >= 0.5f)
          {
            float nx = -dy / len * (curHW + 1.0f), ny = dx / len * (curHW + 1.0f);
            int16_t a0, b0, a1, b1;
            a0 = prevX + nx;
            b0 = prevY + ny;
            a1 = sx + nx;
            b1 = sy + ny;
            if (clipLine(a0, b0, a1, b1))
              sprite->drawLine(a0, b0, a1, b1, TFT_WHITE);
            a0 = prevX - nx;
            b0 = prevY - ny;
            a1 = sx - nx;
            b1 = sy - ny;
            if (clipLine(a0, b0, a1, b1))
              sprite->drawLine(a0, b0, a1, b1, TFT_WHITE);
          }
        }
        prevX = sx;
        prevY = sy;
        first = false;
      }

      // PASS 2 — fill inside with black (narrower than border)
      prevX = 0;
      prevY = 0;
      curHW = 2.0f;
      first = true;

      for (int i = 0; i <= *secBinaryLen - 5; i += 5)
      {
        uint8_t cmd = (*secBinary)[i];
        int16_t mapX = (*secBinary)[i + 1] | ((*secBinary)[i + 2] << 8);
        int16_t mapY = (*secBinary)[i + 3] | ((*secBinary)[i + 4] << 8);
        int16_t sx, sy;
        TO_SCREEN(mapX, mapY, sx, sy);
        if (cmd != 255)
        {
          first = true;
          curHW = (cmd <= 6) ? widthTable[cmd] : 2.0f;
        }
        else if (!first)
        {
          float dx = sx - prevX, dy = sy - prevY;
          float len = sqrtf(dx * dx + dy * dy);
          if (len >= 0.5f)
          {
            float nx = -dy / len * curHW, ny = dx / len * curHW;
            sprite->fillTriangle(
                prevX + nx, prevY + ny,
                prevX - nx, prevY - ny,
                sx + nx, sy + ny, TFT_BLACK);
            sprite->fillTriangle(
                prevX - nx, prevY - ny,
                sx + nx, sy + ny,
                sx - nx, sy - ny, TFT_BLACK);
          }
        }
        prevX = sx;
        prevY = sy;
        first = false;
      }
    }

    // ══════════════════════════════════════════
    // 2. MAIN ROUTE (filled, drawn on top)
    // ══════════════════════════════════════════
    if (*mapBinaryLen >= 5)
    {
      int mainN = 0;
      for (int i = 0; i <= *mapBinaryLen - 5 && mainN < 2048; i += 5)
      {
        int16_t mapX = (*mapBinary)[i + 1] | ((*mapBinary)[i + 2] << 8);
        int16_t mapY = (*mapBinary)[i + 3] | ((*mapBinary)[i + 4] << 8);
        int16_t sx, sy;
        TO_SCREEN(mapX, mapY, sx, sy);
        _scrX[mainN] = sx;
        _scrY[mainN] = sy;
        mainN++;
      }
      if (mainN >= 2)
        drawPolyFilled(_scrX, _scrY, mainN, TFT_WHITE, MAIN_HALF);
    }

#undef TO_SCREEN

    // ══════════════════════════════════════════
    // 3. RIDER ARROW (always on top)
    // ══════════════════════════════════════════
    const float RIDER_SCALE = 1.5f; // tune this

    // Black outline
    sprite->fillTriangle(CX, CY - 14 * RIDER_SCALE, CX - 10 * RIDER_SCALE, CY + 8 * RIDER_SCALE, CX + 10 * RIDER_SCALE, CY + 8 * RIDER_SCALE, TFT_BLACK);
    // White arrow
    sprite->fillTriangle(CX, CY - 11 * RIDER_SCALE, CX - 8 * RIDER_SCALE, CY + 6 * RIDER_SCALE, CX + 8 * RIDER_SCALE, CY + 6 * RIDER_SCALE, TFT_WHITE);
    // Black notch
    sprite->fillTriangle(CX, CY + 2 * RIDER_SCALE, CX - 5 * RIDER_SCALE, CY + 8 * RIDER_SCALE, CX + 5 * RIDER_SCALE, CY + 8 * RIDER_SCALE, TFT_BLACK);

    sprite->pushSprite(0, 0);
  }
};