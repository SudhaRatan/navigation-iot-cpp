#include "State.h"
#include "TFT_eSPI.h"

class ConfigScreen : public State
{
private:
  TFT_eSprite *tft;
  bool *deviceConnected;
  StateMachine *stateManager;

public:
  ConfigScreen(TFT_eSprite &_tft, bool &_deviceConnected, StateMachine &_stateManager);
  void loop() override;
  void enter() override;
  void exit() override;
  void btnPress(String button) override;
  void list_1();
};