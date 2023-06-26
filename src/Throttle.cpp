#define speeddebug

#include "Throttle.h"
#include "SerialCommand.h"
#include "Function.h"
#include "Arduino.h"
#include "MQTT.h"
#include "Preferences.h"
#include "PubSubClient.h"
#include "WebSerial.h"
#include "Fifo.h"

extern PubSubClient client;
extern Fifo commandFifo;

// Constructor
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
    // _trainlinePSI = rand() % TRAINLINE_SET_PSI; // random value between 0 and 75
    _trainlineSetPSI = TRAINLINE_SET_PSI;
    _neutral = true;
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
    commandFifo.pushCommand(functionNotchingEnable, true);
    commandFifo.pushCommand(functionNotchUp, 0);
    commandFifo.pushCommand(functionNotchDown, 0);
}

void Throttle::getLocoPrefs(void)
{
    Preferences myPrefs;

    myPrefs.begin("loco");
    _dccAddress = myPrefs.getInt("dccaddress", 3);
    _roadNumber = myPrefs.getInt("roadnum", 0);
    // // myPrefs.getBool("shortLong", 0);
    _horsepower = myPrefs.getInt("horsepower", 1500);
    _locoWeight = myPrefs.getULong("locoweight", 250000);
    _tractiveEffort = myPrefs.getLong("tractiveeffort", 70000.); // TBD why float?
    _odometer = myPrefs.getFloat("odometer", 0.0);
    _muLeadLoco = myPrefs.getUInt("mulead", 0);
    myPrefs.end();
    _locoMass = _locoWeight / 32; // slugs

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
    _feedbackTopic = myPrefs.getString("feedbacktopic", "tlm/ols/") + String(_roadNumber) + "/";
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

void Throttle::report()
{
    String reportTopic = _feedbackTopic + "report";
    String x = "{id:";
    x.concat(String (getRoadNumber()));
    x.concat(",ip:");
    x.concat(WiFi.localIP().toString());
    x.concat("}");
    client.publish(reportTopic.c_str(), x.c_str());
}

uint32_t Throttle::getTime()
// try to get GMT and return as unix time variable
{
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        // Serial.println("Failed to obtain time");
        return (0);
    }
    time(&now);
    return now;
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

uint Throttle::getLastIntCurrentSpeed()
{
    return _lastIntCurrentSpeed;
}

void Throttle::pmOnOff(bool onOff)
{
    Preferences myPrefs;
    uint32_t thisStartupTime;
    uint32_t deltaTime;

    _running = onOff;
    if (!onOff) // save the mileage
    {
        myPrefs.begin("loco", false);
        myPrefs.putFloat("odometer", _odometer);
        myPrefs.putULong("lastshuttime", getTime());
        myPrefs.end();
        _opMode = off;
    }
    else
    {
        _opMode = idle;
        _startTimestamp = millis();

        // find the elapsed time since last shutdown
        thisStartupTime = getTime();
        myPrefs.begin("loco", false);
        _lastShutdownTime = myPrefs.getULong("lastshuttime", 0);
        if (_lastShutdownTime > thisStartupTime)
            deltaTime = 0;
        else
            deltaTime = thisStartupTime - _lastShutdownTime;
        myPrefs.end();

        // _trainlinePSI = rand() % TRAINLINE_SET_PSI; // random value between 0 and 75, TBD this should depend on how long we have been shut down
        if (deltaTime > 48 * 3600)
            _trainlinePSI = 0;
        else if (deltaTime <= 0)
            _trainlinePSI = TRAINLINE_SET_PSI;
        else
            // bleeds down to zero after 48 hours
            _trainlinePSI = (1 - (deltaTime / (48. * 3600))) * TRAINLINE_SET_PSI;
    }

    commandFifo.pushCommand(functionPM, onOff);
    commandFifo.pushCommand(functionNotchingEnable, onOff); // TBD can't turn off PM unless this is here WMNS
}

void Throttle::headlight(int offDimBright)
{
    if (offDimBright == 0)
    {
        commandFifo.pushCommand(functionHeadlightDim, false);
        commandFifo.pushCommand(functionHeadlightBright, false);
    }
    else if (offDimBright == 1)
    {
        commandFifo.pushCommand(functionHeadlightDim, true);
        commandFifo.pushCommand(functionHeadlightBright, false);
    }
    else if (offDimBright == 2)
    {
        commandFifo.pushCommand(functionHeadlightDim, false);
        commandFifo.pushCommand(functionHeadlightBright, true);
    }
}

void Throttle::rearlight(int offDimBright)
{
    if (offDimBright == 0)
    {
        commandFifo.pushCommand(functionRearlightDim, false);
        commandFifo.pushCommand(functionRearlightBright, false);
    }
    else if (offDimBright == 1)
    {
        commandFifo.pushCommand(functionRearlightDim, true);
        commandFifo.pushCommand(functionRearlightBright, false);
    }
    else if (offDimBright == 2)
    {
        commandFifo.pushCommand(functionRearlightDim, false);
        commandFifo.pushCommand(functionRearlightBright, true);
    }
}

void Throttle::panicStop()
{
    char dummyChars[31];
    // TBD this - likely doesn't match ETL
    String dummyString = "t 1 ";
    dummyString.concat(String(_dccAddress) + " ");
    dummyString.concat("0");

    strcpy(dummyChars, dummyString.c_str());
    SerialCommand::parse(dummyChars);

    _currentSpeed = 0;
    while (_notch > 0)
    {
        manualNotch(false);
    }
}

void Throttle::bell(bool onOff)
{
    if (_running)
        commandFifo.pushCommand(functionBell, onOff);
}

void Throttle::horn(bool onOff)
{
    if (_running)
        commandFifo.pushCommand(functionHorn, onOff);
}

void Throttle::setThrottleLever(int throttleLever)
{
    while (throttleLever != _notch)
    {
        if (throttleLever > _notch)
            manualNotch(true);
        else
            manualNotch(false);
    }
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
    {
        _opMode = braking;
        commandFifo.pushCommand(functionIndependentBrake, true);
    }
    else
    {
        _opMode = idle;
        commandFifo.pushCommand(functionIndependentBrake, false);
    }
}

void Throttle::setTBrake(uint16_t val)
{

    if (val > 0)
    {
        // _trainlinePSI -= val;
        // _trainlineSetPSI = _trainlinePSI;
        _trainlineSetPSI = val;
        if (_trainlinePSI < 0)
            _trainlinePSI = 0;
        commandFifo.pushCommand(functionTrainBrake, true);
    }
    else
    {
        _trainlineSetPSI = TRAINLINE_SET_PSI;
        commandFifo.pushCommand(functionTrainBrake, false);
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

void Throttle::manualNotch(bool up)
// this routine just sets the notch to be later processed in computeVelocity
{
    uint32_t now;

    if (!_running) // nothing to do here, move along
        return;

    now = millis();
    if ((now - _startTimestamp) < 8000) // wait for decoder to prime itself
        return;

    if (up && _opMode != braking)
    // notching up
    {
        if (_notch == 8)
            return;
        _opMode = powered;
        commandFifo.pushCommand(functionNotchUp, true);
        commandFifo.pushCommand(functionNotchUp, false);
        _notch++;
    }
    else if (up && _opMode == braking)
    // release the brakes
    {
        _opMode = idle;
        _independentBrake = 0;
        setIBrake(_independentBrake);
        setTBrake(75); // TBD this, avoid numbers
    }
    else if (!up && _opMode == powered)
    // notching down
    {
        if (_notch == 0)
            return;
        commandFifo.pushCommand(functionNotchDown, true);
        commandFifo.pushCommand(functionNotchDown, false);
        _notch--;
        if (_notch == 0)
            _opMode = idle;
    }
    else if (!up && ((_opMode == idle) || (_opMode == braking)))
    // incrementally apply brakes
    {
        _independentBrake += 20;
        setIBrake(_independentBrake);
        // TBD tbrake required
    }

    String throttleFeedback = _feedbackTopic + "notch";
    String glarb = String(_notch);
    client.publish(throttleFeedback.c_str(), glarb.c_str());
}

void Throttle::longPress(bool up)
// long press on the volume up or down buttons in Android app
{
    if (_opMode == off)
        return;

    if (up && _opMode == braking)
        // all brakes off
        setIBrake(0);
    else if (up && (_opMode == idle || _opMode == powered)) // TBD shouldn't need more than up
    // straight to eight
    {
        while (_notch < 8)
            manualNotch(true);
    }
    else if (!up && _opMode == powered)
    // straight to zero
    {
        while (_notch > 0)
            manualNotch(false);
    }
    else if (!up && _mph > 10 && _opMode == idle)
        // emergency
        panicStop(); // TBD fix this
    else if (!up && _mph <= 10 && ((_opMode == idle) || (_opMode == braking)))
        // quick stop at low speed
        panicStop();
}

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

    if (!_running)
        return;

    setAirGauge();

    if (_neutral)
        return; // if in neutral don't waste time in here

    if (_notch == 0)
        effectiveHP = 0;
    else if (_notch == 1)
        effectiveHP = _horsepowerAtIdle;
    else
        effectiveHP = (_horsepower * (_notch - 1) / 7) - 50;

        // if (effectiveHP < 0)
        //     effectiveHP = 0;

#ifdef speeddebug
    Serial.print("_mph  ");
    Serial.println(_mph);
#endif

    if (_mph <= 0)
        tractiveForce = effectiveHP * 308;
    else
    {
        tractiveForce = effectiveHP * 308 / _mph;
        if (tractiveForce > _tractiveEffort)
            tractiveForce = _tractiveEffort;
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

    // this routine attempts to simulate spooling up
    if (_notch > 1)
        if (tractiveForce > 1.25 * _lastTractiveForce)
            tractiveForce = _lastTractiveForce + .15 * tractiveForce; // TBD was .25

    _lastTractiveForce = tractiveForce;

#ifdef speeddebug
    Serial.print("tractiveForce ");
    Serial.println(tractiveForce);
#endif

    // there must be some drag effect that varies with speed that is peculiar to locos - this is a guess
    variableLocoDragForce = _locoMass * 32 * _currentSpeed * VARIABLE_LOCO_DRAG_COEFICIENT;

#ifdef speeddebug
    Serial.print("variableLocoDragForce ");
    Serial.println(variableLocoDragForce);
#endif

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
    Serial.print("_currentSpeed factored ");
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
        String speedFeedback = _feedbackTopic + "speed";
        String glarb = String(intSpeedoSpeed); // TBD fix this
        client.publish(speedFeedback.c_str(), glarb.c_str());

        // build the command string
        String dummyString = "t 1 ";
        dummyString.concat(String(_dccAddress) + " ");
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
        String odometerFeedback = _feedbackTopic + "odometer";
        String odometerString = String(_odometer / 5280);
        client.publish(odometerFeedback.c_str(), odometerString.c_str());
    }
}

void Throttle::setAirGauge(void)
{
    int intCurrentPsi;
    static int lastIntCurrentPsi;
    static int countDown;

    if ((_trainlineSetPSI > _trainlinePSI) && (_running))
    {
        if (!_compressorRunning)
        {
            _compressorRunning = true;
            commandFifo.pushCommand(functionCompressor, 1);
            countDown = 20;
        }
        countDown--;
        if (countDown <= 0)
        {
            // this holds off the pressure rise until compressor finally starts
            countDown = 0;
            _trainlinePSI += (.3 * (_notch + 1));
        }
    }

    if ((_trainlineSetPSI <= _trainlinePSI) && _running && _compressorRunning)
    {
        _compressorRunning = false;
        commandFifo.pushCommand(functionCompressor, 0);
    }

    if (_trainlinePSI > _trainlineSetPSI)
        _trainlinePSI = _trainlineSetPSI;
    else if (_trainlineSetPSI == 0)
        _trainlinePSI = 0;
    else if (_trainlineSetPSI < _trainlinePSI)
        _trainlinePSI -= 3;

    if (_trainlineSetPSI < _trainlinePSI)
        _trainlinePSI = _trainlineSetPSI; // TBD wtf? see above

    intCurrentPsi = int(_trainlinePSI);

    if (intCurrentPsi != lastIntCurrentPsi)
    {
        lastIntCurrentPsi = intCurrentPsi;
        String speedFeedback = _feedbackTopic + "trainline";
        // Serial.print("in air gauge ");
        // Serial.println(speedFeedback);
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

    // if in neutral when commanded then bail and reset
    // if (_neutral)
    // {
    //     _calibrationStage = 0;
    //     return;
    // }

    // user is canceling
    if (speed == 0)
    {
        String stopString = "t 1 ";
        stopString.concat(String(_dccAddress) + " ");
        stopString.concat(String(0) + " ");
        stopString.concat("1");
        strcpy(dummyChars, stopString.c_str());
        SerialCommand::parse(dummyChars);
        _calibrationStage = 0;
        return;
    }

    // reverser position determines forward or reverse computation
    // as well as setting direction of motion
    // if (!_direction)
    //     speed = speed * -1;

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

    // calibration stages
    // 0 = startup to first gate
    // 1 = between gates
    // 2 = stopping after second gate

    if (_calibrationStage == 2)
    {
        trapLength = _calibrationTrapLength;
        trapLength = 1; // TBD remove this
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

        commandFifo.pushCommand(functionBell, false);
        getLocoPrefs(); // read storage into variables
    }
    else if (_calibrationStage == 1)
    // passing the start gate
    {
        _calibrationTimer = millis();
        commandFifo.pushCommand(functionBell, true);
    }

    if (_calibrationStage != 1) // either starting movement or stopping (0 or 2)
    {
        String dummyString = "t 1 ";
        dummyString.concat(String(_dccAddress) + " ");
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
