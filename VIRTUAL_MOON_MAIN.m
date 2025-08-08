%%%%% This MATLAB programme is designed to integrate with an Arduino %%%%%
%%%%% C++ programme to run the Virtual Moon experiment on a mock end %%%%%
%%%%%                           station.                             %%%%%
%%%%% It currently requires a 0 - 1000 SCCM mass flow controller     %%%%%
%%%%% (MFC) to be connected electrically. If an alternative MFC is   %%%%%
%%%%% used, certain aspects of the code should be updated. These     %%%%%
%%%%%               are detailed in code comments.                   %%%%%


% Clear previous COM port connections, Arduino objects, figures etc 
clc;
clearvars;
close all;
clear arduino;

% create Excel file to store results for future use
fileNamePrompt = "Enter file name for results file: ";
fileNameAnswer = input(fileNamePrompt, "s");
resultsFile = convertCharsToStrings(fileNameAnswer) + '.xlsx';

% warn user that if the filename is already in use, the data will be lost
warningFigure = uifigure;
selection = uiconfirm(warningFigure,['If the filename is the same as an'...
' existing results file, the existing data will be OVERWRITTEN. Press OK'...
' to continue or CANCEL to stop the current operation'],'WARNING');
if selection == "Cancel"
    return
end
close(warningFigure);

% set experiment runtime
runTimePrompt = "Enter experiment run time in minutes: ";
runTimeAnswer = input(runTimePrompt);

% calculate number of data readings based on runtime
numReadings = runTimeAnswer*60*10; % samples at 10 Hz so e.g. 1 hour = 36000 readings

% Set up serialport object for communication with Arduino
arduino = serialport("COM9",9600);
arduino.Timeout = 20; % 20 sec allows time for 10 sec N2 purge of gas feeds

% Set the terminator property to match Arduino code (linefeed)
configureTerminator(arduino,10);
flush(arduino);

% confirm Arduino is ready to send/receive data
readyConfirmation = waitForArduino(arduino);

% prompt user to set desired flow rate and communicate this to Arduino
flowRate = inputFlowRate();
confirmedFlowRate = setFlowRate(arduino, flowRate);

% Set up UserData property of arduino object for in-programme data storage
arduino.UserData = struct("FlowData",[],"TempData",[],"AvgTempData",[],"TimeData",[],"Count",1);

% set up live plot to monitor temperature in real time
livePlot = animatedline;
xlabel("Time(mins)");
ylabel("Temperature (°C)");
maxX = numReadings/10/60; % 10 readings per second, using mins as graph scale
xlim([0 maxX]);
title("Live Temperature Plot");

% Configure callback to execute "readSerialData" function when new datum is available
configureCallback(arduino,"terminator", @(src, event) readSerialData(src,event,numReadings,livePlot));

% force MATLAB to wait until all data has been collected over serial
while arduino.UserData.Count <= numReadings
    pause(0.1); % 100ms between data packets - must match Arduino
    if arduino.UserData.Count == numReadings
        pause(0.1);
        writeline(arduino,'1001'); % !! stop command "1001" - MUST BE CHANGED 
        % IN MATLAB AND ARDUINO IF AN MFC WITH FLOW RANGE > 1000 SCCM IS USED!!
        disp("<finished>");
    end
end

% extract data from UserData in the Arduino object
flowData = arduino.UserData.FlowData;
tempData = arduino.UserData.TempData;
avgTempData = arduino.UserData.AvgTempData; %% stores 20-point moving average used for temperature holding function
timeData = arduino.UserData.TimeData;

% process millis() time data to start counting from 0 and convert ms to s
processedTimeData = processTimeData(timeData);
processedTimeDataMins = processedTimeData./60; % convert again from s to mins

% reshape data into column vectors
flowDataVector = reshape(flowData,[numReadings,1]);
tempDataVector = reshape(tempData,[numReadings,1]);
avgTempDataVector = reshape(avgTempData,[numReadings,1]);
timeDataVector = reshape(processedTimeData,[numReadings,1]);

% store data in excel spreadsheet 
writematrix(flowDataVector,resultsFile,'Sheet',1,'Range','A1');
writematrix(tempDataVector,resultsFile,'Sheet',1,'Range','B1');
writematrix(avgTempDataVector,resultsFile,'Sheet',1,'Range','C1');
writematrix(timeDataVector,resultsFile,'Sheet',1,'Range','D1');

