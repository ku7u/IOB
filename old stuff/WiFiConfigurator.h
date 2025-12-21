#pragma once

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include "SPIFFS.h"

// WiFiConfigurator that uses an existing AsyncWebServer reference.
// - safe SPIFFS mount (no format)
// - uses only the provided server (no internal AsyncWebServer instance)
// - robust SoftAP startup (ensures SSID present, sets mode, small delays)
// - registers only captive-portal endpoints (/apselect.html, /aplist, /status, /connect)
// - onNotFound redirects to captive portal only when STA is NOT connected
class WiFiConfigurator
{
public:
    WiFiConfigurator(AsyncWebServer &srv,
                     const char *prefsNamespace = "wifi_conf",
                     const char *softApSSID = "IOB_AP",
                     IPAddress apIP = IPAddress(192, 168, 4, 1),
                     unsigned long portalTimeoutMs = 180000UL)
        : server(srv),
          _softApSSID(softApSSID),
          _prefsNamespace(prefsNamespace),
          _apIP(apIP),
          _portalTimeoutMs(portalTimeoutMs),
          _portalActive(false),
          _portalStartMillis(0)
    {
    }

    // call once in setup()
    void begin()
    {
#ifdef SERIAL_ON
        Serial.println("[WiFiConfigurator] begin()");
#endif
        // Mount SPIFFS but DO NOT format (preserve existing files)
        if (!SPIFFS.begin(false))
        {
#ifdef SERIAL_ON
            Serial.println("[WiFiConfigurator] SPIFFS mount failed (not formatting).");
#endif
        }
        else
        {
#ifdef SERIAL_ON
            Serial.println("[WiFiConfigurator] SPIFFS mounted.");
#endif
        }

        // Start SoftAP safely
        startSoftAP();

        // Register captive-portal routes on the provided server
        setupWebServerRoutes();

        // Do NOT call server.begin() here — let app start server if needed.
        // Attempt connection to saved house AP (background)
        connectToSavedHouseAP();

        _portalActive = true;
        _portalStartMillis = millis();

#ifdef SERIAL_ON
        Serial.println("[WiFiConfigurator] Captive portal initialized.");
#endif
    }

    // call regularly in loop()
    void handle()
    {
        if (_portalActive && (millis() - _portalStartMillis >= _portalTimeoutMs))
        {
            stopPortal();
        }
    }

    // optionally restart portal timer and ensure AP running
    void restartPortal(unsigned long timeoutMs = 180000UL)
    {
        _portalTimeoutMs = timeoutMs;
        if (!_portalActive)
        {
            startSoftAP();
            _portalActive = true;
        }
        _portalStartMillis = millis();
    }

    void setSoftApSSID(const String &ssid)
    {
        _softApSSID = ssid;
    }

private:
    AsyncWebServer &server; // reference to caller's server

    String _softApSSID;
    const char *_prefsNamespace;
    IPAddress _apIP;
    unsigned long _portalTimeoutMs;

    Preferences _prefs;
    bool _portalActive;
    unsigned long _portalStartMillis;

    const char *KEY_SSID = "ssid";
    const char *KEY_PASS = "pass";

    void startSoftAP()
    {
        WiFi.mode(WIFI_MODE_APSTA);
        delay(100);

        if (_softApSSID.isEmpty())
        {
            _softApSSID = "IOB_AP"; // fallback if nothing set
        }

        WiFi.softAPConfig(
            IPAddress(192, 168, 4, 1),
            IPAddress(192, 168, 4, 1),
            IPAddress(255, 255, 255, 0));

        if (WiFi.softAP(_softApSSID.c_str()))
        {
#ifdef SERIAL_ON
            Serial.printf("[WiFiConfigurator] SoftAP started: %s, IP: %s\n",
                          _softApSSID.c_str(),
                          WiFi.softAPIP().toString().c_str());
#endif
        }
        else
        {
#ifdef SERIAL_ON
            Serial.println("[WiFiConfigurator] Failed to start SoftAP!");
#endif
        }
    }

