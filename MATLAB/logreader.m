% logreader.m
% Use this script to read data from your micro SD card

clear;
%clf;

filenum = '043'; % file number for the data you want to read
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

%%Section 2.3 

% %define x and y acceleration in the appropriate units (mg -> m/s^2)
% ax = accelX*0.0098; %m/s^2
% ay = accelY*0.0098; 
% 
% %remove bias
% ax = ax - mean(ax(1:100)); %offset!
% ay = ay - mean(ay(1:100));
% 
% %create time vector
% LOOP_PERIOD = 99;
% dt = LOOP_PERIOD / 1000;
% t = (0:length(ax)-1)*dt; 
% 
% %integrate
% vx = cumtrapz(t, ax);
% vy = cumtrapz(t, ay);
% 
% x = cumtrapz(t, vx);
% y = cumtrapz(t, vy);
% 
% %ideal path
% idealx = [0 0.5 0];
% idealy = [0 0 0];
% 
% %uncertainty bounds
% sigma = std(ay(1:end)); %standard dev of Gaussian white noise
% confLev = 0.95;
% lambda = sqrt(2)*erfinv(confLev)*sigma;
% lambda2 = lambda*(2/3)*sqrt(dt)*t'.^(3/2); %z*sigma*f(t)
% 
% %establish upper and lower bounds
% uppery = y + lambda2;
% lowery = y - lambda2;
% 
% %plot 1
% figure;
% plot(x,y,'LineWidth',1.5) %actual
% hold on
% plot(idealx,idealy,'LineWidth',1.5) %ideal
% title('Measured and ideal path of board')
% xlabel('x (m)')
% ylabel('y (m)')
% legend('Measured path','Ideal path')
% 
% %plot 2
% figure;
% plot(t,y,'LineWidth',1.5)
% hold on
% plot(t,uppery,'LineWidth',1.5)
% plot(t, lowery,'LineWidth',1.5)
% xlabel('t (s)')
% ylabel('y (m)')
% title('Y position vs time with uncertainty bounds')
% legend('Estimated position', 'Upper bound', 'lower bound')

% 
% 

%%Section 3.3
% LOOP_PERIOD = 99; % need for time axis (method robot records time)
% 
% N = min([length(depth), length(depth_des), length(uV)]); %lenght of data is all the same so no errors occur
% dt = LOOP_PERIOD / 1000; %(change from ms to seconds)
% t = 0:dt:(N-1)*dt; % time based on loop period and length of data
% 
% depth = depth(1:N);
% depth_des = depth_des(1:N);
% uV = uV(1:N);
% 
% figure
% subplot(2,1,1)
% 
% plot(t, depth); 
% hold on;
% plot(t, depth_des);
% 
% xlabel('Time [s]');
% ylabel('Depth [m]');
% title('Depth and Desired Depth vs. Time');
% legend('depth', 'depth_des', 'Location', 'best');
% grid on;
% 
% subplot(2,1,2)
% plot(t, uV);
% xlabel('Time [s]');
% ylabel('uV');
% title('Vertical Motor Control Effort vs. Time [s]');
% grid on;
% 
% 
% N = min([length(clear), length(red), length(green), length(blue)]);
% clear = clear(1:N);
% red   = red(1:N);
% green = green(1:N);
% blue  = blue(1:N);


%%rgb sensor??
LOOP_PERIOD = 99;   
dt = LOOP_PERIOD / 1000;
t = 0:dt:(N-1)*dt;

figure;
plot(t, red, 'r');
hold on;
plot(t, green, 'g');
plot(t, blue, 'b');
plot(t, clear, 'k');
xlabel('Time [s]');
ylabel('Sensor Counts');
title('TCS34725 Raw Color Readings vs Time');
legend('Red','Green','Blue','Clear');
grid on;