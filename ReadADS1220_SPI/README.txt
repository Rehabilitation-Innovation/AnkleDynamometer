Lookup table with linear interpolation between points
The calibration x points must be sorted in ascending order
I used PROGMEM so that the calibration table lives in flash and not RAM
Values outside of the table range are extrapolated using the slope of the nearest end segment