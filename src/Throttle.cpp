#include <iostream>
#include "WiFi.h"
#include "Arduino.h"
#include "PubSubClient.h"
#include "defines.h"
#include "Throttle.h"
#include "SerialCommand.h"
#include "Function.h"
#include "MQTT.h"
#include "Preferences.h"
#include "Fifo.h"
#include "BrakeSystem.h"
#include <WiFiUdp.h> // UDP
#include <optional>

#ifdef USING_MQTT
extern PubSubClient client;
// #elif defined(USING_UDP)
// #include "TelemetryHandler.h" already in .h
#endif

extern Fifo commandFifo;
extern BrakeSystem bs;

#ifdef USING_UDP
extern WiFiUDP udp; // UDP
// extern const int TELEMETRY_PORT;
// extern const int COMMAND_PORT;
extern WiFiUDP udpCommand;
extern WiFiUDP udpTelemetry;
// extern WiFiUDP udpRollcall;
#endif

// Constructor
Throttle::Throttle(void)
    : telemetryPort(TELEMETRY_PORT_A), // initialize UdpTransport
      telemetry(telemetryPort)         // pass that into TelemetryHandler

{
    _notch = 0;
    _currentSpeed = 0;
    _lastCurrentSpeed = 0;
    _mph = 0;
    _lastNotch = 0;
    _horsepowerAtIdle = 15;

    _direction = true; // forward
    _throttleLever = 0;
    _carCount = 0;
    _tonnage = 0;
    // _trainlinePSI = rand() % TRAINLINE_SET_PSI; // random value between 0 and 75
    _trainlineSetPSI = TRAINLINE_SET_PSI;
    _neutral = true;
    _lastIntCurrentSpeed = 0;
    _lastTractiveForce = 0;
    _lastIntCurrentPsi = 0;
    _running = false;
    _compressorRunning = false;
    _compressorCountdown = 0;
}

// Getter implementation
bool Throttle::inUse() const
{
    return _inUse;
}

// Setter implementation
void Throttle::inUse(bool inUseValue)
{
    _inUse = inUseValue;
}

void Throttle::init()
{
    commandFifo.pushCommand(functionNotchingEnable, true);
    commandFifo.pushCommand(functionNotchUp, 0);
    commandFifo.pushCommand(functionNotchDown, 0);
}

void Throttle::setControllingIP(IPAddress ip)
{
    _controllingIP = ip;
}

void Throttle::getLocoPrefs(void)
{
    Preferences myPrefs;

    myPrefs.begin("loco");
    _dccAddress = myPrefs.getInt("dccaddress", 3);
    _roadNumber = myPrefs.getInt("roadnum", 0);
    _locoID = myPrefs.getString("locoid", "none");
    _locoType = myPrefs.getString("locotype", "none");
    // myPrefs.getBool("shortLong", 0);
    _horsepower = myPrefs.getInt("horsepower", 1500);
    _locoWeight = myPrefs.getULong("locoweight", 250000);
    _tractiveEffort = myPrefs.getLong("tractiveeffort", 70000);
    _topSpeed = myPrefs.getInt("topspeed", 60 * 5280 / 3600); // default to 60 mph or 88 fps gfh add 020525
    _odometer = myPrefs.getFloat("odometer", 0.0);
    _muState = static_cast<MuState>(myPrefs.getUInt("mustate", 0));
    _muLeadLoco = myPrefs.getString("muleadloco", "3");
    _muReversed = myPrefs.getBool("mureversed", false);
    myPrefs.end();

    myPrefs.begin("consist", false); // v0.26 ff

    // convert the saved json consist string to json document of loco data objects
    String muString;
    muString = myPrefs.getString("consist", "{}").c_str(); // seed the pref with "{}"
    DeserializationError error = deserializeJson(muDoc, muString.c_str());
    myPrefs.end();

    myPrefs.begin("calibration", true);
    // _calibrationTrapLength = myPrefs.getInt("traplength", 3);
    _fpsDccFactorForward2 = myPrefs.getFloat("speed2forward", 1.);
    _fpsDccFactorForward5 = myPrefs.getFloat("speed5forward", 1.);
    _fpsDccFactorForward10 = myPrefs.getFloat("speed10forward", 1.);
    _fpsDccFactorForward20 = myPrefs.getFloat("speed20forward", 1.);
    _fpsDccFactorForward50 = myPrefs.getFloat("speed50forward", 1.);
    _fpsDccFactorReverse2 = myPrefs.getFloat("speed2reverse", 1.);
    _fpsDccFactorReverse5 = myPrefs.getFloat("speed5reverse", 1.);
    _fpsDccFactorReverse10 = myPrefs.getFloat("speed10reverse", 1.);
    _fpsDccFactorReverse20 = myPrefs.getFloat("speed20reverse", 1.);
    _fpsDccFactorReverse50 = myPrefs.getFloat("speed50reverse", 1.);
    myPrefs.end();

    myPrefs.begin("general", true);
    _feedbackTopic = myPrefs.getString("feedbacktopic", "tlm/ols/");
    _commandTopic = myPrefs.getString("commandtopic", "cmd/ols/");
    myPrefs.end();

    _locoMass = _locoWeight / 32; // slugs

    if ((_muState == mid) || (_muState == trailing)) // v0.26 TBD this does not work here
        muSubscribe(true);
    else
    {
        muSumPerformanceValues(); // v0.26
        muSubscribe(false);
    }
}

void Throttle::getFunctionPrefs(void)
{
    Preferences myPrefs;

    myPrefs.begin("functions");
    functionPM = myPrefs.getInt("pm", 8);
    functionBell = myPrefs.getInt("bell", 1);
    functionHorn = myPrefs.getInt("horn", 2);
    functionHeadlightBright = myPrefs.getInt("headlightBright", 0);
    functionHeadlightDim = myPrefs.getInt("headlightDim", 6);
    functionRearlightBright = myPrefs.getInt("rearlightBright", 7);
    functionRearlightDim = myPrefs.getInt("rearlightDim", 10);
    functionNotchUp = myPrefs.getInt("notchUp", 26);
    functionNotchDown = myPrefs.getInt("notchDown", 27);
    functionNotchingEnable = myPrefs.getInt("notchingEnable", 28);
    functionIndependentBrake = myPrefs.getInt("iBrake", 5);
    functionTrainBrake = myPrefs.getInt("tBrake", 4);
    functionEmergencyBrake = myPrefs.getInt("eBrake", 0);
    functionCompressor = myPrefs.getInt("compressor", 20);
    functionBrakeSqueal = myPrefs.getInt("brakesqueal", -1);
    myPrefs.end();

    bs.setCompressorFunction(functionCompressor);
}

void Throttle::report() // TBR TBD obsolete
// respond to a broadcast message that requests who is online
// send locoID, ip address, loco type
{
    char topicChars[TOPIC_CHAR_SIZE]; // v0.26 was 30
    strcpy(topicChars, _feedbackTopic.c_str());
    strcat(topicChars, "report");

    String x = "{\"id\":\"";
    x.concat(_locoID);
    x.concat("\",\"ip\":\"");
    x.concat(WiFi.localIP().toString());
    x.concat("\",\"type\":\"");
    x.concat(_locoType);
    // x.concat("\",\"mu\":\"");    v0.26 removing quotes around _muState
    x.concat("\",\"mu\":");
    x.concat(_muState);
    // x.concat("\"}");
    x.concat("}");
#ifdef USING_MQTT
    client.publish(topicChars, x.c_str());
#endif
}

uint32_t Throttle::getTime()
// try to get GMT and return as unix time variable
{
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return (0);
    }
    time(&now);
    return now;
}

void Throttle::setRoadNumber(int roadNumber)
{
    _roadNumber = roadNumber;
}

int Throttle::getRoadNumber(void)
{
    return _roadNumber;
}

int Throttle::getDccAddress(void)
{
    return _dccAddress;
}

bool Throttle::isForward()
{
    if (_direction)
        return true;
    else
        return false;
}

bool Throttle::isRunning()
{
    return _running;
}

uint Throttle::getLastIntCurrentSpeed()
{
    return _lastIntCurrentSpeed;
}

void Throttle::setWaypoint(uint8_t waypoint, bool eastbound)
{
}

