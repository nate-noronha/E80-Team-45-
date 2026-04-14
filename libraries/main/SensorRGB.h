#ifndef SENSOR_RGB_H
#define SENSOR_RGB_H

#include <Arduino.h>
#include "DFRobot_TCS34725.h"
#include "DataSource.h"   // ADD THIS

class SensorRGB : public DataSource {   // ADD inheritance
public:
  uint16_t r, g, b, c;
  int lastExecutionTime;
  bool isReady;
  DFRobot_TCS34725* tcs_ptr;

  // DataSource constructor requires column names and types as strings
  SensorRGB()
  : DataSource("rgb_r,rgb_g,rgb_b,rgb_c", "uint16,uint16,uint16,uint16"),
  isReady(false), r(0), g(0), b(0), c(0), lastExecutionTime(0)
  {}

  void init(DFRobot_TCS34725* sensor) {
    tcs_ptr = sensor;
    r = 0; g = 0; b = 0; c = 0;
    lastExecutionTime = 0;
    isReady = true;
  }

  void read() {
    if (!isReady) return;  
    tcs_ptr->getRGBC(&r, &g, &b, &c);
    tcs_ptr->lock();
  }

  String printHeader() {
    return String("rgb_r,rgb_g,rgb_b,rgb_c");
  }

  String printState() {
    return String(r) + "," +
           String(g) + "," +
           String(b) + "," +
           String(c);
  }

  // DataSource requires this method to write raw bytes to the SD buffer
  size_t writeDataBytes(unsigned char* buffer, size_t idx) {
    uint16_t* data_slot = (uint16_t*)&buffer[idx];
    data_slot[0] = r;
    data_slot[1] = g;
    data_slot[2] = b;
    data_slot[3] = c;
    return idx + 4 * sizeof(uint16_t);
  }
};

#endif