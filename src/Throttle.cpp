// #define speeddebug

#include "Throttle.h"
#include "SerialCommand.h"
#include "Function.h"
#include "Arduino.h"
#include "MQTT.h"
#include "Preferences.h"
#include "PubSubClient.h"
#include "WebSerial.h"
#include "Fifo.h"
#include "ArduinoJson.h"

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
    _locoID = myPrefs.getString("locoid", "none");
    // _locoType = myPrefs.getString("locotype", "diesel");
    // myPrefs.getBool("shortLong", 0);
    _horsepower = myPrefs.getInt("horsepower", 1500);
    _locoWeight = myPrefs.getULong("locoweight", 250000);
    _tractiveEffort = myPrefs.getLong("tractiveeffort", 70000);
    _odometer = myPrefs.getFloat("odometer", 0.0);
    _muActive = myPrefs.getBool("muactive", false);
    _muState = myPrefs.getUInt("mustate", 0);
    _muLeadLoco = myPrefs.getString("muleadloco", "3");
    _muReversed = myPrefs.getBool("mureversed", false);
    myPrefs.end();

    myPrefs.begin("calibration", true);
    // _calibrationTrapLength = myPrefs.getInt("traplength", 3);
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
    // _feedbackTopic = myPrefs.getString("feedbacktopic", "tlm/ols/") + String(_roadNumber) + "/";
    // _feedbackTopic = myPrefs.getString("feedbacktopic", "tlm/ols/") + _locoID + "/";
    _feedbackTopic = myPrefs.getString("feedbacktopic", "tlm/ols/");
    _commandTopic = myPrefs.getString("commandtopic", "cmd/ols/");
    myPrefs.end();

    _locoMass = _locoWeight / 32; // slugs

    muSubscribe();
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
// respond to a broadcast message that requests who is online
// send locoID, ip address, loco type
{
    char topicChars[30];
    strcpy(topicChars, _feedbackTopic.c_str());
    strcat(topicChars, "report");

    // String locoType = "GP18";              // TBD fix this
    String x = "{\"id\":\"";
    x.concat(_locoID);
    x.concat("\",\"ip\":\"");
    x.concat(WiFi.localIP().toString());
    x.concat("\",\"type\":\"");
    x.concat(_locoType);
    x.concat("\",\"mu\":\"");
    x.concat(_muState);
    x.concat("\"}");
    client.publish(topicChars, x.c_str());
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
    sendStatus();
}

void Throttle::headlight(int offDimBright)
{
    // if mued no headlights are active unless it is the lead unit
    if (_muActive)
        if ((_muState != 1))
            return;

    _headlight = offDimBright;

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
    if (_muActive)
    {
        if ((_muState == 2)) // no lights for mid consist locos
            return;

        else if ((_muState == 3) && (_muReversed)) // if reversed the headlight is the rearlight
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
            return;
        }
    }

    _rearlight = offDimBright; // TBD on this

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
    char dccCommandChars[20];
    char buffer[10];
    itoa(_dccAddress, buffer, 10);

    strcpy(dccCommandChars, "t 1 ");
    strcat(dccCommandChars, buffer);
    strcat(dccCommandChars, " 0");
    SerialCommand::parse(dccCommandChars);

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
    {
        commandFifo.pushCommand(functionHorn, onOff);
    }
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

void Throttle::setTBrake(float val)
{
    // TBD explain logic here
    if (val > 0)
    {
        _trainlineSetPSI = val;
        if (_trainlinePSI > _trainlineSetPSI)
            _trainlinePSI = _trainlineSetPSI;
        commandFifo.pushCommand(functionTrainBrake, true);
    }
    else if (val == 0)
    {
        _trainlineSetPSI = TRAINLINE_SET_PSI;
        commandFifo.pushCommand(functionTrainBrake, false);
    }
    else
    {
        _trainlinePSI = 0;
        _trainlineSetPSI = 0;
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
    if ((now - _startTimestamp) < 6000) // wait for decoder to prime itself
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
        setIBrake(0);
        setTBrake(0);
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
        setIBrake(_independentBrake + 20);
        if (_trainlineConnected)
            // setTBrake(_trainlinePSI - TRAINLINE_SET_PSI * .07);
            setTBrake(_trainlinePSI - TRAINLINE_SET_PSI * .058);
        return; // so that notch is not redundantly returned to throttle
    }

    char topicChars[50];
    strcpy(topicChars, _feedbackTopic.c_str());
    strcat(topicChars, _locoID.c_str());
    strcat(topicChars, "/notch");

    char buffer[10];
    itoa(_notch, buffer, 10);
    char msgChars[20];
    strcpy(msgChars, buffer);

    client.publish(topicChars, msgChars);
}

