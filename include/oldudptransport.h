

#pragma once
#include "WiFi.h"

class UdpTransport {
public:
    UdpTransport(uint16_t listenPort)
        : port(listenPort), multicastEnabled(false) {}

    // --- Initialization ------------------------------------------------------

    // Unicast-only receive
    bool begin() {
        return udp.begin(port);
    }

    // Multicast receive (also receives unicast on the same port)
    bool beginMulticast(const IPAddress& group) {
        multicastEnabled = udp.beginMulticast(group, port);
        return multicastEnabled;
    }

    // Do both: unicast listener + multicast group join
    bool beginBoth(const IPAddress& group) {
        bool a = udp.begin(port);
        bool b = udp.beginMulticast(group, port);
        multicastEnabled = b;
        return a && b;
    }

    // --- Receiving -----------------------------------------------------------

    int receive(uint8_t* buffer, size_t len, IPAddress& senderIP, uint16_t& senderPort) {
        int packetSize = udp.parsePacket();
        if (packetSize > 0) {
            int count = udp.read(buffer, len);
            senderIP   = udp.remoteIP();
            senderPort = udp.remotePort();
            return count;   // Number of bytes received
        }
        return 0;
    }

    // --- Sending -------------------------------------------------------------

    // unicast send
    bool sendUnicast(const IPAddress& ip, uint16_t portOut,
                     const uint8_t* data, size_t len) 
    {
        udp.beginPacket(ip, portOut);
        udp.write(data, len);
        return udp.endPacket() == 1;
    }

    // broadcast send
    bool sendBroadcast(uint16_t portOut, const uint8_t* data, size_t len) {
        return sendUnicast(IPAddress(255,255,255,255), portOut, data, len);
    }

    // multicast send
    bool sendMulticast(const IPAddress& group, const uint8_t* data, size_t len) {
        udp.beginPacket(group, port);
        udp.write(data, len);
        return udp.endPacket() == 1;
    }

private:
    WiFiUDP udp;
    uint16_t port;
    bool multicastEnabled;
};
