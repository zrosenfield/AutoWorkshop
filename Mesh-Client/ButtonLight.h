#ifndef ButtonLight_h
#define ButtonLight_h
#include <Arduino.h> 

class ButtonLight {
public:
  //constructor
  ButtonLight(uint8_t  lightPin, uint8_t  buttonPin);
	ButtonLight(unsigned int id, uint8_t  lightPin, uint8_t  buttonPin);
  ButtonLight(unsigned int id, uint8_t  lightPin, uint8_t  buttonPin, bool isDigital);

  //pinConfig
  void enableAnalogMode();
  void disableAnalogMode();

  //Light
  void turnOnLight(); //turns light on
  void turnOffLight(); //turns light off
  void setLightState(bool value); //sets light to value
  bool getLightState(); //returns light state (0 or 1)
  void powerLight(); //Loop function to power
 
  //Button
  bool getButtonState(); // returns button click state (0 or 1)
  unsigned int getId();

private:
	uint8_t  _lightPin;
	uint8_t  _buttonPin;
  unsigned int _id;
  bool  _lightState;
  bool _digital;

  //debounce:

  enum buttonState {
    STATE_DEBOUNCING,
    STATE_IDLE,
    STATE_PRESSED
  };
  buttonState _buttonStateMachine;
  uint8_t _lastButtonState;
  unsigned long _lastDebounceTime;
};
#endif