% Reads serial data (mass flow and temperature) and stores in UserData

function readSerialData(src,~,maxReadings)
% accept a comma-separated data string and split into separate values
data = readline(src);
datasplit = split(data,',');
mass_flow = datasplit(1,:);
temperature = datasplit(2,:);
% add data to the UserData property of the arduino object
src.UserData.FlowData(end+1) = str2double(mass_flow);
src.UserData.TempData(end+1) = str2double(temperature);
src.UserData.Count = src.UserData.Count + 1;

% end data stream after specified amount of readings
if src.UserData.Count > maxReadings
    configureCallback(src,"off");
end  
end