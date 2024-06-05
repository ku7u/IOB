// #ifndef THROTTLE_H
// #define THROTTLE_H
#pragma once

#include <stdint.h>
#include "Arduino.h"

class Throttle
{
public:
    // Throttle(int roadNumber);
    Throttle(void);

    void init(void);
    void getFunctionPrefs(void);
    void getLocoPrefs(void);
    void report(void);
    void setRoadNumber(int roadNumber);
    void pmOnOff(bool onOff);
    void bell(bool onOff);
    void horn(bool onOff);
    void headlight(int offDimBright);
    void rearlight(int offDimBright);
    // void startPM(void);
    // void stopPM(void);
    void setDirection(int direction);
    void setThrottleLever(int throttleLever);
    void setCarCount(uint16_t carcount);
    void setTonnage(uint16_t tonnage);
    // void setHorsepower(int HP);
    // void setGrade(int grade);
    void manualNotch(bool up);
    void longPress(bool up);
    // void compute(void);
    void computeVelocity(void);
    void setIBrake(uint16_t val);
    void setTBrake(float val);
    void trainline(bool connect);
    void panicStop(void);
    void setAirGauge(void);
    int getRoadNumber(void);
    int getDccAddress(void);
    void calibrate(int speed);
    uint16_t interpolateSpeedFactor(float fps);
    bool isForward();
    uint getLastIntCurrentSpeed();
    void setMuState(char *);
    void setMuStateFromLead(char *);
    // void setMuSpeed(float);
    void setMuSpeed(char *);
    void setMuPerformance(char *);
    void muSubscribe();
    void reportCondition(void);
    void reportStatus(void);
    void reportFunctionLabels(void);
    void setCV(int, int);
    void setFunction(char *);

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
    int functionBrakeSqueal;

private:
    uint32_t getTime(void);
    void brakeSqueal(bool);
    void setEBrake(bool);

    enum opModeType {off, idle, braking, powered};
    opModeType _opMode;
    uint16_t _roadNumber = 3;
    String _locoID;
    String _locoType;
    uint16_t _dccAddress = 3;
    uint8_t _muState;
    // const char * _muLeadLoco;
    String _muLeadLoco;
    bool _muTrailingUnit; // false = mid, true = trailing unit
    bool _muReversed; // true if running reversed in consist
    bool _muActive; // false = not MUed
    bool _running = false;
    // bool _hornState = false;
    uint _headlight;
    uint _rearlight;
    bool _bell;
    bool _direction; // true = forward
    uint16_t _throttleLever;
    uint16_t _notch;
    uint16_t _lastNotch;
    bool _neutral = true;
    float _currentSpeed;
    float _lastCurrentSpeed;
    uint16_t _lastIntCurrentSpeed;
    uint16_t _targetSpeed;
    float _mph;
    float _odometer;
    float _speedFactor = 0.5;      // multiply by commanded speed to get real mph
    float _speedoCalFactor = .250; // calibrate speedometer
    int _calibrationStage = 0;
    float _divisor;
    uint16_t _horsepower; // HP
    uint16_t _horsepowerAtIdle;
    uint16_t _carCount;
    uint32_t _tonnage = 0; // tons
    uint32_t _locoWeight;
    // long _locoMass; // slugs
    uint16_t _locoMass; // slugs
    uint32_t _tractiveEffort;
    float _lastTractiveForce;
    uint16_t _independentBrake; // percent
    // uint16_t _iBrakeVal = 0;
    uint16_t _trainBrake; // percent
    uint8_t _emergencyBrake; // on or off
    enum _mode
    {
        MAN_NOTCHING,
        DRIVE_HOLD,
        NORMAL
    };
    // bool _manualNotching; // true if using manual notching TBD this
    // bool _manualNotchingMode = false;
    uint16_t _manualNotchingLogicFunction;
    uint16_t _notchUpFunction = 26;
    uint16_t _notchDownFunction = 27;
    uint16_t _driveHoldFunction;
    uint32_t _lastCommandTime;
    uint32_t _lastShutdownTime;
    float _trainlinePSI;
    uint16_t _trainlineSetPSI;
    bool _trainlineConnected;
    uint16_t _lastIntCurrentPsi;
    bool _compressorRunning;
    uint16_t _compressorCountdown;
    uint32_t _startTimestamp;
    long _calibrationTimer;
    const int _calibrationTrapLength = 3;
    const int _calibrationTrapLength2 = 1;
    const int _calibrationTrapLength5 = 2;
    const int _calibrationTrapLength10 = 3;
    const int _calibrationTrapLength20 = 4;
    const int _calibrationTrapLength50 = 5;
    float _fpsDccFactorForward2;
    float _fpsDccFactorForward5;
    float _fpsDccFactorForward10;
    float _fpsDccFactorForward20;
    float _fpsDccFactorForward50;
    float _fpsDccFactorReverse2;
    float _fpsDccFactorReverse5;
    float _fpsDccFactorReverse10;
    float _fpsDccFactorReverse20;
    float _fpsDccFactorReverse50;
    String _feedbackTopic;
    String _commandTopic;
    struct functionParms
    {
        int functionID;
        bool onOff;
    };

    const float ROLLING_RESISTANCE_COEFICIENT = .0020;  // was .0015
    const float VARIABLE_LOCO_DRAG_COEFICIENT = .000; // was .0003
    const float FPS_TO_MPH_FACTOR = 3600. / 5280;

    const float FPS_AT_MPH_FACTOR2 = 2 * 5280 / 3600.;
    const float FPS_AT_MPH_FACTOR5 = 5 * 5280 / 3600.;
    const float FPS_AT_MPH_FACTOR10 = 10 * 5280 / 3600.;
    const float FPS_AT_MPH_FACTOR20 = 20 * 5280 / 3600.;
    const float FPS_AT_MPH_FACTOR50 = 50 * 5280 / 3600.;
    const float LOCO_FRICTION_COEFICIENT = .1; // similar to friction coefficient for brakes TBD
    const float TRAIN_BRAKE_FRICTION_COEFICIENT = .1;  // v 0.15 was .2
    const float MAX_TRACTIVE_FORCE = 75000;    // assumed
    const float MAX_ACCEL = 3.;                // to limit accel on starting movement
    const float MIN_EFFECTIVE_BRAKE_LINE_PRESSURE = 60;
    const float MIN_EFFECTIVE_EMERGENCY_BRAKE_LINE_PRESSURE = 40; // TBD likely bogus
    const float EMERGENCY_BRAKE_FACTOR = 1.5;  // to be more effective brake than automatic brake max which is computed to be 1.0
    const int AVERAGE_CAR_TONNAGE = 75;
    const int TRAINLINE_SET_PSI = 90;
};