void Throttle::pmOnOff(bool onOff)
{
    // starts or stops the prime mover

    // this is to determine how long it has been since last run
    // could be used to set effects like main reservoir pressure due to leakdown
    Preferences myPrefs;
    uint32_t thisStartupTime;
    uint32_t deltaTime;
    const uint16_t DOWNTIME = 96;

    _running = onOff;

    if (!onOff) // shutdown

    {
        // save the mileage
        myPrefs.begin("loco", false);
        myPrefs.putFloat("odometer", _odometer);
        myPrefs.putULong("lastshuttime", getTime());
        myPrefs.end();
        _opMode = off;

        // do something to the brake system TBD
        bs.cycle(false); // v027

        // turn off PM on the mued loco(s) if this loco is a lead
        if (_muState == lead)
        {
            JsonObject root = muDoc.as<JsonObject>();
            char topicChars[TOPIC_CHAR_SIZE];

            for (JsonPair kv : root)
            {
                JsonObject obj = kv.value();
                const char *muIP = obj["muip"];

#ifdef USING_MQTT
                strcpy(topicChars, _commandTopic.c_str());
                strcat(topicChars, kv.key().c_str());
                strcat(topicChars, "/startstop");
                client.publish(topicChars, "0");

#elif defined(USING_UDP)
                JsonDocument doc;
                String jsonString;
                doc["topic"] = "startstop";
                doc["value"] = "0";
                serializeJson(doc, jsonString);

                udpCommand.beginPacket(muIP, COMMAND_PORT);
                udpCommand.write((uint8_t *)jsonString.c_str(), strlen(jsonString.c_str()));
                udpCommand.endPacket();
#ifdef DEBUG_UDP
                Serial.println("[pmOnOff] command sent: " + jsonString + " to: " + muIP);
#endif
#endif
            }
        }
    }
    else // power up
    {
        _opMode = idle;

        // in case the brakes were left on at last shutdown
        bs.applyEmmergency(false);
        _independentBrake = bs.applyLocoBrake(false);
        _trainBrake = bs.applyTrainBrake(false);
        // _independentBrake = 0;
        // _trainBrake = 0;

        // find the elapsed time since last shutdown
        _startTimestamp = millis();
        thisStartupTime = getTime();
        myPrefs.begin("loco", false);
        _lastShutdownTime = myPrefs.getULong("lastshuttime", 0);
        if (_lastShutdownTime > thisStartupTime)
            deltaTime = 0;
        else
            deltaTime = thisStartupTime - _lastShutdownTime;
        myPrefs.end();

        if (deltaTime > DOWNTIME * 3600)
            _trainlinePSI = 0;
        else if (deltaTime <= 0)
            // _trainlinePSI = TRAINLINE_SET_PSI;
            _trainlinePSI = rand() % TRAINLINE_SET_PSI + 50; // random value between 50 and TRAINLINE_SET_PSI
        else
            // bleeds down to zero after 48 hours
            _trainlinePSI = (1 - (deltaTime / (DOWNTIME * 3600.))) * TRAINLINE_SET_PSI;

        // syncronization scheme - run at powerup
        // use saved data in muDoc to query each loco that should be in consist
        // send individual query requesting mu status from muReport in the trailer
        // receive muLocoData responses
        // polled locos must respond whether they think they are in consist or not, reply contains muState
        // if muState > 1 keep in muDoc, update performance
        // if muState = 0 or no response, remove trailer from muDoc
        // locos that don't respond are likely orphans in purgatory
        // this may be enough. operator investigates loco using app and remediates the issue

        // possibly alternatively, any loco at pm turnon that is in consist queries the lead (assumes lead is alive)
        // if lead agrees then stay in consist
        // if not, change muState to 0
        // TBA the app must show consist status for trailers
        // TBA the app must restrict choice for trailers to noncontrolled and nonconsisted locos

        String myIP = WiFi.localIP().toString();
        JsonDocument doc;
        doc["topic"] = "muReport";
        String docString;

        // Iterate key-value pairs at top level
        // root is the loco id (key), its value is a json object
        JsonObject root = muDoc.as<JsonObject>();
        for (JsonPair kv : root)
        {
            const char *key = kv.key().c_str();
            JsonObject child = kv.value(); // this form is important, don't use 'as' clause
            const char *muip = child["muip"];

            doc["value"] = myIP;
            serializeJson(doc, docString);
#ifdef DEBUG_UDP
            Serial.println("[pmOnOff] docString: " + docString + " to " + String(muip));
#endif

            udpCommand.beginPacket(muip, COMMAND_PORT);
            udpCommand.write((uint8_t *)docString.c_str(), strlen(docString.c_str()));
            udpCommand.endPacket();
#ifdef DEBUG_UDP
            Serial.println("[pmOnOff] command sent: " + docString + " to: " + muip);
#endif
        }
        // we have sent commands to all known trailing locos
        // they will respond asynchronously
        // on subsequent itterations of main.loop the responses will be processed
        // this will rebuild muDoc so we can clear it now
        muDoc.clear();
    }

    commandFifo.pushCommand(functionPM, onOff);
    commandFifo.pushCommand(functionNotchingEnable, onOff); // TBD can't turn off PM unless this is here WMNS
    reportStatus();

    // TBD TBA add code here to start/stop all mued locos, or might just leave this as an operator task, like real world

    // #ifdef USING_MQTT
    //         client.publish(topicChars, msgChars);
    // #endif
    //     }
}

void Throttle::headlight(int offDimBright)
{
    // if mued no headlights are active unless it is the lead unit
    if ((_muState == mid) || (_muState == trailing))
        return;

    _headlight = offDimBright;

    if (offDimBright == 0)
    {
        commandFifo.pushCommand(functionHeadlightDim, false);
        commandFifo.pushCommand(functionHeadlightBright, false);
    }
    else if (offDimBright == 1)
    {
        commandFifo.pushCommand(functionHeadlightDim, true);
        commandFifo.pushCommand(functionHeadlightBright, false);
    }
    else if (offDimBright == 2)
    {
        commandFifo.pushCommand(functionHeadlightDim, false);
        commandFifo.pushCommand(functionHeadlightBright, true);
    }
}

void Throttle::rearlight(int offDimBright)
{
    switch (_muState)
    {
    case solo:
        if (offDimBright == 0)
        {
            commandFifo.pushCommand(functionRearlightDim, false);
            commandFifo.pushCommand(functionRearlightBright, false);
        }
        else if (offDimBright == 1)
        {
            commandFifo.pushCommand(functionRearlightDim, true);
            commandFifo.pushCommand(functionRearlightBright, false);
        }
        else if (offDimBright == 2)
        {
            commandFifo.pushCommand(functionRearlightDim, false);
            commandFifo.pushCommand(functionRearlightBright, true);
        }
        break;

    case mid:
        break;
    case lead:
    {
        String jsonString;
        JsonDocument doc;
        char buffer[4];
        itoa(offDimBright, buffer, 10);
        doc["topic"] = "rearlight";
        doc["value"] = buffer;

        // Iterate key-value pairs at top level
        // root is the loco id (key), its value is a json object
        // we are examining all mued locos attached to this lead
        // looking for the trailing (last) one
        // will send the rearlight command to that one and only that one
        JsonObject root = muDoc.as<JsonObject>();
        const char *muip;
        for (JsonPair kv : root)
        {
            const char *key = kv.key().c_str();
            JsonObject child = kv.value(); // this form is important, don't use 'as' clause
            muip = child["muip"];
            const int st = child["st"];
            if (st == 3) // trailing
                break;
        }
        serializeJson(doc, jsonString);
#ifdef DEBUG_UDP
        Serial.print("[pmOnOff] jsonString: " + jsonString + " to ");
        Serial.println(muip);
#endif

        udpCommand.beginPacket(muip, COMMAND_PORT);
        udpCommand.write((uint8_t *)jsonString.c_str(), strlen(jsonString.c_str()));
        udpCommand.endPacket();
    }
    break;

    case trailing:
        if (_muReversed) // the rear facing light is the physical headlight
        {
            if (offDimBright == 0)
            {
                commandFifo.pushCommand(functionHeadlightDim, false);
                commandFifo.pushCommand(functionHeadlightBright, false);
            }
            else if (offDimBright == 1)
            {
                commandFifo.pushCommand(functionHeadlightDim, true);
                commandFifo.pushCommand(functionHeadlightBright, false);
            }
            else if (offDimBright == 2)
            {
                commandFifo.pushCommand(functionHeadlightDim, false);
                commandFifo.pushCommand(functionHeadlightBright, true);
            }
        }
        else // not reversed so the rear facing light is the physical rearlight
        {
            if (offDimBright == 0)
            {
                commandFifo.pushCommand(functionRearlightDim, false);
                commandFifo.pushCommand(functionRearlightBright, false);
            }
            else if (offDimBright == 1)
            {
                commandFifo.pushCommand(functionRearlightDim, true);
                commandFifo.pushCommand(functionRearlightBright, false);
            }
            else if (offDimBright == 2)
            {
                commandFifo.pushCommand(functionRearlightDim, false);
                commandFifo.pushCommand(functionRearlightBright, true);
            }
        }
    }

    // if ((_muState == mid) || (_muState == lead)) // no rear lights for lead or mid consist locos v 0.18
    //     return;

    // else if ((_muState == trailing) && (_muReversed)) // if reversed the headlight is the rearlight
    // {
    //     if (offDimBright == 0)
    //     {
    //         commandFifo.pushCommand(functionHeadlightDim, false);
    //         commandFifo.pushCommand(functionHeadlightBright, false);
    //     }
    //     else if (offDimBright == 1)
    //     {
    //         commandFifo.pushCommand(functionHeadlightDim, true);
    //         commandFifo.pushCommand(functionHeadlightBright, false);
    //     }
    //     else if (offDimBright == 2)
    //     {
    //         commandFifo.pushCommand(functionHeadlightDim, false);
    //         commandFifo.pushCommand(functionHeadlightBright, true);
    //     }
    //     return;
    // }

    // _rearlight = offDimBright;

    // if (offDimBright == 0)
    // {
    //     commandFifo.pushCommand(functionRearlightDim, false);
    //     commandFifo.pushCommand(functionRearlightBright, false);
    // }
    // else if (offDimBright == 1)
    // {
    //     commandFifo.pushCommand(functionRearlightDim, true);
    //     commandFifo.pushCommand(functionRearlightBright, false);
    // }
    // else if (offDimBright == 2)
    // {
    //     commandFifo.pushCommand(functionRearlightDim, false);
    //     commandFifo.pushCommand(functionRearlightBright, true);
    // }
}

void Throttle::panicStop()
{
    char dccCommandChars[30];
    char buffer[10];
    itoa(_dccAddress, buffer, 10);

    strcpy(dccCommandChars, "t 1 ");
    strcat(dccCommandChars, buffer);
    strcat(dccCommandChars, " 0");
    SerialCommand::parse(dccCommandChars);

    _currentSpeed = 0;
    while (_notch > 0)
    {
        manualNotch(false);
    }
}

void Throttle::bell(bool onOff)
{
    if (_running)
    {
        commandFifo.pushCommand(functionBell, onOff);
        _bell = onOff;
    }
}

void Throttle::horn(bool onOff)
{

    if (_running)
    {
        commandFifo.pushCommand(functionHorn, onOff);
    }
}

void Throttle::setThrottleLever(int throttleLever)
{
    while (throttleLever != _notch)
    {
        if (throttleLever > _notch)
            manualNotch(true);
        else
            manualNotch(false);
    }
}

void Throttle::setDirection(int direction)
{
    char dummyChars[31];
    _neutral = false;
    _direction = true;

    if (direction == 0)
        _direction = false;
    else if (direction == 2)
        _direction = true;
    else
        _neutral = true;

    // send a speed command with zero speed just to set the current direction correctly in loco
    String dummyString = "t 1 ";
    dummyString.concat(String(_dccAddress) + " ");
    dummyString.concat(String(0) + " "); // zero speed

    if (_direction)
        dummyString.concat("1");
    else
        dummyString.concat("0");

    strcpy(dummyChars, dummyString.c_str());
    SerialCommand::parse(dummyChars);
}

