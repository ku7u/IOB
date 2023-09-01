/**********************************************************************

DCC++ BASE STATION
COPYRIGHT (c) 2013-2016 Gregg E. Berman

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see http://www.gnu.org/licenses

**********************************************************************/
/**********************************************************************

DCC++ BASE STATION is a C++ program written for the Arduino Uno and Arduino Mega
using the Arduino IDE 1.6.6.

It allows a standard Arduino Uno or Mega with an Arduino Motor Shield (as well as others)
to be used as a fully-functioning digital command and control (DCC) base station
for controlling model train layouts that conform to current National Model
Railroad Association (NMRA) DCC standards.

This version of DCC++ BASE STATION supports:

  * 2-byte and 4-byte locomotive addressing
  * Simultaneous control of multiple locomotives
  * 128-step speed throttling
  * Cab functions F0-F28
  * Activate/de-activate accessory functions using 512 addresses, each with 4 sub-addresses
      - includes optional functionailty to monitor and store of the direction of any connected turnouts
  * Programming on the Main Operations Track
      - write configuration variable bytes
      - set/clear specific configuration variable bits
  * Programming on the Programming Track
      - write configuration variable bytes
      - set/clear specific configuration variable bits
      - read configuration variable bytes

DCC++ BASE STATION is controlled with simple text commands received via
the Arduino's serial interface.  Users can type these commands directly
into the Arduino IDE Serial Monitor, or can send such commands from another
device or computer program.

When compiled for the Arduino Mega, an Ethernet Shield can be used for network
communications instead of using serial communications.

DCC++ CONTROLLER, available separately under a similar open-source
license, is a Java program written using the Processing library and Processing IDE
that provides a complete and configurable graphic interface to control model train layouts
via the DCC++ BASE STATION.

With the exception of a standard 15V power supply that can be purchased in
any electronics store, no additional hardware is required.

Neither DCC++ BASE STATION nor DCC++ CONTROLLER use any known proprietary or
commercial hardware, software, interfaces, specifications, or methods related
to the control of model trains using NMRA DCC standards.  Both programs are wholly
original, developed by the author, and are not derived from any known commercial,
free, or open-source model railroad control packages by any other parties.

However, DCC++ BASE STATION and DCC++ CONTROLLER do heavily rely on the IDEs and
embedded libraries associated with Arduino and Processing.  Tremendous thanks to those
responsible for these terrific open-source initiatives that enable programs like
DCC++ to be developed and distributed in the same fashion.

REFERENCES:

  NMRA DCC Standards:          http://www.nmra.org/index-nmra-standards-and-recommended-practices
  Arduino:                     http://www.arduino.cc/
  Processing:                  http://processing.org/
  GNU General Public License:  http://opensource.org/licenses/GPL-3.0

BRIEF NOTES ON THE THEORY AND OPERATION OF DCC++ BASE STATION:

DCC++ BASE STATION for the Uno configures the OC0B interrupt pin associated with Timer 0,
and the OC1B interupt pin associated with Timer 1, to generate separate 0-5V
unipolar signals that each properly encode zero and one bits conforming with
DCC timing standards.  When compiled for the Mega, DCC++ BASE STATION uses OC3B instead of OC0B.

Series of DCC bit streams are bundled into Packets that each form the basis of
a standard DCC instruction.  Packets are stored in Packet Registers that contain
methods for updating and queuing according to text commands sent by the user
(or another program) over the serial interface.  There is one set of registers that controls
the main operations track and one that controls the programming track.

For the main operations track, packets to store cab throttle settings are stored in
registers numbered 1 through MAX_MAIN_REGISTERS (as defined in RBot.h).
It is generally considered good practice to continuously send throttle control packets
to every cab so that if an engine should momentarily lose electrical connectivity with the tracks,
it will very quickly receive another throttle control signal as soon as connectivity is
restored (such as when a trin passes over  rough portion of track or the frog of a turnout).

DCC++ Base Station therefore sequentially loops through each main operations track packet regsiter
that has been loaded with a throttle control setting for a given cab.  For each register, it
transmits the appropriate DCC packet bits to the track, then moves onto the next register
without any pausing to ensure continuous bi-polar power is being provided to the tracks.
Updates to the throttle setting stored in any given packet register are done in a double-buffered
fashion and the sequencer is pointed to that register immediately after being changes so that updated DCC bits
can be transmitted to the appropriate cab without delay or any interruption in the bi-polar power signal.
The cabs identified in each stored throttle setting should be unique across registers.  If two registers
contain throttle setting for the same cab, the throttle in the engine will oscillate between the two,
which is probably not a desireable outcome.

For both the main operations track and the programming track there is also a special packet register with id=0
that is used to store all other DCC packets that do not require continious transmittal to the tracks.
This includes DCC packets to control decoder functions, set accessory decoders, and read and write Configuration Variables.
It is common practice that transmittal of these one-time packets is usually repeated a few times to ensure
proper receipt by the receiving decoder.  DCC decoders are designed to listen for repeats of the same packet
and provided there are no other packets received in between the repeats, the DCC decoder will not repeat the action itself.
Some DCC decoders actually require receipt of sequential multiple identical one-time packets as a way of
verifying proper transmittal before acting on the instructions contained in those packets

An Arduino Motor Shield (or similar), powered by a standard 15V DC power supply and attached
on top of the Arduino Uno or Mega, is used to transform the 0-5V DCC logic signals
produced by the Uno's Timer interrupts into proper 0-15V bi-polar DCC signals.

This is accomplished on the Uno by using one small jumper wire to connect the Uno's OC1B output (pin 10)
to the Motor Shield's DIRECTION A input (pin 12), and another small jumper wire to connect
the Uno's OC0B output (pin 5) to the Motor Shield's DIRECTION B input (pin 13).

For the Mega, the OC1B output is produced directly on pin 12, so no jumper is needed to connect to the
Motor Shield's DIRECTION A input.  However, one small jumper wire is needed to connect the Mega's OC3B output (pin 2)
to the Motor Shield's DIRECTION B input (pin 13).

Other Motor Shields may require different sets of jumper or configurations (see Config.h and RBot.h for details).

When configured as such, the CHANNEL A and CHANNEL B outputs of the Motor Shield may be
connected directly to the tracks.  This software assumes CHANNEL A is connected
to the Main Operations Track, and CHANNEL B is connected to the Programming Track.

DCC++ BASE STATION in split into multiple modules, each with its own header file:

  DCCpp:            declares required global objects and contains initial Arduino setup()
                    and Arduino loop() functions, as well as interrput code for OC0B and OC1B.
                    Also includes declarations of optional array of Turn-Outs and optional array of Sensors

  SerialCommand:    contains methods to read and interpret text commands from the serial line,
                    process those instructions, and, if necessary call appropriate Packet RegisterList methods
                    to update either the Main Track or Programming Track Packet Registers

  PacketRegister:   contains methods to load, store, and update Packet Registers with DCC instructions

  CurrentMonitor:   contains methods to separately monitor and report the current drawn from CHANNEL A and
                    CHANNEL B of the Arduino Motor Shield's, and shut down power if a short-circuit overload
                    is detected

  Accessories:      contains methods to operate and store the status of any optionally-defined turnouts controlled
                    by a DCC stationary accessory decoder.

  Sensor:           contains methods to monitor and report on the status of optionally-defined infrared
                    sensors embedded in the Main Track and connected to various pins on the Arudino Uno

  Outputs:          contains methods to configure one or more Arduino pins as an output for your own custom use

  EEStore:          contains methods to store, update, and load various DCC settings and status
                    (e.g. the states of all defined turnouts) in the EEPROM for recall after power-up

DCC++ BASE STATION is configured through the Config.h file that contains all user-definable parameters

  Pin assignments:
   01  TX0
   02
   03  RX0
   04
   05
   12
   13
   14
   15
   16  SIGNAL_ENABLE_PIN_MAIN - turns on the output TBD likely unecessary, will set low signal pins low to turn off
   17  SIGNAL_ENABLE_PIN_PROG - as above, prog track (not used)  TBD likely unecessary
   18  CURRENT_MONITOR_PIN_MAIN
   19  CURRENT_MONITOR_PIN_PROG
   21  LEFT_HED_PIN
   22  RIGHT_HED_PIN
   23
   25  DCC_SIGNAL_PIN_MAIN_2 - other side of H bridge
   26  DCC_SIGNAL_PIN_MAIN - one side of H bridge
   27  DCC_SIGNAL_PIN_PROG - prog track (not used)
   32
   33
   34  input requires pullup on board
   35  input requires pullup on board
   36  input requires pullup on board
   39  input requires pullup on board

GPIO Pins:

  Motor driver - DRV8801
**********************************************************************/

