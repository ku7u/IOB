#pragma once

#include "Arduino.h"


struct MagnetReader
{
    MagnetReader(int x, int y);

    const uint MAX_BITS = 4;    // number of bits in the location code = number of magnets on sync side

    enum sampleState {clear, started };
    sampleState detectState;
    bool errorState;

    uint someStates;
    uint bothStates;
    uint digitState;
    uint digitValue[4];

    uint32_t startOfPeriod;

    int leftPin;
    int rightPin;

    bool check(uint);
    uint process(bool);
};
