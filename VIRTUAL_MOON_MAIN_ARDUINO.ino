///// This version of the code is based on the use of a mass flow controller (MFC)  /////
///// with a range of 0-1000 SCCM. If an alternative mass flow controller is used,  /////
/////   certain features must be updated. These are detailed in code comments.      /////

///// This code is only suitable for Arduino Uno R3, as it uses the TimerOne library /////
///// which is not supported on Uno R4 architecture. The serial comms code builds on /////
///// an example from Robin2 on Arduino.cc forums. The millis() code functions build /////
/////             on examples from UKHelibob on Arduino.cc forums.                   /////

/////   The constants included with #define should be adjusted to desired values!    /////

#include <TimerOne.h>
#include <movingAvg.h>
#define PWM_PIN 9 // NB: only pins 9 or 10 are available for TimerOne-based PWM
#define MFC_READOUT A0
#define TC_READOUT A5
#define RELAY_1 4 // NB: relay pins are set by board architecture so cannot be changed
#define RELAY_2 7 
#define lowerTempBound -60 // sets lower temperature bound if temperature holding function is used
#define upperTempBound -55 // sets upper temperature bound if temperature holding function is used
#define tempRegulationPeriod 7200000 // time period (in ms) for temperature holding function to run after it is triggered

/// INITIALISE GLOBAL VARIABLES ///

// flow rate input variables - do these need to be global?
const byte numInputChars = 5; // sets size of input flow rate (max. 4 digit number + new line character)
char receivedSerialInput[numInputChars]; // array to store received input string
int serialFlowRate = 0;

// boolean flags
boolean newInputData = false;
boolean flowRateFlag = false;
boolean stopFlag = false;
boolean printFlag = false;
boolean minTempFlag = false;
boolean maxTimeFlag = false;

/// SET UP OBJECTS ///
movingAvg tempMovMean(20); // creates a moving mean using 20 datapoints

void setup() {
Serial.begin(9600);

// initialise moving average object
tempMovMean.begin();

// set digital pins to output mode
pinMode(PWM_PIN,OUTPUT); 
pinMode(RELAY_1,OUTPUT);
pinMode(RELAY_2,OUTPUT);

// set PWM frequency
Timer1.initialize(100); // 100us = 10kHz frequency (10x the flow controller sampling rate)

// perform 10-second system purge using nitrogen flow through the bypass line
digitalWrite(RELAY_1,LOW);
digitalWrite(RELAY_2,HIGH);
delay(10000); // delay() is blocking, which is desired behaviour here
digitalWrite(RELAY_2,LOW);

// inform MATLAB that setup has been successfully completed
Serial.println("<Arduino is ready>");
}

void loop() {
  readSerialInput();
  processSerialInput();
  processData();
  }

void readSerialInput(){
  // initialise local variables
  static byte charIndex = 0;
  char endMarker = '\n'; // terminator set to "new line"
  char receivedChar;

// read data, if available
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
      newInputData = true; // flag that data has been received
    }
  }
}

void processSerialInput() {
    if (newInputData == true) { 
      // process input flow rate value
      serialFlowRate = 0;            
      serialFlowRate = atoi(receivedSerialInput); // convert character input to integer format

      // if data received is NOT the terminate command (integer 1001), set flow rate to the value requested. 
      if (serialFlowRate != 1001){
        setFlowRate();
         // Turn on S1 and ensure S2 is off - allows flow through the heat exchanger
        digitalWrite(RELAY_1, HIGH);
        digitalWrite(RELAY_2,LOW);
        Serial.println(serialFlowRate); // sends confirmation of set flow rate over serial to MATLAB
        newInputData = false; // reset flag
      }

      // if data received IS the terminate command (integer 1001), perform stop sequence to finish the experiment
      else { 
          // turn off both solenoids - temporary no flow state
          digitalWrite(RELAY_1,LOW); 
          digitalWrite(RELAY_2,LOW);
          // set up a millis-based timer
          static unsigned long terminateStartMillis = millis(); 
          unsigned long terminateCurrentMillis = millis();
            if ((terminateCurrentMillis - terminateStartMillis) >= 2000){ 
              digitalWrite(RELAY_2,LOW); // this can be set to high if faster warming is desired (flows warm N2 through the system)
              // flags to stop data being transmitted and reset newInputData flag
              stopFlag = true; 
              newInputData = false;
            }
          }
    }
}

