#pragma once

#include <stdint.h>
#include "defines.h"
#include "Arduino.h"
#include <functional> // for callbacks


class DCCFormatter
{
public:

    // callbacks
    std::function<void(int, bool)> callbackSetThrottle;

    void setFunction(int dccAddress, int function, bool onOff);
    void setThrottle(int dccAddress, int function, bool onOff);
    void setCV(int dccAddress, int cv, int value);
};