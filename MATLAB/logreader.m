% logreader.m
% Use this script to read data from your micro SD card

clear;
%clf;

filenum = '020'; % file number for the data you want to read
infofile = strcat('INF', filenum, '.TXT');
datafile = strcat('LOG', filenum, '.BIN');

%% map from datatype to length in bytes
dataSizes.('float') = 4;
dataSizes.('ulong') = 4;
dataSizes.('int') = 4;
dataSizes.('int32') = 4;
dataSizes.('uint8') = 1;
dataSizes.('uint16') = 2
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
LOOP_PERIOD = 99;
dt = LOOP_PERIOD / 1000;

%% DEPTH — already converted by ZStateEstimator, logged as 'z'
N = length(z);
t = (0:N-1) * dt;

figure;
plot(t, z, 'b', 'LineWidth', 1.5);
set(gca, 'YDir', 'reverse'); % flips it dowanrds, can change, just for intuition
xlabel('Time [s]');
ylabel('Depth [m]');
title('Depth vs Time');
grid on;

%% RGB
N_rgb = min([length(rgb_r), length(rgb_g), length(rgb_b), length(rgb_c)]);
rgb_r_data = double(rgb_r(1:N_rgb));
rgb_g_data = double(rgb_g(1:N_rgb));
rgb_b_data = double(rgb_b(1:N_rgb));
rgb_c_data = double(rgb_c(1:N_rgb));
t_rgb = (0:N_rgb-1) * dt;

% Raw counts vs time
figure;
plot(t_rgb, rgb_r_data, 'r', 'LineWidth', 1.5); hold on;
plot(t_rgb, rgb_g_data, 'g', 'LineWidth', 1.5);
plot(t_rgb, rgb_b_data, 'b', 'LineWidth', 1.5);
plot(t_rgb, rgb_c_data, 'k', 'LineWidth', 1.5);
xlabel('Time [s]');
ylabel('Sensor Counts');
title('RGB Raw Readings vs Time');
legend('Red','Green','Blue','Clear');
grid on;

% Attenuation vs depth — normalize to surface reference
n_ref = 10; % samples to average for surface reference
ref_r = mean(rgb_r_data(1:n_ref));
ref_g = mean(rgb_g_data(1:n_ref));
ref_b = mean(rgb_b_data(1:n_ref));

T_r = rgb_r_data / ref_r;
T_g = rgb_g_data / ref_g;
T_b = rgb_b_data / ref_b;

N_align = min(length(z), N_rgb);
figure;
plot(double(z(1:N_align)), T_r(1:N_align), 'r', 'LineWidth', 1.5); hold on;
plot(double(z(1:N_align)), T_g(1:N_align), 'g', 'LineWidth', 1.5);
plot(double(z(1:N_align)), T_b(1:N_align), 'b', 'LineWidth', 1.5);
xlabel('Depth [m]');
ylabel('Transmission Ratio');
title('Light Attenuation vs Depth');
legend('Red','Green','Blue');
grid on;

%% THERMISTOR — raw ADC only, conversion done here %%MAY NEED REWORKING%%
% Proposal values: Rn=10k, Rf=47k, Rp=15k, Rg=10k
Rn      = 10000;
Rf_t    = 47000;
gain_t  = Rf_t / Rn;
Rp_t    = 15000;
Rg_t    = 10000;
Vplus_t = (Rg_t / (Rp_t + Rg_t)) * 5.0;
R2      = 47000;
VDD     = 5.0;
THERM_B  = 4050 %murata data sheet, 25-50C
THERM_R0 = 47000;
THERM_T0 = 298.15;


%NEED 6 POINTS, DO AS WE DID IN LAB 4, but 
adc_therm = double(A01);
Vout_t    = (adc_therm / 1023.0) * 3.3;
Vin_t     = ((1 + gain_t) * Vplus_t - Vout_t) / gain_t;
RT        = R2 .* (VDD ./ Vin_t - 1.0);
invT      = (1.0 / THERM_T0) + (1.0 / THERM_B) .* log(RT / THERM_R0);
tempC     = (1.0 ./ invT) - 273.15;
tempC(tempC > 100) = NaN;
tempC(tempC < -10) = NaN;

N_t = length(tempC);
t_t = (0:N_t-1) * dt;

figure;
plot(t_t, tempC, 'r', 'LineWidth', 1.5);
xlabel('Time [s]');
ylabel('Temperature [°C]');
title('Thermistor: Temperature vs Time');
grid on;


%% TEMPERATURE VS DEPTH
N_temp_depth = min(length(z), length(tempC));
z_aligned    = double(z(1:N_temp_depth));
tempC_aligned = tempC(1:N_temp_depth);

figure;
plot(tempC_aligned, z_aligned, 'r', 'LineWidth', 1.5);
set(gca, 'YDir', 'reverse');  % deeper = down
xlabel('Temperature [°C]');
ylabel('Depth [m]');
title('Temperature vs Depth');
grid on;