void Throttle::longPress(bool up)
// long press on the volume up or down buttons in Android app
{
    if (_opMode == off)
        return;

    if (up && _opMode == braking)
    {
        // all brakes off
        setIBrake(0);
        setTBrake(0);
    }
    // else if (up && (_opMode == idle || _opMode == powered)) // TBD shouldn't need more than up
    else if (up && (_opMode == idle)) // TBD shouldn't need more than up
    // switch direction
    {
        if (_neutral)
        {
            _direction = true;
            _neutral = false;
        }
        else
            _direction = !_direction;

        // send back new direction as telemetry
        char topicChars[30]; // TBD the 30
        strcpy(topicChars, _feedbackTopic.c_str());
        strcat(topicChars, _locoID.c_str());
        strcat(topicChars, "/reverser");

        char msgChars[2];
        if (_direction)
            strcpy(msgChars, "2");
        else
            strcpy(msgChars, "0");

        client.publish(topicChars, msgChars);
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
        // quick stop at low speed TBD maybe release brakes also
        panicStop();
}

void Throttle::computeVelocity(void)
{
    float effectiveHP;
    float tractiveForce;
    float dragForce;
    float variableLocoDragForce;
    float startingForce;
    // float gradeForce;  //TBD
    float independentBrakeForce;
    float trainBrakeForce;
    float accel;
    char dummyChars[31];
    uint16_t intCurrentSpeed;
    static bool zeroWasSent = false;
    static uint16_t lastIntCurrentSpeed;
    // String feedbackPrefix;

    if ((!_running) || (_muActive))
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
        tractiveForce = effectiveHP * 308 / (_mph * 1); // TBD REALITY_FACTOR goes here to replace the 1
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
    variableLocoDragForce = _locoMass * 32 * _currentSpeed * VARIABLE_LOCO_DRAG_COEFICIENT; // TBD now at zero

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
    if (_currentSpeed < 0.)
        _currentSpeed = 0;

    _odometer = _odometer + abs(_currentSpeed); // TBD on this (seems to work)

#ifdef speeddebug
    Serial.print("_currentSpeed ");
    Serial.println(_currentSpeed);
#endif

    // get the appropriate calibrated and interpolated speed compensation value
    intCurrentSpeed = interpolateSpeedFactor(_currentSpeed);

#ifdef speeddebug
    Serial.print("_currentSpeed factored ");
    Serial.println(intCurrentSpeed);
    Serial.print('\n');
#endif

    if (_mph > 0)
        zeroWasSent = false; // flags status to be sent for special case of intCurrentSpeed going to zero

    if ((intCurrentSpeed != lastIntCurrentSpeed) || (!zeroWasSent))
    {
        _mph = _currentSpeed * FPS_TO_MPH_FACTOR; // TBD fix this (why?)

        lastIntCurrentSpeed = intCurrentSpeed;

        // build the command string
        char buffer[20];
        strcpy(dummyChars, "t 1 ");

        itoa(_dccAddress, buffer, 10);
        strcat(dummyChars, buffer);
        strcat(dummyChars, " ");

        itoa(intCurrentSpeed, buffer, 10);
        strcat(dummyChars, buffer);
        strcat(dummyChars, " ");

        if (_direction)
            strcat(dummyChars, "1");
        else
            strcat(dummyChars, "0");

        SerialCommand::parse(dummyChars);
    }

    // send back odometer data to operator
    // TBD maybe send this once on startup and/or shutdown as well
    if ((_mph > 0) || (!zeroWasSent))
    {
        if (_mph == 0)
            zeroWasSent = true;
        sendStatus();
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
            // this holds off the pressure rise until compressor finally starts, have no way of knowing when the compressor sound starts (startup only)
            countDown = 0;
            _trainlinePSI += (.3 * (_notch + 1));
        }
        else if (_currentSpeed > 0)
            _trainlinePSI += (3 * (_notch + 1));
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

        // char topicChars[30];
        // strcpy(topicChars, _feedbackTopic.c_str());
        // strcat(topicChars, _locoID.c_str());
        // strcat(topicChars, "/trainline");

        // char msgChars[5];
        // char buffer[20];
        // itoa(intCurrentPsi, buffer, 10);
        // strcpy(msgChars, buffer);

        if (_mph == 0)
            // client.publish(topicChars, msgChars);
            // else
            sendStatus();
    }
}

