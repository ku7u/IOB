/**
 * @file main.cpp
 * @author  George F. Hofmann
 * @copyright 2026
 * @date 5/9/2026
 *
 */

/* issues  (version changes on upload to github)
MU code still needs performance handoff to lead
emergency brake too abrupt with jerking

history
04/27/24  v0.15 send back car count and tonnage
*/

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
This version of DCC++ BASE STATION supports:

  * 2-byte and 4-byte locomotive addressing
  * Simultaneous control of multiple locomotives
  * 128-step speed throttling
  * Cab functions F0-F28
  * Programming on the Main Operations Track
      - write configuration variable bytes
      - set/clear specific configuration variable bits

DCC++ BASE STATION is controlled with simple text commands received via
the Arduino's serial interface.  Users can type these commands directly
into the Arduino IDE Serial Monitor, or can send such commands from another
device or computer program.

DCC++ CONTROLLER, available separately under a similar open-source
license, is a Java program written using the Processing library and Processing IDE
that provides a complete and configurable graphic interface to control model train layouts
via the DCC++ BASE STATION.

Neither DCC++ BASE STATION nor DCC++ CONTROLLER use any known proprietary or
commercial hardware, software, interfaces, specifications, or methods related
to the control of model trains using NMRA DCC standards.  Both programs are wholly
original, developed by the author, and are not derived from any known commercial,
free, or open-source model railroad control packages by any other parties.

However, DCC++ BASE STATION and DCC++ CONTROLLER do heavily rely on the IDEs and
embedded libraries associated with Arduino and Processing.  Tremendous thanks to those
responsible for these terrific open-source initiatives that enable programs like
DCC++ to be developed and distributed in the same fashion.


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
TBD these pin assignments need to be cleaned up for both WROOM and C3
  ESP WROOM 32 pin assignments:
  Pin IO  Usage
  01      GND
  02      3V3
  03      EN
  10  25  DCC_SIGNAL_PIN_MAIN_2 - other side of H bridge (WROOM only)
  11  26  DCC_SIGNAL_PIN_MAIN - one side of H bridge (WROOM only)
  12  27  DCC_SIGNAL_PIN_PROG - prog track (not used)
  27  16  SIGNAL_ENABLE_PIN_MAIN - turns on the output TBD likely unecessary, will set low signal pins low to turn off
  28  17  SIGNAL_ENABLE_PIN_PROG - as above, prog track (not used)  TBD likely unecessary
  30  18  CURRENT_MONITOR_PIN_MAIN - not used
  31  19  CURRENT_MONITOR_PIN_PROG - not used
  33  21  LEFT_HED_PIN
  36  22  RIGHT_HED_PIN
  38      GND

  ESP32 C3
  Pin IO  Usage
  01      GND
  03      3V3
  06  03  HED
  08      EN
  12  00
  18  04  HED
  19  05  DCC_SIGNAL_PIN_MAIN_2 - other side of H bridge (WROOM only)
  20  06  DCC_SIGNAL_PIN_MAIN - one side of H bridge (WROOM only)
  ??  27  DCC_SIGNAL_PIN_PROG - prog track (not used)
  ??  16  SIGNAL_ENABLE_PIN_MAIN - turns on the output TBD likely unecessary, will set low signal pins low to turn off
  ??  17  SIGNAL_ENABLE_PIN_PROG - as above, prog track (not used)  TBD likely unecessary
  ??  18  CURRENT_MONITOR_PIN_MAIN - not used
  ??  19  CURRENT_MONITOR_PIN_PROG - not used
  30      RXD0
  31      TXD0

  ESP32 C3 DK
  Pin IO  Usage
  01      5V
  02      GND
  03      3V3
  04  00  HED
  05  01  HED
  14  10  NeoPixel LED
  17  06  DCC_SIGNAL_PIN_MAIN_2 - other side of H bridge (WROOM only)
  18  07  DCC_SIGNAL_PIN_MAIN - one side of H bridge (WROOM only)
  ??  27  DCC_SIGNAL_PIN_PROG - prog track (not used)
  ??  06  SIGNAL_ENABLE_PIN_MAIN - turns on the output TBD likely unecessary, will set low signal pins low to turn off
  ??  06  SIGNAL_ENABLE_PIN_PROG - as above, prog track (not used)  TBD likely unecessary
  ??  06  CURRENT_MONITOR_PIN_MAIN - not used
  ??  06  CURRENT_MONITOR_PIN_PROG - not used


**********************************************************************/

#include "version.h"
#include "devices.h" // modify the contents as required to match the hardware
// #include <WiFiManager.h> // https://github.com/tzapu/WiFiManager  gfh