#include <Arduino.h>
#include <iostream>
// there is a problem in ESPAsyncWebServer and the dorks don't fix it
// I forced the platform version in platformio.ini per recommendation in following
// https://github.com/me-no-dev/ESPAsyncWebServer/issues/1147
#include <ESPmDNS.h>
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include "ESPConnect.h"
#include "AsyncElegantOTA.h"
#include "Preferences.h"
#include "PubSubClient.h"
#include "WiFi.h"
#include "MQTT.h"
#include "FS.h"
#include "SPIFFS.h"
#include "RBot.h"
#include "Function.h"
#include "PacketRegister.h"
#include "CurrentMonitor.h"
#include "Sensor.h"
#include "SerialCommand.h"
#include "Config.h"
#include "Comm.h"
#include "Throttle.h"
#include "Location.h"
#include "Fifo.h"
#include "MultiTimer.h"
#include "time.h"
#include "ArduinoJson.h"

// #include "WebSerial.h"

using namespace std;

#define LEFT_HED_PIN 21
#define RIGHT_HED_PIN 22

Preferences myPrefs;
// void showConfiguration();

AsyncWebServer server(80);

// wifi
WiFiClient espClient;

// time
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;

// mqtt
PubSubClient client(espClient);
String mqttServer = "192.168.0.109"; // TBD fix this
String mqttNode = "OLSdevice";
String topicCommandLeftEnd;
String topicFeedbackLeftEnd;