void Throttle::setCarCount(uint16_t carcount)
{
    _carCount = carcount;
    _tonnage = carcount * AVERAGE_CAR_TONNAGE;
    reportCondition(); // v 0.15
}

void Throttle::setTonnage(uint16_t tonnage)
{
    _tonnage = tonnage;
    reportCondition(); // v 0.15
}

void Throttle::setTrainData(char *traindata)
{
    JsonDocument doc;

    // Deserialize the JSON document coming from candidate
    // DeserializationError error = deserializeJson(doc, jsonMsg);
    deserializeJson(doc, traindata);

    int length = doc["length"];
    int tonnage = doc["tonnage"];

    setTonnage(tonnage);
}

void Throttle::setLBrake(bool applying)
{
    if (applying)
        _opMode = braking;
    else
        _opMode = idle;

    commandFifo.pushCommand(functionIndependentBrake, applying);
#ifdef DEBUG_MQTT
    reportMqttDebug("locoBrake ", applying);
#endif

    _independentBrake = bs.applyLocoBrake(applying);
}

void Throttle::setABrake(bool applying)
{
    if (applying)
        _opMode = braking;
    else
        _opMode = idle;

    _trainBrake = bs.applyTrainBrake(applying);
    commandFifo.pushCommand(functionTrainBrake, applying);
}

void Throttle::setEBrake(bool applying)
{
    if (applying)
        _opMode = braking;
    else
        _opMode = idle;
    // TBD how to reset trainline pressure

    bs.applyEmmergency(applying);
    _trainBrake = bs.getEffectiveTrainBrake();
    _independentBrake = bs.getEffectiveLocoBrake();
    commandFifo.pushCommand(functionEmergencyBrake, applying);
}

void Throttle::trainline(bool connect)
// TBD this has to be linked to tbrake
//  also should respond to increase in car count rather than total cars if already connected
{
    if (connect)
    {
        bs.connectAirLine(true, _carCount);
        _trainlineConnected = true;
    }
    else
    {
        bs.connectAirLine(false);
        _trainlineConnected = false;
    }
}

void Throttle::manualNotch(bool up)
// this routine just sets the notch to be later processed in computeVelocity
{
#ifdef SERIAL_ON
    Serial.println("(manualNotch) " + up);
#endif

    uint32_t now;

    if (!_running) // nothing to do here, move along
        return;

    now = millis();
    if ((now - _startTimestamp) < 10000) // wait for decoder to prime itself v029A
        return;

    if (up && _opMode != braking)
    // notching up
    {
        if (_notch == 8)
            return;
        _opMode = powered;
        commandFifo.pushCommand(functionNotchUp, true);
        commandFifo.pushCommand(functionNotchUp, false);
        _notch++;
    }
    else if (up && _opMode == braking)
    // release the brakes
    {
        _opMode = idle;
        if (bs.emergencyBrakeOn())
            setEBrake(false);
        if (bs.trainBrakeOn())
            setABrake(false);
        if (bs.locoBrakeOn())
            setLBrake(false);
    }
    else if (!up && _opMode == powered)
    // notching down
    {
        if (_notch == 0)
            return;

        commandFifo.pushCommand(functionNotchDown, true);
        commandFifo.pushCommand(functionNotchDown, false);
        _notch--;
        if (_notch == 0)
        {
            _opMode = idle;
            // TBD adding next two lines as test to fix the hanging notch 1 issue
            // not an issue anymore, so delete these 012325 gfh
            // commandFifo.pushCommand(functionNotchDown, true); // this didn't work
            // commandFifo.pushCommand(functionNotchDown, false);
        }
    }
    else if (!up && ((_opMode == idle) || (_opMode == braking)))
    // incrementally apply brakes
    {
        setLBrake(true);
        // #ifdef SERIAL_ON
        //         Serial.println("after setLBrake");
        // #endif
        if (_trainlineConnected)
            setABrake(true);
        return; // so that notch is not redundantly returned to throttle
    }

    if ((_muState == lead) && (_mph == 0))
        reportStatus(); // TBD this might work, forces lead to emit status absolutely which trailers need

    if ((_muState == solo) || (_muState == lead)) // trailers in consist should not report notch
    {
        reportNotch();
    }
}

void Throttle::reportNotch()
{
    // force notch report to elicit haptic on throttle indicating idle

    char topicChars[TOPIC_CHAR_SIZE];
    strcpy(topicChars, _feedbackTopic.c_str());
    strcat(topicChars, _locoID.c_str());
    strcat(topicChars, "/notch");

    char buffer[10];
    itoa(_notch, buffer, 10);
    char msgChars[20];
    strcpy(msgChars, buffer);

#ifdef USING_MQTT
    client.publish(topicChars, msgChars);
#elif defined(USING_UDP)
    JsonDocument doc;
    String jsonString;
    doc["topic"] = "notch";
    doc["value"] = buffer;
    serializeJson(doc, jsonString);

    // udp publish via telemetry
    telemetry.setTarget(_controllingIP, 50003); // TBD this is ridiculous, fix in TelemetryHandler and remove this line
    telemetry.sendTelemetry(jsonString.c_str());
#endif
}

void Throttle::longPress(bool up)
// long press on the volume up or down buttons in Android app
{
    if ((_opMode == off) || (_muState == mid) || (_muState == trailing)) // if mued nothing to do here v0.26
        return;

    if (up && _opMode == braking)
    {
        // force notch report to elicit haptic on throttle indicating idle
        reportNotch();

        // release all brakes
        if (bs.emergencyBrakeOn())
            setEBrake(false);
        if (bs.trainBrakeOn())
            setABrake(false);
        if (bs.locoBrakeOn())
            setLBrake(false);
    }

    if (up && (_opMode == idle) && (_currentSpeed == 0))
    {
        // force notch report to elicit haptic on throttle indicating idle
        reportNotch();

        // set direction forward if in neutral
        if (_neutral)
        {
            _direction = true;
            _neutral = false;
        }
        else
            // switch direction
            _direction = !_direction;

        // send back new direction as telemetry
        char topicChars[TOPIC_CHAR_SIZE]; // TBD the 30
        strcpy(topicChars, _feedbackTopic.c_str());
        strcat(topicChars, _locoID.c_str());
        strcat(topicChars, "/reverser");

        char msgChars[10];
        if (_direction)
            strcpy(msgChars, "2");
        else
            strcpy(msgChars, "0");

#ifdef USING_MQTT
        client.publish(topicChars, msgChars);
#elif defined(USING_UDP)
        JsonDocument doc;
        String jsonString;
        doc["topic"] = "reverser";
        if (_direction)
            doc["value"] = "2";
        else
            doc["value"] = "0";
        serializeJson(doc, jsonString);

        // udp publish via telemetry
        telemetry.setTarget(_controllingIP, 50003); // TBD this is ridiculous, fix in TelemetryHandler and remove this line
        telemetry.sendTelemetry(jsonString.c_str());
#endif
    }
    else if (!up && _opMode == powered)
    // straight to zero
    {
        while (_notch > 0)
            manualNotch(false);
    }
    else if (!up && _mph > 10 && _opMode == idle)
        // apply emergency brake
        setEBrake(true);
    else if (!up && _mph <= 10 && ((_opMode == idle) || (_opMode == braking)))
        // quick stop at low speed TBD maybe release brakes also
        panicStop();
}

