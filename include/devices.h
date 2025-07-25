#pragma once

#include "Arduino.h"
#include "defines.h"

#ifdef ESP32C3DK
const uint8_t LEFT_HED_PIN = 0;
const uint8_t RIGHT_HED_PIN = 1;
const uint8_t DCC_SIGNAL_PIN_MAIN = 7;
const uint8_t DCC_SIGNAL_PIN_MAIN_2 = 6;
// const uint8_t SIGNAL_ENABLE_PIN_MAIN = 2; // these 4 are dummies for now, should remove
// const uint8_t SIGNAL_ENABLE_PIN_PROG = 2;
// const uint8_t CURRENT_MONITOR_PIN_MAIN = 2;
// const uint8_t CURRENT_MONITOR_PIN_PROG = 2;
#endif

#ifdef ESP32C3
#define LEFT_HED_PIN 3
#define RIGHT_HED_PIN 4
#define DCC_SIGNAL_PIN_MAIN 6
#define DCC_SIGNAL_PIN_MAIN_2 5
// #define SIGNAL_ENABLE_PIN_MAIN 2 // these 4 are dummies for now, should remove
// #define SIGNAL_ENABLE_PIN_PROG 2
// #define CURRENT_MONITOR_PIN_MAIN 2
// #define CURRENT_MONITOR_PIN_PROG 2
#endif

#ifdef ESP32C3F
#define LEFT_HED_PIN 8
#define RIGHT_HED_PIN 10
#define HW_SERIAL_PIN 10
// #define NORTH_UART_PIN 8
// #define SOUTH_UART_PIN 10
#define DCC_SIGNAL_PIN_MAIN 18
#define DCC_SIGNAL_PIN_MAIN_2 19
// #define SIGNAL_ENABLE_PIN_MAIN 4 // these 4 are dummies for now, should remove
// #define SIGNAL_ENABLE_PIN_PROG 4
// #define CURRENT_MONITOR_PIN_MAIN 4
// #define CURRENT_MONITOR_PIN_PROG 4  // TBD to remove this remove writecvbyte and writecvbit functions from packetregister
#endif

#ifdef ESP8685_05
#define LEFT_HED_PIN 5
#define RIGHT_HED_PIN 6
#define DCC_SIGNAL_PIN_MAIN 3
#define DCC_SIGNAL_PIN_MAIN_2 4
#define HW_SERIAL_PIN 5
// #define SIGNAL_ENABLE_PIN_MAIN 7 // these 4 are dummies for now, should remove
// #define SIGNAL_ENABLE_PIN_PROG 7
// #define CURRENT_MONITOR_PIN_MAIN 7
// #define CURRENT_MONITOR_PIN_PROG 7
#endif

#ifdef ESP32WROOM
#define DCC_SIGNAL_PIN_MAIN 26
#define DCC_SIGNAL_PIN_MAIN_2 25
#define HW_SERIAL_PIN 21 
// #define SIGNAL_ENABLE_PIN_MAIN 16
// #define SIGNAL_ENABLE_PIN_PROG 17
// #define CURRENT_MONITOR_PIN_MAIN 18
// #define CURRENT_MONITOR_PIN_PROG 19
#define LEFT_HED_PIN 21
#define RIGHT_HED_PIN 22
#endif