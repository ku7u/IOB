#include "RollcallHandler.h"
#include "defines.h"

RollcallHandler::RollcallHandler(UdpTransport &transport)
    : _transport(transport)
{
}

void RollcallHandler::begin()
{
    // Join the multicast group for rollcall
    IPAddress group(239, 255, 0, 1); // <-- use your actual group address
    _transport.joinMulticast(group);
}

void RollcallHandler::loop()
{
    uint8_t buf[256];
    IPAddress senderIP;
    uint16_t senderPort;

    int len = _transport.receive(buf, sizeof(buf), senderIP, senderPort);
    if (len == 0)
        return;

    buf[len] = 0; // <- null terminate

    // At this point:
    //   buf       = rollcall message payload
    //   senderIP  = device that sent rollcall multicast
    //   senderPort= their UDP port

#ifdef SERIAL_ON
    Serial.printf("[Rollcall] Received %s having %d bytes from %s:%u\n",
                  buf,
                  len,
                  senderIP.toString().c_str(),
                  senderPort);
#endif

    // -----------------------------------------------
    //  DEVICE SENDS UNICAST RESPONSE BACK TO SENDER
    // -----------------------------------------------

    // Build JSON response
    JsonDocument doc;
    myPrefs.begin("loco");
    doc["locoID"] = myPrefs.getString("locoid", "none");
    doc["ip"] = WiFi.localIP().toString();
    doc["type"] = myPrefs.getString("locotype", "none");
    myPrefs.end();

    char jsonBuf[200];
    serializeJson(doc, jsonBuf);

    // bool ok = _transport.send(senderIP, senderPort,
    bool ok = _transport.send(senderIP, 
                              (const uint8_t *)jsonBuf,
                              strlen(jsonBuf));

    if (ok)
    {
#ifdef SERIAL_ON
        Serial.printf("[Rollcall] Sent response %s to %s:%u\n",
                      jsonBuf,
                      senderIP.toString().c_str(),
                      50005);
#endif
    }
    else
    {
#ifdef SERIAL_ON
        Serial.println("[Rollcall] Failed to send reply");
#endif
    }
}
