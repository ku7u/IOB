#pragma once

#include <stdint.h>
#include "defines.h"
#include "Arduino.h"
#include "ArduinoJson.h"
#include "ILocoStatus.h"
#include "IBrakeSystem.h"
#include "IDccHardware.h"
#include "UdpTransport.h"
#include "TelemetryHandler.h" // TBA
// #include <optional> was for optional parameter functions
// #include <functional> // for callbacks

class Throttle : public ILocoStatus // inherits the interface
{
public:
    Throttle(void);

    // callbacks
    // std::function<void(int, bool)> callbackPushCommand;

    void setBrakeSystem(IBrakeSystem *b) { _brakes = b; }
    void setDccHardware(IDccHardware *d) { _dcc = d; }

    // std::function<void(char *)> callbackCommandDCC;
    // std::function<void(int, int, bool)> callbackThrottleDCC;
    // std::function<void(int, int, int)> callbackSetCvDCC;

    void init(void);
    void loop(void);
    void setControllingIP(IPAddress);
    void getFunctionPrefs(void);
    void getLocoPrefs(void);
    void report(void);
    void udpReport(void);
    void setRoadNumber(int roadNumber);
    void pmOnOff(bool onOff);
    void bell(bool onOff);
    void horn(bool onOff);
    void headlight(int offDimBright);
    void rearlight(int offDimBright);
    void setDirection(int direction);
    void setThrottleLever(int throttleLever);
    void setCarCount(uint16_t carcount);
    void setTonnage(uint16_t tonnage);
    void setTrainData(char *);
    // void setGrade(int grade);
    void manualNotch(bool up);
    void reportNotch(void);
    void longPress(bool up);
    void computeVelocity(void);
    void setLBrake(bool); // loco (independent) brake using BrakeSystem
    // void setIBrake(uint16_t val);
    void setABrake(bool); // automatic (train) brake using BrakeSystem
    // void setTBrake(float val);
    void trainline(bool connect);
    void panicStop(void);
    // void setAirGauge(void);
    int getRoadNumber(void);
    int getDccAddress(void);
    void calibrate(int speed);
    uint16_t interpolateSpeedFactor(float fps);
    bool isForward();
    uint getLastIntCurrentSpeed();
    void muSetState(const char *); // added const for arduinoJson 7.x
    void muSetSpeed(const char *);
    void queryMuTrailer(void);
    void muSetPerformance(const char *);
    void muSubscribe(bool);
    // void muReport(const char *, const char *);
    void muReport(const char *);
    void reportCondition(void);
    void reportStatus(void);
    void reportFunctionLabels(void);
    void setCV(int, int);
    void setFunction(char *);
    bool isRunning(void);
    void setWaypoint(uint8_t, bool); // waypoint ID, direction E=true
    bool inUse() const;              // Getter function (const version is good practice)
    void inUse(bool inUseValue);     // Setter function
    void muMemberCheck();
    void muMemberCheck(bool);
    void muMemberResponse(const char *);

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
    int functionCompressor = 0;
    int functionBrakeSqueal;

private:
    IBrakeSystem *_brakes = nullptr; // One pointer instead of 15 wires
    IDccHardware *_dcc = nullptr;    // One pointer replaces the rest

    uint32_t getTime(void);
    void brakeSqueal(bool);
    void setEBrake(bool);
    void muSumPerformanceValues(void);

    UdpTransport telemetryPort; // declared only
    TelemetryHandler telemetry; // declared only

    enum opModeType
    {
        off,
        idle,
        braking,
        powered
    } _opMode;

    IPAddress _controllingIP; // the app runs here

    bool _inUse = false; // true if some app has latched on to this device
    uint16_t _roadNumber = 3;
    String _locoID;
    String _locoType;
    uint16_t _dccAddress = 3;
    char _leadIpAdr[20];
    enum MuState
    {
        solo = 0,
        lead = 1,
        mid = 2,
        trailing = 3
    };
    MuState _muState; // v0.26 was uint8_t
    String _muLeadLoco;
    bool _muTrailingUnit; // false = mid, true = trailing unit
    bool _muReversed;     // true if running reversed in consist
    bool _muActive;       // false = not MUed

    bool _running = false;
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
    uint16_t _horsepower;   // HP
    uint16_t _muHorsepower; // total HP of mued locos, excluding lead
    uint16_t _horsepowerAtIdle;
    uint16_t _carCount;
    uint32_t _tonnage = 0; // train tons excluding locos
    uint32_t _locoWeight;  // pounds
    uint16_t _locoMass;    // slugs
    uint16_t _muLocoMass;  // total mass of mued locos, excluding lead
    uint32_t _tractiveEffort;
    uint16_t _topSpeed;         // fps gfh add 020525
    uint32_t _muTractiveEffort; // total te of mued locos, excluding lead
    float _lastTractiveForce;
    float _independentBrake; // percent
    float _trainBrake;       // percent
    uint8_t _emergencyBrake; // on or off

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
    bool _consistMember = false;

    JsonDocument muDoc; // holds state of consist v 0.26
    // {"GN123":{"hp":123, "lm":123, "te":123}, {...}, ...}   an object of objects, locoID as key
    // hp=horsepower, lm=loco mass (tons?), te=tractive effort (lbf)

    const int TOPIC_CHAR_SIZE = 200;
    const float ROLLING_RESISTANCE_COEFICIENT = .0020; // was .0015
    const float VARIABLE_LOCO_DRAG_COEFICIENT = .0004; // gfh 020825 was 0.0006 / 021725 was 0005
    const float LOCO_FRICTION_COEFICIENT = .2;         // similar to friction coefficient for brakes gfh 020725 was .1
    const float TRAIN_BRAKE_FRICTION_COEFICIENT = .05; // v027D was .1

    const float FPS_TO_MPH_FACTOR = 3600. / 5280;
    const float FPS_AT_MPH_FACTOR2 = 2 * 5280 / 3600.;
    const float FPS_AT_MPH_FACTOR5 = 5 * 5280 / 3600.;
    const float FPS_AT_MPH_FACTOR10 = 10 * 5280 / 3600.;
    const float FPS_AT_MPH_FACTOR20 = 20 * 5280 / 3600.;
    const float FPS_AT_MPH_FACTOR50 = 50 * 5280 / 3600.;
    const float MAX_TRACTIVE_FORCE = 75000;                       // assumed
    const float MAX_ACCEL = 3.;                                   // to limit accel on starting movement
    const int TRAINLINE_SET_PSI = 90;                             // TBD now in BrakeSystem?
    const float MIN_EFFECTIVE_BRAKE_LINE_PRESSURE = 60;           // TBD now in BrakeSystem?
    const float MIN_EFFECTIVE_EMERGENCY_BRAKE_LINE_PRESSURE = 40; // TBD now in BrakeSystem? could be bogus
    const float EMERGENCY_BRAKE_FACTOR = 1.5;                     // to be more effective brake than automatic brake max which is computed to be 1.0
    const int AVERAGE_CAR_TONNAGE = 75;
};
