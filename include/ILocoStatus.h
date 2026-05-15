#pragma once
#include <stdint.h>

/**
 * @brief Interface for basic locomotive identification and movement status.
 */
class ILocoStatus {
public:
    virtual ~ILocoStatus() {}

    virtual int getDccAddress() = 0;
    // virtual int getRoadNumber() = 0;
};