void Throttle::computeVelocity(void)
{
    uint16_t consistHorsepower = 0; // v0.26 ff
    uint16_t consistMass = 0;
    uint32_t consistTractiveEffort = 0;

    float effectiveHP = 0.0;
    float tractiveForce;
    float dragForce;
    float variableLocoDragForce;
    float startingForce;
    // float gradeForce;  //TBD TBA
    float independentBrakeForce;
    float emergencyBrakeForce;
    float trainBrakeForce;
    float accel;
    char dummyChars[31];
    uint16_t intCurrentSpeed;
    static bool zeroWasSent = false;
    static uint16_t lastIntCurrentSpeed;
    static uint16_t lastTrainlinePSI;
    static uint8_t startupCounter;

#ifdef DEBUG_SPEED
    // Serial.println("computeVelocity");
#endif

    if (!_running)
    {
        startupCounter = 0;
        bs.setPMRunning(false);
    }

    if (startupCounter < 15)
        startupCounter++;
    else if (startupCounter == 15)
    {
        bs.setPMRunning(true);
        startupCounter++;
    }

    bool compressorRunning = bs.cycle(true); // v027
    if ((!_running) || ((_muState == mid) || (_muState == trailing)))
        return;

    _trainlinePSI = bs.getTrainlinePSI();
    _trainBrake = bs.getEffectiveTrainBrake();
    _independentBrake = bs.getEffectiveLocoBrake();

#ifdef DEBUG_MQTT
    // reportMqttDebug("trainBrake ", _trainBrake);
#endif

    if (_trainlinePSI != lastTrainlinePSI)
    {
        reportStatus();
        lastTrainlinePSI = _trainlinePSI;
    }
    else if ((_mph == 0) && compressorRunning)
        reportStatus();

    consistHorsepower = _horsepower + _muHorsepower; // v0.26 ff
    consistMass = _locoMass + _muLocoMass;
    consistTractiveEffort = _tractiveEffort + _muTractiveEffort;
    _horsepowerAtIdle = consistHorsepower / 100;

    if (_neutral == true)
    {
#ifdef DEBUG_SPEED
        Serial.println("(computeVelocity) returning 2 (neutral)" + _neutral);
#endif
        return; // if in neutral don't waste time in here
    }

    if (_notch == 0)
        effectiveHP = 0;
    else if (_notch == 1)
        effectiveHP = _horsepowerAtIdle;
    // else
    //     effectiveHP = (_horsepower * (_notch - 1) / 7) - 50;
    else if (_notch == 2)
        effectiveHP = (consistHorsepower * (_notch) / 20.) - 50;
    else if (_notch == 3)
        effectiveHP = (consistHorsepower * (_notch) / 17.) - 50;
    else if (_notch == 4)
        effectiveHP = (consistHorsepower * (_notch) / 15.) - 50;
    else if (_notch == 5)
        effectiveHP = (consistHorsepower * (_notch) / 10.) - 50;
    else if (_notch == 6)
        effectiveHP = (consistHorsepower * (_notch) / 9.) - 50;
    else if (_notch == 7)
        effectiveHP = (consistHorsepower * (_notch) / 8.) - 50;
    else if (_notch == 8)
        effectiveHP = consistHorsepower - 50;

#ifdef DEBUG_SPEED
    Serial.println("(computeVelocity) _mph:  " + String(_mph));
// Serial.println(_mph);
#endif

    // compute the tractive force
    if (_mph <= 0)
        tractiveForce = effectiveHP * 308;
    else
    {
        tractiveForce = effectiveHP * 308 / (_mph * 1); // TBD REALITY_FACTOR goes here to replace the 1
        // if (tractiveForce > _tractiveEffort)
        //     tractiveForce = _tractiveEffort; // cap it
        if (tractiveForce > consistTractiveEffort)
            tractiveForce = consistTractiveEffort; // cap it
    }

    // reduce tractive force by the starting force effect only when starting
    if (_currentSpeed <= 0)
    {
        dragForce = 0;
        // startingForce = 20 * ((_locoMass * 32 / 2000) + _tonnage); // convert from slugs to lbs to tons
        startingForce = 20 * ((consistMass * 32 / 2000) + _tonnage); // convert from slugs to lbs to tons
        if (startingForce >= tractiveForce)
            tractiveForce = 0;
        else
            tractiveForce -= startingForce;
    }
    else
    // compute the moving drag force for any rolling stock
    {
        startingForce = 0;
        dragForce = (consistMass * 32 * ROLLING_RESISTANCE_COEFICIENT) + (_tonnage * 2000 * ROLLING_RESISTANCE_COEFICIENT);
    }
#ifdef DEBUG_SPEED
    Serial.println("(computeVelocity) _tonnage: " + _tonnage);
    Serial.println("(computeVelocity) _locoMass: " + _locoMass);
    Serial.println("computeVelocity) startingForce: " + startingForce);
#endif

    // this routine attempts to simulate spooling up
    if (_notch > 1)
        if (tractiveForce > 1.25 * _lastTractiveForce)
            tractiveForce = _lastTractiveForce + .15 * tractiveForce; // TBD was .25

    _lastTractiveForce = tractiveForce;

#ifdef DEBUG_SPEED
    Serial.println("(compuuteVelocity) tractiveForce: " + tractiveForce);
#endif

    // compute the drag forces
    // there must be some drag effect that varies with speed that is peculiar to locos - this is a guess
    variableLocoDragForce = consistMass * 32 * _currentSpeed * VARIABLE_LOCO_DRAG_COEFICIENT;

#ifdef DEBUG_SPEED
    Serial.println("computeVelocity) variableLocoDragForce: " + variableLocoDragForce);
#endif

    // consider brake forces if any
    independentBrakeForce = (_independentBrake / 100.) * consistMass * 32. * LOCO_FRICTION_COEFICIENT;

    if (_trainlineConnected)
    {
        trainBrakeForce = (_trainBrake)*_tonnage * 2000 * TRAIN_BRAKE_FRICTION_COEFICIENT; // v 0.11

        // emergencyBrakeForce = ((MIN_EFFECTIVE_EMERGENCY_BRAKE_LINE_PRESSURE / TRAINLINE_SET_PSI) * (_locoMass * 32 + _tonnage * 2000.) * TRAIN_BRAKE_FRICTION_COEFICIENT) * _emergencyBrake; // v 0.11 was .2
        // emergencyBrakeForce = (EMERGENCY_BRAKE_FACTOR * (_locoMass * 32. + _tonnage * 2000.) * TRAIN_BRAKE_FRICTION_COEFICIENT) * _emergencyBrake; // v 0.11 was .2
        // emergencyBrakeForce = EMERGENCY_BRAKE_FACTOR * _tonnage * 2000. * TRAIN_BRAKE_FRICTION_COEFICIENT * _emergencyBrake; // v 0.25
    }
    else
    {
        trainBrakeForce = 0;
        // emergencyBrakeForce = 0; // TBD
    }
#ifdef DEBUG_SPEED
    Serial.println("(computeVelocity) dragForce: " + dragForce);
    Serial.print("independentBrake and ...Force ");
    Serial.print(_independentBrake);
    Serial.print("   ");
    Serial.println(independentBrakeForce);
#endif

    // if (_emergencyBrake == 1) // v 0.13
    //     accel = (tractiveForce - dragForce - variableLocoDragForce - independentBrakeForce - emergencyBrakeForce) / (consistMass + (_tonnage * 2000 / 32));
    // else
    accel = (tractiveForce - dragForce - variableLocoDragForce - independentBrakeForce - trainBrakeForce) / (consistMass + (_tonnage * 2000 / 32));

    if (accel > MAX_ACCEL)
        accel = MAX_ACCEL;

#ifdef DEBUG_SPEED
    Serial.println("(computeVelocity) accel: " + accel);
#endif

    // integrate acceleration to get speed, here in fps, ultimately required by decoder
    // gfh mod for topSpeed 020525
    float lastCurrentSpeed = _currentSpeed;
    _currentSpeed = _currentSpeed + accel; // accel is feet/sec/sec so if integrated once / sec, accel = vel, _currentSpeed is feet/sec
    if (_currentSpeed < 0.)
        _currentSpeed = 0;
    else if (_currentSpeed > _topSpeed)
        _currentSpeed = lastCurrentSpeed;

    // integrate speed to obtain distance in feet, converted to miles when sent later
    // d = velocity x time but since here v is ft/sec, time is 1 (sec) so d = v x 1
    _odometer = _odometer + abs(_currentSpeed); // TBD on this (seems to work)

    // get the appropriate calibrated and interpolated speed compensation value for the decoder
    intCurrentSpeed = interpolateSpeedFactor(_currentSpeed);

    // prevent calculating a value greater than the max of 126
    if (intCurrentSpeed > 126)
        intCurrentSpeed = 126;

#ifdef DEBUG_SPEED
    Serial.println("(computeVelocity) _currentSpeed: " + _currentSpeed);
    Serial.println("(computeVelocity) _currentSpeed factored: " + intCurrentSpeed);
    Serial.print('\n');
#endif

    if (_mph > 0)
        zeroWasSent = false; // flags status to be sent for special case of intCurrentSpeed going to zero

    if ((intCurrentSpeed != lastIntCurrentSpeed) || (!zeroWasSent))
    {
        _mph = _currentSpeed * FPS_TO_MPH_FACTOR; // TBD fix this (why?)

        lastIntCurrentSpeed = intCurrentSpeed;

        // build the command string
        char buffer[20];
        strcpy(dummyChars, "t 1 ");

        itoa(_dccAddress, buffer, 10);
        strcat(dummyChars, buffer);
        strcat(dummyChars, " ");

        itoa(intCurrentSpeed, buffer, 10);
        strcat(dummyChars, buffer);
        strcat(dummyChars, " ");

        if (_direction)
            strcat(dummyChars, "1");
        else
            strcat(dummyChars, "0");

        SerialCommand::parse(dummyChars);
    }

    // send back odometer data to operator
    // TBD maybe send this once on startup and/or shutdown as well
    if ((_mph > 0) || (!zeroWasSent))
    {
        if (_mph == 0)
            zeroWasSent = true;
        reportStatus();

        if (_independentBrake >= 1)
            brakeSqueal(true);
        else
            brakeSqueal(false);
    }
}

