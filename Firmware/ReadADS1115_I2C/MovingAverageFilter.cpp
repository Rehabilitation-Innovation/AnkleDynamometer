#include "MovingAverageFilter.h"

MovingAverageFilter::MovingAverageFilter() : index(0), bufferFilled(false) { 
    for (int i = 0; i < WINDOW_SIZE; i++) {
        window[i] = 0;
    }
}

void MovingAverageFilter::addValue(uint16_t value) {
    window[index] = value;
    index = (index + 1) % WINDOW_SIZE;
    
    if (index == 0) {
        bufferFilled = true;
    }
}

float MovingAverageFilter::calculateFilteredValue() {
    if (!bufferFilled) {
        int validCount = index == 0 ? WINDOW_SIZE : index;

        uint32_t sum = 0;

        for (int i = 0; i < validCount; i++) {
            sum += window[i];
        }
        
        return (float)sum / validCount;
    }
        
    // Create a copy of the window for sorting
    for (int i = 0; i < WINDOW_SIZE; i++) {
        sortedWindow[i] = window[i];
    }
    
    // Sort the array (bubble sort - simple for small arrays)
    for (int i = 0; i < WINDOW_SIZE - 1; i++) {
        for (int j = 0; j < WINDOW_SIZE - i - 1; j++) {
            if (sortedWindow[j] > sortedWindow[j + 1]) {
                // Swap
                uint16_t temp = sortedWindow[j];
                sortedWindow[j] = sortedWindow[j + 1];
                sortedWindow[j + 1] = temp;
            }
        }
    }

    // Calculate sum excluding smallest and largest (outliers)
    uint32_t sum = 0;
    for (int i = 1; i < WINDOW_SIZE - 1; i++) {
        sum += sortedWindow[i];
    }
    
    // Return average of remaining values
    return (float)sum / (WINDOW_SIZE - 2);
}

void MovingAverageFilter::clear() {
    index = 0;
    bufferFilled = false;
    for (int i = 0; i < WINDOW_SIZE; i++) {
        window[i] = 0;
    }
}