#include <Arduino.h>
#include <ArduinoJson.h>
#include "AsyncTCP.h"
#include "ElegantOTA.h"
// there is a problem in ESPAsyncWebServer and the dorks don't fix it
// I forced the platform version in platformio.ini per recommendation in following
// https://github.com/me-no-dev/ESPAsyncWebServer/issues/1147
#include "ESPAsyncWebServer.h"
#include <ESPmDNS.h>
#include <iostream>
#include "nvs_flash.h"
#include "WiFi.h"
#include "FS.h"
#include "RBot.h"
#include "Function.h"
#include "PacketRegister.h"
// #include "CurrentMonitor.h"  //TBD remove current monitor
#include "Sensor.h"
#include "SerialCommand.h"
#include "Config.h"
#include "Comm.h"
#include "Throttle.h"
#include "Location.h"
#include "Train.h"
#include "Fifo.h"
#include "MultiTimer.h"
#include "time.h"
#include "ArduinoJson.h"
#include "defines.h"
#include "BrakeSystem.h"
#include "UartReader.h"
#include "HardwareSerial.h"
#include "WiFiConfigurator.h"
#include "Wiring.h"
#include "DCCFormatter.h"

#include <WiFiUdp.h>      // UDP
#include "udpTransport.h" // TBR
#include "UdpTransport.h"
#include "RollcallHandler.h"
#include "CommandHandler.h"   // TBA
#include "TelemetryHandler.h" // TBA

#include "WebPages.h"
#include <LittleFS.h>

using namespace std;

#ifdef RGB_BUILTIN
#undef RGB_BUILTIN
#endif
#define RGB_BUILTIN 10


// Allocate a "ledger" in memory.
// For an ESP32-C3, 4KB (4096) is plenty for hundreds of locomotive settings. (per Gemini)
JsonDocument config;

// wifi
WiFiClient espClient;
// bool eraseSSID = false;
String mdnsURL;
AsyncWebServer server(80);
WiFiConfigurator wifiConfigurator(server); // defaults: SoftAP "IOB_AP", prefs ns "wifi_conf"

// for mDNS discovery process
// Define the Service Name and Type that Android will look for
const char *serviceName = "myesp32device"; // A unique name, perhaps derived from MAC address
const char *serviceType = "_myesp32app";   // MUST MATCH Android NsdHelper.SERVICE_TYPE (minus the ._udp)
const char *serviceProto = "_udp";         // The protocol
const uint16_t servicePort = 12345;        // The UDP port your ESP32 is listening on

// udp
#ifdef USING_UDP
WiFiUDP udp;
// dedicated ports for rollcall, commands and telemetry
WiFiUDP udpCommand;
WiFiUDP udpTelemetry;

UdpTransport rollcallPort(ROLLCALL_PORT);   // only for rollcall queries, multicast
UdpTransport commandPort(COMMAND_PORT);     // commands are sent to this port, unicast
UdpTransport telemetryPort(TELEMETRY_PORT); // telemetry, multicast, might be unicast but would require multiple messages for trailing units

#endif

// time
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;

String topicCommandLeftEnd;
String topicFeedbackLeftEnd;

Fifo commandFifo;
BrakeSystem bs;
UartReader uartReader;
DCCFormatter df;

// --- UDP handlers and throttle (intertwingled) ---
Throttle throttle; // requires telemetry object
#ifdef USING_UDP
CommandHandler commands(commandPort, throttle); // requires throttle object
RollcallHandler rollcall(rollcallPort);
#endif

Train train;

// position on layout
MagnetReader magReader(LEFT_HED_PIN, RIGHT_HED_PIN);

// functions
String headlightFunction;

// timers
MultiTimer timer1sec(1000);
MultiTimer timer200ms(200);
MultiTimer timer60000ms(60000);

// NEXT DECLARE GLOBAL OBJECTS TO PROCESS AND STORE DCC PACKETS AND MONITOR TRACK CURRENTS.
// NOTE REGISTER LISTS MUST BE DECLARED WITH "VOLATILE" QUALIFIER TO ENSURE THEY ARE PROPERLY UPDATED BY INTERRUPT ROUTINES

volatile RegisterList mainRegs(MAX_MAIN_REGISTERS); // create list of registers for MAX_MAIN_REGISTER Main Track Packets
volatile RegisterList progRegs(2);                  // create a shorter list of only two registers for Program Track Packets

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

/*****************************************************************************/
void IRAM_ATTR onTimer0()
{
  portENTER_CRITICAL_ISR(&timerMux);

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

/*****************************************************************************/
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
    } /*   END-ELSE */
  } /* END-IF: currentReg, activePacket, and currentBit should now be properly set to point to next DCC bit */

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

/*****************************************************************************/
void getGeneralPrefs()
{
  // get the stored configuration values, defaults are the second parameter in the list
  // myPrefs.begin("general", true);

  // topicCommandLeftEnd = myPrefs.getString("commandtopic", "cmd/ols/");
  // topicFeedbackLeftEnd = myPrefs.getString("feedbacktopic", "tlm/ols/");
  // // eraseSSID = myPrefs.getBool("erasessid", false);

  // myPrefs.end();
  topicCommandLeftEnd = config["general"]["topicCommandLeftEnd"].as<String>();
  topicFeedbackLeftEnd = (String)config["general"]["topicFeedbackLeftEnd"].as<String>();
}

/*****************************************************************************/
// JSON config functions
void saveConfig()
{
  File file = LittleFS.open("/config.json", "w");
  if (file)
  {
    serializeJson(config, file);
    file.close();
    Serial.println(F("[JSON] Config saved to LittleFS."));
  }
}