void Throttle::calibrate(int speed)
{
    // this routine sets a factor that is applied to the commanded DCC speed such that
    // the actual scale speed over the rails is accurate with respect to commanded mph

    Preferences myPrefs;
    char dummyChars[31];
    long calibrationPeriod;
    int trapLength;
    long targetTime;
    float newFactor;
    float factorF;
    float factorR;
    int dccVal;

    // user is canceling
    if (speed == 0)
    {
        String stopString = "t 1 ";
        stopString.concat(String(_dccAddress) + " ");
        stopString.concat(String(0) + " ");
        stopString.concat("1");
        strcpy(dummyChars, stopString.c_str());
        SerialCommand::parse(dummyChars);
        _calibrationStage = 0;
        commandFifo.pushCommand(functionBell, false);
        return;
    }

    // sign of speed determines forward or reverse computation
    // as well as setting direction of motion

    if (abs(speed) == 2)
    {
        factorF = _fpsDccFactorForward2;
        factorR = _fpsDccFactorReverse2;
    }
    else if (abs(speed) == 5)
    {
        factorF = _fpsDccFactorForward5;
        factorR = _fpsDccFactorReverse5;
    }
    else if (abs(speed) == 10)
    {
        factorF = _fpsDccFactorForward10;
        factorR = _fpsDccFactorReverse10;
    }
    else if (abs(speed) == 20)
    {
        factorF = _fpsDccFactorForward20;
        factorR = _fpsDccFactorReverse20;
    }
    else
    {
        factorF = _fpsDccFactorForward50;
        factorR = _fpsDccFactorReverse50;
    }

    // calibration stages
    // 0 = startup to first gate
    // 1 = between gates
    // 2 = stopping after second gate

    if (_calibrationStage == 2)
    {
        switch (abs(speed))
        {
        case 2:
            trapLength = _calibrationTrapLength2;
            break;
        case 5:
            trapLength = _calibrationTrapLength5;
            break;
        case 10:
            trapLength = _calibrationTrapLength10;
            break;
        case 20:
            trapLength = _calibrationTrapLength20;
            break;
        case 50:
            trapLength = _calibrationTrapLength50;
            break;
        default:
            trapLength = 1;
            break;
        }
        calibrationPeriod = millis() - _calibrationTimer;
        // compute target time in ms to traverse test section at this speed point
        targetTime = 1000 * trapLength * 87 / (abs(speed) * (5280. / 3600));

        // divide calibrationPeriod by target value
        // multiply result by existing FPS_TO_DCC_FACTOR
        if (speed > 0)
            newFactor = factorF * calibrationPeriod / targetTime;
        else
            newFactor = factorR * calibrationPeriod / targetTime;

        myPrefs.begin("calibration", false);
        if (abs(speed) == 2)
        {
            if (speed > 0)
                myPrefs.putFloat("speed2forward", newFactor);
            else
                myPrefs.putFloat("speed2reverse", newFactor);
        }
        else if (abs(speed) == 5)
        {
            if (speed > 0)
                myPrefs.putFloat("speed5forward", newFactor);
            else
                myPrefs.putFloat("speed5reverse", newFactor);
        }
        else if (abs(speed) == 10)
        {
            if (speed > 0)
                myPrefs.putFloat("speed10forward", newFactor);
            else
                myPrefs.putFloat("speed10reverse", newFactor);
        }
        else if (abs(speed) == 20)
        {
            if (speed > 0)
                myPrefs.putFloat("speed20forward", newFactor);
            else
                myPrefs.putFloat("speed20reverse", newFactor);
        }
        else if (abs(speed) == 50)
        {
            if (speed > 0)
                myPrefs.putFloat("speed50forward", newFactor);
            else
                myPrefs.putFloat("speed50reverse", newFactor);
        }
        myPrefs.end();

        commandFifo.pushCommand(functionBell, false);
        getLocoPrefs(); // read storage into variables
    }
    else if (_calibrationStage == 1)
    // passing the start gate
    {
        _calibrationTimer = millis();
        commandFifo.pushCommand(functionBell, true);
    }

    if (_calibrationStage != 1) // either starting movement or stopping (0 or 2)
    {
        String dummyString = "t 1 ";
        dummyString.concat(String(_dccAddress) + " ");
        if (_calibrationStage == 0)
        {
            if (speed > 0)
                dccVal = abs(speed * factorF / FPS_TO_MPH_FACTOR);
            else
                dccVal = abs(speed * factorR / FPS_TO_MPH_FACTOR);

            dummyString.concat(String(dccVal) + " "); // starting
        }
        else
            dummyString.concat(String(0) + " "); // end of movement

        if (speed >= 0) // forward or reverse
            dummyString.concat("1");
        else
            dummyString.concat("0");

        strcpy(dummyChars, dummyString.c_str());
        SerialCommand::parse(dummyChars);
    }

    if (++_calibrationStage == 3)
        _calibrationStage = 0;

} // calibrate

void Throttle::muSetState(const char *jsonMsg)
{

    /*  from MU fragment on app select trailing locos
        command is sent only to the selected trailing loco

        when selected send mustate command to trailing unit for direction, position, lead loco id
        in response trailing unit replies with muLocoData topic including HP and mass of loco
        lead unit will then send startup command to trailing unit, just in case not running and also to ease operator loading
        lead unit will keep track of the trailers and display those on throttle

        messages causing change of mu state will be sent to this loco address and are processed here
        mu states:
         solo or 0 - not mued so leave consist and inform lead if was mid or trailing
         lead or 1 - mued as lead, look for incoming hp and mass values from locos in consist NOPE TBD why nope?
         mid or 2 - mued as mid, send hp and mass values to lead
         trailing or 3 - mued as trailing, send hp and mass values to lead

        json format
        {muState:"", leadID:"", reversed:""} */

    Preferences myPrefs;
    JsonDocument doc;
    String jsonString = "";

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, jsonMsg);
    if (error)
        return;

    int commandedState = doc["muState"]; // this is the new _muState
    String value = doc["reversed"];
    _muReversed = value.toInt();
    const char *mull;     // v 0.22
    mull = doc["leadID"]; // v 0.22

    const char *leadIpAdr;
    leadIpAdr = doc["leadIpAdr"];
    strlcpy(_leadIpAdr, leadIpAdr, sizeof(_leadIpAdr));

    switch (commandedState)
    {
    case 0: // not mued
        if ((_muState == mid) || (_muState == trailing))
        { // v 0.22 all of this block
            _muState = solo;
            _muLeadLoco = String(mull);
            // send my zero muState to lead now cuz I'm outta here
            char topicChars[TOPIC_CHAR_SIZE];
            strcpy(topicChars, _commandTopic.c_str());
            strcat(topicChars, _muLeadLoco.c_str());
            strcat(topicChars, "/muLocoData");

            char msgChars[100]; // myID, locomass, hp, tractive effort
            char buffer[50];

            strcpy(msgChars, "{\"id\":\"");
            strcat(msgChars, _locoID.c_str());
            strcat(msgChars, "\"");

            strcat(msgChars, ",\"st\":"); // mustate  v0.26 ff
            itoa(_muState, buffer, 10);   // v0.26
            strcat(msgChars, buffer);
            strcat(msgChars, "}");

            JsonDocument doc;
            doc["topic"] = "muLocoData";
            doc["muip"] = WiFi.localIP().toString();
            doc["id"] = _locoID.c_str();
            doc["st"] = String((int)_muState);
            serializeJson(doc, jsonString);

            // send the parameters to lead loco to affect its performance
#ifdef USING_MQTT
            client.publish(topicChars, msgChars);

            // unsubscribe from lead loco messages
            muSubscribe(false);
#elif defined(USING_UDP)
            udpCommand.beginPacket(leadIpAdr, COMMAND_PORT);
            udpCommand.write((uint8_t *)jsonString.c_str(), strlen(jsonString.c_str()));
            udpCommand.endPacket();
            // TBD something about unsubscribe?
#ifdef DEBUG_UDP
            Serial.println("[muSetState] unicast sent: " + jsonString + " to: " + String(leadIpAdr));
#endif
#endif
        }

        _muState = static_cast<Throttle::MuState>(commandedState);
        // store the state
        myPrefs.begin("loco");
        myPrefs.putUInt("mustate", _muState);
        // TBD store other aspects?
        myPrefs.end();
        break;

    // case 2 falls through into 3
    case 2: // mid
    case 3: // trailing
        // if (_muState != 0) // only allowed if coming from not mued  TBD this is questionable v0.26 commented this
        //     return;

        _muState = static_cast<Throttle::MuState>(commandedState);
        _consistMember = true; // tested every 60 seconds

        // const char *mull;    v 0.22
        // mull = doc["leadID"];
        _muLeadLoco = String(mull);
        // send my id, mass, hp and tractive effort to lead now
        // muReport(_muLeadLoco.c_str(), leadIpAdr);
        // muReport(mull, leadIpAdr);
        muReport(leadIpAdr);

        // subscribe to lead loco messages for speed, direction and notch
        // muSubscribe(true);

        break;
    }

    // save it all for next time
    myPrefs.begin("loco");
    myPrefs.putUInt("mustate", commandedState);
    myPrefs.putString("muleadloco", _muLeadLoco);
    myPrefs.putBool("mureversed", _muReversed);
    myPrefs.end();

    getLocoPrefs();

    // TODO: save leadIpAdr in myPrefs
    // change muReport to accept leadIpAdr as parameter
    // add udp transmission from muReport
    // deal with muSubscribe, what does that mean for udp?
} // muSetState

void Throttle::muReport(const char *leadIpAdr) // v0.26
{
    // this routine is run by trailing mued locos and results in a message sent that is received by lead loco to adjust its performance to include trailing units
    // this technique synchronizes lead and mu units on lead startup
    // all locos that think they are mued to a lead loco will respond to the muReport message if the locoID in that message equals their lead
    // the lead will build a roster of all candidates received
    // all received candidates will be included in loco speed status messages
    // a candidate will only respond to speed status messages that include its id, and will change its mu status to not mued if missing

    if ((_muState == mid) || (_muState == trailing))
    {

        String muIP = WiFi.localIP().toString();

        char topicChars[TOPIC_CHAR_SIZE];
        strcpy(topicChars, _commandTopic.c_str());
        strcat(topicChars, _muLeadLoco.c_str());
        strcat(topicChars, "/muperformance");

        char msgChars[256]; // myID, locomass, hp, tractive effort
        char buffer[100];

        String jsonString = "";

#ifndef USING_MQTT
        // strcpy(topicChars, "muperformance");
        strcpy(topicChars, "muLocoData");
#endif
        JsonDocument doc;
        doc["topic"] = topicChars;
        doc["id"] = _locoID.c_str();
        doc["muip"] = muIP.c_str();
        doc["mass"] = _locoMass;
        doc["hp"] = _horsepower;
        doc["te"] = _tractiveEffort;
        doc["st"] = String((int)_muState);
        serializeJson(doc, jsonString);

        strcpy(msgChars, "{\"id\":\"");
        strcat(msgChars, _locoID.c_str());

        strcat(msgChars, "\",\"topic\":\"");
        strcat(msgChars, topicChars);

        strcat(msgChars, "\",\"muip\":\"");
        strcat(msgChars, muIP.c_str());

        strcat(msgChars, "\",\"mass\":");
        itoa(_locoMass, buffer, 10);
        strcat(msgChars, buffer);

        strcat(msgChars, ",\"hp\":");
        itoa(_horsepower, buffer, 10);
        strcat(msgChars, buffer);

        strcat(msgChars, ",\"te\":");
        itoa(_tractiveEffort, buffer, 10); // v0.26
        strcat(msgChars, buffer);

        strcat(msgChars, ",\"st\":"); // mustate  v0.26 ff
        itoa(_muState, buffer, 10);   // v0.26
        strcat(msgChars, buffer);

        strcat(msgChars, "}");

        // send the parameters to lead loco to affect its performance
#ifdef USING_MQTT
        client.publish(topicChars, msgChars);

        // subscribe to the lead messages
        muSubscribe(true);

#elif defined(USING_UDP)
#ifdef DEBUG_UDP
        Serial.println("(muReport) 3");
#endif
        udpCommand.beginPacket(leadIpAdr, COMMAND_PORT);
        udpCommand.write((uint8_t *)jsonString.c_str(), strlen(jsonString.c_str()));
        udpCommand.endPacket();

#ifdef DEBUG_UDP
        Serial.println("[muReport] sent unicast: " + jsonString + " to: " + String(leadIpAdr));
#endif
#endif
    }
}

