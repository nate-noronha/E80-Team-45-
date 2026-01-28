% logreader.m
% Use this script to read data from your micro SD card

clear;
%clf;

filenum = '004'; % file number for the data you want to read
infofile = strcat('INF', filenum, '.TXT');
datafile = strcat('LOG', filenum, '.BIN');

%% map from datatype to length in bytes
dataSizes.('float') = 4;
dataSizes.('ulong') = 4;
dataSizes.('int') = 4;
dataSizes.('int32') = 4;
dataSizes.('uint8') = 1;
dataSizes.('uint16') = 2;
dataSizes.('char') = 1;
dataSizes.('bool') = 1;

%% read from info file to get log file structure
fileID = fopen(infofile);
items = textscan(fileID,'%s','Delimiter',',','EndOfLine','\r\n');
fclose(fileID);
[ncols,~] = size(items{1});
ncols = ncols/2;
varNames = items{1}(1:ncols)';
varTypes = items{1}(ncols+1:end)';
varLengths = zeros(size(varTypes));
colLength = 256;
for i = 1:numel(varTypes)
    varLengths(i) = dataSizes.(varTypes{i});
end
R = cell(1,numel(varNames));

%% read column-by-column from datafile
fid = fopen(datafile,'rb');
for i=1:numel(varTypes)
    %# seek to the first field of the first record
    fseek(fid, sum(varLengths(1:i-1)), 'bof');
    
    %# % read column with specified format, skipping required number of bytes
    R{i} = fread(fid, Inf, ['*' varTypes{i}], colLength-varLengths(i));
    eval(strcat(varNames{i},'=','R{',num2str(i),'};'));
end
fclose(fid);

%% Process your data here
% plot(accelX)
% title('Acceleration in Teensy Units')
% xlabel('Sample #')
% ylabel('Teensy Units')
% hold on;
% plot(accelY)
% hold on;
% plot(accelZ)
% hold off;
% legend('Acceleration X-direction','Acceleration Y-direction', 'Acceleration Z-direction');
% x = accelX
% y = accelY 

% compute mean, standard deviation, standard error, and 95% confidence
% bounds for static accel in the z direction


f1 = figure;
f2 = figure;

subplot(3,1,1)
%put each Z plot, X plot, and Y plot into its own plot in the subplots
subplot(3,1,1)
plot(accelX);
title('Zero in X-direction');
xlabel('Sample #');
ylabel('Teensy Units');
legend('Acceleration X-direction')
hold on;
subplot(3,1,3)
plot(accelZ);
title('Acceleration in Z-direction');
xlabel('Sample #');
ylabel('Teensy Units');
legend('Acceleration Z-direction')
hold on;
subplot(3,1,2)
plot(accelY);
title('Acceleration in Y-direction');
xlabel('Sample #');
ylabel('Teensy Units');
legend('Acceleration Y-direction')
hold off;








