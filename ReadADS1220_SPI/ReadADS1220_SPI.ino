#include "Protocentral_ADS1220.h"
#include <SPI.h>
#include "MovingAverageFilter.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define PGA          1                 // Programmable Gain = 1
#define VREF         0x40            // external reference on REFP0 and REFN0
#define VFSR         VREF/PGA
#define FULL_SCALE   (((long int)1<<23)-1)

// Pin definitions
#define ADS1220_CS_PIN    17
#define ADS1220_DRDY_PIN  2
#define LCD_CS_PIN        7
#define LCD_DC_PIN        8
#define LCD_RST_PIN       9
#define ZERO_BUTTON_PIN   5
#define RESET_BUTTON_PIN  6

// SPI settings for the different devices
SPISettings ADC_SPI_SETTINGS(2000000, MSBFIRST, SPI_MODE1);   // ADC SPI settings
SPISettings LCD_SPI_SETTINGS(80000000, MSBFIRST, SPI_MODE3);   // LCD SPI settings

Protocentral_ADS1220 pc_ads1220;
MovingAverageFilter filter;
Adafruit_ST7789 display(LCD_CS_PIN, LCD_DC_PIN, LCD_RST_PIN);

int32_t adc_data;
int32_t zero;
float currentTorque; 
float filteredValue;
float reading_kg;
float currentMax = 0;
int count = 0;
const int startupReadings = 20;
volatile bool drdyIntrFlag = false;
// initialize button states
bool zeroButtonState = 0; 
bool resetButtonState = 0;
float torqueLength = 0.294;   //m
// Setup for the lookup table
const int TABLE_SIZE = 15;
const float adcPoints[TABLE_SIZE] PROGMEM = {3335661, 3715514, 4125184, 4496124, 4872487, 5245785, 5555336, 5935980, 6411038, 6616478, 6920726, 7331982, 7697140, 8060301, 8372511};
const float massPoints[TABLE_SIZE] PROGMEM = {-18.3, -9.2, 0, 9.0, 18.1, 27.3, 34.9, 44.3, 55.1, 62.8, 70.4, 77.6, 86.6, 95.7, 104.9};

