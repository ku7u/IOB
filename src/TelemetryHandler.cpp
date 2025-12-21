#include "TelemetryHandler.h"
#include <string.h>   // for strlen()

TelemetryHandler::TelemetryHandler(UdpTransport &transport)
: _transport(transport),
  _cachedIP(0,0,0,0),
  _cachedPort(0)
{
}

// ------------------------------------------------------
// 1) Single-call version
// ------------------------------------------------------
void TelemetryHandler::sendTelemetry(const IPAddress &ip, uint16_t port, const char *msg) {
    // _transport.send(ip, port, (const uint8_t*)msg, strlen(msg));
    _transport.send(ip, (const uint8_t*)msg, strlen(msg));
}

// ------------------------------------------------------
// 2) Cached-target version
// ------------------------------------------------------
void TelemetryHandler::setTarget(const IPAddress &ip, uint16_t port) {
    _cachedIP   = ip;
    _cachedPort = port;
}

void TelemetryHandler::sendTelemetry(const char *msg) {
    if (_cachedIP[0] == 0) return;   // not set yet
    // _transport.send(_cachedIP, _cachedPort, (const uint8_t*)msg, strlen(msg));
    _transport.send(_cachedIP,  (const uint8_t*)msg, strlen(msg));
}