    void stopPortal()
    {
#ifdef SERIAL_ON
        Serial.println("[WiFiConfigurator] Stopping SoftAP (portal timeout).");
#endif
        WiFi.softAPdisconnect(true);
        _portalActive = false;
    }

    // --- Web routes on provided server only (no new AsyncWebServer created) ---
    void setupWebServerRoutes()
    {
        // captive page
        server.on("/apselect.html", HTTP_GET, [this](AsyncWebServerRequest *req)
                  { serveApSelectPage(req); });

        // static files (your existing app will also register its own handlers; serving these here is harmless)
        server.serveStatic("/stylesheet.css", SPIFFS, "/stylesheet.css");

        // APIs
        server.on("/aplist", HTTP_GET, [this](AsyncWebServerRequest *req)
                  { handleAPlist(req); });
        server.on("/status", HTTP_GET, [this](AsyncWebServerRequest *req)
                  { handleStatus(req); });
        server.on("/connect", HTTP_POST, [this](AsyncWebServerRequest *req)
                  { handleConnect(req); });

        // onNotFound: only redirect to captive portal if STA is NOT connected
        server.onNotFound([this](AsyncWebServerRequest *req)
                          {
            if (WiFi.status() != WL_CONNECTED) {
                // User is not on a configured network: point them to captive portal
                req->redirect(String("http://") + _apIP.toString() + "/apselect.html");
            } else {
                // STA connected: let the app's handlers (setupWeb) handle known paths; if nothing matches, 404
                req->send(404, "text/plain", "Not found");
            } });
    }

    void serveApSelectPage(AsyncWebServerRequest *req)
    {
        if (SPIFFS.exists("/apselect.html"))
        {
            req->send(SPIFFS, "/apselect.html", "text/html");
        }
        else
        {
            req->send(200, "text/html", embeddedApSelectHtml());
        }
    }

    void handleAPlist(AsyncWebServerRequest *req)
    {
        int n = WiFi.scanNetworks();
        String json = "[";
        bool first = true;
        for (int i = 0; i < n; ++i)
        {
            String ssid = WiFi.SSID(i);
            if (ssid.length() == 0)
                continue; // skip hidden
            if (!first)
                json += ",";
            json += "{\"ssid\":\"" + jsonEscape(ssid) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + ",\"auth\":" + String((int)WiFi.encryptionType(i)) + "}";
            first = false;
        }
        json += "]";
        req->send(200, "application/json", json);
    }

    void handleStatus(AsyncWebServerRequest *req)
    {
        bool connected = (WiFi.status() == WL_CONNECTED);
        String ssid = connected ? WiFi.SSID() : "";
        IPAddress ip = connected ? WiFi.localIP() : IPAddress(0, 0, 0, 0);
        String body = "{\"connected\":" + String(connected ? "true" : "false") + ",";
        body += "\"ssid\":\"" + jsonEscape(ssid) + "\",";
        body += "\"ip\":\"" + ip.toString() + "\"}";
        req->send(200, "application/json", body);
    }

    void handleConnect(AsyncWebServerRequest *req)
    {
        String ssid, password;
        // support form-encoded post-body params (Async lib places them as "params" with isPost = true)
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
        req->send(200, "application/json", "{\"ok\":" + String(ok ? "true" : "false") + ",\"message\":\"" + (ok ? "connecting" : "failed") + "\"}");
    }

    bool attemptConnectAndSave(const char *ssid, const char *password)
    {
#ifdef SERIAL_ON
        Serial.printf("[WiFiConfigurator] attemptConnectAndSave('%s')\n", ssid ? ssid : "<null>");
#endif
        // ensure STA works while AP may remain active
        WiFi.mode(WIFI_MODE_APSTA);
        delay(20);
        WiFi.begin(ssid, password);

        unsigned long start = millis();
        const unsigned long timeout = 10000; // 10s blocking attempt
        while (millis() - start < timeout)
        {
            if (WiFi.status() == WL_CONNECTED)
            {
                saveCredentials(ssid, password);
                // optionally stop AP so client can reach local host directly — we leave AP running for now
#ifdef SERIAL_ON
                Serial.println("[WiFiConfigurator] Connected to STA.");
#endif
                return true;
            }
            delay(200);
        }
        // save for background retry
        saveCredentials(ssid, password);
        WiFi.begin(ssid, password);
        return false;
    }

