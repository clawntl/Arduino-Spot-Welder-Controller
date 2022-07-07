#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>

class Button
{
  private:
    bool pullupMode;
    int buttonPin;
    int outputState;
    int buttonState;
    int lastButtonState;
    unsigned long lastDebounceTime;
    unsigned long debounceDelay; 

  public:
    Button(int btnPin, bool pullup, bool need_pullup);
    void scan();
    int getRawOutput();
    int getOutput();
};
#endif