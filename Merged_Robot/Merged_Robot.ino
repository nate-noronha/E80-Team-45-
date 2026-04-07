/********
Default E80 Code + RGB Sensor
Fix: print RGB through Printer so it actually shows up
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

/////////////////////////* Global Variables *////////////////////////

MotorDriver motor_driver;
XYStateEstimator state_estimator;
SurfaceControl surface_control;
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

// GPS Waypoints
const int number_of_waypoints = 2;
const int waypoint_dimensions = 2;
double waypoints[] = { 0, 10, 0, 0 };   // x0,y0,x1,y1, ... etc.

// RGB timing
int lastRGBTime = 0;
const int RGB_PERIOD = 200;   // ms

// RGB printer string
String rgbState = "RGB: waiting...";

////////////////////////* Setup *////////////////////////////////

void setup() {

  Serial.begin(115200);
  Wire.begin();

  logger.include(&imu);
  logger.include(&gps);
  logger.include(&state_estimator);
  logger.include(&surface_control);
  logger.include(&motor_driver);
  logger.include(&adc);
  logger.include(&ef);
  logger.include(&button_sampler);
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

  surface_control.init(number_of_waypoints, waypoints, DELAY);
  state_estimator.init();

  printer.printMessage("Starting main loop",10);
  loopStartTime = millis();
  printer.lastExecutionTime         = loopStartTime - LOOP_PERIOD + PRINTER_LOOP_OFFSET;
  imu.lastExecutionTime             = loopStartTime - LOOP_PERIOD + IMU_LOOP_OFFSET;
  gps.lastExecutionTime             = loopStartTime - LOOP_PERIOD + GPS_LOOP_OFFSET;
  adc.lastExecutionTime             = loopStartTime - LOOP_PERIOD + ADC_LOOP_OFFSET;
  ef.lastExecutionTime              = loopStartTime - LOOP_PERIOD + ERROR_FLAG_LOOP_OFFSET;
  button_sampler.lastExecutionTime  = loopStartTime - LOOP_PERIOD + BUTTON_LOOP_OFFSET;
  state_estimator.lastExecutionTime = loopStartTime - LOOP_PERIOD + XY_STATE_ESTIMATOR_LOOP_OFFSET;
  surface_control.lastExecutionTime = loopStartTime - LOOP_PERIOD + SURFACE_CONTROL_LOOP_OFFSET;
  logger.lastExecutionTime          = loopStartTime - LOOP_PERIOD + LOGGER_LOOP_OFFSET;
  burst_adc.lastExecutionTime       = loopStartTime;
  led.lastExecutionTime             = loopStartTime;

  Serial.println("Color View Test!");

  while (tcs.begin() != 0) {
    Serial.println("No TCS34725 found ... check your connections");
    delay(1000);
  }

  lastRGBTime = loopStartTime;
}

//////////////////////////////* Loop */////////////////////////

void loop() {
  currentTime = millis();

  if ( currentTime - printer.lastExecutionTime > LOOP_PERIOD ) {
    printer.lastExecutionTime = currentTime;
    printer.printValue(0,adc.printSample());
    printer.printValue(1,ef.printStates());
    printer.printValue(2,logger.printState());
    printer.printValue(3,gps.printState());
    printer.printValue(4,state_estimator.printState());
    printer.printValue(5,surface_control.printWaypointUpdate());
    printer.printValue(6,surface_control.printString());
    printer.printValue(7,motor_driver.printState());
    printer.printValue(8,imu.printRollPitchHeading());
    printer.printValue(9,imu.printAccels());
    printer.printValue(10,rgbState);
    printer.printToSerial();  // To stop printing, just comment this line out
  }

  if ( currentTime - surface_control.lastExecutionTime > LOOP_PERIOD ) {
    surface_control.lastExecutionTime = currentTime;
    surface_control.navigate(&state_estimator.state, &gps.state, DELAY);
    motor_driver.drive(surface_control.uL, surface_control.uR, 0);
  }

  if ( currentTime - adc.lastExecutionTime > LOOP_PERIOD ) {
    adc.lastExecutionTime = currentTime;
    adc.updateSample();
  }

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

  if ( currentTime - button_sampler.lastExecutionTime > LOOP_PERIOD ) {
    button_sampler.lastExecutionTime = currentTime;
    button_sampler.updateState();
  }

  if ( currentTime - imu.lastExecutionTime > LOOP_PERIOD ) {
    imu.lastExecutionTime = currentTime;
    imu.read();     // blocking I2C calls
  }

  gps.read(&GPS); // blocking UART calls, need to check for UART every cycle

  if ( currentTime - state_estimator.lastExecutionTime > LOOP_PERIOD ) {
    state_estimator.lastExecutionTime = currentTime;
    state_estimator.updateState(&imu.state, &gps.state);
  }

  if ( currentTime - led.lastExecutionTime > LOOP_PERIOD ) {
    led.lastExecutionTime = currentTime;
    led.flashLED(&gps.state);
  }

  if ( currentTime - logger.lastExecutionTime > LOOP_PERIOD && logger.keepLogging ) {
    logger.lastExecutionTime = currentTime;
    logger.log();
  }

  // RGB UPDATE
  if (currentTime - lastRGBTime > RGB_PERIOD) {
    lastRGBTime = currentTime;

    uint16_t clear, red, green, blue;
    tcs.getRGBC(&red, &green, &blue, &clear);
    tcs.lock();

    rgbState = "RGB: C: " + String(clear) +
               " R: " + String(red) +
               " G: " + String(green) +
               " B: " + String(blue);

    uint32_t sum = clear;
    float r, g, b;
    r = red;   r /= sum;
    g = green; g /= sum;
    b = blue;  b /= sum;
    r *= 256;  g *= 256;  b *= 256;

    rgbState += " HEX: " + String((int)r, HEX) +
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