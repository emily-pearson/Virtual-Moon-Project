% Clear previous COM port connections, Arduino objects, figures etc 
clc;
clearvars;
close all;
clear arduino;

% Set up serialport object for communication with Arduino
arduino = serialport("COM9",9600);

% Set the terminator property to match Arduino code & clear existing data
configureTerminator(arduino,10);
flush(arduino);

% wait for Arduino to be ready to send/receive data
readiness_check = waitForArduino(arduino);

% ask user to input flow rate and then send over serial
flow_rate = inputFlowRate();
confirmed_flow_rate = setFlowRate(arduino, flow_rate);
disp("Flow rate set to " + confirmed_flow_rate + " SCCM");

% Set up UserData to store arduino data
arduino.UserData = struct("FlowData",[],"TempData",[],"Count",1);

% Configure callback to execute function when a new reading is available
maxReadings = 100;
configureCallback(arduino,"terminator", @(src, event) readSerialData(src,event,maxReadings));

% force MATLAB to wait until all data has been collected over serial
while arduino.UserData.Count <= maxReadings
    pause(0.1); % don't change this value or code doesn't like it (stop message may not be sent)
    if arduino.UserData.Count == maxReadings
        pause(0.1); % this needs to stay here even though it looks like a duplicate!
        writeline(arduino,'1001'); % 1001 cannot be entered as a flow rate as it throws error, so this is safe to use to stop Arduino
        disp("<finished>");
    end
end

% extract data from arduino object
flowData = arduino.UserData.FlowData;
tempData = arduino.UserData.TempData;

%clear arduino object to free up serial port
clear arduino;

function [readiness_check] = waitForArduino(arduino)
configureTerminator(arduino,"CR/LF");
actual_response = readline(arduino);
expected_response = "<Arduino is ready>";
% wait until ready message has been received from Arduino
while strcmp(actual_response,expected_response) == 0
    pause(0.1);
end
readiness_check = actual_response;
disp(readiness_check);
end

function [flow_rate] = inputFlowRate
prompt = "Enter flow rate (SCCM): ";
answer = input(prompt);
if answer < 0 || answer > 1000
    error("Flow rate must be between 0 and 1000");
else
    flow_rate = num2str(answer);
end
end

function [confirmed_flow_rate] = setFlowRate(arduino, flow_rate)
writeline(arduino, flow_rate);
start_time = tic;
flow_rate_string = convertCharsToStrings(flow_rate);
feedback = readline(arduino);
while strcmp(flow_rate_string, feedback) == 0
    pause(0.1);
    end_time = toc(start_time);
    % throw error if correct flow rate confirmation not received within 5 secs
    if end_time > 5
        error("Timed out"); 
    end
end
confirmed_flow_rate = feedback;
end