Fifo commandFifo;

Throttle throttle;

MagnetReader magReader(LEFT_HED_PIN, RIGHT_HED_PIN);

// functions
String headlightFunction;

// timers
MultiTimer timer1sec(1000);
MultiTimer timer200ms(200);
// MultiTimer timer150ms(150);

// NEXT DECLARE GLOBAL OBJECTS TO PROCESS AND STORE DCC PACKETS AND MONITOR TRACK CURRENTS.
// NOTE REGISTER LISTS MUST BE DECLARED WITH "VOLATILE" QUALIFIER TO ENSURE THEY ARE PROPERLY UPDATED BY INTERRUPT ROUTINES

volatile RegisterList mainRegs(MAX_MAIN_REGISTERS); // create list of registers for MAX_MAIN_REGISTER Main Track Packets
volatile RegisterList progRegs(2);                  // create a shorter list of only two registers for Program Track Packets

CurrentMonitor mainMonitor(CURRENT_MONITOR_PIN_MAIN, "<p2>"); // create monitor for current on Main Track
CurrentMonitor progMonitor(CURRENT_MONITOR_PIN_PROG, "<p3>"); // create monitor for current on Program Track

/*
gfh changes to use the ESP timers
timer0 creates the on pulse
timer1 creates the full cycle period
timer1 auto resets
timer1 resets timer0 to keep it in sync
*/

hw_timer_t *pulseTimer0 = NULL;
hw_timer_t *pulseTimer1 = NULL;

#define DCC_ZERO_BIT_TOTAL_DURATION_TIMER1 200
#define DCC_ZERO_BIT_PULSE_DURATION_TIMER0 100 // should be 100, set longer so timer1 resets it before it resets itself

#define DCC_ONE_BIT_TOTAL_DURATION_TIMER1 116
#define DCC_ONE_BIT_PULSE_DURATION_TIMER0 58 // should be 58, set longer so timer1 resets it before it resets itself

portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

///////////////////////////////////////////////////////////////////////////////
// DEFINE THE INTERRUPT LOGIC THAT GENERATES THE DCC SIGNAL
///////////////////////////////////////////////////////////////////////////////

// The code below will be called every time an interrupt is triggered on OCNB, where N can be 0 or 1.
// It is designed to read the current bit of the current register packet and
// updates the OCNA and OCNB counters of Timer-N to values that will either produce
// a long (200 microsecond) pulse, or a short (116 microsecond) pulse, which respectively represent
// DCC ZERO and DCC ONE bits.

// These are hardware-driven interrupts that will be called automatically when triggered regardless of what
// DCC++ BASE STATION was otherwise processing.  But once inside the interrupt, all other interrupt routines are temporarily diabled.
// Since a short pulse only lasts for 116 microseconds, and there are TWO separate interrupts
// (one for Main Track Registers and one for the Program Track Registers), the interrupt code must complete
// in much less than 58 microsends, otherwise there would be no time for the rest of the program to run.  Worse, if the logic
// of the interrupt code ever caused it to run longer than 58 microsends, an interrupt trigger would be missed, the OCNA and OCNB
// registers would not be updated, and the net effect would be a DCC signal that keeps sending the same DCC bit repeatedly until the
// interrupt code completes and can be called again.

