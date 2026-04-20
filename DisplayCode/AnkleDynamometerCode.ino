#include <math.h>
#include <LiquidCrystal.h>

// Set pins and setup lcd
int loadcellPin = A0;
const int zeroButtonPin = 7;
int zeroButtonState = 0;
const int resetButtonPin = 8;
int resetButtonState = 0;
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int reading = 0;
int currentForce = 0;
int currentForceAve = 0;
int zeroOffset = 0;
int sum = 0;
String unit = "X";        
int peakForce = 0;
int forceDisplayRow = 0;    // LCD row displaying force
int peakDisplayRow = 1;     // LCD row displaying peak
int valueStartCol = 7;       // Starting column for the values being updated

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(zeroButtonPin, INPUT);
  pinMode(resetButtonPin, INPUT);
  lcd.begin(16, 2);
  screenSetup();
  zero();
  resetPeak();
}

void loop() {
  zeroButtonState = digitalRead(zeroButtonPin);
  if (zeroButtonState) {
    zero();
  };

  resetButtonState = digitalRead(resetButtonPin);
  if (resetButtonState) {
    resetPeak();
  };

  for (int i = 0; i < 10; i++) {
    reading = analogRead(loadcellPin) - zeroOffset;
    //currentForce = map(reading, 0, 1023,-200, 200);
    currentForce = reading;
    sum = sum + currentForce;
    delay(25);
  };
  currentForceAve = sum / 10;
  sum = 0; 
  Serial.println(currentForceAve);
  updateValue(forceDisplayRow, currentForceAve);

  if (abs(currentForce) > abs(peakForce)) {
    peakForce = currentForce;
    updateValue(peakDisplayRow, peakForce);
  };
}

void updateValue(int row, int value) {
  lcd.setCursor(valueStartCol, row);
  lcd.print("     ");
  lcd.setCursor(valueStartCol, row);
  lcd.print(value);
}

void screenSetup() {
  lcd.clear();
  
  lcd.setCursor(0, forceDisplayRow);
  lcd.print("Force:");
  lcd.setCursor(12, forceDisplayRow);
  lcd.print("["+unit+"]");

  lcd.setCursor(0, peakDisplayRow);
  lcd.print("Peak: ");
  lcd.setCursor(12, peakDisplayRow);
  lcd.print("["+unit+"]");
}

void zero() {
    zeroOffset = zeroOffset + currentForce;
    currentForce = 0;
    updateValue(forceDisplayRow, currentForce);
}

void resetPeak() {
  peakForce = 0;
  updateValue(peakDisplayRow, peakForce);
}