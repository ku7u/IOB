#include "Location.h"

MagnetReader::MagnetReader(int leftPin, int rightPin)
{
    this->leftPin = leftPin;
    this->rightPin = rightPin;
    pinMode(leftPin, INPUT);
    pinMode(rightPin, INPUT);
}

bool MagnetReader::check()
{
    bool finished = false;

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

    if ((someStates > 0) && (detectState == clear))
    // transition into 'over the magnets' state
    {
        detectState = started;
        digitState = digitState + 1;
    }
    else if ((someStates == 0) && (detectState == started))
    // transition to no magnets state, clear values to prep for next magnet or reset if done
    {
        digitValue[digitState - 1] = bothStates;
        detectState = clear;
        someStates = 0;
        bothStates = 0;
        if (digitState == 4)
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
    if ((digitValue[0] && 1) && (digitValue[1] && 1) && (digitValue[2] && 1) && (digitValue[3] && 1))
        // right side was the sync side or it doesn't matter because both sides save all ones
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

    // TBD now what? have to figure which endian it is and maybe switch ends

    return codeValue;
}