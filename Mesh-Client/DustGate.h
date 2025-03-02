#ifndef DustGate_h
#define DustGate_h
#include <Arduino.h> 
#include <Servo.h> 

class DustGate {
public:
  //constructor
	DustGate(uint16_t pin);
  virtual void Attach(uint16_t pin);
  virtual void Open();
  virtual void Close();

  virtual void SetOpenPos(uint16_t i);
  virtual void SetClosedPos(uint16_t i);

  //Button
  uint8_t getState(); // returns button click state (0 or 1)

protected:
	uint8_t _pin;
  unsigned int _id;
  unsigned int _openPos;
  unsigned int _closedPos;
  uint8_t _state;
  Servo _servo;

};
#endif