void setFlowRate(){
  int adjustedFlowRate = serialFlowRate + 5; // adjust +5 to correct for error between desired setpoint and that displayed on the MFC screen (value of 5 gained from testing)
  // convert flow rate into integer PWM duty cycle value
  float dutyCycle = adjustedFlowRate * 1.023; 
  unsigned int dutyCycleInt = round(dutyCycle); 
  Timer1.pwm(PWM_PIN,dutyCycleInt); 
   flowRateFlag = true; // flag that flow rate has been set. 
}

void processData(){
    // only process data and send over serial under set conditions: no new serial input, flow rate has been set, not received stop command
    if (newInputData == false && flowRateFlag == true && stopFlag == false) { 
        
        // read flow rate
        int flowADC = analogRead(MFC_READOUT);
        float flowVoltageRead = flowADC * (5.0/1023); 
        float measuredFlowRate = flowVoltageRead * 200;
        int measuredFlowRateInt = round(measuredFlowRate); 

        // read temperature 
        int thermoADC = analogRead(TC_READOUT);
        float thermoVoltage = thermoADC * (5.06/1023); 

        // NB for the (5.06/1023) multipication factor, the numerator depends on Arduino power cable length. 
        // With long cable (Arduino cable + extender) 5.02V measured, but 5.06V needed to correct offset/error seen in temperature readings
        // With short cable (standard Arduino cable) 5.06V measured on Arduino pins but 5.12V needed to correct offset

        float temperature = (thermoVoltage - 1.25)/0.005;

        // convert temperature to int and calculate approximate moving average (used for holdTemperature function)
        int temperatureInt = round(temperature);
        int tempMovingAverage = tempMovMean.reading(temperatureInt);

        // run temperature holding function (comment/uncomment as desired)
        // holdTemperature(tempMovingAverage);

        // initialise startMillis timing variable
        unsigned long startMillis;

        // write data to serial
        int bytesAvailable = Serial.availableForWrite();
        // only send serial data if 100ms pause time has passed since last transmission (using printFlag)
        if ((bytesAvailable > 29) && (printFlag == false)){ // max of 28 digits for data package (eg. "1000,-110.43,-110,4294967295") + new line character

          // send values using a comma-separated string
          Serial.print(measuredFlowRateInt);
          Serial.print(',');
          Serial.print(temperature);
          Serial.print(',');
          Serial.print(tempMovingAverage);
          Serial.print(',');
          Serial.print(millis()); // to be used for time calculation in MATLAB
          Serial.println();
          printFlag = true;
          startMillis = millis();
        }
        // wait 100ms before sending the next data package
        if (printFlag == true) {
          unsigned long currentMillis = millis();
          if ((currentMillis - startMillis) >= 100){ 
            printFlag = false; // allow values to be sent over serial again
          }
        }
      }
  }

void holdTemperature(int tempMovingAverage){
  // turns off cold gas flow when temperature reaches specified trigger temperature, and raises flag
  if (minTempFlag == false && tempMovingAverage <= lowerTempBound){
    digitalWrite(RELAY_1,LOW);
    minTempFlag = true;
  }
  // regulates temperature between an upper and lower bound using solenoid control, up until a defined time period has passed
  else if (minTempFlag == true && maxTimeFlag == false){
    if (tempMovingAverage >= upperTempBound){
      digitalWrite(RELAY_1,HIGH); // turn on Solenoid 1 to allow gas flow through the heat exchanger
    }
    else if (tempMovingAverage <= -60){
      digitalWrite(RELAY_1,LOW); // turn off cold gas flow
    }
    static unsigned long holdStartMillis = millis();
    unsigned long holdCurrentMillis = millis();
    // ends temperature regulation function if the desired time period has elapsed
      if ((holdCurrentMillis - holdStartMillis) >= tempRegulationPeriod){
      digitalWrite(RELAY_1,LOW);
      digitalWrite(RELAY_2,HIGH); //starts warming the sample using the bypass nitrogen gas line
      maxTimeFlag = true;
    }
  }
}