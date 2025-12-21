#pragma once
#include <Arduino.h>
#include "UdpTransport.h"
#include <ArduinoJson.h>
#include "Preferences.h"


class RollcallHandler {
public:
    RollcallHandler(UdpTransport &transport);

    void begin();   // joins multicast group
    void loop();    // handles incoming rollcalls

private:
    UdpTransport &_transport;
    Preferences myPrefs;
};
