#pragma once

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

class WiFiConfigurator
{
public:
    WiFiConfigurator(AsyncWebServer &srv,
                     const char *prefsNamespace = "wifi_conf",
                     const char *softApSSID = "Device_AP",
                     IPAddress apIP = IPAddress(192, 168, 4, 1),
                     unsigned long portalTimeoutMs = 180000UL);

    void begin();  // call in setup()
    void handle(); // call regularly in loop()
    void restartPortal(unsigned long timeoutMs = 180000UL);
    bool connectSTA(const char *ssid, const char *pass);

private:

    AsyncWebServer &server;

    String _softApSSID;
    const char *_prefsNamespace;
    IPAddress _apIP;
    unsigned long _portalTimeoutMs;

    Preferences _prefs;
    bool _portalActive;
    unsigned long _portalStartMillis;

    const char *KEY_SSID = "ssid";
    const char *KEY_PASS = "pass";

    void startSoftAP();
    void stopPortal();
    void setupWebServerRoutes();
    // void serveApSelectPage(AsyncWebServerRequest *req);
    void handleAPlist(AsyncWebServerRequest *req);
    void handleStatus(AsyncWebServerRequest *req);
    void handleConnect(AsyncWebServerRequest *req);
    bool attemptConnectAndSave(const char *ssid, const char *password);
    void connectToSavedHouseAP();
    void saveCredentials(const char *ssid, const char *pass);
    String jsonEscape(const String &s);
};
