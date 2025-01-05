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
#define olsVersion "0.29A Add Train " + String(__DATE__) + " " + String(__TIME__) + ")" 

// change following depending on processor
// #define ESP32C3
// #define ESP32C3DK   // like a Waveshare device (obsolete TBR)
// #define ESP32WROOM
// #define ESP32C3F        // sparkleiot device and others presumably
#define ESP8685_05   // esp8685-wroom-05

// enable serial for testing, turn off otherwise
#define SERIAL_ON

// enable MQTT debugging
// #define MQTT_DEBUG_ON

// debug tool for throttle
// #define SPEED_DEBUG

// for reinitializing SSID  *** CAUTION *** DO NOT USE ON INSTALLED UNIT
// #define SSID_KILL

// for reinitializing SPIFFS  *** CAUTION *** DO NOT USE ON INSTALLED UNIT
// #define SPIFF_CLEAN

