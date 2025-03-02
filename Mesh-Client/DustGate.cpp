// THIS CODE IS FOR A BLAST GATE CONTROLLER

#include "Arduino.h"
#include "DustGate.h"
#include <Servo.h>

//Constructor -- defaults to digital
DustGate::DustGate(uint16_t pin) {

  _pin = pin;
  _openPos = 145;    //260;  //73?
  _closedPos = 260;  //114; //0?
  _state = -1;       //not set!

  _servo.attach(_pin);
  Close();
}

void DustGate::SetOpenPos(uint16_t i) {
  _openPos = i;
}

void DustGate::SetClosedPos(uint16_t i) {
  _closedPos = i;
}

void DustGate::Attach(uint16_t pin) {
  _servo.attach(_pin);
}

void DustGate::Open() {
  _servo.write(_openPos);
  delay(200);  // Adjust the delay time as needed
  Serial.print("GATE OPENED TO ");
  Serial.println(_openPos);
  _state = 1;
}

void DustGate::Close() {
  _servo.write(_closedPos);
  Serial.print("GATE CLOSED TO ");
  Serial.println(_closedPos);
  _state = 0;
}