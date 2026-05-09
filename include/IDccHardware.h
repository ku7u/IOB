#pragma once
#include <stdint.h>

class IDccHardware {
public:
    virtual ~IDccHardware() {}
    // Grouping your current callbacks into one interface
    virtual void pushCommand(int id, bool state) = 0;
    virtual void parseSerialCommand(char* data) = 0;
    virtual void setThrottle(int addr, int speed, bool dir) = 0;
    virtual void setCV(int addr, int cv, int value) = 0;
};
