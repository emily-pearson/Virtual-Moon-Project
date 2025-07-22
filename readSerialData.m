% Reads serial data (mass flow and temperature) and stores in UserData

function readSerialData(src,~,maxReadings,livePlot)
% accept a comma-separated data string and split into separate values
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
src.UserData.Count = src.UserData.Count + 1;

% print temperature reading to the command window
disp(temperature);

% get start time
startTime = src.UserData.TimeData(1);
currentMillis = str2double(millis);
currentTime = (currentMillis - startTime)/1000;
currentTemp = str2double(temperature);

% create live plot of temperature
addpoints(livePlot,currentTime,currentTemp)
drawnow limitrate

% end data stream after specified amount of readings
if src.UserData.Count > maxReadings
    configureCallback(src,"off");
end  
end