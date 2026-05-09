#pragma once

#include "Arduino.h"
#include "IBrakeSystem.h"
#include "IDccHardware.h"


class BrakeSystem : public IBrakeSystem
{
public:
    BrakeSystem(void);
    void setDccHardware(IDccHardware *d) { _dcc = d; }

    uint16_t getTrainlinePSI(void);
    uint16_t getEffectiveLocoBrake(void); // TBD should be float?
    float getEffectiveTrainBrake(void);
    uint16_t getMainPSI(void);
    void setCompressorFunction(uint16_t);
    bool cycle(bool);
    void connectAirLine(bool connecting, uint16_t carCount = 0); // connecting true if connecting, false if disconnecting
    float applyLocoBrake(bool applying);                         // applying = false if releasing, returns loco brake effectiveness as percent
    float applyTrainBrake(bool applying);                        // applying = false if releasing, returns train brake effectiveness as percent
    void applyEmmergency(bool applying);                         // returns emergency brake effectiveness as percent
    void applyHandbrake(void);
    void release(void);
    void setPMRunning(bool complete); // send this command to tell BS that PM has gone through startup sequence, start the compressor
    bool locoBrakeOn(void);
    bool trainBrakeOn(void);
    bool emergencyBrakeOn(void);

private:
    IDccHardware *_dcc = nullptr; // One pointer replaces the rest

    // https://en.wikipedia.org/wiki/Railway_air_brake
    const uint16_t MAIN_MAX_PSI = 140;        // psi
    const uint16_t MAIN_MIN_PSI = 125;        // psi
    const uint16_t TRAINLINE_MAX_PSI = 90;    // psi
    const int TRAINLINE_MIN_AUTO_PSI = 60;    // psi TBD the value
    const float EMERGENCY_BRAKE_FACTOR = 1.5; // ebrake is more effective than train brake by this factor

    const uint16_t PER_NOTCH = 20; // percent of full throw per notch on brake levers
    const uint16_t CAR_MASS = 75;  // tons

    uint16_t _functionCompressor;
    uint16_t _mainPercentFull; // percent
    uint16_t _mainPSI;         // psi
    bool _compressorRunning;
    float _trainlineSetPSI;     // psi
    float _trainlinePSI;        // psi
    float _effectiveTrainBrake; // percent
    float _effectiveLocoBrake;  // percent
    bool _trainBrakeApplied = false;
    bool _trainlineConnected;
    uint16_t _carCount;
    uint16_t _trainMass; // tons
    bool _handbrake;
    bool _startupComplete;
    bool _locoBrakeOn;
    bool _trainBrakeOn;
    bool _emergencyBrakeOn;
    bool _handBrakeOn;
};
