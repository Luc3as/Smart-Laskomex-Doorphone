## Basic communication characteristics

There is 200ms pulse when the line is 0V before start and at the end of each command.

Then the bus goes up to around 11V, and if there is incoming call, there are number of spikes down to zero, the each spike is 24 us long, and there is 60us to 1000us gap between these pulses. After 2ms we can rise timeout of pulse reading. 

Number of pulses corresponds to number of doorphone configured through jumpers on phones PCB

* we are waiting for 200ms long pulse 
* we enter to the command start mode 
* we are waiting for number of pulses 
* in the loop we check how long it was from last interrupt, if it matches starting pulse, or shor 24us pulse, we increment corresponding variable
* if there is no pulse for 1s , we run timeout, and set variables to start state

