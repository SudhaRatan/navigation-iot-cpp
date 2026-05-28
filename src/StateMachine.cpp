#include "headers/State.h"
#include "headers/StateMachine.h"

#define BTN_UP 0
#define BTN_DOWN 1
#define BTN_LEFT 2
#define BTN_RIGHT 3

class State;

static unsigned long lastDrawTimeBtn = 0;

StateMachine::StateMachine(bool &_deviceConnected, unsigned long &_currentMillis) : deviceConnected(&_deviceConnected), currentMillis(&_currentMillis)
{
}

void StateMachine::init(State *(_states)[4])
{
  Serial.println("Initializing State Machine...");
  for (int i = 0; i < 4; ++i)
  {
    Serial.print("State slot ");
    Serial.print(i);
    Serial.print(": ");
    if (_states[i])
    {
      Serial.println(_states[i]->name);
    }
    else
    {
      Serial.println("Empty");
    }
    states[i] = _states[i];
  }

  if (_states[0])
    changeState(_states[0]->name);

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
}

void StateMachine::changeState(String stateName)
{
  State *newState = nullptr;
  for (int i = 0; i < 4; i++)
  {
    Serial.print("Checking state slot ");
    Serial.print(i);
    Serial.print(": ");
    if (states[i])
    {
      Serial.println(states[i]->name);
    }
    else
    {
      Serial.println("Empty");
    }
    if (states[i] && states[i]->name == stateName)
    {
      Serial.print("Changing state to: ");
      Serial.println(stateName);
      newState = states[i];
      break;
    }
  }
  if (currentState)
  {
    currentState->exit();
  }
  currentState = newState;
  if (currentState)
  {
    currentState->enter();
  }
}

void StateMachine::loop()
{
  if (currentState)
  {
    String button = digitalRead(BTN_UP) == LOW
                        ? "UP"
                    : digitalRead(BTN_DOWN) == LOW
                        ? "DOWN"
                    : digitalRead(BTN_LEFT) == LOW
                        ? "LEFT"
                    : digitalRead(BTN_RIGHT) == LOW
                        ? "RIGHT"
                        : "";
    // Serial.print("Button State: ");
    // Serial.println(button);
    // Serial.print("Current Millis: ");
    // Serial.println(*currentMillis);
    // Serial.print("Device Connected: ");
    // Serial.println(*deviceConnected);
    if (button != "" && ((*currentMillis - lastDrawTimeBtn) >= 200) && !isButtonPressed)
    {
      Serial.print("Button Pressed: ");
      Serial.println(button);
      currentState->btnPress(button);
      lastDrawTimeBtn = *currentMillis;
      isButtonPressed = true;
    }
    else if (isButtonPressed && digitalRead(BTN_UP) == HIGH && digitalRead(BTN_DOWN) == HIGH && digitalRead(BTN_LEFT) == HIGH && digitalRead(BTN_RIGHT) == HIGH)
    {
      isButtonPressed = false;
    }

    currentState->loop();
  }
}