// A tiny macro helper to keep the seeding code incredibly short
#define SEED_DEFAULT(group, key, value) \
if (!config[group].containsKey(key))  \
{                                     \
  config[group][key] = value;         \
  needsSave = true;                   \
}

void enforceConfigDefaults()
{
  bool needsSave = false;
  
  // --- 1. General Namespace ---
  SEED_DEFAULT("general", "topicCommandLeftEnd", "loco/1/cmd");
  SEED_DEFAULT("general", "topicFeedbackLeftEnd", "loco/1/telemetry");
  SEED_DEFAULT("general", "commandtopic", "cmd/ols/");
  SEED_DEFAULT("general", "feedbacktopic", "tlm/ols");
  SEED_DEFAULT("general", "erasessid", 0);
  
  // --- 2. Locomotive Namespace ---
  SEED_DEFAULT("loco", "ip", 0);       // TBD
  SEED_DEFAULT("loco", "type", "Unk"); // TBD
  SEED_DEFAULT("loco", "dccaddress", 3);
  SEED_DEFAULT("loco", "locoid", "Unk");
  SEED_DEFAULT("loco", "locotype", "Unk");
  SEED_DEFAULT("loco", "horsepower", 1500);
  SEED_DEFAULT("loco", "locoweight", 250000);
  SEED_DEFAULT("loco", "tractiveeffort", 60000);
  SEED_DEFAULT("loco", "odometer", 0.0f);
  SEED_DEFAULT("loco", "topspeed", 60.0f);
  SEED_DEFAULT("loco", "lastshuttime", 0U); // TBD for uint32_t
  SEED_DEFAULT("loco", "mustate", 0);
  SEED_DEFAULT("loco", "muleadloco", "Unk");
  SEED_DEFAULT("loco", "mureversed", 0); // TBD for booleans
  SEED_DEFAULT("loco", "consist", "{}"); 

  // --- 3. Calibration Namespace ---
  SEED_DEFAULT("calibration", "speed2forward", 1.0f);
  SEED_DEFAULT("calibration", "speed2reverse", 1.0f);
  SEED_DEFAULT("calibration", "speed5forward", 1.0f);
  SEED_DEFAULT("calibration", "speed5reverse", 1.0f);
  SEED_DEFAULT("calibration", "speed10forward", 1.0f);
  SEED_DEFAULT("calibration", "speed10reverse", 1.0f);
  SEED_DEFAULT("calibration", "speed20forward", 1.0f);
  SEED_DEFAULT("calibration", "speed20reverse", 1.0f);
  SEED_DEFAULT("calibration", "speed50forward", 1.0f);
  SEED_DEFAULT("calibration", "speed50reverse", 1.0f);

  // --- 4. Functions Namespace ---
  SEED_DEFAULT("functions", "headlight", 0);
  SEED_DEFAULT("functions", "headlightdim", 0);
  SEED_DEFAULT("functions", "rearlight", 0);
  SEED_DEFAULT("functions", "rearlightdim", 0);
  SEED_DEFAULT("functions", "bell", 0);
  SEED_DEFAULT("functions", "horn", 0);
  SEED_DEFAULT("functions", "ibrake", 0);
  SEED_DEFAULT("functions", "tbrake", 0);
  SEED_DEFAULT("functions", "ebrake", 0);
  SEED_DEFAULT("functions", "brakesqueal", 0);
  SEED_DEFAULT("functions", "pm", 0);
  SEED_DEFAULT("functions", "compressor", 0);
  SEED_DEFAULT("functions", "notchingenable", 0);
  SEED_DEFAULT("functions", "notchup", 0);
  SEED_DEFAULT("functions", "notchdown", 0);
  
  SEED_DEFAULT("functions", "f0", 0);
  SEED_DEFAULT("functions", "f1", 1);
  SEED_DEFAULT("functions", "f2", 2);
  SEED_DEFAULT("functions", "f3", 3);
  SEED_DEFAULT("functions", "f4", 4);
  SEED_DEFAULT("functions", "f5", 5);
  SEED_DEFAULT("functions", "f6", 6);
  SEED_DEFAULT("functions", "f7", 7);
  SEED_DEFAULT("functions", "f8", 8);
  SEED_DEFAULT("functions", "f9", 9);
  SEED_DEFAULT("functions", "f10", 10);
  SEED_DEFAULT("functions", "f11", 11);
  SEED_DEFAULT("functions", "f12", 12);
  SEED_DEFAULT("functions", "f13", 13);
  SEED_DEFAULT("functions", "f14", 14);
  SEED_DEFAULT("functions", "f15", 15);
  SEED_DEFAULT("functions", "f16", 16);
  SEED_DEFAULT("functions", "f17", 17);
  SEED_DEFAULT("functions", "f18", 18);
  SEED_DEFAULT("functions", "f19", 19);
  SEED_DEFAULT("functions", "f20", 20);
  SEED_DEFAULT("functions", "f21", 21);
  SEED_DEFAULT("functions", "f22", 22);
  SEED_DEFAULT("functions", "f23", 23);
  SEED_DEFAULT("functions", "f24", 24);
  SEED_DEFAULT("functions", "f25", 25);
  SEED_DEFAULT("functions", "f26", 26);
  SEED_DEFAULT("functions", "f27", 27);
  SEED_DEFAULT("functions", "f28", 28);
  
  // --- 4. Wifi Namespace ---
  SEED_DEFAULT("wifi", "ssid", "unk");
  SEED_DEFAULT("wifi", "pass", "unk");

  // TBD TBD TBD
  
  // --- 4. Consist Namespace ---
  
  // If any missing elements were found and fixed, commit the clean schema to flash
  if (needsSave)
  {
    Serial.println(F("[Config] Schema missing elements. Seeding defaults and saving..."));
    saveConfig();
  }
}

