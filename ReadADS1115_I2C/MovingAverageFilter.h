// Moving average filter with outlier removal
#ifndef MOVINGAVERAGEFILTER_H
#define MOVINGAVERAGEFILTER_H

#include <Arduino.h>

class MovingAverageFilter {
    private:
        static const int WINDOW_SIZE = 10;
        uint16_t window[WINDOW_SIZE];
        uint16_t sortedWindow[WINDOW_SIZE];
        int index;
        bool bufferFilled;

    public:
        MovingAverageFilter();
        void addValue(uint16_t value);
        float calculateFilteredValue();
        void clear();
}; 

#endif

