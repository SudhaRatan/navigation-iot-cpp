#include "State.h"
#include "TFT_eSPI.h"
#include "list.h"
#include <BLEServer.h>
#include "QR.h"


class ConfigScreen : public State
{
private:
  TFT_eSprite *tft;
  bool *deviceConnected;
  bool prevConnected = deviceConnected ? true : false;
  StateMachine *stateManager;
  List* list = nullptr;
  BLEService **pService;
  QRCodeGenerator qrGen;

public:
  ConfigScreen(TFT_eSprite &_tft, bool &_deviceConnected, StateMachine &_stateManager, BLEService **_pService);
  void loop() override;
  void enter() override;
  void exit() override;
  void btnPress(String button) override;
  void list_1();
};