// #define speeddebug

#include "Throttle.h"
#include "SerialCommand.h"
#include "Function.h"
#include "Arduino.h"
#include "MQTT.h"
#include "Preferences.h"
#include "PubSubClient.h"
#include "WebSerial.h"

extern PubSubClient client;

// Constructor
// Throttle::Throttle(int roadNumber)
Throttle::Throttle(void)
{
    _notch = 0;
    _currentSpeed = 0;
    _lastCurrentSpeed = 0;
    _mph = 0;
    _lastNotch = 0;
    _horsepowerAtIdle = 15;

    _direction = true; // forward
    _throttleLever = 0;
    _carCount = 0;
    _tonnage = 0;
    _trainlinePSI = rand() % TRAINLINE_SET_PSI; // random value between 0 and 75
    _trainlineSetPSI = TRAINLINE_SET_PSI;
    _neutral = true;
    // _iBrakeVal = 0;
    // _trainBrake = 0;
    // _manualNotchingMode = false;
    _lastIntCurrentSpeed = 0;
    _lastTractiveForce = 0;
    _lastIntCurrentPsi = 0;
    _running = false;
    _compressorRunning = false;
    _compressorCountdown = 0;

    // getLocoPrefs();
    // _locoMass = _locoWeight / 32; // poundals

    // getFunctionPrefs();
}

void Throttle::init()
{
    setFunction(functionNotchingEnable, 1);
    setFunction(functionNotchUp, 0);
    setFunction(functionNotchDown, 0);
}

void Throttle::getLocoPrefs(void)
{
    Preferences myPrefs;

    myPrefs.begin("loco");
    _roadNumber = myPrefs.getInt("roadnum", 16);
    // // myPrefs.getBool("shortLong", 0);
    _horsepower = myPrefs.getInt("horsepower", 1500);
    _locoWeight = myPrefs.getUInt("locoweight", 250000);
    _tractiveEffort = myPrefs.getUInt("tractiveeffort", 70000.); // TBD why float?
    _odometer = myPrefs.getFloat("odometer", 0.0);
    myPrefs.end();
    _locoMass = _locoWeight / 32; // poundals

    myPrefs.begin("calibration", true);
    _calibrationTrapLength = myPrefs.getInt("traplength", 3);
    _fpsDccFactorForward2 = myPrefs.getFloat("speed2forward", 2.);
    _fpsDccFactorForward5 = myPrefs.getFloat("speed5forward", 2.);
    _fpsDccFactorForward10 = myPrefs.getFloat("speed10forward", 2.);
    _fpsDccFactorForward20 = myPrefs.getFloat("speed20forward", 2.);
    _fpsDccFactorForward50 = myPrefs.getFloat("speed50forward", 2.);
    _fpsDccFactorReverse2 = myPrefs.getFloat("speed2reverse", 2.);
    _fpsDccFactorReverse5 = myPrefs.getFloat("speed5reverse", 2.);
    _fpsDccFactorReverse10 = myPrefs.getFloat("speed10reverse", 2.);
    _fpsDccFactorReverse20 = myPrefs.getFloat("speed20reverse", 2.);
    _fpsDccFactorReverse50 = myPrefs.getFloat("speed50reverse", 2.);
    myPrefs.end();

    myPrefs.begin("general", true);
    _feedbackTopic = myPrefs.getString("feedbacktopic", "tlm/");
    myPrefs.end();
}

void Throttle::getFunctionPrefs(void)
{
    Preferences myPrefs;

    myPrefs.begin("functions");
    functionPM = myPrefs.getInt("pm", 28);
    functionBell = myPrefs.getInt("bell", 1);
    functionHorn = myPrefs.getInt("horn", 2);
    functionHeadlightBright = myPrefs.getInt("headlightBright", 0);
    functionHeadlightDim = myPrefs.getInt("headlightDim", 6);
    functionRearlightBright = myPrefs.getInt("rearlightBright", 7);
    functionRearlightDim = myPrefs.getInt("rearlightDim", 10);
    functionNotchingEnable = myPrefs.getInt("notchingEnable", 25);
    functionNotchUp = myPrefs.getInt("notchUp", 26);
    functionNotchDown = myPrefs.getInt("notchDown", 27);
    functionIndependentBrake = myPrefs.getInt("iBrake", 5);
    functionTrainBrake = myPrefs.getInt("tBrake", 4);
    functionEmergencyBrake = myPrefs.getInt("emergencyBrake", 5);
    functionCompressor = myPrefs.getInt("compressor", 20);
    myPrefs.end();
}