// A significant portion of this entire program is designed to do as much of the heavy processing of creating a properly-formed
// DCC bit stream upfront, so that the interrupt code below can be as simple and efficient as possible.

// Note that we need to create two very similar copies of the code --- one for the Main Track OC1B interrupt and one for the
// Programming Track OCOB interrupt.  But rather than create a generic function that incurrs additional overhead, we create a macro
// that can be invoked with proper paramters for each interrupt.  This slightly increases the size of the code base by duplicating
// some of the logic for each interrupt, but saves additional time.

// As structured, the interrupt code below completes at an average of just under 6 microseconds with a worse-case of just under 11 microseconds
// when a new register is loaded and the logic needs to switch active register packet pointers.

void IRAM_ATTR onTimer0()
{
  portENTER_CRITICAL_ISR(&timerMux);
  // digitalWrite(DCC_SIGNAL_PIN_MAIN, !digitalRead(DCC_SIGNAL_PIN_MAIN));
  // digitalWrite(DCC_SIGNAL_PIN_MAIN_2, !digitalRead(DCC_SIGNAL_PIN_MAIN));

  // the motor driver does not like both inputs to be zero simultaneously, takes 40 usec to recover
  if (digitalRead(DCC_SIGNAL_PIN_MAIN))
  {
    // write a one first to the pin that was zero so that both are one rather than both zero
    digitalWrite(DCC_SIGNAL_PIN_MAIN_2, 1);
    digitalWrite(DCC_SIGNAL_PIN_MAIN, 0);
  }
  else
  {
    digitalWrite(DCC_SIGNAL_PIN_MAIN, 1);
    digitalWrite(DCC_SIGNAL_PIN_MAIN_2, 0);
  }

  portEXIT_CRITICAL_ISR(&timerMux);
}

void IRAM_ATTR onTimer1()
{
  portENTER_CRITICAL_ISR(&timerMux);
  // gfh this was added because timer0 is now configured to not autoreload TBD nope
  // digitalWrite(DCC_SIGNAL_PIN_MAIN, !digitalRead(DCC_SIGNAL_PIN_MAIN));
  // digitalWrite(DCC_SIGNAL_PIN_MAIN_2, !digitalRead(DCC_SIGNAL_PIN_MAIN));

  if (mainRegs.currentBit == mainRegs.currentReg->activePacket->nBits)
  {                          /* IF no more bits in this DCC Packet */
    mainRegs.currentBit = 0; /*   reset current bit pointer and determine which Register and Packet to process next--- */

    if (mainRegs.nRepeat > 0 && mainRegs.currentReg == mainRegs.reg)
    {                     /*   IF current Register is first Register AND should be repeated */
      mainRegs.nRepeat--; /*     decrement repeat count; result is this same Packet will be repeated */
    }
    else if (mainRegs.nextReg != NULL)
    {                                                          /*   ELSE IF another Register has been updated */
      mainRegs.currentReg = mainRegs.nextReg;                  /*     update currentReg to nextReg */
      mainRegs.nextReg = NULL;                                 /*     reset nextReg to NULL */
      mainRegs.tempPacket = mainRegs.currentReg->activePacket; /*     flip active and update Packets */

      mainRegs.currentReg->activePacket = mainRegs.currentReg->updatePacket;
      mainRegs.currentReg->updatePacket = mainRegs.tempPacket;
    }
    else
    {                                                   /*   ELSE simply move to next Register */
      if (mainRegs.currentReg == mainRegs.maxLoadedReg) /*     BUT IF this is last Register loaded */
        mainRegs.currentReg = mainRegs.reg;             /*       first reset currentReg to base Register, THEN */
      mainRegs.currentReg++;                            /*     increment current Register (note this logic causes Register[0] to be skipped when simply cycling through all Registers) */
    }                                                   /*   END-ELSE */
  }                                                     /* END-IF: currentReg, activePacket, and currentBit should now be properly set to point to next DCC bit */

  if (mainRegs.currentReg->activePacket->buf[mainRegs.currentBit / 8] & mainRegs.bitMask[mainRegs.currentBit % 8])
  { /* IF bit is a ONE */
    timerAlarmWrite(pulseTimer0, DCC_ONE_BIT_PULSE_DURATION_TIMER0, true);
    timerAlarmWrite(pulseTimer1, DCC_ONE_BIT_TOTAL_DURATION_TIMER1, true);
  }
  else
  { /* ELSE it is a ZERO */
    timerAlarmWrite(pulseTimer0, DCC_ZERO_BIT_PULSE_DURATION_TIMER0, true);
    timerAlarmWrite(pulseTimer1, DCC_ZERO_BIT_TOTAL_DURATION_TIMER1, true);
  } /* END-ELSE */

  mainRegs.currentBit++; /* point to next bit in current Packet */

  portEXIT_CRITICAL_ISR(&timerMux);
}

