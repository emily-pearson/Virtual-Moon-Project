/// Basic code to purge the bypass gas line (S2 solenoid). Currently no time limit and  ///
///   is stopped via manual power off. Could be modified to run for set time period.    ///

#define RELAY_1 4 // NB: relay pins are set by board architecture so cannot be changed
#define RELAY_2 7

void setup() {
Serial.begin(9600);
// set pins to output mode
pinMode(RELAY_1, OUTPUT);
pinMode(RELAY_2, OUTPUT);
}

void loop() {
// turn S1 off, S2 on
digitalWrite(RELAY_1,LOW);
digitalWrite(RELAY_2,HIGH);
delay(50);
}