void loadConfig()
{
  if (!LittleFS.begin(true))
  { // true = format if corrupted
    Serial.println(F("[JSON] LittleFS Mount Failed"));
    return;
  }
  File file = LittleFS.open("/config.json", "r");
  if (file)
  {
    DeserializationError error = deserializeJson(config, file);
    if (error)
      Serial.println(F("[JSON] Failed to parse config.json"));
    file.close();
  }
}

/*****************************************************************************/
// this function converts placeholders in index.html into active data values
String processorIndex(const String &var)
{
  if (var == "version")
    return VERSION_STRING;

  return String(); // in case nothing matched
}

/*****************************************************************************/
// this function converts placeholders in network.html into active data values
String processorNetwork(const String &var)
{
  // String returnString = ""; // this is unnecessary, just troubleshooting, can be reset back to original returns

  if (var == "IP")
    return WiFi.localIP().toString();
    // returnString = WiFi.localIP().toString();
  else if (var == "SSID")
    return WiFi.SSID();
    // returnString = WiFi.SSID();
  else if (var == "RSSI")
    return String(WiFi.RSSI());
    // returnString = String(WiFi.RSSI());
  else if (var == "MAC")
    return WiFi.macAddress();
    // returnString = WiFi.macAddress();
  else if (var == "MDNS")
    return mdnsURL;
    // returnString = mdnsURL;
  else if (var == "TOPICCOMMANDLEFTEND")
  {
    topicCommandLeftEnd.replace("%", "");
    return topicCommandLeftEnd;
  }
  else if (var == "TOPICFEEDBACKLEFTEND")
  {
    topicFeedbackLeftEnd.replace("%", "");
    return topicFeedbackLeftEnd;
  }

  // if (returnString != "")
  //   return returnString;
  // else
    return String(); // in case nothing matched
}

/*****************************************************************************/
// this function converts placeholders in locoparms.html into active data values
String processorLocoparms(const String &var)
{
  String returnVal;
  returnVal = "";

  if (var == "ODOMETER")
    return String(config["loco"]["odometer"].as<float>() / 5280.0, 2);
  else if (var == "DCCADDRESS")
    return String(config["loco"]["dccaddress"].as<String>());
  else if (var == "LOCOID")
    return String(config["loco"]["locoid"].as<String>());
  else if (var == "LOCOTYPE")
    return String(config["loco"]["locotype"].as<String>());
  else if (var == "HORSEPOWER")
    return String(config["loco"]["horsepower"].as<String>());
  else if (var == "WEIGHT")
    return String(config["loco"]["locoweight"].as<String>());
  else if (var == "TRACTIVEEFFORT")
    return String(config["loco"]["tractiveeffort"].as<String>());
  else if (var == "TOPSPEED")
    return String(config["loco"]["topspeed"].as<String>());

  return (returnVal);
}

/*****************************************************************************/
// this function converts placeholders in calibration.html into active data values
String processorCalibrationparms(const String &var)
{
  String returnVal;
  returnVal = "";

  if (var == "SPEED2FORWARD")
    return config["calibration"]["speed2forward"];
  else if (var == "SPEED2REVERSE")
    return config["calibration"]["speed2reverse"];
  else if (var == "SPEED5FORWARD")
    return config["calibration"]["speed5forward"];
  else if (var == "SPEED5REVERSE")
    return config["calibration"]["speed5reverse"];
  else if (var == "SPEED10FORWARD")
    return config["calibration"]["speed10forward"];
  else if (var == "SPEED10REVERSE")
    return config["calibration"]["speed10reverse"];
  else if (var == "SPEED20FORWARD")
    return config["calibration"]["speed20forward"];
  else if (var == "SPEED20REVERSE")
    return config["calibration"]["speed20reverse"];
  else if (var == "SPEED50FORWARD")
    return config["calibration"]["speed50forward"];
  else if (var == "SPEED50REVERSE")
    return config["calibration"]["speed50reverse"];

  return (returnVal);
}

