#pragma once
#include "Arduino.h"

class IBrakeSystem {
public:
    virtual uint16_t getTrainlinePSI(void) = 0;
    virtual uint16_t getEffectiveLocoBrake(void) = 0; // TBD or float?
    virtual float getEffectiveTrainBrake(void) = 0;
    virtual uint16_t getMainPSI(void) = 0;
    virtual void setCompressorFunction(uint16_t) = 0;
    virtual bool cycle(bool state) = 0;
    virtual void connectAirLine(bool connecting, uint16_t carCount = 0); // connecting true if connecting, false if disconnecting
    virtual float applyLocoBrake(bool applying) = 0;                      // applying = false if releasing, returns loco brake effectiveness as percent
    virtual float applyTrainBrake(bool applying) = 0;                        // applying = false if releasing, returns train brake effectiveness as percent
    virtual void applyEmmergency(bool applying) = 0;                         // returns emergency brake effectiveness as percent
    virtual void applyHandbrake(void) = 0;
    virtual void release(void) = 0;
    virtual void setPMRunning(bool complete) = 0;    // send this command to tell BS that PM has gone through startup sequence, start the compressor
    virtual bool locoBrakeOn(void) = 0;
    virtual bool trainBrakeOn(void) = 0;
    virtual bool emergencyBrakeOn(void) = 0;
};
