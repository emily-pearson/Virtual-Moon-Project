// requires Uno R3 as TimerOne library is not supported on Uno R4 architecture
// serial comms code builds on example from Robin2 on Arduino.cc forums

#include <TimerOne.h>
#define PWM_PIN 9 // can only use pin 9 or 10 for timer1 based PWM
#define MFC_READOUT A0
#define TC_READOUT A5
#define RELAY_1 4 // NB: relay pins are set by board architecture and cannot be changed
#define RELAY_2 7

// initialise global variables
const byte numInputChars = 5; // max input flow rate is 4 digits (range 0-1000) + new line character = 5
char receivedSerialInput[numInputChars]; // array to store received input string
int serialFlowRate = 0;
unsigned long startMillis;
unsigned long currentMillis;

// initialise boolean flags
boolean newInputData = false;
boolean flowRateFlag = false;
boolean stopFlag = false;
boolean printFlag = false;

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
  showSerialInput();
  readData();
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
    }
  }
}

void showSerialInput() {
    if (newInputData == true) { 
        serialFlowRate = 0;            
        serialFlowRate = atoi(receivedSerialInput); // convert character input to integer format
        
        // stop transmission if terminate command (integer 1001) is sent from MATLAB. Turn off S1, wait 2 seconds, then turn on S2
        if (serialFlowRate == 1001){ // 
          stopFlag = true;
          digitalWrite(RELAY_1,LOW); 
          delay(2000); // WARNING delay is a blocking function and will potentially need to be replaced with millis() later in order to record temperature data during this period
          digitalWrite(RELAY_2,HIGH); 
        }
        // otherwise set the flow rate to the value requested. Turn on S1 and turn off S2
        else{
        setFlowRate();
        digitalWrite(RELAY_1, HIGH);
        digitalWrite(RELAY_2,LOW);
        Serial.println(serialFlowRate); // for confirmation in MATLAB
        newInputData = false; 
        }
    }
}

void setFlowRate(){
  int adjustedFlowRate = serialFlowRate + 5; //adjust for error correction (error between chosen setpoint and that displayed on the MFC screen)
  float dutyCycle = adjustedFlowRate * 1.023; // convert input flow rate to duty cycle value for PWM
  unsigned int dutyCycleInt = round(dutyCycle); // round result to nearest integer 
  Timer1.pwm(PWM_PIN,dutyCycleInt); 
   flowRateFlag = true; // flag that flow rate has been set. 
}

void readData(){
    if (newInputData == false && flowRateFlag == true && stopFlag == false) { // only write to serial under set conditions (no new serial input, already sent confirmation of flow rate, not received stop command)
        
        // read flow rate
        int flowADC = analogRead(MFC_READOUT);
        float flowVoltageRead = flowADC * (5.06/1023); //5.06V measured off Arduino with multimeter
        float measuredFlowRate = flowVoltageRead * 200;
        int measuredFlowRateInt = round(measuredFlowRate); // round to nearest integer flow rate

        // read temperature 
        int thermoADC = analogRead(TC_READOUT);
        float thermoVoltage = thermoADC * (5.06/1023); //5.06V measured off Arduino with multimeter
        float temperature = (thermoVoltage - 1.25)/0.005;

        // check there is enough buffer space available to write to serial
        int bytesAvailable = Serial.availableForWrite();
        if ((bytesAvailable > 5) && (printFlag == false)){ // max of 4 digits for flow rate (1000 SCCM)

          // send values over serial using a comma-separated string
          Serial.print(measuredFlowRateInt);
          Serial.print(',');
          Serial.print(temperature);
          Serial.println();
          printFlag = true;
          startMillis = millis();
        }
        // if a value has been sent over serial, wait 100ms before sending the next one
        if (printFlag == true) {
          currentMillis = millis();
          if (currentMillis - startMillis >= 100){ // check is 100ms has elapsed
            printFlag = false; // allow values to be sent over serial again
          }
        }
      }
  }