% plot graphs of flow rate and temperature
flowRatePlot = plotFlowRate(flowData,processedTimeDataMins);
tempPlot = plotTemp(tempData,processedTimeDataMins);

%clear arduino object to free up serial port
clear arduino;

%%%%%%%%%%%%%%%%%%%%%%%%%%%% FUNCTIONS %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function [readinessCheck] = waitForArduino(arduino)
configureTerminator(arduino,"CR/LF");
actualResponse = readline(arduino);
expectedResponse = "<Arduino is ready>";
startTime = tic;
% wait until expected "ready message" has been received
while strcmp(actualResponse,expectedResponse) == 0
    pause(0.1);
    endTime = toc(startTime);
    % throw error if "ready message" not received within 30 secs
    if endTime > 30
        error("Not running on Arduino. Timed out");
    end
end
readinessCheck = actualResponse;
disp(readinessCheck); % print ready message to command window
end


function [flowRate] = inputFlowRate
% prompt user to input the desired flow rate (0 - 1000 SCCM)
flowPrompt = "Enter flow rate (0 - 1000 SCCM): ";
flowAnswer = input(flowPrompt);
% throw error if flow rate is outside the specified range (MUST BE CHANGED
% IF A DIFFERENT MFC IS USED)
if flowAnswer < 0 || flowAnswer > 1000
    error("Flow rate must be between 0 and 1000");
else
    flowRate = num2str(flowAnswer);
end
end


function [confirmedFlowRate] = setFlowRate(arduino, flowRate)
% send desired flow rate to Arduino over serial
writeline(arduino, flowRate);
startTime = tic;
flowRateString = convertCharsToStrings(flowRate); % convert to string to allow for comparison with arduino feedback
flowRateFeedback = readline(arduino);
% wait until flow rate has been set
while strcmp(flowRateString, flowRateFeedback) == 0
    pause(0.1);
    endTime = toc(startTime);
    % throw error if flow rate confirmation not received within 5 secs
    if endTime > 5
        error("Flow rate not set. Timed out"); 
    end
end
confirmedFlowRate = flowRateFeedback;
disp("Flow rate set to " + confirmedFlowRate + " SCCM"); % print confirmation to command window
end


function readSerialData(src,~,numReadings,livePlot)
% split received comma-separated data package into separate strings
data = readline(src);
datasplit = split(data,',');
massFlow = datasplit(1,:);
temperature = datasplit(2,:);
movAvgTemperature = datasplit(3,:);
millis = datasplit(4,:);

% add data to the UserData property of the arduino object
src.UserData.FlowData(end+1) = str2double(massFlow);
src.UserData.TempData(end+1) = str2double(temperature);
src.UserData.AvgTempData(end+1) = str2double(movAvgTemperature);
src.UserData.TimeData(end+1) = str2double(millis);

% increment count to update total number of readings collected
src.UserData.Count = src.UserData.Count + 1;

% print temperature reading to the command window for monitoring
disp(temperature);

% get current time by processing millis() data from Arduino
startMillis = src.UserData.TimeData(1);
currentMillis = str2double(millis);
currentTime = (currentMillis - startMillis)/1000; % gets time since start in seconds
currentTimeMins = currentTime/60;

% convert current temperature from string to numerical value
currentTemp = str2double(temperature);

% update the live plot with the current temperature
addpoints(livePlot,currentTimeMins,currentTemp)
drawnow limitrate

% end data stream after specified time period has elapsed
if src.UserData.Count > numReadings
    configureCallback(src,"off");
end  
end


function [flowRatePlot] = plotFlowRate(flowData,timeData)
flowRatePlot = figure(2);
plot(timeData,flowData);
xlabel("Time(mins)");
ylabel("Flow Rate (SCCM)");
xlim([0 max(timeData)]);
title("Flow Rate During Experiment");
end


function [tempPlot] = plotTemp(tempData,timeData)
tempPlot = figure(3);
plot(timeData,tempData);
xlabel("Time(mins)");
ylabel("Temperature (C))");
xlim([0 max(timeData)]);
title("Temperature During Experiment");
end


function [timeArray] = processTimeData(timeData)
% initialise array to store the processed millis() time data
timeArray = zeros(1,length(timeData));
for i = 1:1:length(timeArray)
    % convert millis() values into time elapsed since start (in seconds)
    timeArray(:,i) = (timeData(i) - timeData(1))/1000;
end
end

