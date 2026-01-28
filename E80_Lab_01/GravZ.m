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
bar = mean(accelZ); %mean of all Y values
S = std(accelZ);    %standard deviation of Y values
N = length(accelZ)  %length of accelY 
df = N-1
ESE = S/sqrt(N)
t = 1.999
lambda = t * ESE

g = bar/1000 %converts milli-g into g
mg = 9.8 * g %converts into grav in m/s^2?





% Create the uncertainty line for the Y data
upperBound = bar + lambda
lowerBound = bar - lambda


%plot 3 accel datas

figure
plot(accelX, '.')
hold on
plot(accelY, '.')
plot(accelZ, '.')



% create mean and CL lines for zero in Z
yline(bar, 'b-', 'Mean Z')
yline(upperBound, 'r--', 'CI upper')
yline(lowerBound, 'r--', 'CI lower')


%label everything
ylabel('Teensy Units')
xlabel('Sample Number')
title('Lab Trial: Zero in Z')
legend('X data', 'Y data', 'Z data', 'Mean Z', 'CI bounds')