void Throttle::muSubscribe(bool subUnsub)
{
    // v026 changed all of this to handle the parameter

    // subscribe to lead loco messages for speed, direction and notch
    // subUnsub true to subscribe, false to unsubscribe
    char subscription[TOPIC_CHAR_SIZE];

    strcpy(subscription, _feedbackTopic.c_str());
    strcat(subscription, _muLeadLoco.c_str());
    strcat(subscription, "/status");

#ifdef USING_MQTT
    if (subUnsub)
        // client.subscribe(subscription, 1);
        client.subscribe(subscription, 0); // TBD testing QOS effect 7/12/24
    else
        client.unsubscribe(subscription);
#endif

    // subscribe to lead loco messages for headlight
    // strcpy(subscription, _feedbackTopic.c_str());
    strcpy(subscription, _commandTopic.c_str()); // v 0.17
    strcat(subscription, _muLeadLoco.c_str());
    strcat(subscription, "/headlight");

#ifdef USING_MQTT
    if (subUnsub)
        // client.subscribe(subscription, 1);
        client.subscribe(subscription, 0); // TBD testing QOS effect 7/12/24
    else
        client.unsubscribe(subscription);
#endif

    // subscribe to lead loco messages for rearlight
    // strcpy(subscription, _feedbackTopic.c_str());
    strcpy(subscription, _commandTopic.c_str()); // v 0.17
    strcat(subscription, _muLeadLoco.c_str());
    strcat(subscription, "/rearlight");

#ifdef USING_MQTT
    if (subUnsub)
        // client.subscribe(subscription, 1);
        client.subscribe(subscription, 0); // TBD testing QOS effect 7/12/24
    else
        client.unsubscribe(subscription);
#endif

} // muSubscribe

void Throttle::muSetPerformance(const char *jsonMsg)
{
    // TBD
    // v 0.20 complete

    // runs in response to muLocoData message from trailing loco(s)
    // set muState to 1 if adding trailers
    // set muState to 0 if depleting trailers
    // lead unit keeps track of the consist in muDoc and stores in Preferences for future use TBD is there a need to store? yes there is, can't broadcast so need ip addrs.
    // also saves trailers hp, mass and tractive effort to be added to lead unit parameters
    // do nothing else

    // v 0.26 using saved json doc
    // if coming in here, mustate = 1 then add to the existing json doc
    // else clear the existing and start a new one although the existing should already be clear

    // TBD maybe add a reset feature in here
    // if the locoID is mine, then reset muDoc to {}

    Preferences myPrefs;
    JsonDocument doc;
    String consistString;
    StaticJsonDocument<200> doc1;
    String jsonString;

#ifdef DEBUG_UDP
    Serial.print("[muSetPerformance] received jsonMsg ");
    Serial.println(jsonMsg);
#endif

    // Deserialize the JSON document coming from candidate
    // DeserializationError error = deserializeJson(doc, jsonMsg);
    deserializeJson(doc, jsonMsg);

    String locoID = doc["id"];
    const char *muIP = doc["muip"];
    int mass = doc["mass"];
    int hp = doc["hp"];
    uint32_t te = doc["te"]; // tractive effort v0.26
    int st = doc["st"];      // muState v0.26

    char topicChars[TOPIC_CHAR_SIZE]; // v 0.24 all of this startstop stuff
    strcpy(topicChars, _commandTopic.c_str());
    strcat(topicChars, locoID.c_str());
    strcat(topicChars, "/startstop");

    // determine if removing (st==0) or adding (st!=0) a candidate
    if (st == 0) // remove this loco from the json doc, etc.
    {
        // look for this candidate in muDoc and remove it
        JsonObject root = muDoc.as<JsonObject>(); // undocumented as to need for this
        root.remove(locoID);

        if (muDoc.size() == 0)
        {
            // if no more loco ids in muDoc then reset _muState
            _muState = solo;
        }

        serializeJson(muDoc, consistString);

        // save it in Preferences version 0.38.16
        myPrefs.begin("consist"); // v0.26 ff
        myPrefs.putString("consist", consistString);
        myPrefs.end();

        myPrefs.begin("loco"); // v0.26 ff
        myPrefs.putUInt("mustate", _muState);
        myPrefs.end();

        // turn off PM on the mued loco
#ifdef USING_MQTT
        client.publish(topicChars, "0");
#else
        jsonString = "";

        doc1["topic"] = "startstop";
        doc1["value"] = "0";
        serializeJson(doc1, jsonString);
        udpCommand.beginPacket(muIP, COMMAND_PORT);
        udpCommand.write((uint8_t *)jsonString.c_str(), strlen(jsonString.c_str()));
        udpCommand.endPacket();
#ifdef DEBUG_UDP
        Serial.println("[muSetPerformance] command sent: " + jsonString + " to: " + muIP);
#endif
#endif

        muSumPerformanceValues();
    }
    else
    {
        _muState = lead; // we are the lead

        // start PM on the mued loco (may already be running, doesn't matter)
        // if lead is not running then don't start
        // separate logic in startup proc to start mued locos TBD
        if (_running)
        {
#ifdef USING_MQTT
            client.publish(topicChars, "1");
#else
            jsonString = "";

            doc1["topic"] = "startstop";
            doc1["value"] = "1";
            serializeJson(doc1, jsonString);

            udpCommand.beginPacket(muIP, COMMAND_PORT);
            udpCommand.write((uint8_t *)jsonString.c_str(), strlen(jsonString.c_str()));
            udpCommand.endPacket();
#ifdef DEBUG_UDP
            Serial.println("[muSetPerformance] lead running, command sent: " + jsonString + " to: " + muIP);
#endif
#endif
        }

        // v 0.26 ff
        // TBD before we do the following, try to find existing id for this candidate in muDoc
        // if found, replace it or maybe do nothing, although we may be updating performance characteristics (seems unlikely)
        // look for this candidate in muDoc and remove it
        // ...later learned that to(JsonObject) creates or replaces, as(JsonObject) updates an existing entry
        // if (!muDoc.containsKey(locoID))
        {
            JsonObject candidate = muDoc[locoID].to<JsonObject>();
            // else
            // JsonObject candidate = muDoc[locoID].as<JsonObject>(); would this work to update an existing?
            candidate["hp"] = hp;
            candidate["mass"] = mass;
            candidate["te"] = te;
            candidate["muip"] = muIP;

            // save the serialized and changed muDoc in Preferences version 0.38.16
            serializeJson(muDoc, consistString);
            myPrefs.begin("consist"); // v0.26 ff
            myPrefs.putString("consist", consistString);
            myPrefs.end();

            // save the mustates in Preferences
            myPrefs.begin("loco"); // v0.26 ff
            myPrefs.putUInt("mustate", _muState);
            myPrefs.end();

            muSumPerformanceValues();
            reportStatus(); // so the app has a current view of mu status
        }
    }
}

void Throttle::muSetSpeed(const char *jsonMsg)
{
    // receive speed messages from lead unit, set this unit's speed to match

    static bool lastBrake;
    float lastOdo = 0.; // odometer update

    if ((_muState == solo) || (_muState == lead) || (!_running)) // TBD this is a workaround that can't be left in the code why? because we may not be running
        return;

    JsonDocument doc;

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, jsonMsg);
    if (error)
    {
#ifdef SERIAL_ON
        Serial.println("muSetSpeed error");
#endif
        return;
    }

    // retrieve mph value
    float muMPH = doc["mph"];

    // first check to determine if lead thinks we should be in consist
    // my locoID should be in the consist string, if it isn't we give up the consist spot
    String consistString = doc["consist"];
    if ((consistString.indexOf(_locoID) == -1) && (muMPH > 0)) // need to qualify for speed > 0 to avoid disconnecting because of preliminary status msgs
    {
        _muState = solo;

        Preferences myPrefs;
        myPrefs.begin("loco");
        myPrefs.putUInt("mustate", _muState);
        myPrefs.end();

        muSubscribe(false);

        return;
    }

    // wiggle the value to outsmart BEMF
    // TBD in testing, this did not seem to be necessary using LokSound5 with BEMF in use
    // if (muMPH > .5)
    // {
    //     if (alternateSeconds)
    //         muMPH += .25; // v0.26 was .5 both places
    //     else
    //         muMPH -= .25;
    // }

    // retrieve direction, 1=fwd, 0=rev
    bool direction = doc["dir"];

    if (_muReversed) // true if running reversed in consist
        direction = !direction;

    _direction = direction;

    // retrieve notch in order to alter PM sound
    u16_t notch = doc["notch"];
    while (_notch != notch)
        manualNotch(_notch < notch); // if true notch up, else down until equal

    bool brk = doc["brk"];
    if (brk != lastBrake)
        commandFifo.pushCommand(functionIndependentBrake, brk);
    lastBrake = brk;

    float odo = doc["odo"]; // TBD not working v0285
    if (odo != lastOdo)
    {
        _odometer += .01;
        lastOdo = odo;
    }

    // control this loco's speed
    // convert mph to fps
    float muFPS = muMPH / FPS_TO_MPH_FACTOR;

    uint16_t dccFPS = interpolateSpeedFactor(muFPS); // returns dcc val corresponding to fps, interpolated between cal points

    // prevent computing a value higher than 126
    if (dccFPS > 126)
        dccFPS = 126;

    char buffer[20];
    char dccCommandChars[30];
    strcpy(dccCommandChars, "t 1 ");
    itoa(_dccAddress, buffer, 10);
    strcat(dccCommandChars, buffer);
    strcat(dccCommandChars, " ");
    itoa(dccFPS, buffer, 10);
    strcat(dccCommandChars, buffer);
    strcat(dccCommandChars, " ");

    if (direction)
        strcat(dccCommandChars, "1");
    else
        strcat(dccCommandChars, "0");

    SerialCommand::parse(dccCommandChars);
}

