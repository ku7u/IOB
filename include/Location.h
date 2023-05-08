#ifndef LOCATION_H
#define LOCATION_H

#include "Arduino.h"


struct MagnetReader
{
    MagnetReader(int x, int y);

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

#endif