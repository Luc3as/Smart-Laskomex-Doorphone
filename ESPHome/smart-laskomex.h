#include "esphome.h"

class DoorPhone : public Component, UARTDevice {

 public:
  DoorPhone(UARTComponent *parent) : UARTDevice(parent) {}
  #ifdef LOG_SENSOR
    Sensor *called_number = new Sensor();
  #endif


  void setup() override {
    
  }

  void loop() override {
    if (Serial.available() > 0) {
      int received = Serial.readStringUntil('\n').toInt();
      ESP_LOGD("door-phone", "RX: %d", received);
        if (received >= 0 && received <= 99) {
            called_number->publish_state(received);
        }
      
      
    }
  }
};