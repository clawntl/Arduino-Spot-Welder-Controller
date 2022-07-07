#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <ButtonHandler.h>

#define DEBUG false  //set to true for debug output, false for no debug output
#define DEBUG_SERIAL if(DEBUG)Serial

//Define pins for LCD
#define RS_PIN 2
#define E_PIN 3
#define D4_PIN 4
#define D5_PIN 5
#define D6_PIN 6
#define D7_PIN 7

//Macros for aligning text to the centre and right of the 16x2 LCD
#define ALIGN_CENTRE(t)       (7-(t.length())/2)
#define ALIGN_RIGHT(t)       (16-(t.length()))

//Define transformer pins
const int transformer_1_pin = A0;
const int transformer_2_pin = A1;

//Define button pins
const int increments_btn=8, powerMode_btn=9, time_plus_button=10, time_minus_btn=11, weld_btn=12;

//Stores number of transformers currently active
byte powerMode = 1;

//Stores selectable time increments
const unsigned int incrementSteps[] = {50, 100, 200};
const byte numberIncrementSteps = sizeof(incrementSteps) / sizeof(incrementSteps[0]);
unsigned int selectedIncrement = 2;

unsigned int timeVal = 200; //Current time value

//Min and max time values
unsigned int minVal = incrementSteps[selectedIncrement];
unsigned int maxVal = 2000;

bool manualMode = false; //Keeps track of whether manual mode is enabled

bool onCooldown = false; //Keeps track of whether the welder is cooling down

//Create strings to store the prevously displayed text so it can be cleared properly
String prev_powerModeVal_string = "";
const String powerModePrefix_string = "Power:";
const int powerModePrefix_string_len = powerModePrefix_string.length();
const String timeValPrefix_string = "Time:";
const int timeValPrefix_string_len = timeValPrefix_string.length();

//Create lcd object
LiquidCrystal lcd(RS_PIN, E_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);

//Create button objects that use the internal pullup resistors so (btn, true, true)
Button powerModeButton(powerMode_btn, true, true);
Button timePlusButton(time_plus_button, true, true);
Button timeMinusButton(time_minus_btn, true, true);
Button incrementButton(increments_btn, true, true);
Button weldButton(weld_btn, true, true);

int handleInputs(); //Forward declaration of handleInputs function since a higher function references it

//Clear text from a specific area of a row on the display
void clearText(byte from, byte to, byte row)
{
  byte strLen = (to+1) - from; //Calculate the length of the string
  String clearString = ""; //Create string to store the number of spaces
  while (strLen--)
  {
    clearString += " "; //Add the number of spaces to the string
  }
  //Update the display
  lcd.setCursor(from, row);
  lcd.print(clearString);
}

//Display the power mode line
void displayPower()
{
  clearText(0, 9, 0);

  lcd.setCursor(0,0); 
  lcd.print(powerModePrefix_string);
  switch (powerMode)
  {
    case 1:
      lcd.print("LOW");
      break;
    case 2:
      lcd.print("HIGH");
      break;
  }
}

//Switch the power mode
void togglePowerMode()
{
  //Clear the value after the prefix
  clearText(powerModePrefix_string_len, powerModePrefix_string_len  + (prev_powerModeVal_string.length() - 1), 0);

  //Switch the power mode
  String powerModeValue_string = "";
  switch (powerMode)
  {
    case 1:
      powerMode = 2;
      powerModeValue_string = "HIGH";
      break;
    case 2:
      powerMode = 1;
      powerModeValue_string = "LOW";
      break;
  }

  //Update the displayed value
  lcd.setCursor(powerModePrefix_string_len, 0);
  lcd.print(powerModeValue_string);
  prev_powerModeVal_string = powerModeValue_string;
}

//Display the time value on the second line of the display
void displayTime()
{
  clearText(0,15,1);
  lcd.setCursor(0,1);
  lcd.print(timeValPrefix_string + (String)timeVal + "ms");
}

void updateTimeValDisplay()
{
  clearText(timeValPrefix_string_len, 15, 1);
  lcd.setCursor(timeValPrefix_string_len, 1);
  if (manualMode)
  {
    lcd.print("MAN");
  }
  else
  {
    lcd.print((String)timeVal + "ms");
  }
}

void increaseTimeVal()
{
  DEBUG_SERIAL.println(timeVal);
  if (timeVal < maxVal)
  {
    timeVal += incrementSteps[selectedIncrement];
    manualMode = false;
    updateTimeValDisplay();
  }
  else if (timeVal == maxVal) //if at max value then manual mode is engaged
  {
    timeVal = minVal - incrementSteps[selectedIncrement];
    manualMode = true;
    updateTimeValDisplay();
  }
  else if (timeVal > maxVal) //if timeVal has exceeded the max value, reset it to the min value
  {
    timeVal = minVal;
    manualMode = false;
    updateTimeValDisplay();
  }
}

void decreaseTimeVal()
{
  if (timeVal > minVal)
  {
    timeVal -= incrementSteps[selectedIncrement];
    manualMode = false;
    updateTimeValDisplay();
  }
  else if (timeVal == minVal) //if at min value then manual mode is engaged
  {
    timeVal = maxVal + incrementSteps[selectedIncrement];
    manualMode = true;
    updateTimeValDisplay();
  }
  else if (timeVal < minVal) //if timeVal has exceeded the min value, reset it to the max value
  {
    timeVal = maxVal;
    manualMode = false;
    updateTimeValDisplay();
  }
}

