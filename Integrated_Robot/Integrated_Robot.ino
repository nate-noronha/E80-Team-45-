/********
Integrated Robot
********/

#include <Arduino.h>
#include <Wire.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include <Pinouts.h>
#include <TimingOffsets.h>
#include <SensorGPS.h>
#include <SensorIMU.h>
#include <XYStateEstimator.h>
#include <ADCSampler.h>
#include <ErrorFlagSampler.h>
#include <ButtonSampler.h> // A template of a data source library
#include <MotorDriver.h>
#include <Logger.h>
#include <Printer.h>
#include <SurfaceControl.h>
#define UartSerial Serial1
#define DELAY 0
#include <GPSLockLED.h>
#include <BurstADCSampler.h>
#include "DFRobot_TCS34725.h"

#include <ZStateEstimator.h>
#include <DepthControl.h> 

#include <SensorRGB.h>


/////////////////////////* Global Variables *////////////////////////

MotorDriver motor_driver;
XYStateEstimator xy_state_estimator;
ZStateEstimator z_state_estimator;      
DepthControl depth_control; // CHANGED: replaced surface_control
SensorGPS gps;
Adafruit_GPS GPS(&UartSerial);
ADCSampler adc;
ErrorFlagSampler ef;
ButtonSampler button_sampler;
SensorIMU imu;
Logger logger;
Printer printer;
GPSLockLED led;
BurstADCSampler burst_adc;
SensorRGB rgb_sensor; 


// COMMENT OUT WHEN DIVING
#define THERMISTOR_TESTING   // <-- comment this line out for actual dive
// Add these constants at the top of your .ino with your globals
#ifdef THERMISTOR_TESTING
  const float THERM_B   = 4050.0; // B-constant from Murata datasheet
  const float THERM_R0  = 47000.0;  // 47kΩ at reference temp
  const float THERM_T0  = 298.15; // 25°C in Kelvin
  const float THERM_R2  = 47000.0; // your voltage divider R2 from schematic
  const float VDD       = 5.0;  // supply voltage
  const float ADC_MAX   = 1023.0;
#endif

DFRobot_TCS34725 tcs = DFRobot_TCS34725(
  &Wire,
  TCS34725_ADDRESS,
  TCS34725_INTEGRATIONTIME_24MS,
  TCS34725_GAIN_1X
);

// loop start recorder
int loopStartTime;
int currentTime;
int current_way_point = 0;
volatile bool EF_States[NUM_FLAGS] = {1,1,1};

// GPS Waypoints NOT NEEDED BC DIVING
// const int number_of_waypoints = 2;
// const int waypoint_dimensions = 2;
// double waypoints[] = { 0, 10, 0, 0 };   // x0,y0,x1,y1, ... etc.

// // RGB timing NO LONGER NEEDED, located now IN SENSORRGB.H, period integrated and time
// int lastRGBTime = 0;
// const int RGB_PERIOD = 200;   // ms

// RGB printer string
String rgbState = "RGB: waiting...";

////////////////////////* Setup *////////////////////////////////

