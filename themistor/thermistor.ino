/********
Amplified Thermistor Test Script
For Murata NXFT15WB473FA2B150 thermistor circuit

Teensy reads OP-AMP OUTPUT (Vout), not raw divider Vin.

Flow:
analogRead -> Vout -> Vin -> RT -> TempC
********/

#include <Arduino.h>
#include <math.h>

#define THERM_PIN A1   // change if needed

// Teensy ADC settings
const float V_ADC_REF = 3.3;     // Teensy analog reference
const float ADC_MAX = 1023.0;    // assume 10-bit ADC unless changed

// Divider / thermistor constants
const float VDD = 5.0;           // divider supply voltage
const float R2 = 47000.0;        // fixed resistor to ground in divider
const float R0 = 47000.0;        // thermistor resistance at 25 C
const float T0 = 298.15;         // 25 C in Kelvin
const float BETA = 4050.0;       // Murata beta value

// Op-amp constants
const float Rn = 10000.0;        // inverting input resistor
const float Rf = 47000.0;        // feedback resistor
const float G = Rf / Rn;         // amplifier ratio = Rf/Rn

// Offset divider for V+
const float Rp = 15000.0;        // from +5V to V+
const float Rg = 10000.0;        // from V+ to GND
const float Vplus = VDD * (Rg / (Rp + Rg));

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Amplified Thermistor Test Starting...");
  Serial.print("Vplus = "); Serial.println(Vplus, 4);
  Serial.print("Gain ratio G = "); Serial.println(G, 4);
}

void loop() {
  // 1) Read ADC count
  int raw = analogRead(THERM_PIN);

  // 2) Convert ADC count to op-amp output voltage seen by Teensy
  float Vout = raw * V_ADC_REF / ADC_MAX;

  // 3) Recover divider voltage Vin from op-amp equation:
  // Vout = (1+G)Vplus - G*Vin
  float Vin = ((1.0 + G) * Vplus - Vout) / G;

  // 4) Recover thermistor resistance RT from divider equation:
  // Vin = VDD * R2 / (RT + R2)
  float RT = NAN;
  if (Vin > 0.001 && Vin < (VDD - 0.001)) {
    RT = R2 * (VDD / Vin - 1.0);
  }

  // 5) Convert resistance to temperature using beta equation
  float tempC = NAN;
  float tempK = NAN;
  if (!isnan(RT) && RT > 0.0) {
    tempK = 1.0 / ( (1.0 / T0) + (1.0 / BETA) * log(RT / R0) );
    tempC = tempK - 273.15;
  }

  // Print everything for debugging
  Serial.print("RAW:\t");   Serial.print(raw);
  Serial.print("\tVout:\t"); Serial.print(Vout, 4);
  Serial.print("\tVin:\t");  Serial.print(Vin, 4);
  Serial.print("\tRT:\t");   Serial.print(RT, 1);
  Serial.print("\tTempC:\t");Serial.println(tempC, 2);

  delay(500);
}