#pragma once

// this file is included in every file that needs one of these constants or macros

// version
#define olsVersion "0.1  (compile date: " + String(__DATE__) + " " + String(__TIME__) + ")"

// change following depending on processor
// #define ESP32WROOM
// #define ESP32C3
// #define ESP32C3DK   // like a Waveshare device
#define ESP32C3F

// enable serial for testing, turn off otherwise
// #define SERIAL_ON

// debug tool for throttle
// #define SPEED_DEBUG