void getGeneralPrefs()
{
  // get the stored configuration values, defaults are the second parameter in the list
  myPrefs.begin("general", true);
  mqttServer = myPrefs.getString("mqttserver", "192.168.99.99");
  topicCommandLeftEnd = myPrefs.getString("commandtopic", "cmd/ols/");
  topicFeedbackLeftEnd = myPrefs.getString("feedbacktopic", "tlm/ols/");
  myPrefs.end();
}

/*****************************************************************************/
// this function converts placeholders in the html into active data values
String processorNetwork(const String &var)
{
  if (var == "IP")
    return WiFi.localIP().toString();
  else if (var == "SSID")
    return WiFi.SSID();
  else if (var == "RSSI")
    // int8_t myRSSI = WiFi.RSSI();
    return String(WiFi.RSSI());
  else if (var == "MAC")
    return WiFi.macAddress();
  else if (var == "MQ")
    return mqttServer;
  else if (var == "MQTTSERVERIPADR")
    return mqttServer;
  else if (var == "TOPICCOMMANDLEFTEND")
    return topicCommandLeftEnd;
  else if (var == "TOPICFEEDBACKLEFTEND")
    return topicFeedbackLeftEnd;

  return String(); // in case nothing matched
}

/*****************************************************************************/
// this function converts placeholders in the html into active data values
String processorLocoparms(const String &var)
{
  String returnVal;
  returnVal = "";

  myPrefs.begin("loco", true);
  if (var == "ODOMETER")
    returnVal = String(myPrefs.getFloat("odometer", 0.));
  else if (var == "DCCADDRESS")
    returnVal = String(myPrefs.getInt("dccaddress", 3));
  else if (var == "ADDRESS") // TBD this should be called roadnum now
    returnVal = String(myPrefs.getInt("roadnum", 3));
  else if (var == "LOCOID")
    returnVal = myPrefs.getString("locoid", "3");
  else if (var == "HORSEPOWER")
    returnVal = String(myPrefs.getInt("horsepower", 1500));
  else if (var == "WEIGHT")
    returnVal = String(myPrefs.getULong("locoweight", 200000));
  else if (var == "TRACTIVEEFFORT")
    returnVal = String(myPrefs.getLong("tractiveeffort", 70000));
  myPrefs.end();

  return (returnVal);
}

/*****************************************************************************/
// this function converts placeholders in the html into active data values
String processorCalibrationparms(const String &var)
{
  String returnVal;
  returnVal = "";

  myPrefs.begin("calibration", true);
  if (var == "SPEED2FORWARD")
    returnVal = String(myPrefs.getFloat("speed2forward", 1.));
  else if (var == "SPEED2REVERSE")
    returnVal = String(myPrefs.getFloat("speed2reverse", 1.));
  else if (var == "SPEED5FORWARD")
    returnVal = String(myPrefs.getFloat("speed5forward", 1.));
  else if (var == "SPEED5REVERSE")
    returnVal = String(myPrefs.getFloat("speed5reverse", 1.));
  else if (var == "SPEED10FORWARD")
    returnVal = String(myPrefs.getFloat("speed10forward", 1.));
  else if (var == "SPEED10REVERSE")
    returnVal = String(myPrefs.getFloat("speed10reverse", 1.));
  else if (var == "SPEED20FORWARD")
    returnVal = String(myPrefs.getFloat("speed20forward", 1.));
  else if (var == "SPEED20REVERSE")
    returnVal = String(myPrefs.getFloat("speed20reverse", 2.));
  else if (var == "SPEED50FORWARD")
    returnVal = String(myPrefs.getFloat("speed50forward", 2.));
  else if (var == "SPEED50REVERSE")
    returnVal = String(myPrefs.getFloat("speed50reverse", 2.));
  myPrefs.end();

  return (returnVal);
}

