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
#define olsVersion "0.26 mu functionality " + String(__DATE__) + " " + String(__TIME__) + ")"

// change following depending on processor
// #define ESP32WROOM
// #define ESP32C3
// #define ESP32C3DK   // like a Waveshare device
#define ESP32C3F

// enable serial for testing, turn off otherwise
// #define SERIAL_ON

// enable MQTT debugging
// #define MQTT_DEBUG_ON

// debug tool for throttle
// #define SPEED_DEBUG

// for reinitializing SSID
// #define SSID_KILL

// for reinitializing SPIFFS
// #define SPIFF_CLEAN