void Throttle::setRoadNumber(int roadNumber)
{
    _roadNumber = roadNumber;
}

int Throttle::getRoadNumber(void)
{
    return _roadNumber;
}

bool Throttle::isForward()
{
    if (_direction)
        return true;
    else
        return false;
}

void Throttle::pmOnOff(bool onOff)
{
    Preferences myPrefs;
    _running = onOff;
    if (!onOff) // save the mileage
    {
        myPrefs.begin("loco", false);
        myPrefs.putFloat("odometer", _odometer);
        myPrefs.end();
    }
    setFunction(functionPM, onOff);
    setFunction(functionNotchingEnable, onOff); // TBD can't turn off PM unless this is here WMNS
    // TBD ETL has set notchup, down functions to off here
}

void Throttle::headlight(int offDimBright)
{
    if (offDimBright == 0)
    {
        setFunction(functionHeadlightDim, false);
        setFunction(functionHeadlightBright, false);
    }
    else if (offDimBright == 1)
    {
        setFunction(functionHeadlightDim, true);
        setFunction(functionHeadlightBright, false);
    }
    else if (offDimBright == 2)
    {
        setFunction(functionHeadlightDim, false);
        setFunction(functionHeadlightBright, true);
    }
}

void Throttle::rearlight(int offDimBright)
{
    if (offDimBright == 0)
    {
        setFunction(functionRearlightDim, false);
        setFunction(functionRearlightBright, false);
    }
    else if (offDimBright == 1)
    {
        setFunction(functionRearlightDim, true);
        setFunction(functionRearlightBright, false);
    }
    else if (offDimBright == 2)
    {
        setFunction(functionRearlightDim, false);
        setFunction(functionRearlightBright, true);
    }
}

void Throttle::panicStop()
{
    char dummyChars[31];
    // TBD this - likely doesn't match ETL
    String dummyString = "t 1 ";
    dummyString.concat(String(_roadNumber) + " ");
    dummyString.concat("0");

    strcpy(dummyChars, dummyString.c_str());
    SerialCommand::parse(dummyChars);

    _currentSpeed = 0;
    while (_notch > 0)
    {
        manualNotch(false);
        delay(100);
    }
}

void Throttle::bell(bool onOff)
{
    setFunction(functionBell, onOff);
}

void Throttle::horn(bool onOff)
{
    setFunction(functionHorn, onOff);
}

void Throttle::setThrottleLever(int throttleLever)
{
    // _throttleLever = throttleLever;
    // Throttle::compute();
}

void Throttle::setDirection(int direction)
{
    _neutral = false;
    _direction = true;

    if (direction == 0)
        _direction = false;
    else if (direction == 2)
        _direction = true;
    else
        _neutral = true;
    // TBD doesn't match ETL but seems to work as is
}

void Throttle::setCarCount(uint16_t carcount)
{
    // _trainlinePSI = 0;
    _carCount = carcount;
    _tonnage = carcount * 50;
}

void Throttle::setTonnage(uint16_t tonnage)
{
    _tonnage = tonnage;
}

void Throttle::setIBrake(uint16_t val)
{
    _independentBrake = val;

    if (val > 0)
        setFunction(functionIndependentBrake, true);
    else
        setFunction(functionIndependentBrake, false);
}

void Throttle::setTBrake(uint16_t val)
{

    if (val > 0)
    {
        _trainlinePSI -= val;
        _trainlineSetPSI = _trainlinePSI;
        if (_trainlinePSI < 0)
            _trainlinePSI = 0;
        setFunction(functionTrainBrake, true);
    }
    else
    {
        _trainlineSetPSI = TRAINLINE_SET_PSI;
        setFunction(functionTrainBrake, false);
    }
}

void Throttle::trainline(bool connect)
// TBD this has to be linked to tbrake
//  also should respond to increase in car count rather than total cars if already connected
{
    if (connect)
    {
        _trainlineConnected = true;
        _trainlinePSI = _trainlinePSI - _carCount * 1.5; // TBD totally made this up
        if (_trainlinePSI <= 0)
            _trainlinePSI = 0;
    }
    else
        _trainlineConnected = false;
}

