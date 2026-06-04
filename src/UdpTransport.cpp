#include "UdpTransport.h"

UdpTransport::UdpTransport(uint16_t port)
: _port(port)
{
}

bool UdpTransport::begin() {
    return _udp.begin(_port);
}

bool UdpTransport::joinMulticast(const IPAddress &group) {
    return _udp.beginMulticast(group, _port);
}

int UdpTransport::receive(uint8_t *buffer, size_t maxLen,
                          IPAddress &senderIP, uint16_t &senderPort) 
{
    int packetSize = _udp.parsePacket();
    if (!packetSize) return 0;
    
    senderIP   = _udp.remoteIP();
    senderPort = _udp.remotePort();

    log_d("received from %s", senderIP.toString().c_str());

    return _udp.read(buffer, maxLen);
}

// bool UdpTransport::send(const IPAddress &addr, uint16_t port,
//                         const uint8_t *data, size_t len)
bool UdpTransport::send(const IPAddress &addr, const uint8_t *data, size_t len)
{
    _udp.beginPacket(addr, _port);
    _udp.write(data, len);
    return _udp.endPacket() == 1;
}




