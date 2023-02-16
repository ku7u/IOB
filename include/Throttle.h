#ifndef THROTTLE_H
#define THROTTLE_H

#include <stdint.h>
#include "Arduino.h"

class Throttle
{
public:
    Throttle(int roadNumber);

    void setRoadNumber(int roadNumber);
    void bell(bool onOff);
    void horn(bool onOff);
    void headlight(int offDimBright);
    void startPM(void);
    void stopPM(void);
    void setDirection(int direction);
    void setThrottleLever(int throttleLever);
    void setMass(uint16_t mass);
    void setHorsepower(int HP);
    void setGrade(int grade);
    void manualNotch(bool up);
    void compute(void);
    void computeVelocity(void);
    void setIBrake(uint16_t val);
    void setTBrake(uint16_t val);
    void setAirGauge(void);

private:
    uint16_t    _roadNumber = 16;   // TBD set this programatically
    bool        _running = false;
    bool        _direction; // true = forward
    uint16_t    _throttleLever;
    uint16_t    _notch;
    bool        _neutral = false;
    float       _currentSpeed;
    uint16_t    _targetSpeed;
    float       _speedFactor = 0.5; // multiply by commanded speed to get real mph
    float       _speedoCalFactor = .250;    // calibrate speedometer
    float       _divisor;
    uint16_t    _horsepower = 1500;     // HP
    uint16_t    _tonnage = 0;           // tons
    uint16_t    _locoMass = 300000/32;  // slugs
    uint16_t    _independentBrake;      // percent
    uint16_t    _trainBrake;            // percent
    enum        _mode {MAN_NOTCHING, DRIVE_HOLD, NORMAL};
    bool        _manualNotching;    // true if using manual notching
    uint16_t    _manualNotchingLogicFunction;
    uint16_t    _notchUpFunction = 26;
    uint16_t    _notchDownFunction = 27;
    uint16_t    _driveHoldFunction;
    uint32_t    _lastCommandTime;
    float       _trainlinePSI;
    uint16_t    _trainlineSetPSI;

};

#endif