void Throttle::reportCondition()
{
    // sends static condition to app whenever app opens throttle fragment
    // this sets the state of various views in the fragment

    String jsonString = "";

    char topicChars[TOPIC_CHAR_SIZE];
    char msgChars[200]; // v 0.16
    char charPsi[10];
    char charCc[4];        // car count
    char charCarCount[10]; // v 0.15
    char charTonnage[10];

    // build the topic string
    strcpy(topicChars, _feedbackTopic.c_str());
    strcat(topicChars, _locoID.c_str());
    strcat(topicChars, "/condition");

    // build the msg json string
    // PM status, on or off
    strcpy(msgChars, "{\"pm\":");
    const char charPm[2] = {char(_running + 48), 0}; // 48 = ascii zero, so sends back "0" or "1"
    strcat(msgChars, charPm);

    strcat(msgChars, ",\"rvrsr\":");
    uint revPos;
    if (_neutral)
        revPos = 1;
    else if (_direction)
        revPos = 2;
    else
        revPos = 0;
    const char charDir[2] = {char(revPos + 48), 0}; // 48 = ascii zero
    strcat(msgChars, charDir);

    // headlight status off, dim, bright
    strcat(msgChars, ",\"hl\":");                      // new 10/29
    const char charHl[2] = {char(_headlight + 48), 0}; // 48 = ascii zero
    strcat(msgChars, charHl);

    // rearlight status off, dim, bright
    strcat(msgChars, ",\"rl\":");                      // new 10/29
    const char charRl[2] = {char(_rearlight + 48), 0}; // 48 = ascii zero
    strcat(msgChars, charRl);

    // bell
    strcat(msgChars, ",\"bell\":");               // new 11/7
    const char charBl[2] = {char(_bell + 48), 0}; // 48 = ascii zero
    strcat(msgChars, charBl);

    // car count    TBD this and 'cars' below one is superfluous
    // strcat(msgChars, ",\"cc\":"); // new 11/8
    // dtostrf(_carCount, 2, 0, charCc);
    // strcat(msgChars, charCc);

    // trainline
    strcat(msgChars, ",\"psi\":"); // new 10/29
    dtostrf(_trainlinePSI, 2, 0, charPsi);
    strcat(msgChars, charPsi);

    // mu status
    strcat(msgChars, ",\"mu\":");                    // new 10/29
    const char charMu[2] = {char(_muState + 48), 0}; // 48 = ascii zero
    strcat(msgChars, charMu);

    // if mued, the lead loco id  TBD this may not be necessary as it is handled elsewhere
    if ((_muState == mid) || (_muState == trailing))
    {
        strcat(msgChars, ",\"muto\":");
        strcat(msgChars, _muLeadLoco.c_str());
    }

    // trainline connection status
    strcat(msgChars, ",\"tl\":");                               // v 0.15
    const char charTl[2] = {char(_trainlineConnected + 48), 0}; // 48 = ascii zero
    strcat(msgChars, charTl);

    strcat(msgChars, ",\"cars\":"); // v 0.15
    dtostrf(_carCount, 3, 0, charCarCount);
    strcat(msgChars, charCarCount);

    // hauled tonnage
    strcat(msgChars, ",\"tons\":"); // v 0.15
    dtostrf(_tonnage, 5, 0, charTonnage);
    strcat(msgChars, charTonnage);

    strcat(msgChars, "}");

    JsonDocument doc;
    doc["topic"] = "condition";
    doc["id"] = "_locoID.c_str()";
    doc["pm "] = charPm;
    doc["rvrsr"] = charDir;
    doc["hl"] = charHl;
    doc["rl"] = charRl;
    doc["bell"] = charBl;
    doc["psi"] = charPsi;
    doc["mu"] = charMu;
    doc["muto"] = _muLeadLoco.c_str();
    doc["tl"] = charTl;
    doc["cars"] = charCarCount;
    doc["tons"] = charTonnage;
    serializeJson(doc, jsonString);

#ifdef USING_MQTT
    client.publish(topicChars, msgChars);
#elif defined(USING_UDP)
    // udp publish via telemetry
#ifdef DEBUG_UDP
    Serial.println("[reportCondition] sending " + jsonString);
#endif
    telemetry.setTarget(_controllingIP, 50003); // TBD this is ridiculous, fix in TelemetryHandler and remove this line
    telemetry.sendTelemetry(jsonString.c_str());
#endif
}

void Throttle::reportStatus()
{
    // reports various current values back to the app for display there
    // the mqtt msg is in json format

    String jsonString = "";

    _trainlinePSI = bs.getTrainlinePSI();

    int intSpeedoSpeed = _mph * 10;
    float speedoSpeed = intSpeedoSpeed / 10.; // to get tenths of mph

    char topicChars[TOPIC_CHAR_SIZE]; // v 0.25
    strcpy(topicChars, _feedbackTopic.c_str());
    strcat(topicChars, _locoID.c_str());
    strcat(topicChars, "/status");

    char msgChars[100];
    char charSpeed[10];
    char charOdo[10];
    char charPsi[10];

    JsonDocument doc;
    bool locoBrkOn = bs.locoBrakeOn();
    uint16_t mainPsi = bs.getMainPSI();
    // round to 1, 2 decimal places
    float speedo2 = round(speedoSpeed * 10.f) / 10.f;
    float odo2 = round((_odometer * 100.) / 5280.) / 100.0f;
    doc["topic"] = "status";
    doc["id"] = _locoID.c_str();
    doc["mph"] = speedo2;
    doc["dir"] = (uint16_t)_direction;
    doc["notch"] = (uint16_t)_notch;
    doc["brk"] = (uint16_t)locoBrkOn;
    doc["odo"] = odo2;
    doc["psi"] = _trainlinePSI, 0;
    doc["mp"] = (float)mainPsi, 0;

    // add trailing loco IDs if lead loco in consist v 0.26 ff
    // JSON document muDoc was populated by muSetPerformance
    if (_muState == lead) // loco is the lead loco in a consist
    {
        // build a string of the consisted loco IDs as derived from muDoc
        // add them to the json string
        JsonObject root = muDoc.as<JsonObject>();
        if (muDoc.isNull())
            _muState = solo; // failsafe here
        else
        {
            uint8_t counter = 0;
            char muStr[256] = "";

            // https://arduinojson.org/v7/api/jsonobject/begin_end/
            for (JsonPair kv : root)
            {
                if (counter == 0)
                    strcpy(muStr, kv.key().c_str());
                else
                {
                    strcat(muStr, ",");
                    strcat(muStr, kv.key().c_str());
                }
                counter++;
            }

            doc["consist"] = muStr;
        }
    }
    serializeJson(doc, jsonString);

#ifdef USING_MQTT
    client.publish(topicChars, jsonString.c_str());
#elif defined(USING_UDP)
    // udp publish via telemetry
    telemetry.setTarget(_controllingIP, 50003); // TBD this is ridiculous, fix in TelemetryHandler and remove this line
    telemetry.sendTelemetry(jsonString.c_str());
#ifdef DEBUG_UDP
    Serial.println("[reportStatus] Sent telemetry: " + jsonString + " to: " + _controllingIP);
#endif

    if (_muState == lead) // check for lead unit
    {
        // send status to each of the trailing locos, if any
        // Iterate key-value pairs at top level
        // root is the loco id (key), its value is a json object
        // first remove the unneeded bits
        doc["topic"] = "muLeadStatus";
        doc.remove("psi");
        doc.remove("mp");
        serializeJson(doc, jsonString);

        JsonObject root = muDoc.as<JsonObject>();
        for (JsonPair kv : root)
        {
            const char *key = kv.key().c_str();
            JsonObject child = kv.value(); // this form is important, don't use 'as' clause
            const char *muip = child["muip"];
            udpCommand.beginPacket(muip, COMMAND_PORT);
            udpCommand.write((uint8_t *)jsonString.c_str(), strlen(jsonString.c_str()));
            udpCommand.endPacket();
#ifdef DEBUG_UDP
            Serial.println("[reportStatus] Sent unicast to trailer: " + jsonString + " to: " + muip);
#endif
        }
#endif
}
}

void Throttle::reportMqttDebug(String parameter, float value) // v 0.25
{
    // report debug messages via mqtt
    char topicChars[TOPIC_CHAR_SIZE];
    strcpy(topicChars, _feedbackTopic.c_str());
    strcat(topicChars, _locoID.c_str());
    strcat(topicChars, "/debug/");
    strcat(topicChars, parameter.c_str());

    char msgChars[40];
    char charValue[20];

    dtostrf(value, 8, 2, charValue);
    strcpy(msgChars, charValue);

#ifdef USING_MQTT
    client.publish(topicChars, msgChars);
#endif
}

void Throttle::reportMqttDebugString(String parameter, String data) // v 0.25
{
    // report debug messages via mqtt
    char topicChars[TOPIC_CHAR_SIZE];
    strcpy(topicChars, _feedbackTopic.c_str());
    strcat(topicChars, _locoID.c_str());
    strcat(topicChars, "/debug/");
    strcat(topicChars, parameter.c_str());

    char msgChars[400];

    strcpy(msgChars, data.c_str());

#ifdef USING_MQTT
    client.publish(topicChars, msgChars);
#endif
}

