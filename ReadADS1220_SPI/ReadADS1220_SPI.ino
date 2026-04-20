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

#define PGA          1                 // Programmable Gain = 1
#define VREF         0x40            // external reference on REFP0 and REFN0
#define VFSR         VREF/PGA
#define FULL_SCALE   (((long int)1<<23)-1)

#define ADS1220_CS_PIN    7
#define ADS1220_DRDY_PIN  8

Protocentral_ADS1220 pc_ads1220;
MovingAverageFilter filter;
uint64_t adc_data;
volatile bool drdyIntrFlag = false;

int count = 1;

void drdyInterruptHndlr(){
  drdyIntrFlag = true;
}

void enableInterruptPin(){
  attachInterrupt(digitalPinToInterrupt(ADS1220_DRDY_PIN), drdyInterruptHndlr, FALLING);
}

void setup()
{
    Serial.begin(9600);

    pc_ads1220.begin(ADS1220_CS_PIN,ADS1220_DRDY_PIN);

    pc_ads1220.set_data_rate(DR_20SPS);
    pc_ads1220.set_pga_gain(PGA);
    pc_ads1220.PGA_OFF();
    pc_ads1220.external_reference();
    pc_ads1220.set_VREF(VREF); 
    pc_ads1220.set_conv_mode_continuous();          //Set continuous conversion mode
    pc_ads1220.Start_Conv();  //Start continuous conversion mode

    pc_ads1220.PrintRegisterValues(); 
    
    enableInterruptPin();

    delay(100);
}

void loop()
{
   if(drdyIntrFlag){
      drdyIntrFlag = false;

      adc_data=pc_ads1220.Read_Data_Samples();  
      filter.addValue(adc_data);
      float filteredValue = filter.calculateFilteredValue();

      Serial.print(count);
      Serial.print(" ");
      Serial.println(adc_data);
      //Serial.print(" ");
      //Serial.println(filteredValue, 0);
      count += 1;   
      delay(20);   
    }
}

