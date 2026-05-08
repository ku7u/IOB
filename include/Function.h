#pragma once
#include <functional>
#include "ILocoStatus.h"

// #include "Arduino.h"
// #include "Throttle.h"

// extern int roadNum;
// extern Throttle throttle;

// extern uint16_t functionBell;
// extern uint16_t functionHorn;
// extern uint16_t functionHeadlightDim;
// extern uint16_t functionHeadlightBright;
// extern uint16_t functionRearlightDim;
// extern uint16_t functionRearlightBright;
// extern uint16_t functionPM;
// extern uint16_t functionNotchingEnable;
// extern uint16_t functionNotchUp;
// extern uint16_t functionNotchDown;
// extern uint16_t functionIndependentBrake;
// extern uint16_t functionTrainBrake;
// extern uint16_t functionEmergencyBrake;

void setLocoStatusProvider(ILocoStatus *provider);
void setFunction(int function, bool onOff);
// void startStop(bool start);

