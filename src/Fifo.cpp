#include "Arduino.h"
#include "Fifo.h"
#include "Function.h"

void Fifo::pushCommand(int myFunction, bool myBool)
{
    _functionParms myParms;
    myParms.functionID = myFunction;
    myParms.onOff = myBool;
    myQ.push(myParms);
}

void Fifo::pop()
{
    int myFunction;
    int myValue;
    _functionParms myParms;

    if (!myQ.empty())
    {
        myParms = myQ.front();
        setFunction(myParms.functionID, myParms.onOff);
        // Serial.println("pop");
        myQ.pop();
    }
}
