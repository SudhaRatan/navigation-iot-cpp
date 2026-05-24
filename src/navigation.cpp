#pragma once
#include "display.cpp"
// Buttons
#define BTN_UP 0
#define BTN_DOWN 1
#define BTN_LEFT 2
#define BTN_RIGHT 3

static unsigned long lastDrawTimeBtnUp = 0;
static unsigned long lastDrawTimeBtnDown = 0;
static unsigned long lastDrawTimeBtnLeft = 0;
static unsigned long lastDrawTimeBtnRight = 0;

class ScreenNavigation
{
private:
  TFT_Display *display;
  bool *deviceConnected;
  unsigned long *currentMillis;
  bool isButtonPressed = false;
  bool *receivingSec;
  bool *receivingMap;
  int *secBinaryLen;
  int *mapBinaryLen;
  unsigned long *lastDrawTime;

public:
  ScreenNavigation(TFT_Display &_tftDisplay,
                   bool &_deviceConnected,
                   unsigned long &_currentMillis,
                   bool &_receivingSec,
                   bool &_receivingMap,
                   int &_secBinaryLen,
                   int &_mapBinaryLen,
                   unsigned long &_lastDrawTime) : display(&_tftDisplay),
                                                   deviceConnected(&_deviceConnected),
                                                   currentMillis(&_currentMillis),
                                                   receivingSec(&_receivingSec),
                                                   receivingMap(&_receivingMap),
                                                   secBinaryLen(&_secBinaryLen),
                                                   mapBinaryLen(&_mapBinaryLen),
                                                   lastDrawTime(&_lastDrawTime)
  {
  }

  void init()
  {
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
  }

  void start()
  {
  }

  void loop()
  {
    if (digitalRead(BTN_UP) == LOW)
    {
      if (*currentMillis - lastDrawTimeBtnUp >= 200 && *deviceConnected && !isButtonPressed)
      {
        display->stopNavigation();
        display->showConnected();
        lastDrawTimeBtnUp = *currentMillis;
        isButtonPressed = true;
      }
    }
    else if (digitalRead(BTN_DOWN) == LOW)
    {
      if (*currentMillis - lastDrawTimeBtnDown >= 300 && *deviceConnected && !isButtonPressed)
      {
        lastDrawTimeBtnDown = *currentMillis;
        isButtonPressed = true;
      }
    }
    else if (digitalRead(BTN_LEFT) == LOW)
    {
      if (*currentMillis - lastDrawTimeBtnLeft >= 300 && *deviceConnected && !isButtonPressed)
      {
        lastDrawTimeBtnLeft = *currentMillis;
        isButtonPressed = true;
      }
    }
    else if (digitalRead(BTN_RIGHT) == LOW)
    {
      if (*currentMillis - lastDrawTimeBtnRight >= 300 && *deviceConnected && !isButtonPressed)
      {
        lastDrawTimeBtnRight = *currentMillis;
        isButtonPressed = true;
      }
    }
    else if (isButtonPressed && digitalRead(BTN_UP) == HIGH && digitalRead(BTN_DOWN) == HIGH && digitalRead(BTN_LEFT) == HIGH && digitalRead(BTN_RIGHT) == HIGH)
    {
      isButtonPressed = false;
    }

    if ((!*receivingSec && *secBinaryLen >= 5) || (!*receivingMap && *mapBinaryLen >= 5))
    {
      startNavigation();
    }
  }

  void startNavigation()
  {
    if (*currentMillis - *lastDrawTime >= 33)
    {
      *lastDrawTime = *currentMillis;

#pragma region TiltCompensation
// Tilt-compensated heading
// mpu.update_accel_gyro();
// sensors_event_t magEvent;
// mag.getEvent(&magEvent);

// float ax = mpu.getAccX(), ay = mpu.getAccY(), az = mpu.getAccZ();
// float roll = atan2f(ay, az);
// float pitch = atan2f(-ax, sqrtf(ay * ay + az * az));

// float mx = magEvent.magnetic.x;
// float my = magEvent.magnetic.y;
// float mz = magEvent.magnetic.z;

// float Xh = mx * cosf(pitch) + mz * sinf(pitch);
// float Yh = mx * sinf(roll) * sinf(pitch) + my * cosf(roll) - mz * sinf(roll) * cosf(pitch);

// float heading = atan2f(-Yh, Xh);
// if (heading < 0)
//   heading += 2 * PI;
// float newHeading = heading * 180.0f / M_PI;

// if (!headingInitialized)
// {
//   smoothHeading = newHeading;
//   headingInitialized = true;
// }
// float delta = newHeading - smoothHeading;
// if (delta > 180)
//   delta -= 360;
// if (delta < -180)
//   delta += 360;
// smoothHeading += delta * 0.25f;
// if (smoothHeading < 0)
//   smoothHeading += 360;
// if (smoothHeading >= 360)
//   smoothHeading -= 360;
// heading_degrees = smoothHeading;
#pragma endregion TiltCompensation

      display->drawSecondaryRoads();
    }
    delay(1);
  }
};