//Change the current increment amount
void changeIncrement()
{
  selectedIncrement = (selectedIncrement + 1) % numberIncrementSteps; //go to next item in circuluar queue
  minVal = incrementSteps[selectedIncrement]; //set the min value

  //Print to the display
  clearText(0, 9, 0);
  lcd.setCursor(0,0);
  lcd.print("Incr:" + (String)incrementSteps[selectedIncrement] + "ms");

  unsigned long startMillis = millis();
  bool changed = false;
  while (!changed)
  {
    int scanInputs = handleInputs();

    if ((millis() - startMillis >= 1500) || scanInputs == 1 || scanInputs == 5)
    {
      displayPower();
      changed = true;
    }
  }
}

void weld()
{
  int cooldownDelay = 3000;

  clearText(10, 15, 1); //clear time displayed from manual weld

  String weld_string = "WELD";
  clearText(ALIGN_RIGHT(weld_string), 15, 0);
  lcd.setCursor(ALIGN_RIGHT(weld_string), 0);
  lcd.print(weld_string);

  if (manualMode)
  {
    bool finishedWeld = false;
    unsigned long timeBefore, timeAfter;

    switch (powerMode)
    {
      case 1:
        timeBefore = millis();
        digitalWrite(transformer_1_pin, HIGH);
        while (!finishedWeld)
        {
          weldButton.scan();
          if (!weldButton.getRawOutput() == 0)
          {
            digitalWrite(transformer_1_pin, LOW);
            timeAfter = millis();
            finishedWeld = true;
          }
        }
        break;
      case 2:
        timeBefore = millis();
        digitalWrite(transformer_1_pin, HIGH);
        digitalWrite(transformer_2_pin, HIGH);
        while (!finishedWeld)
        {
          weldButton.scan();
          if (!weldButton.getRawOutput() == 0)
          {
            digitalWrite(transformer_1_pin, LOW);
            digitalWrite(transformer_2_pin, LOW);
            timeAfter = millis();
            finishedWeld = true;
          }
        }
        break;
    }

    int weld_time = 10*(((timeAfter - timeBefore) + 5)/10);
    String weld_time_string = (String)weld_time + "ms";
    lcd.setCursor(ALIGN_RIGHT(weld_time_string),1);
    lcd.print(weld_time_string);
  }
  else
  {
    switch (powerMode)
    {
      case 1:
        digitalWrite(transformer_1_pin, HIGH);
        delay(timeVal);
        digitalWrite(transformer_1_pin, LOW);
        break;
      case 2:
        digitalWrite(transformer_1_pin, HIGH);
        digitalWrite(transformer_2_pin, HIGH);
        delay(timeVal);
        digitalWrite(transformer_1_pin, LOW);
        digitalWrite(transformer_2_pin, LOW);
        break;
    }
  }

  clearText(ALIGN_RIGHT(weld_string), 15, 0);
  String cool_string = "COOL";
  lcd.setCursor(ALIGN_RIGHT(cool_string), 0);
  lcd.print(cool_string);

  unsigned long startMillis = millis();
  onCooldown = true;
  while (onCooldown)
  {
    handleInputs();
    if (millis() - startMillis >= cooldownDelay)
    {
      clearText(ALIGN_RIGHT(cool_string), 15, 0);
      onCooldown = false;
    }
  }
}


int handleInputs()
{
  //Scan the inputs
  powerModeButton.scan();
  timePlusButton.scan();
  timeMinusButton.scan();
  incrementButton.scan();
  weldButton.scan();

  /*Keep a variable so we know what function has been executed
  Useful for limiting actions when calling handleInputs() from
  other methods*/
  int executedFunction = 0;

  if (powerModeButton.getOutput() == 1)
  {
    DEBUG_SERIAL.println("Pressed mode button");
    executedFunction = 1;
    togglePowerMode();
  }

  else if (timePlusButton.getOutput() == 1)
  {
    DEBUG_SERIAL.println("Pressed time plus button");
    executedFunction = 2;
    increaseTimeVal();
  }

  else if (timeMinusButton.getOutput() == 1)
  {
    DEBUG_SERIAL.println("Pressed time minus button");
    executedFunction = 3;
    decreaseTimeVal();
  }

  else if (incrementButton.getOutput() == 1)
  {
    DEBUG_SERIAL.println("Pressed increments button");
    executedFunction = 4;
    changeIncrement();
  }

  else if (weldButton.getOutput() == 1)
  {
    DEBUG_SERIAL.println("Pressed weld button");
    if (!onCooldown)
    {
      executedFunction = 5;
      weld();
    }
  }

  return executedFunction;
}

//Simple splash screen displayed on startup
void splashScreen(int delay_time)
{
  String splashText0 = "Spot";
  String splashText1 = "Welder";
  lcd.setCursor(ALIGN_CENTRE(splashText0), 0);
  lcd.print(splashText0);
  lcd.setCursor(ALIGN_CENTRE(splashText1), 1);
  lcd.print(splashText1);
  delay(delay_time);
  clearText(0,15,0);
  clearText(0,15,1);
}

void setup() 
{
  DEBUG_SERIAL.begin(115200);

  pinMode(transformer_1_pin, OUTPUT);
  pinMode(transformer_2_pin, OUTPUT);

  digitalWrite(transformer_1_pin, LOW);
  digitalWrite(transformer_2_pin, LOW);

  lcd.begin(16,2);

  splashScreen(2000);
  displayPower();
  displayTime();
}

void loop() 
{
  handleInputs();
}