void Throttle::reportFunctionLabels()
{
    // reports all of the configured function labels to the app for display there
    // called whenever sendStatus is requested by the app

    char topicChars[40];
    Preferences myPrefs;

    strcpy(topicChars, _feedbackTopic.c_str());
    strcat(topicChars, _locoID.c_str());
    strcat(topicChars, "/functionLabels");

    char msgChars[1024]; // TBD limit the labels (elsewhere) to 30 characters
    // String iString;
    // String labelString;

    // strcpy(msgChars, "{");
    // // open the spiff
    // myPrefs.begin("functions", true);
    // for (int i = 0; i < 29; i++)
    // {
    //     // convert i to string
    //     iString = "f" + String(i);
    //     // get the string from spiff
    //     labelString = myPrefs.getString(iString.c_str(), "");
    //     // // build the string including the i string
    //     strcat(msgChars, "\"");
    //     strncat(msgChars, iString.c_str(), 3);
    //     strcat(msgChars, "\":\"");
    //     // concat the label
    //     strncat(msgChars, labelString.c_str(), 10);
    //     strcat(msgChars, "\"");
    //     if (i < 28)
    //         strcat(msgChars, ",");
    // }
    // myPrefs.end();

    // strcat(msgChars, "}");

    // following from chatGPT 12/07/25
    // places the labels into a json array, {"labels":["label1","label2"...]}
    StaticJsonDocument<2048> doc; // enough for 29 labels

    doc["topic"] = "functionLabels";

    JsonArray labels = doc["labels"].to<JsonArray>();

    myPrefs.begin("functions", true);
    for (int i = 0; i < 29; i++)
    {
        String key = "f" + String(i);
        String value = myPrefs.getString(key.c_str(), "");
        labels.add(value);
    }
    myPrefs.end();

    String output;
    serializeJson(doc, output);

#ifdef USING_MQTT
    // client.publish(topicChars, msgChars);
    client.publish(topicChars, output.c_str());
#elif defined(USING_UDP)
    // TBA UDP
    // udp publish via telemetry
    telemetry.setTarget(_controllingIP, 50003); // TBD this is ridiculous, fix in TelemetryHandler and remove this line
    telemetry.sendTelemetry(output.c_str());
#ifdef DEBUG_UDP
    Serial.println("[reportFunctionLabels] " + output);
#endif
#endif
}

uint16_t Throttle::interpolateSpeedFactor(float fps)
{
    // provides an interpolated value of calibration factors between stored values

    float factorF;
    float factorR;

    float mph = fps * FPS_TO_MPH_FACTOR;

    if (fps <= FPS_AT_MPH_FACTOR2)
    {
        factorF = _fpsDccFactorForward2;
        factorR = _fpsDccFactorReverse2;
    }
    else if (fps <= FPS_AT_MPH_FACTOR5)
    {
        factorF = _fpsDccFactorForward2 + ((_fpsDccFactorForward5 - _fpsDccFactorForward2) * (mph - 2) / 3);
        factorR = _fpsDccFactorReverse2 + ((_fpsDccFactorReverse5 - _fpsDccFactorReverse2) * (mph - 2) / 3);
    }
    else if (fps <= FPS_AT_MPH_FACTOR10)
    {
        factorF = _fpsDccFactorForward5 + ((_fpsDccFactorForward10 - _fpsDccFactorForward5) * (mph - 5) / 5);
        factorR = _fpsDccFactorReverse5 + ((_fpsDccFactorReverse10 - _fpsDccFactorReverse5) * (mph - 5) / 5);
    }
    else if (fps <= FPS_AT_MPH_FACTOR20)
    {
        factorF = _fpsDccFactorForward10 + ((_fpsDccFactorForward20 - _fpsDccFactorForward10) * (mph - 10) / 10);
        factorR = _fpsDccFactorReverse10 + ((_fpsDccFactorReverse20 - _fpsDccFactorReverse10) * (mph - 10) / 10);
    }
    else if (fps <= FPS_AT_MPH_FACTOR50)
    {
        factorF = _fpsDccFactorForward20 + ((_fpsDccFactorForward50 - _fpsDccFactorForward20) * (mph - 20) / 30);
        factorR = _fpsDccFactorReverse20 + ((_fpsDccFactorReverse50 - _fpsDccFactorReverse20) * (mph - 20) / 30);
    }
    else
    {
        factorF = _fpsDccFactorForward50;
        factorR = _fpsDccFactorReverse50;
    }

    if (_direction)
        return (fps * factorF);
    else
        return (fps * factorR);
}

void Throttle::setCV(int cv, int value)
{
    char dummyChars[31];

    // build the command string
    char buffer[20];
    strcpy(dummyChars, "w ");

    itoa(_dccAddress, buffer, 10);
    strcat(dummyChars, buffer);
    strcat(dummyChars, " ");

    itoa(cv, buffer, 10);
    strcat(dummyChars, buffer);
    strcat(dummyChars, " ");

    itoa(value, buffer, 10);
    strcat(dummyChars, buffer);

    SerialCommand::parse(dummyChars);
}

void Throttle::setFunction(char *jsonMsg)
{
    JsonDocument doc;

    // convert the json string to a json doc
    DeserializationError error = deserializeJson(doc, jsonMsg);
    if (error)
        return;

    int function = doc["f"];
    bool state = doc["s"];

    commandFifo.pushCommand(function, state);
}

void Throttle::brakeSqueal(bool on)
{
    static bool squealOn;
    bool activate;

    if (((_mph > 1) && (_mph < 3)) && on) // gfh 3 was 8
        activate = true;
    else
        activate = false;

    if (activate && squealOn)
        return;

    else if (activate && !squealOn)
    {
        // turn on the sound
        if (_independentBrake > 40) // for light braking, no sound
        {
            commandFifo.pushCommand(functionBrakeSqueal, true);
            squealOn = true;
        }
    }
    else if (!activate && !squealOn)
        return;
    else if (!activate && squealOn)
    {
        // turn off the sound
        commandFifo.pushCommand(functionBrakeSqueal, false);
        squealOn = false;
    }

    return;
}

void Throttle::muSumPerformanceValues() // v0.26
{
    // ref: https://arduinojson.org/v7/api/jsonobject/begin_end/

    _muHorsepower = 0;
    _muLocoMass = 0;
    _muTractiveEffort = 0;

    if (_muState == solo)
    {
        // #ifdef DEBUG_MQTT
        //         reportMqttDebug("combinedHP", (float)_muHorsepower);
        // #endif
        return;
    }

    JsonObject root = muDoc.as<JsonObject>();

    for (JsonPair kv : root)
    {
        // the object attached to each key (locoID) contains the parms we are after
        JsonObject obj = kv.value().as<JsonObject>();
        JsonVariant variant = obj["hp"].as<JsonVariant>();
        uint16_t value = variant.as<uint16_t>();
        _muHorsepower += value;
        variant = obj["mass"].as<JsonVariant>();
        value = variant.as<uint16_t>();
        _muLocoMass += value;
        variant = obj["te"].as<JsonVariant>();
        uint32_t value32 = variant.as<uint32_t>();
        _muTractiveEffort = _muTractiveEffort + value32;
    }

    // #ifdef DEBUG_MQTT
    //     reportMqttDebug("combinedHP", (float)_muHorsepower);
    // #endif
}

void Throttle::muMemberCheck(bool consistMember)
{

    // this runs on a trailing loco in the consist in response to the message sent by lead after muMemberCheck() message was sent to it
    // normally runs once per minute if consisted and lead is replying
    // if the parameter is true then the lead affirms this loco is in the consist
    // if false the lead loco wants nothing to do with this loco
    // if false this loco removes itself from the consist by setting _muState to solo
    // if no response from lead was received then muMemberCheck will set _muState to solo

    Preferences myPrefs;

    if ((_muState == solo) || (_muState == lead))
        return;

    _consistMember = consistMember;

    if (!consistMember)
    {
        _muState = solo;
        // save in prefs
        myPrefs.begin("loco");
        myPrefs.putUInt("mustate", _muState);
        myPrefs.end();
    }
}

void Throttle::muMemberCheck()
{
    // this runs on any loco trailing the lead in a consist
    // if a loco believes that it is in a consist (_muState = mid or trailing) then it runs this code once per minute
    // the message is sent to the lead engine asking for confirmation and then sets a flag, consistMember (bool)
    // the lead will reply affirmative, negative or not at all
    // if affirmative this loco will clear the flag
    // if negative this loco will reset its _muState to solo
    // if missing after another period has expired and the flag is still set then this loco resets _muState to solo

    String jsonString = "";
    Preferences myPrefs;

    if ((_muState == solo) || (_muState == lead))
        return;

    if (!_consistMember) // wasn't reset by a response to the last membercheck (no response at all)
    {
        _muState = solo;
#ifdef SERIAL_ON
        Serial.println("[muMemberCheck] no response, terminating");
#endif
        // save in prefs
        myPrefs.begin("loco");
        myPrefs.putUInt("mustate", _muState);
        myPrefs.end();
        return;
    }

    IPAddress ip = WiFi.localIP();
    char ipBuffer[16]; // Maximum length of an IPv4 string (including null terminator)

    // Format the 4 octets into the buffer
    snprintf(ipBuffer, sizeof(ipBuffer), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]); // TBD make a private class var and do this once, ref later says gfh

    JsonDocument doc;
    doc["topic"] = "muMemberCheck";
    doc["value"] = ipBuffer;
    serializeJson(doc, jsonString);

    udpCommand.beginPacket(_leadIpAdr, COMMAND_PORT);
    udpCommand.write((uint8_t *)jsonString.c_str(), strlen(jsonString.c_str()));
    udpCommand.endPacket();
}

void Throttle::muMemberResponse(const char *muip)
{
    // this runs on lead
    // in response to membercheck udp command from supposed consist member
    // this function checks the incoming ip address against those stored for members in muDoc
    // if found in the list return true, else false in another udp message

    bool isMember = false;
    String jsonString;
    char isMemberChar[12]; // this is bullshit

// This prints the entire nested structure with line breaks and indentation
#ifdef SERIAL_ON
    serializeJsonPretty(muDoc, Serial);
    Serial.println(); // Add a newline at the end for readability
#endif

    JsonObject root = muDoc.as<JsonObject>();

    if (muDoc.size() == 0) // we don't need no stinkin' consist
        strcpy(isMemberChar, "false");
    else
    {
        for (JsonPair kv : root)
        {
            JsonObject obj = kv.value();
            const char *muIP = obj["muip"];

            if (strcmp(muIP, muip) == 0)
                isMember = true;
        }

        if (isMember)
            strcpy(isMemberChar, "true");
        else
            strcpy(isMemberChar, "false");
    }

    JsonDocument doc;
    doc["topic"] = "muMemberResponse";
    doc["value"] = isMemberChar;

    serializeJson(doc, jsonString);

    udpCommand.beginPacket(muip, COMMAND_PORT);
    udpCommand.write((uint8_t *)jsonString.c_str(), strlen(jsonString.c_str()));
    udpCommand.endPacket();
}