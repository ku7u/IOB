#ifndef LOCATION_H
#define LOCATION_H

#include "Arduino.h"


struct MagnetReader
{
    MagnetReader(int x, int y);

    enum sampleState {clear, started };
    sampleState detectState;

    uint someStates;
    uint bothStates;
    uint digitState;
    uint digitValue[4];

    int leftPin;
    int rightPin;

    bool check();
    uint process(bool);
};

#endif