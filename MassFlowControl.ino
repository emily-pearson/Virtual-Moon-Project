// requires Uno R3 as TimerOne library is not supported on Uno R4 architecture
// serial comms code builds on example from Robin2 on Arduino.cc forums
// millis code builds on example from UKHelibob on Arduino.cc forums

#include <TimerOne.h>
#include <movingAvg.h>
#define PWM_PIN 9 // NB: only pins 9 or 10 are available for TimerOne-based PWM
#define MFC_READOUT A0
#define TC_READOUT A5
#define RELAY_1 4 // NB: relay pins are set by board architecture so cannot be changed
#define RELAY_2 7

// initialise global variables //

// flow rate input variables
const byte numInputChars = 5; // sets size of input flow rate (range of 0 - 1000, + new line character)
char receivedSerialInput[numInputChars]; // array to store received input string
int serialFlowRate = 0;

// timing variable - needs to be global?
unsigned long startMillis;

// boolean flags
boolean newInputData = false;
boolean flowRateFlag = false;
boolean stopFlag = false;
boolean printFlag = false;
boolean minTempFlag = false;
boolean maxTimeFlag = false;

// set up moving average object
movingAvg tempMovMean(20); //uses 20 datapoints for moving mean

void setup() {

Serial.begin(9600);
// set digital pins to output mode
pinMode(PWM_PIN,OUTPUT); 
pinMode(RELAY_1,OUTPUT);
pinMode(RELAY_2,OUTPUT);
// set PWM frequency
Timer1.initialize(100); // 100us = 10kHz frequency (10x the MFC sampling rate)

// initialise moving average object
tempMovMean.begin();

// flush room temperature nitrogen through the system for 10 sec 
digitalWrite(RELAY_1,LOW);
digitalWrite(RELAY_2,HIGH);
delay(10000);
digitalWrite(RELAY_2,LOW);

// tell MATLAB that Arduino is ready to communicate
Serial.println("<Arduino is ready>");
}

void loop() {
  readSerialInput();
  setSerialInput();
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

void setSerialInput() {
    if (newInputData == true) { 
      serialFlowRate = 0;            
      serialFlowRate = atoi(receivedSerialInput); // convert character input to integer format

      // if data received is NOT the terminate command (integer 1001), set flow rate to the value requested. Turn on S1 and ensure S2 is off.
      if (serialFlowRate != 1001){
        setFlowRate();
        digitalWrite(RELAY_1, HIGH);
        digitalWrite(RELAY_2,LOW);
        Serial.println(serialFlowRate); // for confirmation in MATLAB
        newInputData = false; 
      }

      // stop transmission if terminate command (integer 1001) is sent from MATLAB. Turn off S1, wait 2 seconds, then turn on S2
      else { 
          digitalWrite(RELAY_1,LOW); 
          static unsigned long terminateStartMillis = millis(); // static to prevent overwriting each loop call
          unsigned long terminateCurrentMillis = millis();
            if ((terminateCurrentMillis - terminateStartMillis) >= 2000){ // acts like a delay(2000) but is non-blocking
              digitalWrite(RELAY_2,LOW); //
              stopFlag = true; // flags to stop data being transmitted
              newInputData = false;
            }
          }
    }
}

void setFlowRate(){
  int adjustedFlowRate = serialFlowRate + 5; //adjust +5 for error correction (error between chosen setpoint and that displayed on the MFC screen)
  float dutyCycle = adjustedFlowRate * 1.023; // convert input flow rate to duty cycle value for PWM
  unsigned int dutyCycleInt = round(dutyCycle); // round result to nearest integer 
  Timer1.pwm(PWM_PIN,dutyCycleInt); 
   flowRateFlag = true; // flag that flow rate has been set. 
}

void readData(){
    if (newInputData == false && flowRateFlag == true && stopFlag == false) { // only write to serial under set conditions (no new serial input, already sent confirmation of flow rate, not received stop command)
        
        // read flow rate
        int flowADC = analogRead(MFC_READOUT);
        float flowVoltageRead = flowADC * (5.0/1023); // change to 5.06V??
        float measuredFlowRate = flowVoltageRead * 200;
        int measuredFlowRateInt = round(measuredFlowRate); // round to nearest integer flow rate

        // read temperature 
        int thermoADC = analogRead(TC_READOUT);
        float thermoVoltage = thermoADC * (5.06/1023); // numerator dependent on Arduino power cable length. Long cable - measured 5.02V but use 5.06V to error correct. Short cable - measured 5.06V, use 5.12V as error corrector
        float temperature = (thermoVoltage - 1.25)/0.005;

        // convert temperature to int and calculate approximate moving average
        int temperatureInt = round(temperature);
        int tempMovingAverage = tempMovMean.reading(temperatureInt);

        // check if temperature has reached min threshold to hold at temp
        holdTemperature(tempMovingAverage);

        // check there is enough buffer space available to write to serial
        int bytesAvailable = Serial.availableForWrite();
        if ((bytesAvailable > 22) && (printFlag == false)){ // max of 21 digits for data package (eg. "1000,24.43,4294967295")

          // send values over serial using a comma-separated string
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
        // if a value has been sent over serial, wait 100ms before sending the next one
        if (printFlag == true) {
          unsigned long currentMillis = millis();
          if ((currentMillis - startMillis) >= 100){ // check if 100ms has elapsed
            printFlag = false; // allow values to be sent over serial again
          }
        }
      }
  }

void holdTemperature(int tempMovingAverage){
  // turns off gas flow when temperature reaches -40C, and raises flag
  if (minTempFlag == false && tempMovingAverage <= -40){
    digitalWrite(RELAY_1,LOW);
    minTempFlag = true;
  }
  // regulates temperature between -30 and -40C using solenoid control, up until a defined time period has passed
  else if (minTempFlag == true && maxTimeFlag == false){
    if (tempMovingAverage >= -30){
      digitalWrite(RELAY_1,HIGH);
    }
    else if (tempMovingAverage <= -40){
      digitalWrite(RELAY_1,LOW);
    }
    static unsigned long holdStartMillis = millis();
    unsigned long holdCurrentMillis = millis();
    else if ((holdCurrentMillis - holdStartMillis) >= 3600000){
      // ends temperature hold if time period has elapsed, starts warming the sample using S2 line
      digitalWrite(RELAY_1,LOW);
      digitalWrite(RELAY_2,HIGH);
      maxTimeFlag = true;
    }
  }
}
