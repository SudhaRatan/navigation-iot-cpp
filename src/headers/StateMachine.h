#pragma once
#include "State.h"
class State;

class StateMachine
{
private:
  State *currentState;
  bool *deviceConnected;
  unsigned long *currentMillis;
  bool isButtonPressed = false;

public:
  State *states[4];
  StateMachine(bool &_deviceConnected, unsigned long &_currentMillis);
  void init(State *(_states)[4]);
  void changeState(String stateName);
  void loop();
};