/*****************************************************************************/
// this function converts placeholders in functions.html into active data values
String processorFunctions(const String &var)
{
  String returnVal;
  returnVal = "";

  if (var == "HEADLIGHT")
    return config["functions"]["headlight"].as<String>();
  else if (var == "HEADLIGHTDIM")
    return config["functions"]["headlightdim"].as<String>();
  else if (var == "REARLIGHT")
    return config["functions"]["rearlight"].as<String>();
  else if (var == "REARLIGHTDIM")
    return config["functions"]["rearlightdim"].as<String>();
  else if (var == "BELL")
    return config["functions"]["bell"].as<String>();
  else if (var == "HORN")
    return config["functions"]["horn"].as<String>();
  else if (var == "IBRAKE")
    return config["functions"]["ibrake"].as<String>();
  else if (var == "TBRAKE")
    return config["functions"]["tbrake"].as<String>();
  else if (var == "EBRAKE")
    return config["functions"]["ebrake"].as<String>();
  else if (var == "BRAKESQUEAL")
    return config["functions"]["brakesqueal"].as<String>();
  else if (var == "PM")
    return config["functions"]["pm"].as<String>();
  else if (var == "COMPRESSOR")
    return config["functions"]["compressor"].as<String>();
  else if (var == "NOTCHINGENABLE")
    return config["functions"]["notchingenable"].as<String>();
  else if (var == "NOTCHUP")
    return config["functions"]["notchup"].as<String>();
  else if (var == "NOTCHDOWN")
    return config["functions"]["notchdown"].as<String>();

  // following are labels for functions 0-28
  else if (var == "F0")
    return config["functions"]["f0"].as<String>();
  else if (var == "F1")
    return config["functions"]["f1"].as<String>();
  else if (var == "F2")
    return config["functions"]["f2"].as<String>();
  else if (var == "F3")
    return config["functions"]["f3"].as<String>();
  else if (var == "F4")
    return config["functions"]["f4"].as<String>();
  else if (var == "F5")
    return config["functions"]["f5"].as<String>();
  else if (var == "F6")
    return config["functions"]["f6"].as<String>();
  else if (var == "F7")
    return config["functions"]["f7"].as<String>();
  else if (var == "F8")
    return config["functions"]["f8"].as<String>();
  else if (var == "F9")
    return config["functions"]["f9"].as<String>();
  else if (var == "F10")
    return config["functions"]["f10"].as<String>();
  else if (var == "F11")
    return config["functions"]["f11"].as<String>();
  else if (var == "F12")
    return config["functions"]["f12"].as<String>();
  else if (var == "F13")
    return config["functions"]["f13"].as<String>();
  else if (var == "F14")
    return config["functions"]["f14"].as<String>();
  else if (var == "F15")
    return config["functions"]["f15"].as<String>();
  else if (var == "F16")
    return config["functions"]["f16"].as<String>();
  else if (var == "F17")
    return config["functions"]["f17"].as<String>();
  else if (var == "F18")
    return config["functions"]["f18"].as<String>();
  else if (var == "F19")
    return config["functions"]["f19"].as<String>();
  else if (var == "F20")
    return config["functions"]["f20"].as<String>();
  else if (var == "F21")
    return config["functions"]["f21"].as<String>();
  else if (var == "F22")
    return config["functions"]["f22"].as<String>();
  else if (var == "F23")
    return config["functions"]["f23"].as<String>();
  else if (var == "F24")
    return config["functions"]["f24"].as<String>();
  else if (var == "F25")
    return config["functions"]["f25"].as<String>();
  else if (var == "F26")
    return config["functions"]["f26"].as<String>();
  else if (var == "F27")
    return config["functions"]["f27"].as<String>();
  else if (var == "F28")
    return config["functions"]["f28"].as<String>();
  // myPrefs.end();

  return (returnVal);
}

