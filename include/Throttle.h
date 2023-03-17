#ifndef THROTTLE_H
#define THROTTLE_H

#include <stdint.h>
#include "Arduino.h"

class Throttle
{
public:
    // Throttle(int roadNumber);
    Throttle(void);

    void init(void);
    void setRoadNumber(int roadNumber);
    void pmOnOff(bool onOff);
    void bell(bool onOff);
    void horn(bool onOff);
    void headlight(int offDimBright);
    void rearlight(int offDimBright);
    void startPM(void);
    void stopPM(void);
    void setDirection(int direction);
    void setThrottleLever(int throttleLever);
    void setMass(uint16_t mass);
    void setTonnage(uint16_t tonnage);
    void setHorsepower(int HP);
    void setGrade(int grade);
    void manualNotch(bool up);
    void compute(void);
    void computeVelocity(void);
    void setIBrake(uint16_t val);
    void setTBrake(uint16_t val);
    void panicStop(void);
    void setAirGauge(void);
    int getRoadNumber(void);

    int functionPM;
    int functionBell;
    int functionHorn;
    int functionHeadlightBright;
    int functionHeadlightDim;
    int functionRearlightBright;
    int functionRearlightDim;
    int functionNotchingEnable;
    int functionNotchUp;
    int functionNotchDown;
    int functionIndependentBrake;
    int functionTrainBrake;
    int functionEmergencyBrake;
    int functionCompressor;

private:
    uint16_t _roadNumber = 16; // TBD set this programatically
    bool _running = false;
    bool _direction; // true = forward
    uint16_t _throttleLever;
    uint16_t _notch;
    uint16_t _lastNotch;
    bool _neutral = true;
    float _currentSpeed;
    uint16_t _targetSpeed;
    float _mph;
    float _speedFactor = 0.5;      // multiply by commanded speed to get real mph
    float _speedoCalFactor = .250; // calibrate speedometer
    float _divisor;
    uint16_t _horsepower = 1500;      // HP
    uint16_t _horsepowerAtIdle = 15;
    uint16_t _carCount;
    uint16_t _tonnage = 0;            // tons
    long _locoWeight;
    long _locoMass; // slugs
    float _tractiveEffort;
    uint16_t _independentBrake;       // percent
    uint16_t _iBrakeVal = 0;
    uint16_t _trainBrake;             // percent
    enum _mode
    {
        MAN_NOTCHING,
        DRIVE_HOLD,
        NORMAL
    };
    bool _manualNotching; // true if using manual notching TBD this
    bool _manualNotchingMode = false;
    uint16_t _manualNotchingLogicFunction;
    uint16_t _notchUpFunction = 26;
    uint16_t _notchDownFunction = 27;
    uint16_t _driveHoldFunction;
    uint32_t _lastCommandTime;
    float _trainlinePSI;
    uint16_t _trainlineSetPSI;
    bool _compressorRunning;

    const float ROLLING_RESISTANCE_COEFICIENT = .0015;
    const float VARIABLE_LOCO_DRAG_COEFICIENT = .0003;
    const float FPS_TO_DCC_FACTOR = 2.5; // was 1.3, then 2.0 (70%)
    const float FPS_TO_MPH_FACTOR = 3600. / 5280;
    const float LOCO_FRICTION_COEFICIENT = .1; // similar to friction coefficient for brakes TBD
    const float MAX_TRACTIVE_FORCE = 75000;    // assumed
    const float MAX_ACCEL = 3.;                // to limit accel on starting movement
};

#endif