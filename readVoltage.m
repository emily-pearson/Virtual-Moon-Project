% Reads ADC values, stores in UserData and creates a simple plot

function readVoltage(src,~,maxReadings)
% accept a comma-separated data string and split into separate values
data = readline(src);
datasplit = split(data,',');
voltage = datasplit(1,:);
ADC_value = datasplit(2,:);
% add data to the UserData property of the arduino object
src.UserData.VoltageData(end+1) = str2double(voltage);
src.UserData.ConverterData(end+1) = str2double(ADC_value);
src.UserData.Count = src.UserData.Count + 1;

% end data stream after specified amount of readings
if src.UserData.Count > maxReadings
    configureCallback(src,"off");
    
    % plot voltage graph
    figure(1)
    plot(src.UserData.VoltageData);
    xlim([0 maxReadings]);
    ylim([0 5]); % Arduino voltages from 0-5V
    xlabel("Readings");
    ylabel("Analog Voltage");
    title("Potentiometer analog voltage over " + maxReadings + " readings");
    
    % plot ADC value graph
    figure(2)
    plot(src.UserData.ConverterData(1:end));
    xlim([0 maxReadings]);
    ylim([0 1023]); % ADC levels from 0 to 1023
    xlabel("Readings");
    ylabel("ADC Value");
    title("Potentiometer ADC values over " + maxReadings + " readings");
end
end

