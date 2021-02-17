## Basic communication characteristics

There is 200ms pulse when the line is 0V before start and at the end of each command.

Then the bus goes up to around 11V, and if there is incoming call, there are number of spikes down to zero, the each spike is 24 us long, and there is 60us to 1000us gap between these pulses. After 2ms we can rise timeout of pulse reading. 

Number of pulses corresponds to number of doorphone configured through jumpers on phones PCB

* we are waiting for 200ms long pulse 
* we enter to the command start mode 
* we are waiting for number of pulses 
* in the loop we check how long it was from last interrupt, if it matches starting pulse, or shor 24us pulse, we increment corresponding variable
* if there is no pulse for 1s , we run timeout, and set variables to start state

If there is voltage ( AC with square wave ) at GND and BR contacts, the ringing is coming from corridor behind the doors. 


## Sensing the ringining
I used separate arduino for sensing the tight timing of ringing patterns, as the ESP8266 was not able to count all interrupts while handling other functions like maintaining wifi or MQTT connection. 

Output from arduino is serial line, where decoded numbers of ringing phone address is sent. 

There is ESPHome sketch receiving this serial messages.

## Connection
I used zener diodes to limit inputs of voltage line for ESP8266 3v3 logic. 

## Opening the door
Work in progress
I need to solve relay configuration to act as picking up the phone and pressing button for opening the doors.

## Images
Images from PCB and osciloscope are in Images folder. 


### If You liked my work, You can buy me a coffee :)

<a class="" target="_blank" href="https://www.buymeacoffee.com/luc3as"><img src="https://lukasporubcan.sk/images/buymeacoffee.png" alt="Buy Me A Coffee" style="max-width: 217px !important;"></a>

### Or send some crypto

<a class="" target="_blank" href="https://lukasporubcan.sk/donate"><img src="https://lukasporubcan.sk/images/donatebitcoin.png" alt="Donate Bitcoin" style="max-width: 217px !important;"></a>	
			