void setup()
{
  Serial.begin(115200);
  SPI.begin();
  
  setupLCD();
  setupADC(); 

  pc_ads1220.PrintRegisterValues(); 
  enableInterruptPin();
  pinMode(ZERO_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  int32_t total = 0;

  delay(100);

  for (int i = 0; i < startupReadings; i++) {
    SPI.beginTransaction(ADC_SPI_SETTINGS); 
    total += -1 * pc_ads1220.Read_Data_Samples();        //pulls CS pin to low, reads data, pulls pin high again. Library manages CS toggling internally
    SPI.endTransaction();
    Serial.print(total);
    Serial.print(" ");
    delay(100); // small delay between readings
  }

  zero = total / startupReadings; 
  Serial.print("Average: ");
  Serial.println(zero);

  delay(100);
}

void loop()
{
  // read sensor and calculate torque
  if(drdyIntrFlag){
    drdyIntrFlag = false;

    SPI.beginTransaction(ADC_SPI_SETTINGS); 
    adc_data = -1 * pc_ads1220.Read_Data_Samples();    //function pulls CS pin to low, reads data, pulls pin high again. Library manages CS toggling internally
    SPI.endTransaction();

    // filter the adc data
    filter.addValue(adc_data); 
    filteredValue = filter.calculateFilteredValue();
    reading_kg = lookup(filteredValue) - lookup(zero);                     // convert the filtered value to kg 
    currentTorque = roundToHalf(reading_kg * 9.81 * torqueLength);       // calculate torque

    // Serial prints for testing
    Serial.print(count);
    Serial.print(" ");
    Serial.print(adc_data);
    Serial.print(" ");
    Serial.println(reading_kg);
    //Serial.print(" ");
    //Serial.println(filteredValue, 0);
    count += 1;     
  }

  // Check if the current reading beats the maximum
  if(abs(currentTorque*10) > abs(currentMax*10)) {
    currentMax = currentTorque;
    updateMaxReading(currentMax);
  }

  // Read button states
  zeroButtonState = digitalRead(ZERO_BUTTON_PIN);
  resetButtonState = digitalRead(RESET_BUTTON_PIN);

  // Check for zero button press
  if (zeroButtonState == LOW) {
    zeroSystem(filteredValue);
  } 

  // Check for reset button press
  if (resetButtonState == LOW) {
    currentMax = 0;
    updateMaxReading(currentMax);
  }

  updateCurrentReading(currentTorque);

  delay(100); 
}

void zeroSystem(float num) {
  zero = num; 
}

float roundToHalf(float value) {
  return round(value*2) / 2.0;
}

float lookup(float x) {
  // Read endpoint values from PROGMEM
  float xFirst = pgm_read_float(&adcPoints[0]);
  float xLast  = pgm_read_float(&adcPoints[TABLE_SIZE - 1]);

  // Below range: extrapolate using the slope of the first segment
  if (x < xFirst) {
    float x0 = pgm_read_float(&adcPoints[0]);
    float x1 = pgm_read_float(&adcPoints[1]);
    float y0 = pgm_read_float(&massPoints[0]);
    float y1 = pgm_read_float(&massPoints[1]);
    float slope = (y1 - y0) / (x1 - x0);
    return y0 + slope * (x - x0);
  }

  // Above range: extrapolate using the slope of the last segment
  if (x > xLast) {
    float x0 = pgm_read_float(&adcPoints[TABLE_SIZE - 2]);
    float x1 = pgm_read_float(&adcPoints[TABLE_SIZE - 1]);
    float y0 = pgm_read_float(&massPoints[TABLE_SIZE - 2]);
    float y1 = pgm_read_float(&massPoints[TABLE_SIZE - 1]);
    float slope = (y1 - y0) / (x1 - x0);
    return y0 + slope * (x - x0);
  }

  // Find the points that bracket x
  for (int i = 0; i < TABLE_SIZE - 1; i++) {
    float x0 = pgm_read_float(&adcPoints[i]);
    float x1 = pgm_read_float(&adcPoints[i + 1]);

    if (x >= x0 && x <= x1) {
      float y0 = pgm_read_float(&massPoints[i]);
      float y1 = pgm_read_float(&massPoints[i + 1]);

      // Linear interpolation formula
      float t = (x - x0) / (x1 - x0);
      return y0 + t * (y1 - y0);
    }
  }

  return 0.0; // shouldn't reach here
}

void updateCurrentReading(float reading) {
  SPI.beginTransaction(LCD_SPI_SETTINGS);
  display.setTextColor(ST77XX_BLACK, ST77XX_WHITE);
  display.setTextSize(4);
  display.setCursor(10, 100);
  display.print(String(reading, 1) + "  ");
  SPI.endTransaction();
}

void updateMaxReading(float reading) {
  SPI.beginTransaction(LCD_SPI_SETTINGS);
  display.setTextColor(ST77XX_BLACK, ST77XX_WHITE);
  display.setTextSize(4);
  display.setCursor(10, 200);
  display.print(String(reading, 1) + "  ");
  SPI.endTransaction();
}

void setupADC() {
  pc_ads1220.begin(ADS1220_CS_PIN,ADS1220_DRDY_PIN);

  pc_ads1220.set_data_rate(DR_20SPS);
  pc_ads1220.set_pga_gain(PGA);
  pc_ads1220.PGA_OFF();
  pc_ads1220.external_reference();
  pc_ads1220.set_VREF(VREF); 
  pc_ads1220.set_conv_mode_continuous();          //Set continuous conversion mode
  pc_ads1220.Start_Conv();  //Start continuous conversion mode
}

void setupLCD() {
  display.init(240, 320);
  display.fillScreen(ST77XX_WHITE);
  display.setTextColor(ST77XX_BLACK, ST77XX_WHITE);
  display.setTextSize(2);
  display.setCursor(10, 10);
  display.print("Ankle Dynamometer");
  display.setCursor(0, 30);
  display.print("--------------------");
  
  display.setCursor(10, 60);
  display.setTextColor(ST77XX_BLACK, ST77XX_WHITE);
  display.setTextSize(3);
  display.print("Current [Nm]");
  display.setCursor(10, 100);
  display.setTextSize(4);
  display.print(String(currentTorque, 1) + "   ");

  display.setCursor(10, 160);
  display.setTextColor(ST77XX_BLACK, ST77XX_WHITE);
  display.setTextSize(3);
  display.print("Max [Nm]");
  display.setCursor(10, 200);
  display.setTextSize(4);
  display.print(String(currentMax, 1) + "   ");

  display.setTextColor(ST77XX_BLACK, ST77XX_WHITE);
  display.setTextSize(1);
  display.setCursor(10, 300);
  display.print("Any questions contact:");
  display.setCursor(10, 310);
  display.print("tod.vandenberg@ahs.ca");
}

void drdyInterruptHndlr(){
  drdyIntrFlag = true;
}

void enableInterruptPin(){
  attachInterrupt(digitalPinToInterrupt(ADS1220_DRDY_PIN), drdyInterruptHndlr, FALLING);
}

