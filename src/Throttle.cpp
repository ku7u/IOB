#include "Throttle.h"
#include "SerialCommand.h"
#include "Function.h"
#include "Arduino.h"
#include "MQTT.h"

// Constructor
Throttle::Throttle(int roadNumber)
{
    _direction = true; // forward
    _throttleLever = 0;
    _roadNumber = roadNumber;
    _trainlinePSI = 42;
    _trainlineSetPSI = 75;
    _neutral = true;

    // send back trainline pressure TBD this will be removed
    // String speedFeedback = "IOB/" + String(_roadNumber) + "/feedback/trainline";
    // String glarb = String(85);
    // client.publish(speedFeedback.c_str(), glarb.c_str());
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

void Throttle::bell(bool onOff)
{
    if (onOff)
    {
        setFunction(functionBell, true);
    }
    else
    {
        setFunction(functionBell, false);
    }
}

void Throttle::horn(bool onOff)
{
    if (onOff)
    {
        setFunction(functionHorn, true);
    }
    else
    {
        setFunction(functionHorn, false);
    }
}

void Throttle::startPM(void)
{
    _running = true;
}

void Throttle::stopPM(void)
{
    _running = false;
}

void Throttle::setRoadNumber(int roadNumber)
{
    _roadNumber = roadNumber;
}

void Throttle::setThrottleLever(int throttleLever)
{
    // _throttleLever = throttleLever;
    // Throttle::compute();
}

void Throttle::setDirection(int direction)
{
    _neutral = false;
    if (direction == 0)
        _direction = false;
    else if (direction == 2)
        _direction = true;
    else
        _neutral = true;

    // Throttle::compute(); // TBD really? maybe ignore until throttle moved?
}

void Throttle::setMass(uint16_t mass)
{
    _trainlinePSI = 0;
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

// void Throttle::compute(void)
// {
//     char dummyChars[31];
//     int speedDifferential;
//     uint16_t intCurrentSpeed;
//     static uint16_t lastTarget;
//     static uint16_t lastIntCurrentSpeed;
//     static int spoolUp;
//     float speedoSpeed;
//     static int intSpeedoSpeed;

//     speedDifferential = _targetSpeed - _currentSpeed; // negative if reducing
//     if (speedDifferential == 0)
//         return;

//     // Serial.print("target ");
//     // Serial.println((_targetSpeed));
//     // Serial.print("current ");
//     // Serial.println(_currentSpeed);
//     // Serial.print("differential ");
//     // Serial.println(speedDifferential);

//     if ((_targetSpeed != lastTarget) && (_currentSpeed > 10))
//         spoolUp = 2; // spoolUp eases the transition to next higher notch

//     // if ((speedDifferential > 0) && (_targetSpeed - 1 <= _currentSpeed))
//     if ((speedDifferential > 0) && (_targetSpeed - 2 <= _currentSpeed)) // assume target speed if close to it
//         _currentSpeed = _targetSpeed;
//     // else if ((speedDifferential < 0) && (_targetSpeed + 1 >= _currentSpeed))
//     else if ((speedDifferential < 0) && (_targetSpeed + 4 >= _currentSpeed)) // stop abruptly if close to zero TBD ? slowing to intermediate
//         _currentSpeed = _targetSpeed;
//     else if (speedDifferential > 0) // increasing speed
//         if (spoolUp == 2)
//         {
//             spoolUp--;
//             _currentSpeed = _currentSpeed + speedDifferential / (16.0 - _horsepower / 1500. + _tonnage / 100.); // TBD test to see if granularity of 8 is noticable, else 16
//         }
//         else if (spoolUp == 1)
//         {
//             spoolUp--;
//             _currentSpeed = _currentSpeed + speedDifferential / (12.0 - _horsepower / 1500. + _tonnage / 100.); // TBD test to see if granularity of 8 is noticable, else 16
//         }
//         else
//         {
//             Serial.print("speedDifferential/8 ");
//             Serial.println(speedDifferential / 8.);
//             _currentSpeed = _currentSpeed + speedDifferential / (8.0 - _horsepower / 1500. + _tonnage / 100.); // TBD test to see if granularity of 8 is noticable, else 16
//         }
//     else // decreasing speed
//     {
//         if (_currentSpeed < 8)
//             _currentSpeed = _currentSpeed + speedDifferential / (8.0 + _tonnage / 100.); // TBD test to see if granularity of 8 is noticable, else 16
//         // _currentSpeed = _currentSpeed + speedDifferential;
//         else
//             _currentSpeed = _currentSpeed + speedDifferential / (16.0 + _tonnage / 100.); // TBD test to see if granularity of 8 is noticable, else 16
//     }

//     // intCurrentSpeed = _currentSpeed + 0.5; // rounding to an integer
//     intCurrentSpeed = _currentSpeed; // rounding to an integer

//     if (intCurrentSpeed != lastIntCurrentSpeed)
//     {
//         speedoSpeed = _currentSpeed * _speedFactor;
//         intSpeedoSpeed = speedoSpeed;
//         lastIntCurrentSpeed = intCurrentSpeed;
//     }

//     // send back speedometer data to operator
//     String speedFeedback = "IOB/" + String(_roadNumber) + "/feedback/speed";
//     String glarb = String(intSpeedoSpeed);
//     client.publish(speedFeedback.c_str(), glarb.c_str());

//     // build the command string
//     String dummyString = "t 1 ";
//     dummyString.concat(String(_roadNumber) + " ");
//     dummyString.concat(String(intCurrentSpeed) + " ");
//     if (_direction)
//         dummyString.concat("1");
//     else
//         dummyString.concat("0");

//     strcpy(dummyChars, dummyString.c_str());

//     SerialCommand::parse(dummyChars);
// }

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
        _trainlinePSI += (.3 * (_notch + 1));
        if (_trainlinePSI > _trainlineSetPSI)
            _trainlinePSI = _trainlineSetPSI;
    }
    else if (_trainlineSetPSI == 0)
        _trainlinePSI = 0;
    else if (_trainlineSetPSI < _trainlinePSI)
    {
        _trainlinePSI -= 3;
        if (_trainlinePSI < _trainlineSetPSI)
            _trainlinePSI = _trainlineSetPSI;
    }

    intCurrentPsi = _trainlinePSI;

    if (intCurrentPsi != lastIntCurrentPsi)
    {
        lastIntCurrentPsi = intCurrentPsi;
        // send back trainline data to operator
        String speedFeedback = "IOB/" + String(_roadNumber) + "/feedback/trainline";
        String glarb = String(intCurrentPsi);
        client.publish(speedFeedback.c_str(), glarb.c_str());
    }
}