/*****************************************************************************/
// this function converts placeholders in the html into active data values
String processorFunctions(const String &var)
{
  // Serial.println(var);
  String returnVal;
  returnVal = "";

  myPrefs.begin("functions", true);
  if (var == "HEADLIGHT")
    returnVal = String(myPrefs.getInt("headlightBright", 0));
  else if (var == "HEADLIGHTDIM")
    returnVal = String(myPrefs.getInt("headlightDim", 0));
  else if (var == "REARLIGHT")
    returnVal = String(myPrefs.getInt("rearlightBright", 0));
  else if (var == "REARLIGHTDIM")
    returnVal = String(myPrefs.getInt("rearlightDim", 0));
  else if (var == "BELL")
    returnVal = String(myPrefs.getInt("bell", 1));
  else if (var == "HORN")
    returnVal = String(myPrefs.getInt("horn", 2));
  else if (var == "IBRAKE")
    returnVal = String(myPrefs.getInt("iBrake", 2));
  else if (var == "TBRAKE")
    returnVal = String(myPrefs.getInt("tBrake", 2));
  else if (var == "PM")
    returnVal = String(myPrefs.getInt("pm", 2));
  else if (var == "COMPRESSOR")
    returnVal = String(myPrefs.getInt("compressor", 2));
  else if (var == "NOTCHINGENABLE")
    returnVal = String(myPrefs.getInt("notchingEnable", 25));
  else if (var == "NOTCHUP")
    returnVal = String(myPrefs.getInt("notchUp", 25));
  else if (var == "NOTCHDOWN")
    returnVal = String(myPrefs.getInt("notchDown", 25));
  myPrefs.end();

  return (returnVal);
}

