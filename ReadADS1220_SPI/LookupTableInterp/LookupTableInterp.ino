/*
  Lookup Table with Linear Interpolation for Arduino
  ---------------------------------------------------
  Stores a set of (x, y) points and interpolates between them
  for any x value you look up. Useful for sensor calibration
  curves, custom response curves, thermocouple tables, etc.

  - Table must be sorted by x in ascending order.
  - Values outside the table range are extrapolated using
    the slope of the nearest end segment (not clamped).
  - Uses PROGMEM so the table lives in flash, not RAM
    (handy on boards like the Uno with only 2KB of RAM).
*/

const int TABLE_SIZE = 6;
String inputNum;
float inputNumFloat;
float lookupNum; 

// EDIT THESE: your own (x, y) calibration points
const float xPoints[TABLE_SIZE] PROGMEM = {0.0, 10.0, 20.0, 30.0, 40.0, 50.0};
const float yPoints[TABLE_SIZE] PROGMEM = {0.0, 8.0, 19.0, 33.0, 40.0, 41.0};

float lookup(float x) {
  // Read endpoint values from PROGMEM
  float xFirst = pgm_read_float(&xPoints[0]);
  float xLast  = pgm_read_float(&xPoints[TABLE_SIZE - 1]);

  // Below range: extrapolate using the slope of the first segment
  if (x < xFirst) {
    float x0 = pgm_read_float(&xPoints[0]);
    float x1 = pgm_read_float(&xPoints[1]);
    float y0 = pgm_read_float(&yPoints[0]);
    float y1 = pgm_read_float(&yPoints[1]);
    float slope = (y1 - y0) / (x1 - x0);
    return y0 + slope * (x - x0);
  }

  // Above range: extrapolate using the slope of the last segment
  if (x > xLast) {
    float x0 = pgm_read_float(&xPoints[TABLE_SIZE - 2]);
    float x1 = pgm_read_float(&xPoints[TABLE_SIZE - 1]);
    float y0 = pgm_read_float(&yPoints[TABLE_SIZE - 2]);
    float y1 = pgm_read_float(&yPoints[TABLE_SIZE - 1]);
    float slope = (y1 - y0) / (x1 - x0);
    return y0 + slope * (x - x0);
  }

  // Find the two points that bracket x
  for (int i = 0; i < TABLE_SIZE - 1; i++) {
    float x0 = pgm_read_float(&xPoints[i]);
    float x1 = pgm_read_float(&xPoints[i + 1]);

    if (x >= x0 && x <= x1) {
      float y0 = pgm_read_float(&yPoints[i]);
      float y1 = pgm_read_float(&yPoints[i + 1]);

      // Linear interpolation formula
      float t = (x - x0) / (x1 - x0);
      return y0 + t * (y1 - y0);
    }
  }

  return 0.0; // shouldn't reach here
}

void setup() {
  Serial.begin(9600);

  // Example usage (includes points below/above the table range,
  // and a negative x, to demonstrate extrapolation)
  float testInputs[] = {-15.0, -5.0, 0.0, 5.0, 15.0, 25.0, 45.0, 60.0};
  for (int i = 0; i < 8; i++) {
    float x = testInputs[i];
    Serial.print("x = ");
    Serial.print(x);
    Serial.print(" -> y = ");
    Serial.println(lookup(x));
  }
}

void loop() {
  if (Serial.available() > 0) {
    inputNum = Serial.readString();
    inputNumFloat = inputNum.toFloat();
    Serial.print("Input:  ");
    Serial.println(inputNumFloat);
    lookupNum = lookup(inputNumFloat);
    Serial.print("Output: ");
    Serial.println(lookupNum);

  }

  delay(2000);
}