//-------------------------------------------------------------------------------
// use manual notching to control PM sound, throttle for movement
// this routine just sets the notch to be later processed in computeVelocity
void Throttle::manualNotch(bool up)
{
    static int currentNotch = 0;
    uint32_t now;

    now = millis();

    if (now - _lastCommandTime < 250)
        return;
    else
        _lastCommandTime = now;

    if (up) // notching up
    {
        if (currentNotch == 8)
            return;
        setFunction(functionNotchUp, true);
        currentNotch++;
    }
    else // notching down
    {
        if (currentNotch == 0)
            return;
        setFunction(functionNotchDown, true);
        currentNotch--;
    }

    _notch = currentNotch;

    // String throttleFeedback = "IOB/" + String(_roadNumber) + "/feedback/notch";
    String throttleFeedback = _feedbackTopic + "notch";
    String glarb = String(currentNotch);
    client.publish(throttleFeedback.c_str(), glarb.c_str());

    delay(100);
    if (up)
        setFunction(_notchUpFunction, false); // TBD this is really a kludge as workaround for timing issue
    else
        setFunction(_notchDownFunction, false);
}

//-------------------------------------------------------------------------------
void Throttle::computeVelocity(void)
{
    float effectiveHP;
    float tractiveForce;
    float dragForce;
    float variableLocoDragForce;
    float startingForce;
    float gradeForce;
    float independentBrakeForce;
    float trainBrakeForce;
    float accel;
    char dummyChars[31];
    uint16_t intCurrentSpeed;
    static uint16_t intSpeedoSpeed;
    static uint16_t lastIntCurrentSpeed;
    float speedoSpeed;
    float factorF;
    float factorR;
    String feedbackPrefix;

    setAirGauge();

    if (_neutral)
        return; // if in neutral don't waste time in here

    if (_notch == 1)
        effectiveHP = _horsepowerAtIdle;
    else
        effectiveHP = (_horsepower * (_notch - 1) / 7) - 50;

    if (effectiveHP < 0)
        effectiveHP = 0;

#ifdef speeddebug
    Serial.print("_mph  ");
    Serial.println(_mph);
#endif

    if (_mph <= 0)
        tractiveForce = effectiveHP * 308;
    else
    {
        tractiveForce = effectiveHP * 308 / _mph;
        if (tractiveForce > 37000) // TBD this should be continuous tractive effort parameter
            tractiveForce = 37000;
    }

    if (tractiveForce > _tractiveEffort)
        tractiveForce = _tractiveEffort;

    // reduce tractive force by the starting force effect only when starting
    if (_currentSpeed <= 0)
    {
        dragForce = 0;
        startingForce = 20 * ((_locoMass * 32 / 2000) + _tonnage); // convert from poundals to lbs to tons
        if (startingForce >= tractiveForce)
            tractiveForce = 0;
        else
            tractiveForce -= startingForce;
    }
    else
    // compute the moving drag force for any rolling stock
    {
        startingForce = 0;
        dragForce = (_locoMass * 32 * ROLLING_RESISTANCE_COEFICIENT) + (_tonnage * 2000 * ROLLING_RESISTANCE_COEFICIENT);
    }
#ifdef speeddebug
    Serial.print("_tonnage ");
    Serial.println(_tonnage);
    Serial.print("_locoMass  ");
    Serial.println(_locoMass);
    Serial.print("starting force ");
    Serial.println(startingForce);
#endif

    if (tractiveForce > 1.25 * _lastTractiveForce)
        tractiveForce = _lastTractiveForce + .15 * tractiveForce; // TBD was .25

    _lastTractiveForce = tractiveForce;

#ifdef speeddebug
    Serial.print("tractiveForce ");
    Serial.println(tractiveForce);
#endif

    // there must be some drag effect that varies with speed that is peculiar to locos
    variableLocoDragForce = _locoMass * 32 * _currentSpeed * VARIABLE_LOCO_DRAG_COEFICIENT;

    // consider brake forces if any
    independentBrakeForce = (_independentBrake / 100.) * _locoMass * 32 * LOCO_FRICTION_COEFICIENT;
    // trainBrakeForce = (_trainBrake / 100.) * _tonnage * 2000 * .2;
    if (_trainlineConnected)
        trainBrakeForce = (TRAINLINE_SET_PSI - _trainlinePSI) / TRAINLINE_SET_PSI * _tonnage * 2000 * .2;
    else
        trainBrakeForce = 0;

#ifdef speeddebug
    Serial.print("dragForce ");
    Serial.println(dragForce);
#endif

    // independentBrakeForce = _independentBrake / 100. * _locoMass * 32 * factorZ;
    // trainBrakeForce = _trainBrake / 100. * _tonnage * 2000 * .2;

#ifdef speeddebug
    Serial.print("independentBrake and ...Force ");
    Serial.print(_independentBrake);
    Serial.print("   ");
    Serial.println(independentBrakeForce);
#endif

    // accel = (tractiveForce - dragForce - independentBrakeForce) / (_locoMass + _tonnage * 2000 / 32);
    accel = (tractiveForce - dragForce - variableLocoDragForce - independentBrakeForce - trainBrakeForce) / (_locoMass + (_tonnage * 2000 / 32));
    if (accel > MAX_ACCEL)
        accel = MAX_ACCEL;

#ifdef speeddebug
    Serial.print("accel ");
    Serial.println(accel);
#endif

    _currentSpeed = _currentSpeed + accel; // accel is feet/sec/sec so if integrated once / sec, accel = vel
    if (_currentSpeed < 0)
        _currentSpeed = 0;

    _odometer = _odometer + abs(_currentSpeed); // TBD on this

#ifdef speeddebug
    Serial.print("_currentSpeed ");
    Serial.println(_currentSpeed);
#endif

    // get the appropriate calibrated speed compensation value
    if (_currentSpeed <= FPS_AT_MPH_FACTOR2)
    {
        factorF = _fpsDccFactorForward2;
        factorR = _fpsDccFactorReverse2;
    }
    else if (_currentSpeed <= FPS_AT_MPH_FACTOR5)
    {
        factorF = _fpsDccFactorForward5;
        factorR = _fpsDccFactorReverse5;
    }
    else if (_currentSpeed <= FPS_AT_MPH_FACTOR10)
    {
        factorF = _fpsDccFactorForward10;
        factorR = _fpsDccFactorReverse10;
    }
    else if (_currentSpeed <= FPS_AT_MPH_FACTOR20)
    {
        factorF = _fpsDccFactorForward20;
        factorR = _fpsDccFactorReverse20;
    }
    else
    {
        factorF = _fpsDccFactorForward50;
        factorR = _fpsDccFactorReverse50;
    }

    if (_direction)
        intCurrentSpeed = _currentSpeed * factorF;
    else
        intCurrentSpeed = _currentSpeed * factorR;

#ifdef speeddebug
    Serial.print("_currentSpeed ");
    Serial.println(intCurrentSpeed);
    Serial.print('\n');
#endif

    if (intCurrentSpeed != lastIntCurrentSpeed)
    {
        _mph = _currentSpeed * FPS_TO_MPH_FACTOR; // TBD fix this
        speedoSpeed = _mph;
        intSpeedoSpeed = speedoSpeed;
        lastIntCurrentSpeed = intCurrentSpeed;

        // send back speedometer data to operator
        // String speedFeedback = "IOB/" + String(_roadNumber) + "/feedback/speed";
        String speedFeedback = _feedbackTopic + "speed";
        String glarb = String(intSpeedoSpeed); // TBD fix this
        client.publish(speedFeedback.c_str(), glarb.c_str());

        // build the command string
        String dummyString = "t 1 ";
        dummyString.concat(String(_roadNumber) + " ");
        dummyString.concat(String(intCurrentSpeed) + " ");
        if (_direction)
            dummyString.concat("1");
        else
            dummyString.concat("0");

        strcpy(dummyChars, dummyString.c_str());

        SerialCommand::parse(dummyChars);
    }

    // send back odometer data to operator
    // TBD maybe send this once on startup and/or shutdown as well
    if (_currentSpeed != 0)
    {
        // String odometerFeedback = "IOB/" + String(_roadNumber) + "/feedback/odometer";
        String odometerFeedback = _feedbackTopic + "odometer";
        String odometerString = String(_odometer);
        client.publish(odometerFeedback.c_str(), odometerString.c_str());
    }
}

