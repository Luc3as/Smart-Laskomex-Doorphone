#include <Arduino.h>

const byte interruptPin = D7;
volatile byte interruptCounter = 0;
int numberOfInterrupts = 0;
bool startOfCommand = false;
bool endOfCommand = false;
int ringingAddress = 0;
unsigned long lastTime = 0;

#define DEBUG
  
ICACHE_RAM_ATTR void handleInterrupt() {
  interruptCounter++;
}
 
void setup() {
 
  Serial.begin(115200);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, CHANGE);
 
}

void loop() {
   if(interruptCounter>0){
      unsigned long now = micros();
      uint32_t timeDifference = now - lastTime ;
      lastTime = now;
      interruptCounter--;
      //numberOfInterrupts++;

      if( timeDifference >= 180 * 1000  && timeDifference <= 230 * 1000  ) {
        // We have starting or ending bit
        if(startOfCommand == false ) {
          startOfCommand = true;
          endOfCommand = false;
          ringingAddress = 0;
          #ifdef DEBUG
            Serial.println();
            Serial.println();
            Serial.println("Starting command");  
          #endif
        } else {
          endOfCommand = true;
          #ifdef DEBUG
            Serial.println("Ending command");  
          #endif
        }
        #ifdef DEBUG
          Serial.print("Pulse time: ");  
          Serial.print(timeDifference);  
          Serial.println(" us");
        #endif
      }
    
    if( timeDifference >= 18 && timeDifference <= 30 && startOfCommand == true ) {
      // We are in command mode, start counting pulses
      ringingAddress++;
      #ifdef DEBUG
        Serial.print("Ring pulse detected ");  
        Serial.println(ringingAddress);  
      #endif
      #ifdef DEBUG
          Serial.print("Pulse time: ");  
          Serial.print(timeDifference);  
          Serial.println(" us");
      #endif
    }

  }

  // Timeouts
  unsigned long now = micros();
  uint32_t timeDifference = now - lastTime ;
  if( timeDifference >= 3 * 1000  && startOfCommand == true && ringingAddress > 0) {
    // there were no pulses for 3 ms, trigger timeout
    endOfCommand = true;
    startOfCommand = false;
    #ifdef DEBUG
      Serial.print("There is call to phone : ");
      Serial.println(ringingAddress);
    #endif
    #ifdef DEBUG
        Serial.print("Pulse time: ");  
        Serial.print(timeDifference);  
        Serial.println(" us");
    #endif
  }
  if( timeDifference >= 1000 * 1000 && startOfCommand == true ) {
    
    startOfCommand = false;
    endOfCommand = true;
    ringingAddress = 0;
    #ifdef DEBUG
      Serial.println("Timeout for inactivity");  
    #endif
    #ifdef DEBUG
        Serial.print("Pulse time: ");  
        Serial.print(timeDifference);  
        Serial.println(" us");
    #endif
  }
}