void Throttle::calibrate(int speed)
{
    // this routine sets a factor that is applied to the commanded DCC speed such that
    // the actual scale speed over the rails is accurate with respect to commanded mph

    Preferences myPrefs;
    char dummyChars[31];
    long calibrationPeriod;
    int trapLength;
    long targetTime;
    float newFactor;
    float factorF;
    float factorR;
    int dccVal;

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
        commandFifo.pushCommand(functionBell, false);
        return;
    }

    // sign of speed determines forward or reverse computation
    // as well as setting direction of motion

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

} // calibrate

void Throttle::setMuState(char *jsonMsg)
{
    // messages causing change of mu state will be sent to this loco address and are processed here
    // mu states:
    //  0 - not mued so leave consist and inform lead if was mid or trailing
    //  1 - mued as lead, look for incoming hp and mass values from locos in consist
    //  2 - mued as mid, send hp and mass values to lead
    //  3 - mued as trailing, send hp and mass values to lead

    // json format
    // {muState:"", leadID:"", reversed:""}

    Preferences myPrefs;
    StaticJsonDocument<100> doc;

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, jsonMsg);
    if (error)
        return;

    int commandedState = doc["muState"]; // this is the new _muState
    String value = doc["reversed"];
    _muReversed = value.toInt();

    switch (commandedState)
    {
    case 0: // not mued
        _muActive = false;
        if (_muState == 1)
        // if was mu lead then retrieve original hp and mass and exit the consist
        {
            // getLocoPrefs();
            _muState = 0;
        }
        else if ((_muState == 2) || (_muState == 3))
        // if was mu mid or trailing then send negative hp and mass values to lead and exit the consist
        {
            // TBD send negative hp, mass and tractive effort to lead now

            // unsubscribe from lead loco messages
            muSubscribe();
        }

        _muState = commandedState;
        // store the state
        myPrefs.begin("loco");
        myPrefs.putUInt("mustate", _muState);
        // TBD store other aspects?
        myPrefs.end();
        break;

    case 1:                // lead
        if (_muState != 0) // only allowed if coming from not mued
            return;
        // TBD mu setup
        _muState = commandedState;
        // _muActive = true;

        break;

    case 2:                // mid
                           // case 2 falls through into 3
    case 3:                // trailing
        if (_muState != 0) // only allowed if coming from not mued
            return;

        _muState = commandedState;
        _muActive = true;
        const char *mull;
        mull = doc["leadID"];
        _muLeadLoco = String(mull);
        // TBD send my id, mass, hp and tractive effort to lead now
        char topicChars[100];
        strcpy(topicChars, _commandTopic.c_str());
        strcat(topicChars, _muLeadLoco.c_str());
        strcat(topicChars, "/muperformance");

        char msgChars[100]; // myID, locomass, hp, tractive effort
        char buffer[50];

        strcpy(msgChars, "{\"id\":\"");
        strcat(msgChars, _locoID.c_str());

        strcat(msgChars, "\",\"mass\":\"");
        itoa(_locoMass, buffer, 10);
        strcat(msgChars, buffer);

        strcat(msgChars, "\",\"hp\":\"");
        itoa(_horsepower, buffer, 10);
        strcat(msgChars, buffer);

        strcat(msgChars, "\",\"te\":\"");
        String teString = String(_tractiveEffort); // TBD this is double so itoa may not work
        strcat(msgChars, teString.c_str());

        strcat(msgChars, "\"}");

        // send the parameters to lead loco to affect its performance
        client.publish(topicChars, msgChars);

        // subscribe to lead loco messages for speed, direction and notch
        muSubscribe();

        break;
    }

    myPrefs.begin("loco");
    myPrefs.putBool("muactive", _muActive);
    myPrefs.putUInt("mustate", commandedState);
    myPrefs.putString("muleadloco", _muLeadLoco);
    myPrefs.putBool("mureversed", _muReversed);
    myPrefs.end();

    getLocoPrefs();

} // setMuState

