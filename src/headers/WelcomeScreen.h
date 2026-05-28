#pragma once
#include "State.h"
#include "TFT_eSPI.h"

class WelcomeScreen :  public State
{
  private:
  TFT_eSprite *tft;
  int string_1_x = 32;
  StateMachine *stateManager;

public:
  WelcomeScreen(TFT_eSprite &_tft, StateMachine &_stateManager);
  void loop() override;
  void enter() override;
  void exit() override;
  void btnPress(String button) override;
};