/*****************************************************************************/
// this function handles data entry from the web pages
void setupWeb()
{
  // Serve the root directory
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    // We wrap the processor in a lambda to ensure the signature matches exactly
    request->send(200, "text/html", index_html, [](const String& var) {
        return processorIndex(var); 
    }); });

  // Serve /index.html
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", index_html, [](const String &var)
                              { return processorIndex(var); }); });

  server.on("/functions.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", functions_html, [](const String &var)
                              { return processorFunctions(var); }); });

  server.on("/locoparms.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", locoparms_html, [](const String &var)
                              { return processorLocoparms(var); }); });

  server.on("/network.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", network_html, [](const String &var)
                              { return processorNetwork(var); }); });

  server.on("/calibration.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", calibration_html, [](const String &var)
                              { return processorCalibrationparms(var); }); });

  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String inputMessage;
    String inputParam;
    // the hidden param specifies which html file is being accessed
    // if we know that then we know which parameters to expect

    
    if (request->hasParam("EraseParm"))
    {

      config["general"]["erasessid"] = request->getParam("erasessid")->value();
      saveConfig();

      // eraseSSID = inputParam == "Y";
      request->redirect("/network.html");
      ESP.restart();  // v 0.23
    }
    
    else if (request->hasParam("locoparmsParm")) 
    {
      config["loco"]["dccaddress"] = request->getParam("dccaddress")->value();
      config["loco"]["locoid"] = request->getParam("locoid")->value();
      config["loco"]["locotype"] = request->getParam("locotype")->value();
      config["loco"]["horsepower"] = request->getParam("horsepower")->value().toInt();
      config["loco"]["locoweight"] = request->getParam("weight")->value().toDouble();
      config["loco"]["tractiveeffort"] = request->getParam("tractiveeffort")->value().toInt();
      config["loco"]["topspeed"] = request->getParam("topspeed")->value().toInt();
      saveConfig();
            
      throttle.getLocoPrefs();
      request->redirect("/locoparms.html");
    }
    
    else if (request->hasParam("calibrationParm"))
    {
      config["calibration"]["speed2forward"] = request->getParam("speed2forward")->value().toFloat();
      config["calibration"]["speed2reverse"] = request->getParam("speed2reverse")->value().toFloat();
      config["calibration"]["speed5forward"] = request->getParam("speed5forward")->value().toFloat();
      config["calibration"]["speed5reverse"] = request->getParam("speed5reverse")->value().toFloat();
      config["calibration"]["speed10forward"] = request->getParam("speed10forward")->value().toFloat();
      config["calibration"]["speed10reverse"] = request->getParam("speed10reverse")->value().toFloat();
      config["calibration"]["speed20forward"] = request->getParam("speed20forward")->value().toFloat();
      config["calibration"]["speed20reverse"] = request->getParam("speed20reverse")->value().toFloat();
      config["calibration"]["speed50forward"] = request->getParam("speed50forward")->value().toFloat();
      config["calibration"]["speed50reverse"] = request->getParam("speed50reverse")->value().toFloat();
      saveConfig();

      throttle.getLocoPrefs();  // TBD have to add to getLocoPrefs
      request->redirect("/calibration.html");
    }

    // Check if this request is actually coming from the Functions page
    else if (request->hasParam("FunctionsParm")) {
        
        // Loop through every single parameter the browser sent automatically
        size_t paramsCount = request->params();
        for (size_t i = 0; i < paramsCount; i++) {
            const AsyncWebParameter* p = request->getParam(i);
            
            // Skip the "traffic cop" hidden variable itself
            if (p->name() == "FunctionsParm") continue;
            
            // Magically inject the name and value straight into your JSON ledger group
            config["functions"][p->name().c_str()] = p->value().toInt();
            // log_d("function name %s", p->name().c_str());
        }
        
        // Save the whole ledger to LittleFS in one clean transaction
        saveConfig();
        Serial.println(F("[Config] All function parameters updated via loop."));
        
        // Send the user back to the functions page instantly to close the connection safely
        request->redirect("/functions.html");
    }

    else if (request->hasParam("CvParm"))
    {
      String cv = request->getParam("cv")->value();
      String cvValue = request->getParam("cvValue")->value();
      throttle.setCV(cv.toInt(), cvValue.toInt());
      request->redirect("/functions.html");
    } 

    else if (request->hasParam("FunctionLabels")) 
    {
      config["functions"]["f0"] = request->getParam("f0")->value();
      config["functions"]["f1"] = request->getParam("f1")->value();
      config["functions"]["f2"] = request->getParam("f2")->value();
      config["functions"]["f3"] = request->getParam("f3")->value();
      config["functions"]["f4"] = request->getParam("f4")->value();
      config["functions"]["f5"] = request->getParam("f5")->value();
      config["functions"]["f6"] = request->getParam("f6")->value();
      config["functions"]["f7"] = request->getParam("f7")->value();
      config["functions"]["f8"] = request->getParam("f8")->value();
      config["functions"]["f9"] = request->getParam("f9")->value();
      config["functions"]["f10"] = request->getParam("f10")->value();
      config["functions"]["f11"] = request->getParam("f11")->value();
      config["functions"]["f12"] = request->getParam("f12")->value();
      config["functions"]["f13"] = request->getParam("f13")->value();
      config["functions"]["f14"] = request->getParam("f14")->value();
      config["functions"]["f15"] = request->getParam("f15")->value();
      config["functions"]["f16"] = request->getParam("f16")->value();
      config["functions"]["f17"] = request->getParam("f17")->value();
      config["functions"]["f18"] = request->getParam("f18")->value();
      config["functions"]["f19"] = request->getParam("f19")->value();
      config["functions"]["f20"] = request->getParam("f20")->value();
      config["functions"]["f21"] = request->getParam("f21")->value();
      config["functions"]["f22"] = request->getParam("f22")->value();
      config["functions"]["f23"] = request->getParam("f23")->value();
      config["functions"]["f24"] = request->getParam("f24")->value();
      config["functions"]["f25"] = request->getParam("f25")->value();
      config["functions"]["f26"] = request->getParam("f26")->value();
      config["functions"]["f27"] = request->getParam("f27")->value();
      config["functions"]["f28"] = request->getParam("f28")->value();
      saveConfig();

      // TBD get all labels
      request->redirect("/functions.html");
    } });
}