void Throttle::setAirGauge(void)
{
    int intCurrentPsi;
    static int lastIntCurrentPsi;

    if ((_trainlineSetPSI > _trainlinePSI) && (_running))
    {
        if (!_compressorRunning)
        {
            _compressorRunning = true;
            setFunction(functionCompressor, 1);
            // TBD maybe countdown here
        }
        _trainlinePSI += (.3 * (_notch + 1));
    }

    if ((_trainlineSetPSI <= _trainlinePSI) && _running && _compressorRunning)
    {
        _compressorRunning = false;
        setFunction(functionCompressor, 0);
    }

    if (_trainlinePSI > _trainlineSetPSI)
        _trainlinePSI = _trainlineSetPSI;
    else if (_trainlineSetPSI == 0)
        _trainlinePSI = 0;
    else if (_trainlineSetPSI < _trainlinePSI)
        _trainlinePSI -= 3;

    if (_trainlineSetPSI < _trainlinePSI)
        _trainlinePSI = _trainlineSetPSI;

    intCurrentPsi = int(_trainlinePSI);

    if (intCurrentPsi != lastIntCurrentPsi)
    {
        lastIntCurrentPsi = intCurrentPsi;
        // String speedFeedback = "IOB/" + String(_roadNumber) + "/feedback/trainline";
        String speedFeedback = _feedbackTopic + "trainline";
        Serial.print("in air gauge ");
        Serial.println(speedFeedback);
        String glarb = String(intCurrentPsi);
        client.publish(speedFeedback.c_str(), glarb.c_str());
    }
}

