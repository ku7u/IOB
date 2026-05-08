// Wiring.cpp
#include <stdint.h>

#include "Wiring.h"
#include "Throttle.h"
#include "BrakeSystem.h"
#include "Fifo.h"
#include "SerialCommand.h"
#include "DCCFormatter.h"
#include "Function.h"

void connectSystems(Throttle &t, BrakeSystem &b, Fifo &f, DCCFormatter &d)
{

    // Handshake: Throttle is cast to the ILocoStatus interface automatically
    setLocoStatusProvider(&t);

    // define all the callbacks
    // DCC commands
    t.callbackPushCommand = [&](int id, bool state)
    {
        f.pushCommand(id, state);
    };

    t.callbackCommandDCC = [&](char *data)
    {
        // Map the throttle's request to the SerialCommand's static method
        SerialCommand::parse(data);
    };

    t.callbackThrottleDCC = [&](int dccAddress, int function, bool onOff)
    {
        d.setThrottle(dccAddress, function, onOff);
    };

    t.callbackSetCvDCC = [&](int dccAddress, int cv, int value)
    {
        d.setCV(dccAddress, cv, value);
    };

    // brake subsystem
    t.callbackGetTLPsi = [&](void) -> uint16_t
    {
        uint16_t val = b.getTrainlinePSI();
        return val;
    };

    t.callbackGetEffectiveLocoBrake = [&](void) -> float
    {
        float val = b.getEffectiveLocoBrake();
        return val;
    };

    t.callbackGetEffectiveTrainBrake = [&](void) -> float
    {
        float val = b.getEffectiveTrainBrake();
        return val;
    };

    t.callbackGetMainPSI = [&](void) -> uint16_t
    {
        uint16_t val = b.getMainPSI();
        return val;
    };

    t.callbackSetCompressorFunction = [&](uint16_t id)
    {
        b.setCompressorFunction(id);
    };

    t.callbackBrakeSystemCycle = [&](bool state) -> bool
    {
        bool val = b.cycle(state);
        return val;
    };

    t.callbackConnectAirLine = [&](bool connecting)
    {
        b.connectAirLine(connecting);
    };

    t.callbackConnectAirLine2 = [&](bool connecting, uint16_t carcount = 0)
    {
        b.connectAirLine(connecting, carcount);
    };

    t.callbackApplyLocoBrake = [&](bool applying) -> float
    {
        float val = b.applyLocoBrake(applying);
        return val;
    };

    t.callbackApplyTrainBrake = [&](bool applying) -> float
    {
        float val = b.applyTrainBrake(applying);
        return val;
    };

    t.callbackApplyEmergency = [&](bool applying)
    {
        b.applyEmmergency(applying);
    };

    t.callbackApplyHandbrake = [&](void)
    {
        b.applyHandbrake();
    };

    t.callbackRelease = [&](void)
    {
        b.release();
    };

    t.callbackSetPMRunning = [&](bool complete)
    {
        b.setPMRunning(complete);
    };

    t.callbackLocoBrakeOn = [&](void) -> bool
    {
        bool val = b.locoBrakeOn();
        return val;
    };

    t.callbackTrainBrakeOn = [&](void) -> bool
    {
        bool val = b.trainBrakeOn();
        return val;
    };

    t.callbackEmergencyBrakeOn = [&](void) -> bool
    {
        bool val = b.emergencyBrakeOn();
        return val;
    };

    // DCC commands
    b.callbackPushCommand = [&](int id, bool state)
    {
        f.pushCommand(id, state);
    };
}
