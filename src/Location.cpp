/*
This routine reads a position on layout from magnets embedded in the ties
Magnets are embedded in ties on left and right side of tie, near rail
Eastbound there are magnets on left side on every tie, these are the sync magnets
Eastbound the magnets on right side are the data, binary encoded, first tie is bit zero
OLS determines which direction the loco is headed so as to differentiate sync from data and big vs little endian
*/

#include "Location.h"

MagnetReader::MagnetReader(int leftPin, int rightPin)
// constructor
{
    this->leftPin = leftPin;
    this->rightPin = rightPin;
    pinMode(leftPin, INPUT);
    pinMode(rightPin, INPUT);
}

bool MagnetReader::check(uint currentSpeed)
{
    bool finished = false;

    // currentSpeed is FPS
    if (currentSpeed == 0)
        // not moving so nothing to do here
        return false;

    uint maxInterMagnetInterval = 4 * 1000 / currentSpeed;  // interval (ms) between magnets at speed TBD the hard coded footage (4)

    // TBD have to determine if has been a long time since last somestates was true, reset if so
    // had already detected at least one digit but not seeing any others
    // maybe like this
    uint maxDeadInterval = 2 * maxInterMagnetInterval;
    if ((millis() - maxDeadInterval > startOfPeriod) && (digitState > 0))
    {
        detectState = clear;
        errorState = true;  // TBD maybe, maybe not
        digitState = 0;
    }

    // read the Hall effect devices
    int leftMagState = digitalRead(leftPin);
    int rightMagState = digitalRead(rightPin);

    // detect if either detector saw a magnet
    someStates = leftMagState || rightMagState;

    // record the values seen and keep looking on every pass through here
    // keep looking and recording until neither side is detected
    // TBD add timeout here qualified by speed
    bothStates = bothStates || (leftMagState * 2);
    bothStates = bothStates || rightMagState;

    if (someStates == 0 )
        errorState = false;
    else if ((someStates > 0) && errorState)
        return false;

    if ((someStates > 0) && (detectState == clear))
    // transition into 'over the magnets' state
    {
        detectState = started;
        digitState = digitState + 1;
        startOfPeriod = millis();
    }
    else if ((someStates == 0) && (detectState == started))
    // transition to no magnets state, clear values to prep for next magnet or reset if done
    {
        if (millis() - maxInterMagnetInterval > startOfPeriod)
            {
                // error TBD in here
                errorState = true;
                digitState = 0;
                return false;
            }
        digitValue[digitState - 1] = bothStates;
        detectState = clear;
        someStates = 0;
        bothStates = 0;
        if (digitState == MAX_BITS)
        {
            finished = true;
            digitState = 0;
        }
    }

    if (finished)
        // notifiy caller that data is ready to be processed
        return true;
    else
        return false;
}

uint MagnetReader::process(bool isForward)
{
    uint codeValue;
    if ((digitValue[0] && 1) && (digitValue[1] && 1) && (digitValue[2] && 1) && (digitValue[3] && 1))  // TBD TBD TBD this sucks, doesn't allow for MAX_BITS
        // right side was the sync side or it doesn't matter because both sides saw all ones
        // take values from second digit, first digit is sync
        if (isForward)
            // we are heading west, endian is big so switch it
            codeValue = (digitValue[3] / 2) + ((digitValue[2] / 2) * 2) + ((digitValue[1] / 2) * 4) + ((digitValue[0] / 2) * 8);
        else
            // we are backing east, endian is little
            codeValue = (digitValue[0] / 2) + ((digitValue[1] / 2) * 2) + ((digitValue[2] / 2) * 4) + ((digitValue[3] / 2) * 8);

    else
        // left side was the sync side because right side was not so take values from first digit, second digit is sync
        // TBD maybe should make sure this side was all ones, discard if not
        if (isForward)
            // we are heading east, endian is little
            codeValue = (digitValue[0] && 1) + ((digitValue[1] && 1) * 2) + ((digitValue[2] && 1) * 4) + ((digitValue[3] && 1) * 8);
        else
            // we are backing west, endian is big so switch it
            codeValue = (digitValue[3] && 1) + ((digitValue[2] && 1) * 2) + ((digitValue[1] && 1) * 4) + ((digitValue[0] && 1) * 8);

    // TBD now what? have to figure which endian it is and maybe switch ends (still relevent?)

    return codeValue;
}