    void connectToSavedHouseAP()
    {
        _prefs.begin(_prefsNamespace, true);
        String ssid = _prefs.getString(KEY_SSID, "ssidGlarb");
        String pass = _prefs.getString(KEY_PASS, "pwGlarb");
        _prefs.end();
        if (ssid.length() > 0)
        {
#ifdef SERIAL_ON
            Serial.printf("[WiFiConfigurator] Found saved SSID '%s', trying background connect\n", ssid.c_str());
#endif
            WiFi.mode(WIFI_MODE_APSTA);
            delay(20);
            WiFi.begin(ssid.c_str(), pass.c_str());

            unsigned long start = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - start < 8000)
            {
                Serial.print(".");
                delay(500);
            }

            if (WiFi.status() == WL_CONNECTED)
            {
#ifdef SERIAL_ON
                Serial.print("Connected! IP: ");
                Serial.println(WiFi.localIP());
#endif
            }
            else
            {
#ifdef SERIAL_ON
                Serial.println("STA FAILED to connect");
#endif
            }
        }
        else
        {
#ifdef SERIAL_ON
            Serial.println("[WiFiConfigurator] No saved SSID.");
#endif
        }
    }

    void saveCredentials(const char *ssid, const char *pass)
    {
        _prefs.begin(_prefsNamespace, false);
        size_t ssidWritten = _prefs.putString(KEY_SSID, ssid);
        size_t passWritten = _prefs.putString(KEY_PASS, pass);
        _prefs.end();
#ifdef SERIAL_ON
        Serial.println("ssidWritten " + String(ssidWritten));
        Serial.println("passWritten " + String(passWritten));
        Serial.println("[WiFiConfigurator] Credentials saved.");
#endif
    }

    String jsonEscape(const String &s)
    {
        String out;
        for (unsigned int i = 0; i < s.length(); ++i)
        {
            char c = s[i];
            if (c == '\\' || c == '\"')
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

    String embeddedApSelectHtml()
    {
        return R"rawliteral(
<!doctype html>
<html>
<head><meta name="viewport" content="width=device-width, initial-scale=1"><title>WiFi Setup</title>
<link rel="stylesheet" href="/stylesheet.css"></head><body>
<h2>Configure Wi-Fi</h2>
<div id="status">Loading...</div>
<div><input id="ssid" placeholder="SSID"/><input id="password" type="password" placeholder="Password"/><button id="saveBtn">Save & Connect</button></div>
<div id="aplist">Scanning...</div>
<script>
function updateStatus(){fetch('/status').then(r=>r.json()).then(js=>{let s=document.getElementById('status');s.innerText=js.connected?'Connected '+js.ssid:'Not connected';});}
function scanAps(){fetch('/aplist').then(r=>r.json()).then(list=>{let c=document.getElementById('aplist');c.innerHTML='';list.forEach(item=>{let d=document.createElement('div');d.innerText=item.ssid+' RSSI:'+item.rssi;let b=document.createElement('button');b.innerText='Select';b.onclick=()=>{document.getElementById('ssid').value=item.ssid};d.appendChild(b);c.appendChild(d);});});}
document.getElementById('saveBtn').onclick=function(){let ssid=document.getElementById('ssid').value;let pass=document.getElementById('password').value;if(!ssid){alert('Enter SSID');return;}fetch('/connect',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'ssid='+encodeURIComponent(ssid)+'&password='+encodeURIComponent(pass)}).then(r=>r.json()).then(js=>{alert(js.message);updateStatus();});};
window.addEventListener('load',()=>{updateStatus();scanAps();setInterval(updateStatus,5000);setInterval(scanAps,10000);});
</script>
</body></html>
        )rawliteral";
    }
};
