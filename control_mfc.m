% Clear previous COM port connections, Arduino objects, figures etc 
clc;
clearvars;
close all;
clear arduino;

% confirm that the user has changed the file name of the excel file in the
% code (to prevent overwriting)
warningFigure = uifigure;
selection = uiconfirm(warningFigure,['Have you changed the name of the Excel'...
' results file? If not, previous files may be overwritten. Press OK to continue'...
' or CANCEL to stop the current operation'],'WARNING');
switch selection
    case 'Cancel'
        return
end
close(warningFigure);

% Set up serialport object for communication with Arduino
arduino = serialport("COM9",9600);
arduino.Timeout = 20; % allows time for purging of system before arduino is ready

% Set the terminator property to match Arduino code & clear existing data
configureTerminator(arduino,10);
flush(arduino);

% wait for Arduino to be ready to send/receive data
readinessCheck = waitForArduino(arduino);

% ask user to input flow rate and then send over serial
flowRate = inputFlowRate();
confirmedFlowRate = setFlowRate(arduino, flowRate);
disp("Flow rate set to " + confirmedFlowRate + " SCCM");

% Set up UserData to store arduino data and define max readings requested
arduino.UserData = struct("FlowData",[],"TempData",[],"AvgTempData",[],"TimeData",[],"Count",1);
maxReadings = 600;

% set up live plot
livePlot = animatedline;
xlabel("Time(s)");
ylabel("Temperature (C))");
maxX = maxReadings/10; % takes 10 readings per second
xlim([0 maxX]);
title("Live Temperature Plot");

% Configure callback to execute function when a new reading is available
configureCallback(arduino,"terminator", @(src, event) readSerialData(src,event,maxReadings,livePlot));

% force MATLAB to wait until all data has been collected over serial
while arduino.UserData.Count <= maxReadings
    pause(0.1); % don't change this valu0e or code doesn't like it (stop message may not be sent)
    if arduino.UserData.Count == maxReadings
        pause(0.1); % this needs to stay here even though it looks like a duplicate!
        writeline(arduino,'1001'); % 1001 cannot be entered as a flow rate as it throws error, so this is safe to use to stop Arduino
        disp("<finished>");
    end
end

% extract data from arduino object
flowData = arduino.UserData.FlowData;
tempData = arduino.UserData.TempData;
avgTempData = arduino.UserData.AvgTempData;
timeData = arduino.UserData.TimeData;

% process time data to reference from 0 and convert from ms to s
adjustedTimeData = processTimeData(timeData);

% reshape data into column vectors and store in an excel spreadsheet
flowDataVector = reshape(flowData,[maxReadings,1]);
tempDataVector = reshape(tempData,[maxReadings,1]);
avgTempDataVector = reshape(avgTempData,[maxReadings,1]);
timeDataVector = reshape(adjustedTimeData,[maxReadings,1]);

resultsFile = 'Throwaway.xlsx';
writematrix(flowDataVector,resultsFile,'Sheet',1,'Range','A1');
writematrix(tempDataVector,resultsFile,'Sheet',1,'Range','B1');
writematrix(avgTempDataVector,resultsFile,'Sheet',1,'Range','C1');
writematrix(timeDataVector,resultsFile,'Sheet',1,'Range','D1');

% plot graphs of flow rate and temperature
flowRatePlot = plotFlowRate(flowData,adjustedTimeData);
tempPlot = plotTemp(tempData,adjustedTimeData);

%clear arduino object to free up serial port
clear arduino;

function [readinessCheck] = waitForArduino(arduino)
configureTerminator(arduino,"CR/LF");
actualResponse = readline(arduino);
expectedResponse = "<Arduino is ready>";
startTime = tic;
% wait until ready message has been received from Arduino
while strcmp(actualResponse,expectedResponse) == 0
    pause(0.1);
    endTime = toc(startTime);
    % throw error if Arduino ready message not received within 5 secs
    if endTime > 10
        error("Not running on Arduino. Timed out");
    end
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
flowRatePlot = figure(2);
plot(timeData,flowData);
xlabel("Time(s)");
ylabel("Flow Rate (SCCM)");
xlim([0 max(timeData)]);
title("Flow Rate During Experiment");
end

function [tempPlot] = plotTemp(tempData,timeData)
tempPlot = figure(3);
plot(timeData,tempData);
xlabel("Time(s)");
ylabel("Temperature (C))");
xlim([0 max(timeData)]);
title("Temperature During Experiment");
end

function [timeArray] = processTimeData(timeData)
timeArray = zeros(1,length(timeData));
for i = 1:1:length(timeArray)
    timeArray(:,i) = (timeData(i) - timeData(1))/1000;
end
end