/*****************************************************************************/
// configure mDNS - allows access via URL "ols<locoID>.local"
void setupMDNS(String locoid)
{
  // start mDNS, works on Win10, 11, Linux, Mac - supposed to work on Android but doesn't always
  // the web pages will be available at http://"OLS" + "locoID".local and at IP address
  String myNode = "OLS" + locoid;
  mdnsURL = myNode + ".local";
  mdnsURL.toLowerCase();

  log_d("myNode", myNode.c_str());

  if (!MDNS.begin(myNode.c_str()))
  {
    log_e("Error starting mDNS");
    return;
  }

  // Advertise the service details for device discovery
  MDNS.addService(serviceType, serviceProto, servicePort);
  log_i("Advertising service: %s.%s on port %d", serviceType, serviceProto, servicePort);


  String locoId = config["loco"]["locoid"];
  String locoType = config["loco"]["locotype"];
  locoId.trim();
  locoType.trim();

  log_d("Sending locoID: %s Type: %s", locoId.c_str(), locoType.c_str());

  // Use MDNS.addServiceTxt(serviceType, serviceProto, key, value)
  // Convert Arduino Strings to C-style const char* for the function call
  MDNS.addServiceTxt(serviceType, serviceProto, "locoID", locoId.c_str());
  MDNS.addServiceTxt(serviceType, serviceProto, "type", locoType.c_str());
}

const char *getSubstringAfterLastSlash(const char *input)
{
  const char *lastSlash = strrchr(input, '/'); // find last occurrence of '/'
  if (lastSlash)
  {
    return lastSlash + 1; // move past the slash
  }
  return input; // no slash found, return the whole string
}

void processPendingCommands()
{
  PendingCommand cmd;
  while (commands.getNext(cmd))
  {
    if (strcmp(cmd.topic, "sendstatus") == 0)
    {
      throttle.inUse(true);
      throttle.setControllingIP(cmd.ip);
      throttle.reportCondition();
      throttle.reportStatus();
    }

    else if (strcmp(cmd.topic, "startstop") == 0)
      throttle.pmOnOff(strcmp(cmd.value, "1") ? false : true);
    else if (strcmp(cmd.topic, "stop") == 0)
      throttle.panicStop();
    else if (strcmp(cmd.topic, "horn") == 0)
      throttle.horn(strcmp(cmd.value, "1") ? false : true);
    else if (strcmp(cmd.topic, "bell") == 0)
      throttle.bell(strcmp(cmd.value, "1") ? false : true);
    else if (strcmp(cmd.topic, "horn") == 0)
      throttle.horn(strcmp(cmd.value, "1") ? false : true);
    else if (strcmp(cmd.topic, "headlight") == 0)
      throttle.headlight(atoi(cmd.value));
    else if (strcmp(cmd.topic, "rearlight") == 0)
      throttle.rearlight(atoi(cmd.value));
    else if (strcmp(cmd.topic, "notch") == 0)
      throttle.manualNotch(strcmp(cmd.value, "1") ? false : true);
    else if (strcmp(cmd.topic, "longpress") == 0)
      throttle.longPress(strcmp(cmd.value, "1") ? false : true);
    else if (strcmp(cmd.topic, "ibrake") == 0)
      throttle.setLBrake(strcmp(cmd.value, "1") ? false : true);
    else if (strcmp(cmd.topic, "tbrake") == 0)
      throttle.setABrake(strcmp(cmd.value, "1") ? false : true);
    else if (strcmp(cmd.topic, "trainline") == 0)
      throttle.trainline(strcmp(cmd.value, "1") ? false : true);
    else if (strcmp(cmd.topic, "carcount") == 0)
      throttle.setCarCount(atoi(cmd.value));
    else if (strcmp(cmd.topic, "reportlabels") == 0)
      throttle.reportFunctionLabels();
    else if (strcmp(cmd.topic, "calibrate") == 0)
      throttle.calibrate(atoi(cmd.value));
    else if (strcmp(cmd.topic, "function") == 0)
      throttle.setFunction(cmd.value);
    else if (strcmp(cmd.topic, "report") == 0)
      throttle.report();
    else if (strcmp(cmd.topic, "reverser") == 0)
      throttle.setDirection(atoi(cmd.value));
  }
}

