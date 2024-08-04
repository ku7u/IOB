#include "BrakeSystem.h"
#include "Fifo.h"
#include "Throttle.h"

// extern int functionCompressor;
extern Fifo commandFifo;
extern Throttle throttle;

// Constructor
BrakeSystem::BrakeSystem(void)
{
    _mainPSI = 100; // TBD temporary
    _carCount = 0;
    _handbrake = false;
    _compressorRunning = false;
    _trainlineSetPSI = TRAINLINE_MAX_PSI;
}

uint16_t BrakeSystem::getTrainlinePSI(void)
{
    return _trainlinePSI;
}

uint16_t BrakeSystem::getEffectiveLocoBrake(void)
{
    return _effectiveLocoBrake;
}

uint16_t BrakeSystem::getMainPSI(void)
{
    return _mainPSI;
}

float BrakeSystem::getEffectiveTrainBrake(void)
{
    return _effectiveTrainBrake;
}

void BrakeSystem::setCompressorFunction(uint16_t func)
{
    _functionCompressor = func;
}

bool BrakeSystem::cycle(bool pmRunning)
{
    // run this routine once per second while PM is on
    // if (pmRunning) commandFifo.pushCommand(_functionCompressor, 1);

    // static int countDown;

    // turn off compressor if PM shutdown
    if (!pmRunning && _compressorRunning)
    {
        commandFifo.pushCommand(_functionCompressor, 0);
        _compressorRunning = false;
        return false;
    }
    // turn on compressor if needed
    else if (pmRunning && _startupComplete && (_compressorRunning == false) && (_mainPSI < MAIN_MIN_PSI))
    {
        commandFifo.pushCommand(_functionCompressor, 1);
        _compressorRunning = true;
    }
    // or turn it off if not needed
    else if ((_compressorRunning == true) && (_mainPSI >= MAIN_MAX_PSI))
    {
        commandFifo.pushCommand(_functionCompressor, 0);
        _compressorRunning = false;
    }

    // pump up the pressure in main
    if (_compressorRunning && (_mainPSI < MAIN_MAX_PSI))
    {
        _mainPSI += 2;
        if (_mainPSI > MAIN_MAX_PSI)
            _mainPSI = MAIN_MAX_PSI;
    }

    // adjust the trainline pressure
    if ((_trainlineSetPSI > _trainlinePSI))
    {
        if (_carCount < 5)
            _trainlinePSI += 10;
        else if (_carCount < 10)
            _trainlinePSI += 5;
        else
            _trainlinePSI += 2;

        if (_trainlinePSI > TRAINLINE_MAX_PSI)
            _trainlinePSI = TRAINLINE_MAX_PSI;
    }

    return _compressorRunning;
}

float BrakeSystem::applyLocoBrake(bool applying)
{
    // parameter indicates applying (true) or releasing (false)
    // returns effective brake effect which may be reduced by reduced main reservoir pressure
    // reservoir pressure is reduced a little

    static uint16_t leverPosition;

    if (applying)
        leverPosition += PER_NOTCH;
    else
    {
        leverPosition = 0;
        _effectiveLocoBrake = 0;
        return 0;
    }

    if (leverPosition > 100)
        leverPosition = 100;

    if (_mainPSI == 0)
    {
        _effectiveLocoBrake = 0;
        return 0;
    }
    else if (_mainPSI < TRAINLINE_MAX_PSI)
    {
        _mainPSI -= leverPosition / 100. * 5;
        if (_mainPSI < 0)
            _mainPSI = 0;
        _effectiveLocoBrake = (leverPosition / 100. * 5) * (_mainPSI * 1.) / MAIN_MIN_PSI;
    }
    else
    {
        _mainPSI -= leverPosition / 100. * 5;
        _effectiveLocoBrake = leverPosition;
    }

    return _effectiveLocoBrake;
}

float BrakeSystem::applyTrainBrake(bool applying)
{
    // reduce the trainline psi on each call of this function
    // return a factor indicating percent of max train brake effectiveness

    static uint16_t leverPosition;

    _trainBrakeApplied = applying;

    if (applying)
    {
        leverPosition += PER_NOTCH;
    }
    else
    {
        _trainlineSetPSI = TRAINLINE_MAX_PSI;
        leverPosition = 0;
        _effectiveTrainBrake = 0;
        return 0;
    }

    if (leverPosition > 100)
        leverPosition = 100;

    _trainlineSetPSI = (float)TRAINLINE_MAX_PSI - (leverPosition / 100.) * (TRAINLINE_MAX_PSI - TRAINLINE_MIN_AUTO_PSI); // TBD doesn't account for reduced main
    if (_trainlineSetPSI < 0)
        _trainlineSetPSI = 0;
    _trainlinePSI = _trainlineSetPSI;

    _effectiveTrainBrake = (float)(TRAINLINE_MAX_PSI - max((int)_trainlineSetPSI, TRAINLINE_MIN_AUTO_PSI)) / (float)(TRAINLINE_MAX_PSI - TRAINLINE_MIN_AUTO_PSI);

    return _effectiveTrainBrake;
}

void BrakeSystem::applyEmmergency(bool applying)
{
    // TBD do the independent emergency here also
    // force user to request both independent and emergency using two calls

    // applying brake does not affect the main pressure
    // pipe pressure goes to zero immediately
    if (applying)
    {
        _trainlineSetPSI = 0;
        _trainlinePSI = 0;
        _trainBrakeApplied = true;
        _effectiveLocoBrake = 100;                     // TBD this should be changed to 0. to 1. to be consistent
        _effectiveTrainBrake = EMERGENCY_BRAKE_FACTOR; // really 1.0 * EMERGENCY_BRAKE_FACTOR
    }
    // releasing the ebrake will affect main pressure
    // trainline pressure will need to build slowly
    // _effectiveTrainBrake should (theoretically) be max
    // _effectiveTrainBrake should increase from zero with pipe pressure
    else
    {
        // _effectiveLocoBrake = 0;
        // _effectiveTrainBrake = 100;
        _trainBrakeApplied = false;
        _trainlineSetPSI = TRAINLINE_MAX_PSI;
    }
    return;
}

void BrakeSystem::connectAirLine(bool connecting, uint16_t carCount)
{
    _trainlineConnected = connecting;
    _carCount = carCount;

    // reduce the trainline pressure proportional to carcount
    _trainlinePSI -= _carCount * 2;
    if (_trainlinePSI < 0)
        _trainlinePSI = 0; // TBD this is probably trouble due to using uint16_t, should be int
}

void BrakeSystem::release(void)

{
    // TBD
    _trainlineSetPSI = TRAINLINE_MAX_PSI;
}

void BrakeSystem::setPMRunning(bool complete)
{
    _startupComplete = complete;
}