#include "WiFiConfigurator.h"
#include "WebPages.h"

WiFiConfigurator::WiFiConfigurator(AsyncWebServer &srv,
                                   const char *prefsNamespace,
                                   const char *softApSSID,
                                   IPAddress apIP,
                                   unsigned long portalTimeoutMs)
    : server(srv),
      _softApSSID(softApSSID),
      _prefsNamespace(prefsNamespace),
      _apIP(apIP),
      _portalTimeoutMs(portalTimeoutMs),
      _portalActive(false),
      _portalStartMillis(0) {}

void WiFiConfigurator::begin()
{
    connectToSavedHouseAP(); // attempt STA

    if (WiFi.status() != WL_CONNECTED)
    {
        startSoftAP(); // start captive portal
        _portalActive = true;
        _portalStartMillis = millis();
    }

    setupWebServerRoutes();
}

void WiFiConfigurator::handle()
{
    if (_portalActive && (millis() - _portalStartMillis >= _portalTimeoutMs))
    {
        stopPortal();
    }
}

void WiFiConfigurator::restartPortal(unsigned long timeoutMs)
{
    _portalTimeoutMs = timeoutMs;
    if (!_portalActive)
    {
        startSoftAP();
        _portalActive = true;
    }
    _portalStartMillis = millis();
}

bool WiFiConfigurator::connectSTA(const char *ssid, const char *pass)
{
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(ssid, pass);

    Serial.println("[WiFiConfigurator] Connecting to STA...");
    unsigned long start = millis();
    const unsigned long timeout = 10000UL;

    while (WiFi.status() != WL_CONNECTED && millis() - start < timeout)
    {
        delay(200);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print("[WiFiConfigurator] Connected: ");
        Serial.println(WiFi.localIP());
        return true;
    }
    return false;
}

void WiFiConfigurator::startSoftAP()
{
    WiFi.mode(WIFI_MODE_AP); // open AP
    WiFi.softAP(_softApSSID.c_str());
    WiFi.softAPConfig(_apIP, _apIP, IPAddress(255, 255, 255, 0));
    Serial.printf("[WiFiConfigurator] SoftAP started: %s, IP: %s\n",
                  _softApSSID.c_str(), WiFi.softAPIP().toString().c_str());
}

void WiFiConfigurator::stopPortal()
{
    WiFi.softAPdisconnect(true);
    _portalActive = false;
    Serial.println("[WiFiConfigurator] SoftAP stopped (portal timeout)");
}

void WiFiConfigurator::setupWebServerRoutes()
{
    server.on("/apselect.html", HTTP_GET, [this](AsyncWebServerRequest *req)
              { req->send_P(200, "text/html", apselect_html); });

    server.on("/stylesheet.css", HTTP_GET, [](AsyncWebServerRequest *req)
              { req->send_P(200, "text/css", stylesheet_css); });

    server.on("/aplist", HTTP_GET, [this](AsyncWebServerRequest *req)
              { handleAPlist(req); });
    server.on("/status", HTTP_GET, [this](AsyncWebServerRequest *req)
              { handleStatus(req); });
    server.on("/connect", HTTP_POST, [this](AsyncWebServerRequest *req)
              { handleConnect(req); });

    server.onNotFound([this](AsyncWebServerRequest *req)
                      {
        if (WiFi.status() != WL_CONNECTED) {
            req->redirect(String("http://") + _apIP.toString() + "/apselect.html");
        } else {
            req->send(404, "text/plain", "Not found");
        } });
}


void WiFiConfigurator::handleAPlist(AsyncWebServerRequest *req)
{
    int n = WiFi.scanNetworks();
    String json = "[";
    bool first = true;
    for (int i = 0; i < n; ++i)
    {
        String s = WiFi.SSID(i);
        if (s.length() == 0)
            continue;
        if (!first)
            json += ",";
        json += "{\"ssid\":\"" + jsonEscape(s) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
        first = false;
    }
    json += "]";
    req->send(200, "application/json", json);
}

void WiFiConfigurator::handleStatus(AsyncWebServerRequest *req)
{
    bool connected = WiFi.status() == WL_CONNECTED;
    IPAddress ip = connected ? WiFi.localIP() : IPAddress(0, 0, 0, 0);
    String body = "{\"connected\":" + String(connected ? "true" : "false") + ",";
    body += "\"ssid\":\"" + jsonEscape(WiFi.SSID()) + "\",";
    body += "\"ip\":\"" + ip.toString() + "\"}";
    req->send(200, "application/json", body);
}

void WiFiConfigurator::handleConnect(AsyncWebServerRequest *req)
{
    String ssid, password;

    // check form-encoded
    if (req->hasParam("ssid", true))
        ssid = req->getParam("ssid", true)->value();
    if (req->hasParam("password", true))
        password = req->getParam("password", true)->value();

    if (ssid.length() == 0)
    {
        req->send(400, "application/json", "{\"ok\":false,\"message\":\"missing ssid\"}");
        return;
    }

    bool ok = attemptConnectAndSave(ssid.c_str(), password.c_str());
    req->send(200, "application/json",
              "{\"ok\":" + String(ok ? "true" : "false") +
                  ",\"message\":\"" + (ok ? "connecting" : "failed") + "\"}");
}

bool WiFiConfigurator::attemptConnectAndSave(const char *ssid, const char *password)
{
    bool connected = connectSTA(ssid, password);
    saveCredentials(ssid, password);
    return connected;
}

void WiFiConfigurator::connectToSavedHouseAP()
{
    _prefs.begin(_prefsNamespace, true);
    String ssid = _prefs.getString(KEY_SSID, "");
    String pass = _prefs.getString(KEY_PASS, "");
    _prefs.end();

    if (ssid.length() > 0)
    {
        Serial.printf("[WiFiConfigurator] Found SSID '%s', connecting...\n", ssid.c_str());
        connectSTA(ssid.c_str(), pass.c_str());
    }
    else
    {
        Serial.println("[WiFiConfigurator] No saved credentials → SoftAP mode");
    }
}

void WiFiConfigurator::saveCredentials(const char *ssid, const char *pass)
{
    _prefs.begin(_prefsNamespace, false);
    _prefs.putString(KEY_SSID, ssid);
    _prefs.putString(KEY_PASS, pass);
    _prefs.end();
    Serial.println("[WiFiConfigurator] Credentials saved");
}

String WiFiConfigurator::jsonEscape(const String &s)
{
    String out;
    for (unsigned int i = 0; i < s.length(); ++i)
    {
        char c = s[i];
        if (c == '\\' || c == '"')
        {
            out += '\\';
            out += c;
        }
        else if (c == '\n')
            out += "\\n";
        else
            out += c;
    }
    return out;
}