/*****************************************************************************/
void setup()
{
  // runs once

#ifdef ESP32CF
  // don't leave unused pins floating
  // TBD
#endif

#ifdef ESP8685_05
  // don't leave unused pins floating
  pinMode(5, INPUT_PULLDOWN);
  pinMode(6, INPUT_PULLDOWN);
  pinMode(7, INPUT_PULLDOWN);
  pinMode(9, INPUT_PULLDOWN);
#endif

// #ifdef SERIAL_ON TBD probably remove all of this, typically displaced by log_x statements
#if defined(SERIAL_ON) || defined(DEBUG_UDP) || defined(DEBUG_SPEED)
  Serial.begin(115200);
  Serial.println("SERIAL ON");
  // see USB build flags in platformio.ini - set to zeroes to make it all work for C3F
  // per Espressif note, RTS/CTS must be disabled
  // so in monitor pgm set those to none and then restart the device, possibly requires hard reset (power cycle)
  // Serial.println("OLS firmware version " + String(olsVersion));
#endif

#ifdef DEBUG_UDP
  log_v("Debug UDP on");
#endif
#ifdef DEBUG_SPEED
  log_v("Debug speed on");
#endif

#ifdef SERIAL_POL // position on layout reader
  Serial1.begin(9600, SERIAL_8N1, HW_SERIAL_PIN);
  pinMode(HW_SERIAL_PIN, INPUT_PULLDOWN);
#endif

  // getGeneralPrefs();

  loadConfig();            // 1. Read /config.json if it exists
  enforceConfigDefaults(); // 2. Fill in ANY blank slots with factory settings

  // get the road number
  String locoID = config["loco"]["locoid"];

  // using AI generated wificonfigurator object
  // it presents a soft AP that will display a captive portal page to the user
  // the captive page shows the ip address if already connected
  // it also allows connecting to existing APs in the area
  // soft AP terminates automatically in 2 minutes
  // code will connect to a previously selected AP simultaneously with the soft AP creation

  // wifiConfigurator.setSoftApSSID(locoID + "_AP");
  wifiConfigurator.begin();

  setupWeb(); // how to interact with each of the web pages

  server.begin(); // this is the web server

  WiFi.setSleep(false);        // trying to avoid latency  TBD v0282
  WiFi.setAutoReconnect(true); // TBD v0282 could lead to stuck device https://esp32.com/viewtopic.php?f=19&t=39116

  ElegantOTA.setAutoReboot(true); // Ensure this is enabled
  ElegantOTA.begin(&server);      // Start ElegantOTA

  ElegantOTA.onEnd([](bool success) // Hook into OTA completion
                   {
  if (success) {
      // log_i("OTA Success. Manual rebooting...");
      // delay(2000); 
      // ESP.restart();
  } else {
    log_e("OTA update failed, not restarting");
  } });

  setupMDNS(locoID);

  SerialCommand::init(&mainRegs, &progRegs);              // create structure to read and parse commands from serial line TBD remove current monitor
  mainRegs.loadPacket(1, RegisterList::idlePacket, 2, 0); // load idle packet into register 1
  progRegs.loadPacket(1, RegisterList::idlePacket, 2, 0); // load idle packet into register 1

  // opposite phases are sent to these two pins controlling one H bridge pair
  pinMode(DCC_SIGNAL_PIN_MAIN, OUTPUT);
  pinMode(DCC_SIGNAL_PIN_MAIN_2, OUTPUT);

  // timer0 now does not autoreload, timer1 does its work for it at end of cycle and will restart it TBD nope
  pulseTimer0 = timerBegin(0, 80, true);
  timerAttachInterrupt(pulseTimer0, &onTimer0, true);
  timerAlarmWrite(pulseTimer0, DCC_ZERO_BIT_PULSE_DURATION_TIMER0, true);
  timerAlarmEnable(pulseTimer0);

  pulseTimer1 = timerBegin(1, 80, true);
  timerAttachInterrupt(pulseTimer1, &onTimer1, true);
  timerAlarmWrite(pulseTimer1, DCC_ZERO_BIT_TOTAL_DURATION_TIMER1, true);
  timerAlarmEnable(pulseTimer1);

  // define all the callbacks, must be prior to getLocoPrefs
  connectSystems(throttle, bs, commandFifo, df);

  throttle.getLocoPrefs();
  throttle.getFunctionPrefs();
  throttle.init();

  // time is used in throttle object to set trainline psi after extended shutdown
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  pinMode(LEFT_HED_PIN, INPUT_PULLDOWN); // TBD

  rollcallPort.begin();
  commandPort.begin();
  // telemetryPort.begin();

  rollcall.begin();

  log_i("Firmware version: %s", VERSION_STRING.c_str());

  // Now your variables are 100% guaranteed to exist safely in RAM
  int currentAddress = config["loco"]["dccaddress"];
  log_i("Locomotive #%d is ready.", currentAddress);

} // end setup

/*****************************************************************************/
void loop()
{
  timer1sec.tick();
  timer200ms.tick();
  timer60000ms.tick();

  ElegantOTA.loop();

  uint8_t buf[256];
  IPAddress sender;
  uint16_t senderPort;

  commands.loop();
  processPendingCommands();

  if (timer60000ms.expired)
  {
    //   // memory testing
    //   int freeHeap = ESP.getFreeHeap();
    //   String myFreeHeap = String(freeHeap);
    //   unsigned long maxAllocHeap = ESP.getMaxAllocHeap();
    //   String myMaxAllocHeap = String(maxAllocHeap);
    throttle.muMemberCheck();
  }

  // if (magReader.check(throttle.getLastIntCurrentSpeed())) // check for waypoints by reading magnets embedded in track
  //   uint milepost = magReader.process(throttle.isForward()); waddawedo with 'milepost'?
  // maybe set milepost in throttle so it can update POL with odometer calcs

#ifdef SERIAL_POL // also turn on serial above per this switch
  uartReader.check();
#endif

  // read and process a command from the fifo every 200 ms to not overwhelm the decoder
  // if (timer200ms.expired)
  if (timer200ms.expired)
    commandFifo.pop();

  // process the dynamics once per second
  if (timer1sec.expired)
  {
    throttle.loop();
    // #ifdef SERIAL_ON
    //     long myDuration = millis() - myStart;
    //     Serial.print("Duration ");
    //     Serial.println(myDuration);
    // #endif
  }

} // end loop
