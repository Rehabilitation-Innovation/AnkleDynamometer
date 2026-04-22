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
#include "LCD_Driver.h"
#include "GUI_Paint.h"

#define PGA          1                 // Programmable Gain = 1
#define VREF         0x40            // external reference on REFP0 and REFN0
#define VFSR         VREF/PGA
#define FULL_SCALE   (((long int)1<<23)-1)

#define ADS1220_CS_PIN    7
#define ADS1220_DRDY_PIN  8

SPISettings ADC_SPI_SETTINGS(2000000, MSBFIRST, SPI_MODE1);   // ADC SPI settings
SPISettings LCD_SPI_SETTINGS(80000000, MSBFIRST, SPI_MODE3);   // LCD SPI settings

Protocentral_ADS1220 pc_ads1220;
MovingAverageFilter filter;
uint32_t adc_data;
volatile bool drdyIntrFlag = false;

int count = 0;
char myChar[20];
String myString;

void drdyInterruptHndlr(){
  drdyIntrFlag = true;
}

void enableInterruptPin(){
  attachInterrupt(digitalPinToInterrupt(ADS1220_DRDY_PIN), drdyInterruptHndlr, FALLING);
}

void setup()
{
  Serial.begin(9600);
  SPI.begin();

  setupADC(); 
  pc_ads1220.PrintRegisterValues(); 
  enableInterruptPin();
  setupLCD();
  delay(100);
}

void loop()
{
   if(drdyIntrFlag){
      drdyIntrFlag = false;

      SPI.beginTransaction(ADC_SPI_SETTINGS);
      adc_data = pc_ads1220.Read_Data_Samples();        //pulls CS pin to low, reads data, pulls pin high again
      SPI.endTransaction();

      filter.addValue(adc_data);
      float filteredValue = filter.calculateFilteredValue();

      Serial.print(count);
      Serial.print(" ");
      //Serial.println(adc_data);
      //Serial.print(" ");
      //Serial.println(filteredValue, 0);
      count += 1;     
    }

  SPI.beginTransaction(LCD_SPI_SETTINGS);
  //myString = String(adc_data);
  sprintf(myChar, "%lu", adc_data);
  Serial.print(adc_data);
  Serial.print("  ");
  Serial.println(myChar);
  //Serial.println(myString);
  Paint_DrawString_EN(10, 70, myChar, &Font24, YELLOW, BLACK);
  SPI.endTransaction();
  delay(100); 

}

void setupLCD() {
  Config_Init();
  LCD_Init();
  LCD_Clear(0xffff);
  Paint_NewImage(LCD_HEIGHT, LCD_WIDTH, 0, WHITE);
  Paint_Clear(WHITE);
  Paint_DrawString_EN(10, 10, "Ankle", &Font24, WHITE, BLACK);
  Paint_DrawString_EN(10, 30, "Dynamometer", &Font24, WHITE, BLACK);
  //Paint_DrawString_EN(10, 70, "Count:", &Font24, WHITE, BLACK);
  //Paint_DrawString_EN(130, 70, "    ", &Font24, YELLOW, BLACK);
  digitalWrite(6, HIGH);
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