void Throttle::calibrate(int speed)
{
    Preferences myPrefs;
    char dummyChars[31];
    long calibrationPeriod;
    int trapLength;
    long targetTime;
    float newFactor;
    float factorF;
    float factorR;
    int dccVal;

    if (abs(speed) == 2)
    {
        factorF = _fpsDccFactorForward2;
        factorR = _fpsDccFactorReverse2;
    }
    else if (abs(speed) == 5)
    {
        factorF = _fpsDccFactorForward5;
        factorR = _fpsDccFactorReverse5;
    }
    else if (abs(speed) == 10)
    {
        factorF = _fpsDccFactorForward10;
        factorR = _fpsDccFactorReverse10;
    }
    else if (abs(speed) == 20)
    {
        factorF = _fpsDccFactorForward20;
        factorR = _fpsDccFactorReverse20;
    }
    else
    {
        factorF = _fpsDccFactorForward50;
        factorR = _fpsDccFactorReverse50;
    }

    // if (speed == 0)
    if (_calibrationStage == 2)
    {
        trapLength = _calibrationTrapLength;
        trapLength = 2; // TBD remove this
        calibrationPeriod = millis() - _calibrationTimer;
        // compute target time in ms to traverse test section at this speed point
        targetTime = 1000 * trapLength * 87 / (abs(speed) * (5280 / 3600));
        // divide calibrationPeriod by target value
        // multiply result by existing FPS_TO_DCC_FACTOR
        if (speed > 0)
            newFactor = factorF * calibrationPeriod / targetTime;
        else
            newFactor = factorR * calibrationPeriod / targetTime;

        myPrefs.begin("calibration", false);
        if (abs(speed) == 2)
        {
            if (speed > 0)
                myPrefs.putFloat("speed2forward", newFactor);
            else
                myPrefs.putFloat("speed2reverse", newFactor);
        }
        else if (abs(speed) == 5)
        {
            if (speed > 0)
                myPrefs.putFloat("speed5forward", newFactor);
            else
                myPrefs.putFloat("speed5reverse", newFactor);
        }
        else if (abs(speed) == 10)
        {
            if (speed > 0)
                myPrefs.putFloat("speed10forward", newFactor);
            else
                myPrefs.putFloat("speed10reverse", newFactor);
        }
        else if (abs(speed) == 20)
        {
            if (speed > 0)
                myPrefs.putFloat("speed20forward", newFactor);
            else
                myPrefs.putFloat("speed20reverse", newFactor);
        }
        else if (abs(speed) == 50)
        {
            if (speed > 0)
                myPrefs.putFloat("speed50forward", newFactor);
            else
                myPrefs.putFloat("speed50reverse", newFactor);
        }
        myPrefs.end();

        setFunction(functionBell, false);
        getLocoPrefs(); // read storage into variables
    }
    else if (_calibrationStage == 1)
    {
        _calibrationTimer = millis();
        setFunction(functionBell, true);
    }

    if (_calibrationStage != 1) // either starting movement or stopping (0 or 2)
    {
        String dummyString = "t 1 ";
        dummyString.concat(String(_roadNumber) + " ");
        if (_calibrationStage == 0)
        {
            if (speed > 0)
                dccVal = abs(speed * factorF / FPS_TO_MPH_FACTOR);
            else
                dccVal = abs(speed * factorR / FPS_TO_MPH_FACTOR);

            dummyString.concat(String(dccVal) + " "); // starting
        }
        else
            dummyString.concat(String(0) + " "); // end of movement

        if (speed >= 0)
            dummyString.concat("1");
        else
            dummyString.concat("0");

        strcpy(dummyChars, dummyString.c_str());
        SerialCommand::parse(dummyChars);
    }

    if (++_calibrationStage == 3)
        _calibrationStage = 0;
}
