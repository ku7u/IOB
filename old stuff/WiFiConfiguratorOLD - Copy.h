#pragma once

// #ifndef WIFI_CONFIGURATOR_H
// #define WIFI_CONFIGURATOR_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

class WiFiConfigurator {
public:
    WiFiConfigurator(AsyncWebServer& server);
    void begin();
    void loop();

private:
    void startSoftAP();
    void connectToWiFi();
    void setupRoutes();
    void setupCaptivePortal();
    void stopSoftAP();

    void handleScan(AsyncWebServerRequest *request);
    void handleSave(AsyncWebServerRequest *request);
    void handleNotFound(AsyncWebServerRequest *request);
    String processor(const String& var);

    void saveCredentials(const String& ssid, const String& password);
    bool isWiFiConnected();
    String getContentType(const String& filename);

    AsyncWebServer& server;
    Preferences prefs;
    bool softAPActive = false;
    unsigned long softAPStartTime = 0;
    const unsigned long softAPTimeoutMs = 5 * 60 * 1000;  // 5 minutes
    String _locoID;
};

// #endif
