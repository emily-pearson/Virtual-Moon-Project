// requires Uno R3 as TimerOne library is not supported on Uno R4 architecture
// serial comms code builds on example from Robin2 on Arduino.cc forums

#include <TimerOne.h>
#define PWM_PIN 9 // can only use pin 9 or 10 for timer1 based PWM
#define MFC_READOUT A0
#define RELAY_1 4
#define RELAY_2 7

// initialise global variables
const byte numInputChars = 5; // max input flow rate is 4 digits (range 0-1000) + new line character = 5
char receivedSerialInput[numInputChars]; // array to store received input string
boolean newInputData = false;
int serialFlowRate = 0;
boolean flowRateFlag = false;
int terminateCommand = 1001;

void setup() {
Serial.begin(9600);

// set digital pins to output mode
pinMode(PWM_PIN,OUTPUT); 
pinMode(RELAY_1,OUTPUT);
pinMode(RELAY_2,OUTPUT);
// set PWM frequency
Timer1.initialize(100); //100us = 10kHz (10x the MFC sampling rate)

// tell MATLAB that Arduino is ready to communicate
Serial.println("<Arduino is ready>");
}

void loop() {
  readSerialInput();
  // checkSerialInput();
  showSerialInput();
  readFlowRate();
}

void readSerialInput(){
  // initialise local variables
  static byte charIndex = 0;
  char endMarker = '\n'; // terminator set to "new line"
  char receivedChar;

  if (Serial.available() > 0) {
    receivedChar = Serial.read();

    if (receivedChar != endMarker){
      receivedSerialInput[charIndex] = receivedChar; // add each byte (digit) to input character string
      charIndex++;
      if (charIndex >=  numInputChars) { // adjusts index counter to truncate inputs larger than 5 bytes
        charIndex = numInputChars - 1;
      }
    }
    else {
      receivedSerialInput[charIndex] = '\0'; // terminate the string when end marker received
      charIndex = 0;
      newInputData = true;
      flowRateFlag == false;
    }
  }
}

void showSerialInput() {
    if (newInputData == true) {
        serialFlowRate= 0;            
        serialFlowRate = atoi(receivedSerialInput); // convert character input to integer format
        setFlowRate();
        digitalWrite(RELAY_1, HIGH);
        digitalWrite(RELAY_2,LOW);
        Serial.println(serialFlowRate); // for confirmation in MATLAB
        newInputData = false;
    }
}

void setFlowRate(){
  float dutyCycle = serialFlowRate * 1.023; // convert input flow rate to duty cycle value for PWM
  unsigned int dutyCycleInt = round(dutyCycle); // round result to nearest integer
  Timer1.pwm(PWM_PIN,dutyCycleInt); 
  flowRateFlag = true; // flag that flow rate has been set
}

void readFlowRate(){
    while (flowRateFlag == true) { // only write to serial if the flow rate has been received (blocks write until read executed)
        int flowVoltageRead = analogRead(MFC_READOUT);
        float measuredFlowRate = flowVoltageRead * 200;
        int measuredFlowRateInt = round(measuredFlowRate); // round to nearest integer flow rate
        // Serial.print("Measured Flow Rate: ");
        // check there is enough buffer space available to write to serial
        int bytesAvailable = Serial.availableForWrite();
        if (bytesAvailable > 5) { // max of 4 digits for flow rate (1000 SCCM)
          Serial.print(measuredFlowRateInt);
          Serial.println();
          delay(500);
        }
        if (newInputData == true) {
          break;
        }
      }
  }