void Throttle::muSubscribe()
{

    // subscribe to lead loco messages for speed, direction and notch
    char subscription[100];
    strcpy(subscription, _feedbackTopic.c_str());
    strcat(subscription, _muLeadLoco.c_str());
    strcat(subscription, "/status");

    if (_muActive)
        client.subscribe(subscription, 1);
    else
        client.unsubscribe(subscription);

    // subscribe to lead loco messages for headlight
    strcpy(subscription, _feedbackTopic.c_str());
    strcat(subscription, _muLeadLoco.c_str());
    strcat(subscription, "/headlight");

    if (_muActive)
        client.subscribe(subscription, 1);
    else
        client.unsubscribe(subscription);

    // subscribe to lead loco messages for rearklight
    strcpy(subscription, _feedbackTopic.c_str());
    strcat(subscription, _muLeadLoco.c_str());
    strcat(subscription, "/rearlight");

    if (_muActive)
        client.subscribe(subscription, 1);
    else
        client.unsubscribe(subscription);

} // muSubscribe

void Throttle::setMuPerformance(char *jsonMsg)
{
    // TBD
    // Serial.println(jsonMsg);
}

void Throttle::setMuSpeed(char *jsonMsg)
{
    if (!_muActive) // TBD this is a workaround that can't be left in the code
        return;

    static bool alternateSeconds = false;
    StaticJsonDocument<100> doc;

    // switch this each 1 sec cycle, it will be used to add or subtract 1 mph to commanded speed
    // this (theoretically) will defeat the back emf algorithm in the decoder to eliminate tug-of-war effect
    alternateSeconds = !alternateSeconds;

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, jsonMsg);
    if (error)
        return;

    // retrieve mph value
    float muMPH = doc["mph"];

    // wiggle the value to outsmart BEMF
    if (muMPH > .5)
    {
        if (alternateSeconds)
            muMPH += .5;
        else
            muMPH -= .5;
    }

    // retrieve direction, 1=fwd, 0=rev
    bool direction = doc["dir"];

    if (_muReversed) // true if running reversed in consist
        direction = !direction;

    _direction = direction; // TBD this is sloppy, but possibly prototypical

    // retrieve notch in order to alter PM sound
    u16_t notch = doc["notch"];

    while (_notch != notch)
    {
        manualNotch(_notch < notch); // if true notch up, else down until equal
    }

    // convert mph to fps
    float muFPS = muMPH / FPS_TO_MPH_FACTOR;

    uint16_t dccFPS = interpolateSpeedFactor(muFPS); // returns dcc val corresponding to fps, interpolated between cal points

    char buffer[20];
    char dccCommandChars[30];
    strcpy(dccCommandChars, "t 1 ");
    itoa(_dccAddress, buffer, 10);
    strcat(dccCommandChars, buffer);
    strcat(dccCommandChars, " ");
    itoa(dccFPS, buffer, 10);
    strcat(dccCommandChars, buffer);
    strcat(dccCommandChars, " ");

    if (direction)
        strcat(dccCommandChars, "1");
    else
        strcat(dccCommandChars, "0");

    SerialCommand::parse(dccCommandChars);
}

void Throttle::sendCondition()
{
    // sends static condition to app whenever app opens throttle fragment
    // this sets the state of various views in the fragment

    char topicChars[40];
    char msgChars[100];
    char charPsi[10];

    // build the topic string
    strcpy(topicChars, _feedbackTopic.c_str());
    strcat(topicChars, _locoID.c_str());
    strcat(topicChars, "/condition");

    // build the msg json string
    // PM status, on or off
    strcpy(msgChars, "{\"pm\":");
    const char charPm[2] = {char(_running + 48), 0}; // 48 = ascii zero, so sends back "0" or "1"
    strcat(msgChars, charPm);

    strcat(msgChars, ",\"rvrsr\":");
    uint revPos;
    if (_neutral)
        revPos = 1;
    else if (_direction)
        revPos = 2;
    else
        revPos = 0;
    const char charDir[2] = {char(revPos + 48), 0}; // 48 = ascii zero
    strcat(msgChars, charDir);

    // headlight status off, dim, bright
    strcat(msgChars, ",\"hl\":");                      // new 10/29
    const char charHl[2] = {char(_headlight + 48), 0}; // 48 = ascii zero
    strcat(msgChars, charHl);

    // rearlight status off, dim, bright
    strcat(msgChars, ",\"rl\":");                      // new 10/29
    const char charRl[2] = {char(_rearlight + 48), 0}; // 48 = ascii zero
    strcat(msgChars, charRl);

    // bell
    // maybe notch, brake status
    // car count
    // mass

    strcat(msgChars, ",\"psi\":"); // new 10/29
    dtostrf(_trainlinePSI, 2, 0, charPsi);
    strcat(msgChars, charPsi);

    strcat(msgChars, ",\"mu\":");                    // new 10/29
    const char charMu[2] = {char(_muState + 48), 0}; // 48 = ascii zero
    strcat(msgChars, charMu);

    if (_muActive) // new 10/29
    {
        strcat(msgChars, ",\"muto\":");
        strcat(msgChars, _muLeadLoco.c_str());
    }

    strcat(msgChars, "}");

    client.publish(topicChars, msgChars);
}

