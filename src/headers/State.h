#pragma once
#include <Arduino.h>
#include "StateMachine.h"

class StateMachine;

class State
{
public:

  StateMachine *stateManager;
  String name;
  virtual void enter() = 0;
  virtual void loop() = 0;
  virtual void exit() = 0;
  virtual void btnPress(String button) = 0;
};