void setupWeb()
{
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html", false); });
  server.on("/functions.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/functions.html", "text/html", false, processorFunctions); });
  server.on("/locoparms.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/locoparms.html", "text/html", false, processorLocoparms); });
  server.on("/network.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/network.html", "text/html", false, processorNetwork); });
  server.on("/calibration.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/calibration.html", "text/html", false, processorCalibrationparms); });
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/stylesheet.css", "text/css", false); });

  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String inputMessage;
    String inputParam;
    // the hidden param specifies which html file is being accessed
    // if we know that then we know which parameters to expect
    if (request->hasParam("NetworkParm"))
    {
      myPrefs.begin("general", false);
      mqttServer = request->getParam("mqttserver")->value();
      myPrefs.putString("mqttserver", mqttServer);
      topicCommandLeftEnd = request->getParam("commandtopic")->value();
      myPrefs.putString("commandtopic", topicCommandLeftEnd);
      topicFeedbackLeftEnd = request->getParam("feedbacktopic")->value();
      myPrefs.putString("feedbacktopic", topicFeedbackLeftEnd);
      myPrefs.end();

      request->send(SPIFFS, "/network.html", "text/html", false, processorNetwork);
    }

   else if (request->hasParam("locoparmsParm"))
    {
      myPrefs.begin("loco", false);
      inputMessage = request->getParam("dccaddress")->value();
      myPrefs.putInt("dccaddress", inputMessage.toInt());  
      inputMessage = request->getParam("address")->value();
      myPrefs.putInt("roadnum", inputMessage.toInt());  
      inputMessage = request->getParam("locoid")->value();
      myPrefs.putString("locoid", inputMessage);
      inputMessage = request->getParam("horsepower")->value();
      myPrefs.putInt("horsepower", inputMessage.toInt());
      inputMessage = request->getParam("weight")->value();
      myPrefs.putULong("locoweight", inputMessage.toInt());
      inputMessage = request->getParam("tractiveeffort")->value();
      myPrefs.putLong("tractiveeffort", inputMessage.toInt()); // TBD why float?
      myPrefs.end();

      throttle.getLocoPrefs();
      request->send(SPIFFS, "/locoparms.html", "text/html", false, processorLocoparms);
    }
   
   else if (request->hasParam("calibrationParm"))
    {
      myPrefs.begin("calibration", false);
      inputMessage = request->getParam("speed2forward")->value();
      myPrefs.putFloat("speed2forward", inputMessage.toFloat());
      inputMessage = request->getParam("speed2reverse")->value();
      myPrefs.putFloat("speed2reverse", inputMessage.toFloat());

      inputMessage = request->getParam("speed5forward")->value();
      myPrefs.putFloat("speed5forward", inputMessage.toFloat());
      inputMessage = request->getParam("speed5reverse")->value();
      myPrefs.putFloat("speed5reverse", inputMessage.toFloat());
      
      inputMessage = request->getParam("speed10forward")->value();
      myPrefs.putFloat("speed10forward", inputMessage.toFloat());
      inputMessage = request->getParam("speed10reverse")->value();
      myPrefs.putFloat("speed10reverse", inputMessage.toFloat());


      inputMessage = request->getParam("speed20forward")->value();
      myPrefs.putFloat("speed20forward", inputMessage.toFloat());
      inputMessage = request->getParam("speed20reverse")->value();
      myPrefs.putFloat("speed20reverse", inputMessage.toFloat());

      inputMessage = request->getParam("speed50forward")->value();
      myPrefs.putFloat("speed50forward", inputMessage.toFloat());
      inputMessage = request->getParam("speed50reverse")->value();
      myPrefs.putFloat("speed50reverse", inputMessage.toFloat());
      
      myPrefs.end();

      throttle.getLocoPrefs();  // TBD have to add to getLocoPrefs
      request->send(SPIFFS, "/calibration.html", "text/html", false, processorCalibrationparms);
    }

    else if (request->hasParam("FunctionsParm"))
    {
      myPrefs.begin("functions", false);
      inputMessage = request->getParam("headlight")->value();
      myPrefs.putInt("headlightBright", inputMessage.toInt());
      inputMessage = request->getParam("rearlight")->value();
      myPrefs.putInt("rearlightBright", inputMessage.toInt());
      inputMessage = request->getParam("headlightdim")->value();
      myPrefs.putInt("headlightDim", inputMessage.toInt());
      inputMessage = request->getParam("rearlightdim")->value();
      myPrefs.putInt("rearlightDim", inputMessage.toInt());
      inputMessage = request->getParam("bell")->value();
      myPrefs.putInt("bell", inputMessage.toInt());
      inputMessage = request->getParam("horn")->value();
      myPrefs.putInt("horn", inputMessage.toInt());
      inputMessage = request->getParam("ibrake")->value();
      myPrefs.putInt("iBrake", inputMessage.toInt());
      inputMessage = request->getParam("tbrake")->value();
      myPrefs.putInt("tBrake", inputMessage.toInt());
      inputMessage = request->getParam("pm")->value();
      myPrefs.putInt("pm", inputMessage.toInt());
      inputMessage = request->getParam("compressor")->value();
      myPrefs.putInt("compressor", inputMessage.toInt());
      inputMessage = request->getParam("notchingenable")->value();
      myPrefs.putInt("notchingEnable", inputMessage.toInt());
      inputMessage = request->getParam("notchup")->value();
      myPrefs.putInt("notchUp", inputMessage.toInt());
      inputMessage = request->getParam("notchdown")->value();
      myPrefs.putInt("notchDown", inputMessage.toInt());
      myPrefs.end();

      throttle.getFunctionPrefs();
      request->send(SPIFFS, "/functions.html", "text/html", false, processorFunctions);
    } });

  // following from codeproject
  server.serveStatic("/", SPIFFS, "/");
}

void setupMDNS(String locoid)
{
  // start mDNS
  // String myNode = "OLS" + String(roadNum);
  String myNode = "OLS" + locoid;
  Serial.print("myNode ");
  Serial.println(myNode.c_str());
  if (!MDNS.begin(myNode.c_str()))
  {
    Serial.println("Error starting mDNS");
    return;
  }
}

void report()
{
  // String x = "{id:" + "GN2178" + ",ip:" + "1234"}";
  String x = "{id:GN4321,ip:192.168.0.132}";
  client.publish("tlm/ols/16/report", x.c_str());
}

