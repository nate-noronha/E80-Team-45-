/*!
 * @file  colorview.ino
 * @brief Gets the ambient light color
 * @copyright   Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 * @author      PengKaixing(kaixing.peng@dfrobot.com)
 * @version     V1.0.0
 * @date        2022-03-16
 * @url         https://github.com/DFRobot/DFRobot_TCS
 */

#include "DFRobot_TCS34725.h"  //check how to download


// Create sensor object
DFRobot_TCS34725 tcs = DFRobot_TCS34725(&Wire, TCS34725_ADDRESS,TCS34725_INTEGRATIONTIME_24MS, TCS34725_GAIN_1X);

void setup()
{
  Serial.begin(115200);
  Serial.println("Color View Test!");

  // Initialize sensor
  while (tcs.begin() != 0)
  {
    Serial.println("No TCS34725 found ... check your connections");
    delay(1000);
  }

  Serial.println("TCS34725 initialized!");
}

void loop()
{
  uint16_t clear, red, green, blue;

  // Read sensor values
  tcs.getRGBC(&red, &green, &blue, &clear);

  // Print raw values
  Serial.print("C:\t"); Serial.print(clear);
  Serial.print("\tR:\t"); Serial.print(red);
  Serial.print("\tG:\t"); Serial.print(green);
  Serial.print("\tB:\t"); Serial.println(blue);

  // Normalize safely
  if (clear > 0)
  {
    float r = 255.0f * red   / clear;
    float g = 255.0f * green / clear;
    float b = 255.0f * blue  / clear;

    // Clamp values to [0,255]
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;

    // Print normalized RGB
    Serial.print("RGB:\t");
    Serial.print((int)r); Serial.print("\t");
    Serial.print((int)g); Serial.print("\t");
    Serial.println((int)b);

    // Optional HEX output (cleaner formatting)
    Serial.print("HEX:\t#");
    if ((int)r < 16) Serial.print("0");
    Serial.print((int)r, HEX);
    if ((int)g < 16) Serial.print("0");
    Serial.print((int)g, HEX);
    if ((int)b < 16) Serial.print("0");
    Serial.println((int)b, HEX);
  }
  else
  {
    Serial.println("No valid reading (clear = 0)");
  }
}