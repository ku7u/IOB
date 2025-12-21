#pragma once
#include <WiFi.h>
#include <WiFiUdp.h>

class UdpTransport {
public:
    UdpTransport(uint16_t port);

    bool begin();
    bool joinMulticast(const IPAddress &group);

    int  receive(uint8_t *buffer, size_t maxLen, IPAddress &senderIP, uint16_t &senderPort);

    // bool send(const IPAddress &addr, uint16_t port, const uint8_t *data, size_t len);
    bool send(const IPAddress &addr, const uint8_t *data, size_t len);
    




    uint16_t getPort() const { return _port; }

private:
    WiFiUDP _udp;
    uint16_t _port;
};



