#pragma once

// this file is included in every file that needs one of these constants or macros

// version
// #define olsVersion "0.15 trainline algorithm and edits (compile date: " + String(__DATE__) + " " + String(__TIME__) + ")"
// #define olsVersion "0.16 correct error in mu report " + String(__DATE__) + " " + String(__TIME__) + ")"
// #define olsVersion "0.17 fix head/rearlight for mu " + String(__DATE__) + " " + String(__TIME__) + ")"
// #define olsVersion "0.18 fix head/rearlight for mu " + String(__DATE__) + " " + String(__TIME__) + ")"
// #define olsVersion "0.19 t/s ebrake " + String(__DATE__) + " " + String(__TIME__) + ")"
// #define olsVersion "0.20 muperformance " + String(__DATE__) + " " + String(__TIME__) + ")"
// #define olsVersion "0.21 mu states corrections " + String(__DATE__) + " " + String(__TIME__) + ")"
// #define olsVersion "0.22 muperformance corrections " + String(__DATE__) + " " + String(__TIME__) + ")"
// #define olsVersion "0.23 replace ESPConnect with wifimanager  " + String(__DATE__) + " " + String(__TIME__) + ")"
// #define olsVersion "0.24 mu mods  " + String(__DATE__) + " " + String(__TIME__) + ")"
// #define olsVersion "0.25 emergency brake calcs, reportMqttDebug " + String(__DATE__) + " " + String(__TIME__) + ")"
// #define olsVersion "0.263 mu functionality " + String(__DATE__) + " " + String(__TIME__) + ")"   // success
// #define olsVersion "0.27D BrakeSystem " + String(__DATE__) + " " + String(__TIME__) + ")"    // success
// #define olsVersion "0.284 Activate brake sounds in mu " + String(__DATE__) + " " + String(__TIME__) + ")"   // success
// #define olsVersion "0.29A Add Train " + String(__DATE__) + " " + String(__TIME__) + ")" 
// #define olsVersion "0.30 Begin POL adds " + String(__DATE__) + " " + String(__TIME__) + ")" 
// #define olsVersion "0.31 Add top speed " + String(__DATE__) + " " + String(__TIME__) + ")" 
// #define olsVersion "0.32 Speed coefficients loco drag (C3F) " + String(__DATE__) + " " + String(__TIME__) + ")" 
// #define olsVersion "0.33 Speed coefficients loco drag (C3F) " + String(__DATE__) + " " + String(__TIME__) + ")" 
// #define olsVersion "0.33 memory debug " + String(__DATE__) + " " + String(__TIME__) + ")" 
// #define olsVersion "0.34 mqtt keepalive to 60 from default " + String(__DATE__) + " " + String(__TIME__) + ")" 
// #define olsVersion "0.35 mqtt keepalive to 60 from default and check mqtt conn in main loop" + String(__DATE__) + " " + String(__TIME__) + ")" 
// #define olsVersion "0.36 t/s death on braking" + String(__DATE__) + " " + String(__TIME__) + ")" 
// #define olsVersion "0.37 udp substitution  " + String(__DATE__) + " " + String(__TIME__) + ")" 
// #define olsVersion "0.38 more udp sub  " + String(__DATE__) + " " + String(__TIME__)  
constexpr const char* olsVersion = "0.38 more udp sub (" __DATE__ " " __TIME__ ")";

// change following depending on processor 
// #define ESP32C3
// #define ESP32C3DK   // like a Waveshare device (obsolete TBR)
// #define ESP32WROOM
// #define ESP32C3F        // sparkleiot device and others presumably 
#define ESP8685_05   // esp8685-wroom-05 

// one or the other or neither, not both
// enable serial for testing, turn off otherwise
#define SERIAL_ON

// enable serial input for position on layout uart
// #define SERIAL_POL

// select between MQTT or UDP
// #define USING_MQTT
#define USING_UDP

#if defined(USING_MQTT) && defined(USING_UDP)
    #error "Cannot define both USING_MQTT and USING_UDP"
#endif

// enable MQTT debugging
// #define DEBUG_MQTT

// debug tool for throttle
// #define DEBUG_SPEED

// debug tool for UDP
#define DEBUG_UDP

// for reinitializing SSID  *** CAUTION *** DO NOT USE ON INSTALLED UNIT
// #define SSID_KILL

// for reinitializing SPIFFS  *** CAUTION *** DO NOT USE ON INSTALLED UNIT
// #define SPIFF_CLEAN

constexpr uint16_t ROLLCALL_PORT = 50001;  // Port this ESP32 listens on for rollcall
constexpr uint16_t COMMAND_PORT = 50002;   // commands to me use this
constexpr uint16_t TELEMETRY_PORT = 50003; // speed, etc. telementry from lead loco in consist

constexpr uint16_t ROLLCALL_PORT_A = 50001;  // Port this ESP32 listens on for rollcall
constexpr uint16_t COMMAND_PORT_A = 50002;   // commands to me use this
constexpr uint16_t TELEMETRY_PORT_A = 50003; // speed, etc. telementry from lead loco in consist

