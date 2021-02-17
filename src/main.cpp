// Arduino receiver for laskomex ringing signal, output is serial message with decoded ringing phone address

#include <Arduino.h>

//#define DEBUG

const byte interruptPin = 2;
volatile byte interruptCounter = 0;
bool startOfCommand = false;
bool endOfCommand = false;
int ringingAddress = 0;
int maxAddress = 14;
unsigned long lastTime = 0;
String output ;

void handleInterrupt() {
  interruptCounter++;
}

void setup() {
  Serial.begin(115200);
  #ifdef DEBUG
    Serial.println();
    Serial.println("Hold your hats");
  #endif
  Serial.println("0");
   
  // Set interrupt pin
  pinMode(interruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, CHANGE);

  pinMode(LED_BUILTIN, OUTPUT);


} // End Setup

void loop() {
  //Serial.println(interruptPin ? "HIGH" : "LOW") ;
  if(interruptCounter>0){  // We have interrupt, do something
      unsigned long now = micros();
      uint32_t timeDifference = now - lastTime ;
      lastTime = now;
      interruptCounter--;

      if( timeDifference >= 140 * 1000  && timeDifference <= 240 * 1000  ) {  // 200 ms
        // We have starting or ending bit
        // Start command
        startOfCommand = true;
        endOfCommand = false;
        ringingAddress = 0;
        output = "\n \r<Start cmd>";
        output += "t=" + String(timeDifference) + "us";
      }
    
    if( timeDifference >= 1 && timeDifference <= 30 && startOfCommand == true ) {  // 24 us
      // We are in command mode, start counting pulses
      ringingAddress++;
      //output += "\n \r<Ring" + String(ringingAddress) + ">";
      //output += "t=" + String(timeDifference) + "us";
    }

  } else { // We have NO interrupt

    // Timeouts
    unsigned long now = micros();
    uint32_t timeDifference = now - lastTime ;
    if( timeDifference >= 4 * 1000  && startOfCommand == true && ringingAddress > 0) {  // 4 ms
      // there were no pulses, trigger timeout
      startOfCommand = false;
      endOfCommand = true;
      output += "\n \r<End cmd>";
      output += "t=" + String(timeDifference) + "us";

      // Output for ESP
      if (ringingAddress <= maxAddress ) {
        Serial.println(ringingAddress);
      }
      
      #ifdef DEBUG  
        // Verbose output
        Serial.print("There is call to phone : ");
        Serial.println(ringingAddress);
        Serial.print(output);
        Serial.println();
      #endif
      output = "";

      lastTime = now;
    }  // end timeout of dialing

    if( timeDifference >= 1 * 1000000 && startOfCommand == true ) {   // Timeout for inactivity 1 s
      startOfCommand = false;
      endOfCommand = true;
      ringingAddress = 0;
      #ifdef DEBUG
        Serial.print(output);
        Serial.println();
        Serial.println("Timeout for inactivity");  
      #endif
      
    }  // end timeout for inactivity

    if( timeDifference >= (5 * 1000000) && endOfCommand == true ) {   // Revert ringing to zero after  5 s
      startOfCommand = false;
      endOfCommand = false;
      ringingAddress = 0;

      // Output for ESP
      if (ringingAddress <= maxAddress ) {
        Serial.println(ringingAddress);
      }     

      #ifdef DEBUG
        output += "\n \r<Revert>";
        output += "t=" + String(timeDifference) + "us";
        Serial.print(output);
        Serial.println();
      #endif

    }  // end revert to 0
  } 

//  if(digitalRead(interruptPin) == LOW ) {
//    digitalWrite(LED_BUILTIN, LOW);
//  } else {
//    digitalWrite(LED_BUILTIN, HIGH);
//  }


} // end LOOP