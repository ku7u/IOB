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
    _mph = 0;
    _lastNotch = 0;

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
    functionIndependentBrake = myPrefs.getInt("independentBrake", 5);
    functionTrainBrake = myPrefs.getInt("trainBrake", 4);
    functionEmergencyBrake = myPrefs.getInt("emergencyBrake", 5);
    functionCompressor = myPrefs.getInt("compressor", 20);
    myPrefs.end();

    myPrefs.begin("loco");
    _roadNumber = myPrefs.getInt("roadnum", 16);
    // // myPrefs.getBool("shortLong", 0);
    _horsepower = myPrefs.getInt("horsepower", 1500);
    _locoWeight = myPrefs.getLong("locoweight", 250000);
    _tractiveEffort = myPrefs.getFloat("tractiveeffort", 70000.);
    myPrefs.end();

    _direction = true; // forward
    _throttleLever = 0;
    _trainlinePSI = 42;
    _trainlineSetPSI = 75;
    _neutral = true;
    _locoMass = _locoWeight / 32;
    _carCount = 0;

    // send back trainline pressure TBD this will be removed
    // String speedFeedback = "IOB/" + String(_roadNumber) + "/feedback/trainline";
    // String glarb = String(85);
    // client.publish(speedFeedback.c_str(), glarb.c_str());
}

void Throttle::init()
{
   
    // setFunction(functionNotchingEnable, 1);
    setFunction(functionNotchUp, 0);
    setFunction(functionNotchDown, 0);

}

void Throttle::setRoadNumber(int roadNumber)
{
    _roadNumber = roadNumber;
}

int Throttle::getRoadNumber(void)
{
    return _roadNumber;
}

void Throttle::pmOnOff(bool onOff)
{
    _running = onOff;
    setFunction(functionPM, onOff);
    setFunction(functionNotchingEnable, onOff); // TBD had to put here instead of init, don't get it
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

    // TBD don't know if have to actually send speed command here, likely not 

    // Throttle::compute(); // TBD really? maybe ignore until throttle moved?
}

void Throttle::setMass(uint16_t mass)
{
    _trainlinePSI = 0;
    _tonnage = _carCount * 50;
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
        _trainlineSetPSI = val;
        setFunction(functionTrainBrake, true);
    }
    else
        setFunction(functionTrainBrake, false);
}

//-------------------------------------------------------------------------------
// use manual notching to control PM sound, throttle for movement
// this routine just sets the raw setSpeed derived from the notch selected
// not affected by hp, tonnage or grade at this point
// for notch 1, advance throttle slightly with no notching
void Throttle::manualNotch(bool up)
{
    static int currentNotch = 0;
    static int lastNotch = 0;
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

    String throttleFeedback = "IOB/" + String(_roadNumber) + "/feedback/notch";
    String glarb = String(currentNotch);
    client.publish(throttleFeedback.c_str(), glarb.c_str());

    delay(100);
    if (up)
        setFunction(_notchUpFunction, false); // TBD this is really a kludge as workaround for timing issue
    else
        setFunction(_notchDownFunction, false);
}



void Throttle::computeVelocity(void)
{
    float tractiveForce;
    float dragForce;
    float gradeForce;
    float independentBrakeForce;
    float trainBrakeForce;
    uint16_t factorX = 4; // tractive force coefficient
    // float factorY = 0.5;  // drag variable constant
    float factorY = 0.2; // drag variable constant
    // float factorZ = 0.1;  // similar to friction coefficient
    float factorZ = 0.2; // similar to friction coefficient for brakes
    float sumForce;
    float accel;
    char dummyChars[31];
    uint16_t intCurrentSpeed;
    static uint16_t intSpeedoSpeed;
    static uint16_t lastIntCurrentSpeed;
    float speedoSpeed;

    if (_neutral)
        return; // if in neutral don't waste time in here

    tractiveForce = _notch * _horsepower * factorX;
    // Serial.print("tractiveForce ");
    // Serial.println(tractiveForce);

    // create segmented drag force profile
    if (_currentSpeed <= 0)
        dragForce = 0;
    else if (_currentSpeed < 20)
        dragForce = 1000 + 100 * _currentSpeed;
    else if (_currentSpeed < 50)
        dragForce = 1000 + 100 * 20 + 200 * (_currentSpeed - 20);
    else
        dragForce = 1000 + 100 * 20 + 200 * 50 + 1000 * (_currentSpeed - 50);

    dragForce *= factorY;

#ifdef speeddebug
    Serial.print("dragForce ");
    Serial.println(dragForce);
#endif

    independentBrakeForce = _independentBrake / 100. * _locoMass * 32 * factorZ;
    trainBrakeForce = _trainBrake / 100. * _tonnage * 2000 * .2;

#ifdef speeddebug
    Serial.print("independentBrakeForce ");
    Serial.println(independentBrakeForce);
#endif

    accel = (tractiveForce - dragForce - independentBrakeForce) / (_locoMass + _tonnage * 2000 / 32);

#ifdef speeddebug
    Serial.print("accel ");
    Serial.println(accel);
#endif

    _currentSpeed = _currentSpeed + accel; // accel is feet/sec/sec so if integrated once / sec, accel = vel
    if (_currentSpeed < 0)
        _currentSpeed = 0;

    intCurrentSpeed = _currentSpeed * _speedFactor;

#ifdef speeddebug
    Serial.print("_currentSpeed ");
    Serial.println(intCurrentSpeed);
    Serial.print('\n');
#endif

    if (intCurrentSpeed != lastIntCurrentSpeed)
    {
        speedoSpeed = _currentSpeed * _speedoCalFactor;
        intSpeedoSpeed = speedoSpeed;
        lastIntCurrentSpeed = intCurrentSpeed;

        // send back speedometer data to operator
        String speedFeedback = "IOB/" + String(_roadNumber) + "/feedback/speed";
        String glarb = String(intSpeedoSpeed);
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
        String speedFeedback = "IOB/" + String(_roadNumber) + "/feedback/trainline";
        String glarb = String(intCurrentPsi);
        client.publish(speedFeedback.c_str(), glarb.c_str());
    }
}