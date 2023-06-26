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
    void getFunctionPrefs(void);
    void getLocoPrefs(void);
    void report(void);
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
    void setCarCount(uint16_t carcount);
    void setTonnage(uint16_t tonnage);
    void setHorsepower(int HP);
    void setGrade(int grade);
    void manualNotch(bool up);
    void longPress(bool up);
    void compute(void);
    void computeVelocity(void);
    void setIBrake(uint16_t val);
    void setTBrake(uint16_t val);
    void trainline(bool connect);
    void panicStop(void);
    void setAirGauge(void);
    int getRoadNumber(void);
    void calibrate(int speed);
    bool isForward();
    uint getLastIntCurrentSpeed();

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
    uint32_t getTime(void);

    enum opModeType {off, idle, braking, powered};
    opModeType _opMode;
    uint16_t _roadNumber = 3; // TBD set this programatically
    uint16_t _dccAddress = 3;
    uint16_t _muLeadLoco = 0;
    bool _running = false;
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
    long _locoMass; // slugs
    uint32_t _tractiveEffort;
    float _lastTractiveForce;
    uint16_t _independentBrake; // percent
    // uint16_t _iBrakeVal = 0;
    uint16_t _trainBrake; // percent
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
    uint32_t _lastShutdownTime;
    float _trainlinePSI;
    uint16_t _trainlineSetPSI;
    bool _trainlineConnected;
    uint16_t _lastIntCurrentPsi;
    bool _compressorRunning;
    uint16_t _compressorCountdown;
    uint32_t _startTimestamp;
    long _calibrationTimer;
    int _calibrationTrapLength;
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
    struct functionParms
    {
        int functionID;
        bool onOff;
    };

    const float ROLLING_RESISTANCE_COEFICIENT = .0015;
    const float VARIABLE_LOCO_DRAG_COEFICIENT = .0003;
    const float FPS_TO_MPH_FACTOR = 3600. / 5280;

    const float FPS_AT_MPH_FACTOR2 = 2 * 5280 / 3600.;
    const float FPS_AT_MPH_FACTOR5 = 5 * 5280 / 3600.;
    const float FPS_AT_MPH_FACTOR10 = 10 * 5280 / 3600.;
    const float FPS_AT_MPH_FACTOR20 = 20 * 5280 / 3600.;
    const float FPS_AT_MPH_FACTOR50 = 50 * 5280 / 3600.;
    const float LOCO_FRICTION_COEFICIENT = .1; // similar to friction coefficient for brakes TBD
    const float MAX_TRACTIVE_FORCE = 75000;    // assumed
    const float MAX_ACCEL = 3.;                // to limit accel on starting movement
    const int TRAINLINE_SET_PSI = 75;
};

#endif