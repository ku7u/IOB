#pragma once
#include <Arduino.h>

class JsonBuilder {
public:
  JsonBuilder() {
    reset();
  }

  // Reset for a new JSON object
  void reset() {
    json = "{";
    firstField = true;
  }

  // -------------------------
  // String values
  // -------------------------
  void add(const char* key, const char* value) {
    addKey(key);
    json += "\"";
    json += value;
    json += "\"";
  }

  void add(const String& key, const String& value) {
    addKey(key.c_str());
    json += "\"";
    json += value;
    json += "\"";
  }

  // -------------------------
  // Integer values
  // -------------------------
  void add(const char* key, int value) {
    addKey(key);
    json += value;
  }

  void add(const char* key, unsigned int value) {
    addKey(key);
    json += value;
  }

  void add(const char* key, long value) {
    addKey(key);
    json += value;
  }

  void add(const char* key, unsigned long value) {
    addKey(key);
    json += value;
  }

  void add(const String& key, int value) {
    addKey(key.c_str());
    json += value;
  }

  void add(const String& key, unsigned int value) {
    addKey(key.c_str());
    json += value;
  }

  void add(const String& key, long value) {
    addKey(key.c_str());
    json += value;
  }

  void add(const String& key, unsigned long value) {
    addKey(key.c_str());
    json += value;
  }

  // -------------------------
  // Floating-point values
  // -------------------------
  void add(const char* key, float value, int decimals = 2) {
    addKey(key);
    json += String(value, decimals);
  }

  void add(const char* key, double value, int decimals = 2) {
    addKey(key);
    json += String(value, decimals);
  }

  void add(const String& key, float value, int decimals = 2) {
    addKey(key.c_str());
    json += String(value, decimals);
  }

  void add(const String& key, double value, int decimals = 2) {
    addKey(key.c_str());
    json += String(value, decimals);
  }

  // -------------------------
  // Boolean values
  // -------------------------
  void add(const char* key, bool value) {
    addKey(key);
    json += (value ? "true" : "false");
  }

  void add(const String& key, bool value) {
    addKey(key.c_str());
    json += (value ? "true" : "false");
  }

  // -------------------------
  // Finalize JSON
  // -------------------------
  String end() {
    return json + "}";
  }

private:
  String json;
  bool firstField;

  void addKey(const char* key) {
    if (!firstField) {
      json += ",";
    }
    firstField = false;

    json += "\"";
    json += key;
    json += "\":";
  }
};
