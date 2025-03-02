#include "Arduino.h" 
#include "ButtonLight.h"

//Constructor -- defaults to digital
//BUTTONS:  Blue=NC, Yellow=NO, Green=Common, Red=LED, Black=Ground
//FINAL WIRING: Red->LED Pin (13).  Black->Ground.  BLUE->5Vin.  Green->ButtonInput (12) (LOW is press)

ButtonLight::ButtonLight(uint8_t  lightPin, uint8_t  buttonPin){
  _lightPin = lightPin;
  _buttonPin = buttonPin; 
  _lightState = 0; //off by default
  _digital = true; //default to digital pins
  pinMode(lightPin, OUTPUT);   // button light
  pinMode(buttonPin, INPUT);    //button
  _buttonStateMachine = STATE_IDLE; //default debounce
  _lastButtonState = HIGH; //default debounce
  _lastDebounceTime = 0; //default debounce
}

ButtonLight::ButtonLight(unsigned int id, uint8_t  lightPin, uint8_t  buttonPin) {
 ButtonLight(lightPin, buttonPin); 
  _id = id;
   //analog or digital
  _digital = true;
}

//Creates a pin explicitly as Digital or Analog Pins.  Both Light and Button must be the same.
ButtonLight::ButtonLight(unsigned int id, uint8_t  lightPin, uint8_t  buttonPin, bool isDigital) {
  ButtonLight(id, lightPin, buttonPin);
  _digital = isDigital; //override digital/analog
}

//Set to Digital Mode
void ButtonLight::disableAnalogMode(){
  _digital = true;
}

//Set to Analog Mode
void ButtonLight::enableAnalogMode(){
  _digital = false;
}

//sets light
void ButtonLight::setLightState(bool value){
  _lightState = value;
}

//power light
void ButtonLight::powerLight(){
  if(!_digital)
    digitalWrite(_lightPin,!_lightState);
  else
    digitalWrite(_lightPin,!_lightState);
  return;
}

//Turn Light On
void ButtonLight::turnOnLight(){
  setLightState(0);
}

//Turn Light Off
void ButtonLight::turnOffLight(){
  setLightState(1);
}

//Get Light State
bool ButtonLight::getLightState(){
  return _lightState;
}

//Get ID
unsigned int ButtonLight::getId(){
  return _id;
}

//Get Button State
bool ButtonLight::getButtonState(){ 
  int reading;
  bool rValue = false;

  if(!_digital)
    reading = analogRead(_buttonPin);
  else{
    reading = digitalRead(_buttonPin);
  }

    // Update the state machine
  switch (_buttonStateMachine) {
    case STATE_IDLE: // IS IDLE
      if (reading == LOW) {
        // Button pressed, move to the "debouncing" state
        _buttonStateMachine = STATE_DEBOUNCING;
        _lastDebounceTime = millis();

        Serial.println("Button pressed!");
        rValue = true;
      }
      break;

    case STATE_DEBOUNCING:
      if (millis() - _lastDebounceTime > 500) {
        // Button debounced, move to the "pressed" state
        _buttonStateMachine = STATE_IDLE;      
      }
      else{ 
        //Serial.println("Debouncing...");
      }

      break;
  }

  return rValue;
}