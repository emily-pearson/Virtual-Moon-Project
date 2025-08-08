/// Basic code to purge the flow-controlled gas line (S1 solenoid). Currently no time limit ///
///    and is stopped via manual power off. Could be modified to run for set time period    ///


/// This code is only suitable for Arduino Uno R3, as it uses the TimerOne library which is ///
///                       not supported on Uno R4 architecture.                             ///


#include <TimerOne.h>
#define PWM_PIN 9
#define RELAY_1 4 // NB: relay pins are set by board architecture so cannot be changed
#define RELAY_2 7

// set flow rate to the maximum possible on the mass flow controller 
// NB: suitable mass flow controller (max of 20-30 SLPM) required for sufficient flow rate
int serialFlowRate = 1000; // in SCCM

void setup() {
Serial.begin(9600);
// set pins to output mode
pinMode(PWM_PIN,OUTPUT);
pinMode(RELAY_1, OUTPUT);
pinMode(RELAY_2, OUTPUT);
// set PWM frequency
Timer1.initialize(100); // 10 kHz = 100us
}

void loop() {
// turn solenoid S1 on, solenoid S2 off
digitalWrite(RELAY_1,HIGH);
digitalWrite(RELAY_2,LOW);

// convert flow rate to duty cycle (duty cycle = 1 as max flow rate used)
float dutyCycle = serialFlowRate * 1.023;
unsigned int dutyCycleInt = round(dutyCycle);

// set max flow rate on the flow controller
Timer1.pwm(PWM_PIN,dutyCycleInt);
delay(50);
}
