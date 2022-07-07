#include "ButtonHandler.h"

Button::Button(int btnPin, bool pullup = true, bool need_pullup = false)
{
  buttonPin = btnPin;

  if (need_pullup)
  {
    pullupMode = LOW;
    pinMode(buttonPin, INPUT_PULLUP);
  }
  else
  {
    if (pullup == true)
    {
      pullupMode = LOW;
      pinMode(buttonPin, INPUT);
    }
    else
    {
      pullupMode = HIGH;
      pinMode(buttonPin, INPUT);
    }
  }
  
  outputState = LOW;
  lastButtonState = !pullupMode;
  lastDebounceTime = 0;
  debounceDelay = 50;
}

void Button::scan()
{
  int reading = digitalRead(buttonPin);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == pullupMode) {
        outputState = !outputState;
      }
    }
  }

  lastButtonState = reading;
}

int Button::getRawOutput()
{
  return digitalRead(buttonPin);
}

int Button::getOutput()
{
  int temp = outputState;
  outputState = 0;
  return temp;
}