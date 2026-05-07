#include "CommandHandler.h"
#include "defines.h"
#include "Throttle.h"

CommandHandler::CommandHandler(UdpTransport &transport, Throttle &throttle)
    : _transport(transport), _throttle(throttle)
{
}

bool CommandHandler::getNext(PendingCommand &out)
{
    if (_tail == _head)
        return false; // queue empty

    out = _queue[_tail];
    _tail = (_tail + 1) % QUEUE_SIZE;
    return true;
}

void CommandHandler::enqueue(const char *topic,
                             const char *value,
                             IPAddress ip)
{
    int next = (_head + 1) % 8;
    if (next == _tail)
        return; // queue full, drop or log

    strncpy(_queue[_head].topic, topic, sizeof(_queue[_head].topic));
    strncpy(_queue[_head].value, value, sizeof(_queue[_head].value));
    _queue[_head].ip = ip;
    _head = next;
}

void CommandHandler::loop()
{
    uint8_t buf[256];
    IPAddress ip;
    uint16_t port;
    bool b;
    String msg;

    int len = _transport.receive(buf, sizeof(buf), ip, port);
    if (len == 0)
        return;

    buf[len] = 0; // <- null terminate
    msg = String((char *)buf).substring(0, len);

    log_d("Receivd %s having %d bytes from %s:%d", buf, len, ip.toString().c_str(), port);

    // Parse the JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, buf);

    if (error)
    {
#ifdef DEBUG_UDP
        Serial.print("JSON parse failed: ");
        Serial.println(error.c_str());
#endif
        return;
    }

    // Extract fields
    // const char *id = doc["id"];  TBD why is this still sent, fix it in the app
    const char *topic = doc["topic"];
    const char *value = doc["value"]; // TBD why const or not? required const for arduinoJson 7.x

    // enqueue(topic, value, ip);
    // return;

    JsonVariant valueVariant = doc["value"];

    const char *shortTopic = getSubstringAfterLastSlash(topic);

    if (strcmp(shortTopic, "sendstatus") == 0)
    {
        _throttle.inUse(true);
        _throttle.setControllingIP(ip);
        _throttle.reportCondition();
        _throttle.reportStatus();
    }
    else if (strcmp(shortTopic, "startstop") == 0)
        _throttle.pmOnOff(strcmp(value, "1") ? false : true);
    else if (strcmp(shortTopic, "stop") == 0)
        _throttle.panicStop();
    else if (strcmp(shortTopic, "bell") == 0)
        _throttle.bell(strcmp(value, "1") ? false : true);
    else if (strcmp(shortTopic, "horn") == 0)
        _throttle.horn(strcmp(value, "1") ? false : true);
    else if (strcmp(shortTopic, "headlight") == 0)
        _throttle.headlight(atoi(value));
    else if (strcmp(shortTopic, "rearlight") == 0)
        _throttle.rearlight(atoi(value));
    else if (strcmp(shortTopic, "notch") == 0)
        _throttle.manualNotch(strcmp(value, "1") ? false : true);
    else if (strcmp(shortTopic, "longpress") == 0)
        _throttle.longPress(strcmp(value, "1") ? false : true);
    else if (strcmp(shortTopic, "ibrake") == 0)
        _throttle.setLBrake(strcmp(value, "1") ? false : true);
    else if (strcmp(shortTopic, "tbrake") == 0)
        _throttle.setABrake(strcmp(value, "1") ? false : true);
    else if (strcmp(shortTopic, "trainline") == 0)
        _throttle.trainline(strcmp(value, "1") ? false : true);
    else if (strcmp(shortTopic, "carcount") == 0)
        _throttle.setCarCount(atoi(value));
    else if (strcmp(shortTopic, "reportlabels") == 0)
        _throttle.reportFunctionLabels();
    else if (strcmp(shortTopic, "calibrate") == 0)
        _throttle.calibrate(atoi(value));
    else if (strcmp(shortTopic, "function") == 0)
    {
        // valueVariant is a json formatted string like {"f":0,"s":1}
        static char buffer[32];
        serializeJson(valueVariant, buffer, sizeof(buffer));
        _throttle.setFunction(buffer);
    }
    else if (strcmp(shortTopic, "report") == 0)
        _throttle.report();
    else if (strcmp(shortTopic, "reverser") == 0)
        _throttle.setDirection(atoi(value));

    // mu processing
    else if (strcmp(shortTopic, "muSetState") == 0)
    {
        // a loco is chosen to be mued to a lead, value is a json string that includes lead id,
        static char buffer[256];
        serializeJson(valueVariant, buffer, sizeof(buffer));
        _throttle.muSetState(buffer);
    }
    // else if (strcmp(shortTopic, "muperformance") == 0)
    else if (strcmp(shortTopic, "muLocoData") == 0)
    {
        // a loco is chosen to be mued to a lead, value is a json string that includes lead id,
        static char buffer[256];
        // serializeJson(valueVariant, buffer, sizeof(buffer));
        // throttle.muSetPerformance(buffer);
        _throttle.muSetPerformance(msg.c_str());
    }
    else if (strcmp(topic, "muReport") == 0)
        _throttle.muReport(value);

    else if (strcmp(topic, "muMemberCheck") == 0)
        _throttle.muMemberResponse(value);

    else if (strcmp(topic, "muMemberResponse") == 0){
        if (strcmp(value, "true") == 0) _throttle.muMemberCheck(1);
        else _throttle.muMemberCheck(0);
    }

    else if (strcmp(topic, "muLeadStatus") == 0)
    {
        // speed command, possibly other data
        _throttle.muSetSpeed(msg.c_str());
    };
    // else if (strcmp(shortTopic, "muLeadHeadlight") == 0); // headclight command to trailing unit\

    // else if (strcmp(shortTopic, "muLeadRearlight") == 0;    // rearlight command to trailing unit
}

const char *CommandHandler::getSubstringAfterLastSlash(const char *input)
{
    const char *lastSlash = strrchr(input, '/'); // find last occurrence of '/'
    if (lastSlash)
    {
        return lastSlash + 1; // move past the slash
    }
    return input; // no slash found, return the whole string
}