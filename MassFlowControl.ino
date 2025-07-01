// requires Uno R3 as TimerOne library is not supported on Uno R4 architecture

#include <TimerOne.h>
#define PWM_PIN 9 //can only use pin 9 or 10 for timer1 based PWM
#define MFC_READOUT A0;

int set_flow_rate = 500; // desired flow rate in SCCM

void setup() {
Serial.begin(9600);
pinMode(PWM_PIN,OUTPUT); 
Timer1.initialize(100); //100us = 10kHz (10x the MFC sampling rate)

float duty_cycle = set_flow_rate * (1.023); // convert flow rate to duty cycle
duty_cycle = round(duty_cycle); // round up for better replication of desired flow rate
Timer1.pwm(PWM_PIN,duty_cycle); 

// tell MATLAB that Arduino is ready to communicate
Serial.println("Arduino is ready");
}

void loop() {
  // read flow rate from MATLAB
  if (Serial.available() > 0) {
    unsigned int serial_flow_rate = Serial.read();
    Serial.write(serial_flow_rate);
  }
  
  // read back voltage from analog out and transform into a flow rate
// int MFC_voltage_read = analogRead(MFC_READOUT);
// float measured_flow_rate = MFC_voltage_read * 200;
// measured_flow_rate = round(measured_flow_rate) // round up to nearest integer flow rate

// // check there is enough buffer space available to write to serial
//   int bytesAvailable = Serial.availableForWrite();
//   if (bytesAvailable > 5) { // max of 4 digits for flow rate (1000 SCCM)

// // send measured flow rate to MATLAB over serial
// Serial.print(measured_flow_rate);
// Serial.println();
// delay(50);

// // eventually will also send thermocouple data to MATLAB over serial 
//   }
}
