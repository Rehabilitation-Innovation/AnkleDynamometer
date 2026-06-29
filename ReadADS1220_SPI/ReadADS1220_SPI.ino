//////////////////////////////////////////////////////////////////////////////////////////
//
//    Demo code for the ADS1220 24-bit ADC breakout board
//
//    Author: Ashwin Whitchurch
//    Copyright (c) 2018 ProtoCentral
//
//    This example sequentially reads all 4 channels in continuous conversion mode
//
//    Arduino connections:
//
//  |ADS1220 pin label| Pin Function         |Arduino Connection|
//  |-----------------|:--------------------:|-----------------:|
//  | DRDY            | Data ready Output pin|  D8              |
//  | MISO            | Slave Out            |  D12             |
//  | MOSI            | Slave In             |  D11             |
//  | SCLK            | Serial Clock         |  D13             |
//  | CS              | Chip Select          |  D7              |
//  | DVDD            | Digital VDD          |  +5V             |
//  | DGND            | Digital Gnd          |  Gnd             |
//  | AN0-AN3         | Analog Input         |  Analog Input    |
//  | AVDD            | Analog VDD           |  -               |
//  | AGND            | Analog Gnd           |  -               |
//
//    This software is licensed under the MIT License(http://opensource.org/licenses/MIT).
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
//   NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
//
//   For information on how to use, visit https://github.com/Protocentral/Protocentral_ADS1220
//
/////////////////////////////////////////////////////////////////////////////////////////

#include "Protocentral_ADS1220.h"
#include <SPI.h>
#include "MovingAverageFilter.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define PGA          1                 // Programmable Gain = 1
#define VREF         0x40            // external reference on REFP0 and REFN0
#define VFSR         VREF/PGA
#define FULL_SCALE   (((long int)1<<23)-1)

#define ADS1220_CS_PIN    17
#define ADS1220_DRDY_PIN  2
#define LCD_CS_PIN        7
#define LCD_DC_PIN        8
#define LCD_RST_PIN       9
#define ZERO_BUTTON_PIN   5
#define RESET_BUTTON_PIN  6

SPISettings ADC_SPI_SETTINGS(2000000, MSBFIRST, SPI_MODE1);   // ADC SPI settings
SPISettings LCD_SPI_SETTINGS(80000000, MSBFIRST, SPI_MODE3);   // LCD SPI settings

Protocentral_ADS1220 pc_ads1220;
MovingAverageFilter filter;
Adafruit_ST7789 display(LCD_CS_PIN, LCD_DC_PIN, LCD_RST_PIN);

int32_t adc_data;
int32_t zero;
float conversion = 6.0 / (4367025 - 4121339);
float currentReading; 
float reading_kg;
float currentMax = 0;
volatile bool drdyIntrFlag = false;
int count = 0;
int startupReadings = 20;
bool zeroButtonState = 0; 
bool resetButtonState = 0;
float filteredValue = 0;
float torqueLength = 0.294;   //m

void drdyInterruptHndlr(){
  drdyIntrFlag = true;
}

void enableInterruptPin(){
  attachInterrupt(digitalPinToInterrupt(ADS1220_DRDY_PIN), drdyInterruptHndlr, FALLING);
}

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
  if(drdyIntrFlag){
    drdyIntrFlag = false;

    SPI.beginTransaction(ADC_SPI_SETTINGS); 
    adc_data = -1 * pc_ads1220.Read_Data_Samples();        //pulls CS pin to low, reads data, pulls pin high again. Library manages CS toggling internally
    SPI.endTransaction();

    filter.addValue(adc_data); 
    filteredValue = filter.calculateFilteredValue(); 
    reading_kg = (filteredValue - zero) * conversion; 
    currentReading = roundToHalf(reading_kg * 9.81 * torqueLength); 

    Serial.print(count);
    // Serial.print(" ");
    // Serial.println(adc_data);
    Serial.print(" ");
    Serial.print(currentReading);
    Serial.print(" ");
    Serial.println(filteredValue, 0);
    count += 1;     
  }

  if(abs(currentReading) > abs(currentMax)) {
    currentMax = currentReading;
    updateMaxReading(currentMax);
  }

  zeroButtonState = digitalRead(ZERO_BUTTON_PIN);
  resetButtonState = digitalRead(RESET_BUTTON_PIN);

  if (zeroButtonState == LOW) {
    zeroSystem(filteredValue);
  } 

  if (resetButtonState == LOW) {
    updateMaxReading(0);
    currentMax = 0;
  }

  SPI.beginTransaction(LCD_SPI_SETTINGS);
  display.setTextColor(ST77XX_BLACK, ST77XX_WHITE);
  display.setTextSize(4);
  display.setCursor(10, 100);
  display.print(String(currentReading, 1) + "   ");
  SPI.endTransaction();

  delay(100); 
}

void zeroSystem(float filteredValue) {
  zero = filteredValue; 
}

float roundToHalf(float value) {
  return round(value*2) / 2.0;
}

void updateMaxReading(float reading) {
  SPI.beginTransaction(LCD_SPI_SETTINGS);
  display.setTextColor(ST77XX_BLACK, ST77XX_WHITE);
  display.setTextSize(4);
  display.setCursor(10, 200);
  display.print(String(reading, 1) + "   ");
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
  display.print("Current:");
  display.setCursor(180, 108);
  display.print("Nm");
  display.setCursor(10, 100);
  display.setTextSize(4);
  display.print(String(currentReading, 1) + "   ");

  display.setCursor(10, 160);
  display.setTextColor(ST77XX_BLACK, ST77XX_WHITE);
  display.setTextSize(3);
  display.print("Max:");
  display.setCursor(180, 208);
  display.print("Nm");
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

