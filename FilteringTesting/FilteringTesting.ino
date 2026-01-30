#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include "MovingAverageFilter.h"

Adafruit_ADS1115 ads;  // Create ADS1115 object
MovingAverageFilter filter;

const int SAMPLING_DELAY = 10;
int counter = 1;

void setup() {
  Serial.begin(9600);
  Wire.begin();  // Initialize I2C
  Serial.println("begin");

  // Set gain (optional, default is ±4.096V)
  //ads.setGain(GAIN_ONE);  // ±4.096V
  ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V

    if (!ads.begin()) {
    Serial.println("Failed to initialize ADS1115!");
    while (1);
  }

  // for (int i = 0; i <= 9999; i++) {
  //   int16_t adcValue = ads.readADC_SingleEnded(1);  // Read from channel 0
  //   //float voltage = ads.computeVolts(adcValue);     // computes the voltage reading
  //   //float mass = (13334 - adcValue) * 300.0/26667;     // replace 13334 with reading from the reference pin of the amplifier
  //   Serial.print(i);
  //   Serial.print(" ");
  //   Serial.println(adcValue);

  //   delay(10);
  // }
}

void loop() {
  int16_t adcValue = ads.readADC_SingleEnded(0);  // read from channel 0

  filter.addValue(adcValue);    //add value to the window
  float filteredValue = filter.calculateFilteredValue();    // get filtered value from the filter

  Serial.print(counter);
  Serial.print(" ");
  Serial.println(filteredValue, 0);

  counter += 1;
  delay(SAMPLING_DELAY);
}