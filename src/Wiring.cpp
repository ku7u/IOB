// Wiring.cpp
#include <stdint.h>

#include "Wiring.h"
#include "Throttle.h"
#include "BrakeSystem.h"
#include "Fifo.h"
#include "SerialCommand.h"
#include "DCCFormatter.h"
#include "Function.h"
#include "IDccHardware.h"

class DccWrapper : public IDccHardware
{
    Fifo &_f;
    DCCFormatter &_d;

public:
    DccWrapper(Fifo &f, DCCFormatter &d) : _f(f), _d(d) {}
    void pushCommand(int id, bool s) override { _f.pushCommand(id, s); }
    void parseSerialCommand(char *data) override { SerialCommand::parse(data); }
    void setThrottle(int a, int s, bool d) override { _d.setThrottle(a, s, d); }
    void setCV(int a, int c, int v) override { _d.setCV(a, c, v); }
};


void connectSystems(Throttle &t, BrakeSystem &b, Fifo &f, DCCFormatter &d)
{
    static DccWrapper dccPlug(f, d); // Static so it lives forever

    // use interfaces for these
    setLocoStatusProvider(&t);
    t.setBrakeSystem(&b);
    t.setDccHardware(&dccPlug);
    b.setDccHardware(&dccPlug);

    // define all the callbacks
    // DCC commands
    // t.callbackPushCommand = [&](int id, bool state)
    // {
    //     f.pushCommand(id, state);
    // };

    // t.callbackCommandDCC = [&](char *data)
    // {
    //     // Map the throttle's request to the SerialCommand's static method
    //     SerialCommand::parse(data);
    // };

    // t.callbackThrottleDCC = [&](int dccAddress, int function, bool onOff)
    // {
    //     d.setThrottle(dccAddress, function, onOff);
    // };

    // t.callbackSetCvDCC = [&](int dccAddress, int cv, int value)
    // {
    //     d.setCV(dccAddress, cv, value);
    // };

    // DCC commands
    // b.callbackPushCommand = [&](int id, bool state)
    // {
    //     f.pushCommand(id, state);
    // };
}