void setup() {

  Serial.begin(115200);
  Wire.begin();

  logger.include(&imu);
  logger.include(&gps);
  logger.include(&xy_state_estimator);
  logger.include(&z_state_estimator);
  //  logger.include(&surface_control);
  logger.include(&motor_driver);
  logger.include(&adc);
  logger.include(&ef);
  logger.include(&button_sampler);
  logger.include(&rgb_sensor);
  logger.init();

  burst_adc.init();

  printer.init();
  ef.init();
  button_sampler.init();
  imu.init();
  UartSerial.begin(9600);
  gps.init(&GPS);
  motor_driver.init();
  led.init();

  //Depth Waypoints (3 different depths 2m,4m,6m, holds 5s at each, once finished at 6m, comes back up without stoping) 
  int diveDelay = 5000;                          // ms to hold still at each waypoint
  const int num_depth_waypoints = 3;
  double depth_waypoints[] = { 2.0, 4.0, 6.0 }; // meters — change to 3.0/6.0 if needed
  depth_control.init(num_depth_waypoints, depth_waypoints, diveDelay);



  // surface_control.init(number_of_waypoints, waypoints, DELAY); //maybe delete
  xy_state_estimator.init();
  z_state_estimator.init();

  //rgb
bool rgbFound = false;
for (int i = 0; i < 5; i++) {
  if (tcs.begin() == 0) {
    rgbFound = true;
    rgb_sensor.init(&tcs);    // sets isReady = true inside
    Serial.println("RGB sensor found!");
    break;
  }
  Serial.println("No TCS34725 found, retrying...");
  delay(500);
}
if (!rgbFound) {
  Serial.println("RGB not found — continuing without it");
  rgbState = "RGB: not connected";
}
  //   while (tcs.begin() != 0) {
  //   Serial.println("No TCS34725 found ... check your connections");
  //   delay(1000);
  // }
  // rgb_sensor.init(&tcs);
  //start times


  // 30 seconds = 30000ms, adjust as needed
  //Serial.println("Starting in 30 seconds — close and seal the box!");
  //delay(30000);

  printer.printMessage("Starting main loop",10);
  loopStartTime = millis();
  printer.lastExecutionTime         = loopStartTime - LOOP_PERIOD + PRINTER_LOOP_OFFSET;
  imu.lastExecutionTime             = loopStartTime - LOOP_PERIOD + IMU_LOOP_OFFSET;
  gps.lastExecutionTime             = loopStartTime - LOOP_PERIOD + GPS_LOOP_OFFSET;
  adc.lastExecutionTime             = loopStartTime - LOOP_PERIOD + ADC_LOOP_OFFSET;
  ef.lastExecutionTime              = loopStartTime - LOOP_PERIOD + ERROR_FLAG_LOOP_OFFSET;
  button_sampler.lastExecutionTime  = loopStartTime - LOOP_PERIOD + BUTTON_LOOP_OFFSET;
  xy_state_estimator.lastExecutionTime = loopStartTime - LOOP_PERIOD + XY_STATE_ESTIMATOR_LOOP_OFFSET;
  //surface_control.lastExecutionTime = loopStartTime - LOOP_PERIOD + SURFACE_CONTROL_LOOP_OFFSET;
  logger.lastExecutionTime          = loopStartTime - LOOP_PERIOD + LOGGER_LOOP_OFFSET;
  burst_adc.lastExecutionTime       = loopStartTime;
  
  z_state_estimator.lastExecutionTime  = loopStartTime - LOOP_PERIOD + Z_STATE_ESTIMATOR_LOOP_OFFSET;
  depth_control.lastExecutionTime      = loopStartTime - LOOP_PERIOD + DEPTH_CONTROL_LOOP_OFFSET;
  rgb_sensor.lastExecutionTime         = loopStartTime;
  
  led.lastExecutionTime             = loopStartTime;
}
// Temp conversion COMMENT OUT WHEN DIVE< ONLY FOR TESTING
#ifdef THERMISTOR_TESTING
String thermState = "Therm: waiting...";
float adcToTemperature(int adcCount) {
  // Step 1: convert ADC count back to voltage at Teensy pin
  float v_out = (adcCount / ADC_MAX) * 3.3;
  // Step 2: undo the op-amp gain to get Vin (voltage at divider)
  // From your schematic: Vout = (1 + Rf/Rn)*V+ - (Rf/Rn)*Vin
  // Rf=47k, Rn=10k, Rp=15k, Rg=10k → gain = 4.7148, V+ = (Rg/(Rp+Rg))*5V
  float gain  = 47000.0 / 10000.0;
  float Vplus = (10000.0 / (15000.0 + 10000.0)) * VDD;
  float v_in  = ((1 + gain) * Vplus - v_out) / gain;
  // Step 3: recover thermistor resistance from voltage divider
  // Vin = (R2 / (RT + R2)) * VDD  →  RT = R2*(VDD/Vin - 1)
  float RT    = THERM_R2 * (VDD / v_in - 1.0);
  // Step 4: B-constant equation → temperature in Kelvin
  float invT  = (1.0 / THERM_T0) + (1.0 / THERM_B) * log(RT / THERM_R0);
  return (1.0 / invT) - 273.15;
}
#endif




//////////////////////////////* Loop */////////////////////////

