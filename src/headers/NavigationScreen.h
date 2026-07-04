#include "State.h"
#include "TFT_eSPI.h"
#include "list.h"
#include <BLEServer.h>
#include "QR.h"
#include "headers/display.h"


class NavigationScreen : public State
{
private:
  TFT_eSprite *tft;
  bool *deviceConnected;
  bool prevConnected = *deviceConnected ? true : false;
  StateMachine *stateManager;
  List* list = nullptr;
  BLEService **pService;
  QRCodeGenerator qrGen;
  TFT_Display *display;
  unsigned long *currentMillis;
  bool *receivingSec;
  bool *receivingMap;
  int *secBinaryLen;
  int *mapBinaryLen;
  unsigned long *lastDrawTime;

public:
  NavigationScreen(TFT_eSprite &_tft, bool &_deviceConnected, StateMachine &_stateManager, BLEService **_pService, TFT_Display &_display,
  unsigned long &_currentMillis,
  bool &_receivingSec,
  bool &_receivingMap,
  int &_secBinaryLen,
  int &_mapBinaryLen,
  unsigned long &_lastDrawTime
  );
  void loop() override;
  void enter() override;
  void exit() override;
  void btnPress(String button) override;
  void handleDeviceUpdate();
  void startNavigation();
};