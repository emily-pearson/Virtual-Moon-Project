% Clear previous COM port connections, Arduino objects, figures etc 
clc;
close all;
clear arduino;

% Set up serialport object for communication with Arduino
arduino = serialport("COM9",9600);

% clear existing data
flush(arduino);

% wait for Arduino to be ready to send/receive data
waitForArduino(arduino);

% ask user to input flow rate and convert to string to send over serial
flow_rate = num2str(inputFlowRate());

% set flow rate for mass flow controller
writeline(arduino, flow_rate);
feedback = readline(arduino);
disp(feedback);
clear arduino;

function waitForArduino(arduino)
configureTerminator(arduino,"CR/LF");
readiness_check = readline(arduino);
disp(readiness_check);
end

function [flow_rate] = inputFlowRate
prompt = "Enter flow rate (SCCM): ";
answer = input(prompt);
if answer < 0 || answer > 1000
    error("Flow rate must be between 0 and 1000");
else
    flow_rate = answer;
end
end