void Throttle::sendStatus()
{
    // String topic = _feedbackTopic + _locoID + "/status";
    int intSpeedoSpeed = _mph * 10;
    float speedoSpeed = intSpeedoSpeed / 10.; // to get tenths of mph

    char topicChars[40];
    strcpy(topicChars, _feedbackTopic.c_str());
    strcat(topicChars, _locoID.c_str());
    strcat(topicChars, "/status");

    char msgChars[100];
    char charSpeed[10];
    char charOdo[10];
    char charPsi[10];

    strcpy(msgChars, "{\"mph\":");
    dtostrf(speedoSpeed, 4, 2, charSpeed);
    strcat(msgChars, charSpeed);

    strcat(msgChars, ",\"dir\":");
    const char charDir[2] = {char(_direction + 48), 0}; // 48 = ascii zero
    strcat(msgChars, charDir);

    strcat(msgChars, ",\"notch\":");
    const char charNotch[2] = {char(_notch + 48), 0}; // 48 = ascii zero
    strcat(msgChars, charNotch);

    strcat(msgChars, ",\"odo\":");
    dtostrf((_odometer / 5280), 4, 2, charOdo);
    strcat(msgChars, charOdo);

    strcat(msgChars, ",\"psi\":");
    dtostrf(_trainlinePSI, 2, 0, charPsi);
    strcat(msgChars, charPsi);

    strcat(msgChars, "}");

    // client.publish(topic.c_str(), msgChars);
    client.publish(topicChars, msgChars);
}

uint16_t Throttle::interpolateSpeedFactor(float fps)
{
    float factorF;
    float factorR;

    uint16_t mphInt = (int)fps;

    if (mphInt <= FPS_AT_MPH_FACTOR2)
    {
        factorF = _fpsDccFactorForward2;
        factorR = _fpsDccFactorReverse2;
    }
    else if (mphInt <= FPS_AT_MPH_FACTOR5)
    {
        factorF = _fpsDccFactorForward2 + ((_fpsDccFactorForward5 - _fpsDccFactorForward2) * (mphInt - 2) / 3);
        factorR = _fpsDccFactorReverse2 + ((_fpsDccFactorReverse5 - _fpsDccFactorReverse2) * (mphInt - 2) / 3);
    }
    else if (mphInt <= FPS_AT_MPH_FACTOR10)
    {
        factorF = _fpsDccFactorForward5 + ((_fpsDccFactorForward10 - _fpsDccFactorForward5) * (mphInt - 5) / 5);
        factorR = _fpsDccFactorReverse5 + ((_fpsDccFactorReverse10 - _fpsDccFactorReverse5) * (mphInt - 5) / 5);
    }
    else if (mphInt <= FPS_AT_MPH_FACTOR20)
    {
        factorF = _fpsDccFactorForward10 + ((_fpsDccFactorForward20 - _fpsDccFactorForward10) * (mphInt - 10) / 10);
        factorR = _fpsDccFactorReverse10 + ((_fpsDccFactorReverse20 - _fpsDccFactorReverse10) * (mphInt - 10) / 10);
    }
    else if (mphInt <= FPS_AT_MPH_FACTOR50)
    {
        factorF = _fpsDccFactorForward20 + ((_fpsDccFactorForward50 - _fpsDccFactorForward20) * (mphInt - 20) / 30);
        factorR = _fpsDccFactorReverse20 + ((_fpsDccFactorReverse50 - _fpsDccFactorReverse20) * (mphInt - 20) / 30);
    }
    else
    {
        factorF = _fpsDccFactorForward50;
        factorR = _fpsDccFactorReverse50;
    }

    if (_direction)
        return (fps * factorF);
    else
        return (fps * factorR);
}

void Throttle::setCV(int cv, int value)
{
    char dummyChars[31];

    // build the command string
    char buffer[20];
    strcpy(dummyChars, "w ");

    itoa(_dccAddress, buffer, 10);
    strcat(dummyChars, buffer);
    strcat(dummyChars, " ");

    itoa(cv, buffer, 10);
    strcat(dummyChars, buffer);
    strcat(dummyChars, " ");

    itoa(value, buffer, 10);
    strcat(dummyChars, buffer);

    SerialCommand::parse(dummyChars);
}