#define SENSOR_PIN A0 //NB don't need to set pinMode for an analogue pin

// for reference the onboard ADC has a 10-bit resolution (1024 discrete levels)

void setup() {
  // analogReadResolution(RESOLUTION);
  Serial.begin(9600);
}

void loop() {
  // read ADC value and convert to analog voltage
  int ADC_value = analogRead(SENSOR_PIN);
  float analog_voltage = ADC_value * (5.0/1023);

 // check there is enough buffer space available to write to serial
  int bytesAvailable = Serial.availableForWrite();
  if (bytesAvailable > 10) { // largest string is "5.00,1023" which is 9 bytes

   // send values to serial using a comma-separated string
  Serial.print(analog_voltage);
  Serial.print(',');
  Serial.print(ADC_value);
  Serial.println();
  }
  // delay 
  delay(50);
}
