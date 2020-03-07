#include <Arduino.h>

const byte interruptPin = D7;
volatile byte interruptCounter = 0;
bool startOfCommand = false;
bool endOfCommand = false;
int ringingAddress = 0;
unsigned long lastTime = 0;

//#define DEBUG
  
ICACHE_RAM_ATTR void handleInterrupt() {
  interruptCounter++;
}
 
void setup() {
 
  Serial.begin(115200);
  pinMode(interruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, CHANGE);
 
}

void loop() {
  //Serial.println(interruptPin ? "HIGH" : "LOW") ;
  if(interruptCounter>0){  // We have interrupt, do something
      unsigned long now = micros();
      uint32_t timeDifference = now - lastTime ;
      lastTime = now;
      interruptCounter--;

      if( timeDifference >= 150 * 1000  && timeDifference <= 250 * 1000  ) {  // 200 ms
        // We have starting or ending bit
        if(startOfCommand == false ) {  // Start command
          startOfCommand = true;
          endOfCommand = false;
          ringingAddress = 0;
          #ifdef DEBUG
            Serial.println();
            Serial.println();
            Serial.println("Starting command");  
          #endif
        } else {                        // End command
          endOfCommand = true;
          #ifdef DEBUG
            Serial.println("Ending command");  
            Serial.println();
          #endif
        }
        #ifdef DEBUG
          Serial.print("Pulse time: ");  
          Serial.print(timeDifference);  
          Serial.println(" us");
        #endif
      }
    
    if( timeDifference >= 19 && timeDifference <= 29 && startOfCommand == true ) {  // 24 us
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
 if( timeDifference >= 10 * 1000  && startOfCommand == true && ringingAddress > 0) {  // 10 ms
   // there were no pulses, trigger timeout
   endOfCommand = true;
   startOfCommand = false;
   Serial.print("There is call to phone : ");
   Serial.println(ringingAddress);
   #ifdef DEBUG
       Serial.print("Pulse time: ");  
       Serial.print(timeDifference);  
       Serial.println(" us");
   #endif
 }
  if( timeDifference >= 1000 * 1000 && startOfCommand == true ) {   // Timeoute for inactivity 1 s
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