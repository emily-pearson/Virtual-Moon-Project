%%%%% This code is designed to process data collected by hand from %%%%%
%%%%%       the Royce end station in the near target room.         %%%%%

% clear variables, figures etc
clc;
clearvars;
close all;

% extract data from spreadsheet
testData = readmatrix('TEST_11.xlsx', 'NumHeaderLines',1);
tc1_temp = testData(:,2);
tc2_temp = testData(:,3);
timeMins = testData(:,1);

% convert minutes to hours
timeHours = timeMins./60;

% plot temperatures for each thermocouple
temperaturePlot = figure;
scatter(timeHours,tc1_temp, "x");
hold on
scatter(timeHours,tc2_temp, "x");
hold off
% add labels etc
xlabel("Time (hours)");
ylabel("Temperature (°C)");
legend("Thermocouple 1","Thermocouple 2");
title("Temperature during Experiment");