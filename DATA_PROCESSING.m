%%%%%   This MATLAB programme is designed to process and visualise   %%%%%
%%%%%   data collected from the virtual moon temperature experiment. %%%%%
                                                                
%%%%% It is designed to be compatible with previous test data which  %%%%%
%%%%%    is stored in Excel files on GitHub. However, there are      %%%%%
%%%%%     differences between "Tests 1-5" and "Test 6 onwards".      %%%%%
%%%%% CERTAIN VARIABLES MUST BE CHANGED BEFORE EACH RUN, SEE BELOW   %%%%%

%%%%% Data collected by hand (e.g. Test 11) will not work with this  %%%%%
%%%%%   code, see "ROYCE_DATA_PROCESSING" code for this on GitHub    %%%%%


% clear variables, figures etc
clc;
clearvars;
close all;

%%%%%%%%%%% VARIABLES WHICH SHOULD BE ADJUSTED BEFORE EACH RUN %%%%%%%%%%

fileName = 'TEST_3.xlsx';
tempHolding = false; % change to true if temp holding used
earlyTest = true; % change to true if using data from Tests 1-5

% only required for temperature holding tests, define target temp range
tempHoldLow = -40; 
tempHoldHigh = -35; 

%%%%%%%%%%%%%%%%%%%%%%%%%%%%% MAIN CODE %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% extract data from Excel file. File must be located in same folder as 
% the matlab function, or the full path must be provided instead
testData = readmatrix(fileName, 'NumHeaderLines',0);
rawTemps = testData(:,2);

% define which column the time data is in (depends on whether it is pre or
% post Test 6)
if earlyTest == true
    timeColumn = 3;
else 
    timeColumn = 4;
end
time = testData(:,timeColumn); 
timeMins = time./60; % NB: uses hours by default, code must be edited to use mins
timeHours = time./3600; % default

% reconstruct raw data plot from experimental run
[rawTempPlot] = getRawDataPlot(timeHours,rawTemps);

% plot the 100-point moving mean graph (inc error bars)
[smoothedPlot,movingMean] = getSmoothedDataPlot(timeHours,rawTemps,tempHolding,tempHoldLow,tempHoldHigh);

% plot the moving mean on top of the raw data to ensure features not lost
[rawPlotWithMean] = getOverlaidPlot(timeHours,rawTemps,movingMean);


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% FUNCTIONS %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function [rawTempPlot] = getRawDataPlot(time,rawTemps)
% plot raw data
rawTempPlot = figure;
plot(time,rawTemps);
% add labels etc
xlabel("Time (hours)");
ylabel("Temperature (°C)");
xlim([0 max(time)]);
title("Temperature During Experiment");
end

function [smoothedDataPlot,movingMean] = getSmoothedDataPlot(time,rawTemps,tempHolding,tempHoldLow,tempHoldHigh)
% get 100-point moving mean and create error bars (1 standard deviation 
% above and below the mean)
[movingMean, upperBoundErr, lowerBoundErr] = smoothData(rawTemps);

% create boundary box for temperature holding target range
temperatureBoxX = [0 max(time) max(time) 0];
temperatureBoxY = [tempHoldLow tempHoldLow tempHoldHigh tempHoldHigh];

% create smoothed data plot with moving mean and error bounds
smoothedDataPlot = figure;
plot(time,movingMean, "Color", "m");
hold on
plot(time,upperBoundErr, "Color", "#808080");
hold on
plot(time,lowerBoundErr, "Color", "#808080");
% create "band" of target temperature range if temp holding function used
if tempHolding == true
    hold on
    fill(temperatureBoxX,temperatureBoxY, 'k', "FaceAlpha", 0.15, "EdgeColor", "none");
    hold off
    legend("mean temperature","+1σ","-1σ","holding temperature range");
else
    hold off
    legend("mean temperature","+1σ","-1σ");
end
% add labels etc
xlabel("Time (hours)");
ylabel("Temperature (°C)");
xlim([0 max(time)]);
title("Temperature During Experiment, 100-point moving mean");
end

function [movingMean, upperBoundErr, lowerBoundErr] = smoothData(rawTemps)
% calculate 100-point moving mean and standard deviation
movingMean = movmean(rawTemps,100);
movingStd = movstd(rawTemps,100);

% preallocate arrays for error bars to improve efficiency
upperBoundErr = zeros(length(rawTemps),1);
lowerBoundErr = zeros(length(rawTemps),1);

% create error bars (+/- 1 standard deviation from mean)
for i = 1:1:length(movingMean)
    upperBoundErr(i) = movingMean(i) + movingStd(i);
    lowerBoundErr(i) = movingMean(i) - movingStd(i);
end
end

function [rawPlotWithMean] = getOverlaidPlot(time,rawTemps,movingMean)
% plot raw data
rawPlotWithMean = figure;
plot(time,rawTemps);
hold on;
% plot moving mean
plot(time,movingMean, "Color", "y");
hold off
% add labels etc
xlabel("Time (hours)");
ylabel("Temperature (°C)");
xlim([0 max(time)]);
title("Temperature During Experiment");
legend("raw data","100-point moving mean");
end
