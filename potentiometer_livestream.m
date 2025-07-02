% Clear previous COM port connections, Arduino objects, figures etc 
clc;
close all;
clear arduino;

% Set up serialport object to establish connection to Arduino
arduino = serialport("COM9",9600);

% Set the terminator property to match Arduino code & clear existing data
configureTerminator(arduino,13,10);
flush(arduino);

% Set up UserData to store arduino data
arduino.UserData = struct("VoltageData",[],"ConverterData", [],"Count",1);

% Configure callback to execute function when a new reading is available
maxReadings = 100;
configureCallback(arduino,"terminator", @(src, event) readVoltage(src,event,maxReadings));

% force MATLAB to wait until all data has been collected over serial
while arduino.UserData.Count < maxReadings
pause(0.1);
end

% extract data from arduino object
voltageData = arduino.UserData.VoltageData;
converterData = arduino.UserData.ConverterData;
