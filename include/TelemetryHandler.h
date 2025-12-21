#pragma once
#include "UdpTransport.h"
#include <IPAddress.h>

class TelemetryHandler {
public:
    TelemetryHandler(UdpTransport &transport);

    // 1) Single-call send (no cached destination)
    void sendTelemetry(const IPAddress &ip, uint16_t port, const char *msg);

    // 2) Set-and-send model
    void setTarget(const IPAddress &ip, uint16_t port);
    void sendTelemetry(const char *msg);

private:
    UdpTransport &_transport;
    IPAddress _cachedIP;
    uint16_t  _cachedPort;
};