void loop() {
  currentTime = millis();

  if ( currentTime - printer.lastExecutionTime > LOOP_PERIOD ) {
    printer.lastExecutionTime = currentTime;
    printer.printValue(0,adc.printSample());
    printer.printValue(1,ef.printStates());
    printer.printValue(2,logger.printState());
    printer.printValue(3,gps.printState());
    printer.printValue(4,xy_state_estimator.printState());
    printer.printValue(5, z_state_estimator.printState());
    printer.printValue(6, depth_control.printWaypointUpdate());
    printer.printValue(7, depth_control.printString());
    //printer.printValue(5,surface_control.printWaypointUpdate());
    //printer.printValue(6,surface_control.printString());
    printer.printValue(8,motor_driver.printState());
    printer.printValue(9,imu.printRollPitchHeading());
    printer.printValue(10,imu.printAccels());
    printer.printValue(11,rgbState);
    // Add thermistor to printer for test:
    #ifdef THERMISTOR_TESTING
      printer.printValue(12, thermState);
    #endif
    printer.printToSerial();  // To stop printing, just comment this line out
  }
//Surface Stuff?
  // if ( currentTime - surface_control.lastExecutionTime > LOOP_PERIOD ) {
  //   surface_control.lastExecutionTime = currentTime;
  //   surface_control.navigate(&xy_state_estimator.state, &gps.state, DELAY);
  //   motor_driver.drive(surface_control.uL, surface_control.uR, 0);
  // }

  // DEPTH CONTROL (from lab 7 dive code), moight need to look back on it again
  if (currentTime - depth_control.lastExecutionTime > LOOP_PERIOD) {
    depth_control.lastExecutionTime = currentTime;
    if (depth_control.diveState) {
      depth_control.complete = false;
      if (!depth_control.atDepth) {
        depth_control.dive(&z_state_estimator.state, currentTime);
      } else {
        depth_control.diveState = false;
        depth_control.surfaceState = true;
      }
      motor_driver.drive(0, -depth_control.uV, -depth_control.uV); // A,B vertical, c is off
    }
    if (depth_control.surfaceState) {
      if (!depth_control.atSurface) {
        depth_control.surface(&z_state_estimator.state);
      } else if (depth_control.complete) {
        delete[] depth_control.wayPoints;
      }
      motor_driver.drive(0, depth_control.uV, depth_control.uV);
    }
  }



//ADC
  if ( currentTime - adc.lastExecutionTime > LOOP_PERIOD ) {
    adc.lastExecutionTime = currentTime;
    adc.updateSample();

//Thermistor in ADC loop
    #ifdef THERMISTOR_TESTING
      int thermRaw = adc.sample[2];
      float tempC  = adcToTemperature(thermRaw);
      thermState   = "Temp: " + String(tempC, 2) + " C (raw:" + String(thermRaw) + ")";
    #endif
  }

//Errors
  if ( currentTime - ef.lastExecutionTime > LOOP_PERIOD ) {
    ef.lastExecutionTime = currentTime;
    attachInterrupt(digitalPinToInterrupt(ERROR_FLAG_A), EFA_Detected, LOW);
    attachInterrupt(digitalPinToInterrupt(ERROR_FLAG_B), EFB_Detected, LOW);
    attachInterrupt(digitalPinToInterrupt(ERROR_FLAG_C), EFC_Detected, LOW);
    delay(5);
    detachInterrupt(digitalPinToInterrupt(ERROR_FLAG_A));
    detachInterrupt(digitalPinToInterrupt(ERROR_FLAG_B));
    detachInterrupt(digitalPinToInterrupt(ERROR_FLAG_C));
    ef.updateStates(EF_States[0],EF_States[1],EF_States[2]);
    EF_States[0] = 1;
    EF_States[1] = 1;
    EF_States[2] = 1;
  }

  //Button
  if ( currentTime - button_sampler.lastExecutionTime > LOOP_PERIOD ) {
    button_sampler.lastExecutionTime = currentTime;
    button_sampler.updateState();
  }
//IMU
  if ( currentTime - imu.lastExecutionTime > LOOP_PERIOD ) {
    imu.lastExecutionTime = currentTime;
    imu.read();     // blocking I2C calls
  }
//GPS
  gps.read(&GPS); // blocking UART calls, need to check for UART every cycle

  //XY
  if ( currentTime - xy_state_estimator.lastExecutionTime > LOOP_PERIOD ) {
    xy_state_estimator.lastExecutionTime = currentTime;
    xy_state_estimator.updateState(&imu.state, &gps.state);
  }

  //Z
  if (currentTime - z_state_estimator.lastExecutionTime > LOOP_PERIOD) {
    z_state_estimator.lastExecutionTime = currentTime;
    z_state_estimator.updateState(analogRead(PRESSURE_PIN));
  }  
  
//LED
  if ( currentTime - led.lastExecutionTime > LOOP_PERIOD ) {
    led.lastExecutionTime = currentTime;
    led.flashLED(&gps.state);
  }

  //LOGGER
  if ( currentTime - logger.lastExecutionTime > LOOP_PERIOD && logger.keepLogging ) {
    logger.lastExecutionTime = currentTime;
    logger.log();
  }

  // RGB UPDATE
  if (currentTime - rgb_sensor.lastExecutionTime > LOOP_PERIOD) {
    rgb_sensor.lastExecutionTime = currentTime;
    rgb_sensor.read();   // updates rgb_sensor.r/g/b/c, logger will pick these up

    uint32_t sum = rgb_sensor.c > 0 ? rgb_sensor.c : 1; // avoid divide by zero
    float r = (rgb_sensor.r / (float)sum) * 256;
    float g = (rgb_sensor.g / (float)sum) * 256;
    float b = (rgb_sensor.b / (float)sum) * 256;
    rgbState = "RGB C:" + String(rgb_sensor.c) +
               " R:" + String(rgb_sensor.r) +
               " G:" + String(rgb_sensor.g) +
               " B:" + String(rgb_sensor.b) +
               " HEX:" + String((int)r, HEX) +
                         String((int)g, HEX) +
                         String((int)b, HEX);
  }
  
}


void EFA_Detected(void){
  EF_States[0] = 0;
}
void EFB_Detected(void){
  EF_States[1] = 0;
}
void EFC_Detected(void){
  EF_States[2] = 0;
}