// #include "nvs_flash.h"
///////////////////////////////////////////////////////////////////////////////
// INITIAL SETUP
///////////////////////////////////////////////////////////////////////////////
void setup()
{
  // static u32_t timer;
  // nvs_flash_erase();
  // nvs_flash_init();

  Serial.begin(115200);
  SPIFFS.begin(true); // format on fail

  getGeneralPrefs();

  // get the road number
  myPrefs.begin("loco", true);
  int roadNum = myPrefs.getInt("roadnum", 0);
  String locoID = myPrefs.getString("locoid", "undefined");
  myPrefs.end();

  throttle.getLocoPrefs();
  throttle.getFunctionPrefs();

  // use mac address as SSID to assure uniqueness
  String SSID = WiFi.macAddress();
  Serial.println(SSID);
  // ESPConnect.erase();
  ESPConnect.autoConnect(SSID.c_str(), "123456789", 90000);

  // Begin connecting to previous WiFi or start autoConnect AP if unable to connect
  // ESPConnect.erase();
  if (ESPConnect.begin(&server))
  {
    Serial.println("Connected to WiFi");
    Serial.println("IPAddress: " + WiFi.localIP().toString());
    String mySSID = WiFi.SSID();
    Serial.println("SSID: " + mySSID);
    int8_t myRSSI = WiFi.RSSI();
    Serial.println("Signal strength: " + String(myRSSI));
    if (myRSSI == 0)
      ESP.restart();
  }
  else
  {
    Serial.println("Failed to connect to WiFi");
    ESP.restart();
  }

  // while (!ESPConnect.begin(&server));
  // {
  //   ESPConnect.erase();  // this seems to be necessary if can't reconnect to preexisting SSID, i.e. his s/w is hosed
  // }

  AsyncElegantOTA.begin(&server); // Start ElegantOTA
  server.begin();

  setupMDNS(locoID);

  setupWeb();

  SerialCommand::init(&mainRegs, &progRegs, &mainMonitor); // create structure to read and parse commands from serial line

  // timer0 now does not autoreload, timer1 does its work for it at end of cycle and will restart it TBD nope
  pulseTimer0 = timerBegin(0, 80, true);
  timerAttachInterrupt(pulseTimer0, &onTimer0, true);
  timerAlarmWrite(pulseTimer0, DCC_ZERO_BIT_PULSE_DURATION_TIMER0, true);
  timerAlarmEnable(pulseTimer0);

  pulseTimer1 = timerBegin(1, 80, true);
  timerAttachInterrupt(pulseTimer1, &onTimer1, true);
  timerAlarmWrite(pulseTimer1, DCC_ZERO_BIT_TOTAL_DURATION_TIMER1, true);
  timerAlarmEnable(pulseTimer1);

  // opposite phases are sent to these two pins controlling one H bridge pair
  pinMode(DCC_SIGNAL_PIN_MAIN, OUTPUT);
  pinMode(DCC_SIGNAL_PIN_MAIN_2, OUTPUT);

  mainRegs.loadPacket(1, RegisterList::idlePacket, 2, 0); // load idle packet into register 1
  progRegs.loadPacket(1, RegisterList::idlePacket, 2, 0); // load idle packet into register 1

  throttle.init();

  // MQTT
  mqttNode = "OLS"+ locoID;
  // String myNode = "OLS" + String(roadNum);
  mqttSetup(mqttServer, mqttNode);

#ifdef speeddebug
  Serial.println("speeddebug is on");
#endif

  // use millis as seed for random generator
  srand(millis());

  // time is used in throttle object to set trainline psi after extended shutdown
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // steady state test of the motor driver
  // digitalWrite(DCC_SIGNAL_PIN_MAIN, 0);
  // digitalWrite(DCC_SIGNAL_PIN_MAIN_2, 1);
} // end setup

///////////////////////////////////////////////////////////////////////////////
// MAIN LOOP
///////////////////////////////////////////////////////////////////////////////
void loop()
{
  timer1sec.tick();
  timer200ms.tick();
  // timer150ms.tick();

  // process the mqtt input
  if (!client.loop())
  {
    connectMQTT(mqttNode);
    setupSubscriptions();
  }

  // if (magReader.check(throttle.getLastIntCurrentSpeed())) // check for waypoints by reading magnets embedded in track
  //   uint milepost = magReader.process(throttle.isForward());

  // read and process a command from the fifo every 200 ms to not overwhelm the decoder
  if (timer200ms.expired)
    commandFifo.pop();

  // process the dynamics once per second
  if (timer1sec.expired)
  {
    // long  myStart = millis();
    throttle.computeVelocity();
    // long myDuration = millis() - myStart;
    // Serial.print("Duration ");Serial.println(myDuration);
  }
  /*   if(CurrentMonitor::checkTime()){      // if sufficient time has elapsed since last update, check current draw on Main and Program Tracks
      mainMonitor.check();
      progMonitor.check();
    } */

  // Sensor::check();    // check sensors for activate/de-activate

} // end loop
