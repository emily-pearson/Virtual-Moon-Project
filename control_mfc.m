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
readinessCheck = waitForArduino(arduino);

% ask user to input flow rate and then send over serial
flowRate = inputFlowRate();
confirmedFlowRate = setFlowRate(arduino, flowRate);
disp("Flow rate set to " + confirmedFlowRate + " SCCM");

% Set up UserData to store arduino data
arduino.UserData = struct("FlowData",[],"TempData",[],"TimeData",[],"Count",1);

% Configure callback to execute function when a new reading is available
maxReadings = 50;
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
timeData = arduino.UserData.TimeData;

% plot graphs of flow rate and temperature
flowRatePlot = plotFlowRate(flowData,timeData);
tempPlot = plotTemp(tempData,timeData);

%clear arduino object to free up serial port
clear arduino;

function [readinessCheck] = waitForArduino(arduino)
configureTerminator(arduino,"CR/LF");
actualResponse = readline(arduino);
expectedResponse = "<Arduino is ready>";
% wait until ready message has been received from Arduino
while strcmp(actualResponse,expectedResponse) == 0
    pause(0.1);
end
readinessCheck = actualResponse;
disp(readinessCheck);
end

function [flowRate] = inputFlowRate
prompt = "Enter flow rate (SCCM): ";
answer = input(prompt);
if answer < 0 || answer > 1000
    error("Flow rate must be between 0 and 1000");
else
    flowRate = num2str(answer);
end
end

function [confirmedFlowRate] = setFlowRate(arduino, flowRate)
writeline(arduino, flowRate);
startTime = tic;
flowRateString = convertCharsToStrings(flowRate);
feedback = readline(arduino);
while strcmp(flowRateString, feedback) == 0
    pause(0.1);
    endTime = toc(startTime);
    % throw error if correct flow rate confirmation not received within 5 secs
    if endTime > 5
        error("Timed out"); 
    end
end
confirmedFlowRate = feedback;
end

function [flowRatePlot] = plotFlowRate(flowData,timeData)
flowRatePlot = figure;
timeArray = zeros(1,length(timeData));
for i = 1:1:length(timeArray)
    timeArray(:,i) = (timeData(i) - timeData(1))/1000;
    plot(timeArray,flowData);
    xlabel("Time(s)");
    ylabel("Flow Rate (SCCM)");
    title("Flow Rate During Experiment");
end
end

function [tempPlot] = plotTemp(tempData,timeData)
tempPlot = figure;
timeArray = zeros(1,length(timeData));
for i = 1:1:length(timeArray)
    timeArray(:,i) = (timeData(i) - timeData(1))/1000;
    plot(timeArray,tempData);
    xlabel("Time(s)");
    ylabel("Temperature (C))");
    title("Temperature During Experiment");
end
end