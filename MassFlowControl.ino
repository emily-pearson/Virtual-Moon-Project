// requires Uno R3 as TimerOne library is not supported on Uno R4 architecture
// serial comms code builds on example from Robin2 on Arduino.cc forums

#include <TimerOne.h>
#define PWM_PIN 9 //can only use pin 9 or 10 for timer1 based PWM
#define MFC_READOUT A0;

const byte numInputChars = 5; //max input flow rate is 4 digits (range 0-1000) + new line character = 5
char receivedSerialInput[numInputChars]; //array to store received input string
boolean newInputData = false;

int serialFlowRate = 0;

// unsigned int set_flow_rate = 500; // desired flow rate in SCCM
// unsigned int serial_flow_rate;

void setup() {
Serial.begin(9600);
pinMode(PWM_PIN,OUTPUT); 
Timer1.initialize(100); //100us = 10kHz (10x the MFC sampling rate)

// tell MATLAB that Arduino is ready to communicate
Serial.println("<Arduino is ready>");
}

void loop() {
  // read flow rate from MATLAB
  readSerialInput();
  showSerialInput();
  setFlowRate();

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

void readSerialInput(){
  static byte charIndex = 0;
  char endMarker = '\n'; //terminator is new line
  char receivedChar;

  if (Serial.available() > 0) {
    receivedChar = Serial.read();

    if (receivedChar != endMarker){
      receivedSerialInput[charIndex] = receivedChar;
      charIndex++;
      if (charIndex >=  numInputChars) { //resets index counter to prevent inputs larger than 5 bytes
        charIndex = numInputChars - 1;
      }
    }
    else {
      receivedSerialInput[charIndex] = '\0'; //terminate the string
      charIndex = 0;
      newInputData = true;
    }
  }
}

void showSerialInput() {
    if (newInputData == true) {
        serialFlowRate= 0;             // new for this version
        serialFlowRate = atoi(receivedSerialInput);   // new for this version
        Serial.print("Input Flow Rate: ");
        Serial.println(serialFlowRate);
        newInputData = false;
    }
}

void setFlowRate(){
  float dutyCycle = serialFlowRate * 1.023;
  unsigned int dutyCycleInt = round(dutyCycle);
  Timer1.pwm(PWM_PIN,dutyCycleInt);
}