#pragma once

#include "Arduino.h"

struct UartReader
{
    uint8_t lastValue;
    uint8_t currentPOL;
    uint8_t stage;
    uint8_t candidate;
    uint8_t counter;
    bool directionEast;
    
    bool check(void);
};
