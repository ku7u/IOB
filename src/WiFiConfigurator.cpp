#include "WiFiConfigurator.h"
#include <DNSServer.h>
#include <SPIFFS.h>

#define DNS_PORT 53

WiFiConfigurator::WiFiConfigurator(AsyncWebServer& srv) : server(srv) {}

void WiFiConfigurator::begin() {
    // get the road number
    prefs.begin("loco", true);
    _locoID = prefs.getString("locoid", "new");
    prefs.end();

    prefs.begin("wifi", false);
    connectToWiFi();
    startSoftAP();
    setupRoutes();
    setupCaptivePortal();
}

void WiFiConfigurator::loop() {
    if (softAPActive && millis() - softAPStartTime > softAPTimeoutMs) {
        Serial.println("SoftAP timeout reached. Stopping SoftAP...");
        stopSoftAP();
    }
}

void WiFiConfigurator::connectToWiFi() {
    String ssid = prefs.getString("ssid", "");
    String password = prefs.getString("password", "");

    if (ssid.length() > 0) {
        WiFi.begin(ssid.c_str(), password.c_str());
        Serial.printf("Connecting to %s", ssid.c_str());
        unsigned long startAttempt = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
            delay(500);
            Serial.print(".");
        }
        Serial.println(WiFi.status() == WL_CONNECTED ? "\nConnected." : "\nFailed.");
    } else {
        Serial.println("No stored credentials.");
    }
}

void WiFiConfigurator::startSoftAP() {
    char softAPName [50];
    String softAPNameString = "IOB_AP_" + _locoID;
    Serial.println(softAPNameString);
    strcpy(softAPName, softAPNameString.c_str());
    WiFi.softAP(softAPName);
    softAPActive = true;
    softAPStartTime = millis();
    Serial.println("Started SoftAP: " + String(softAPName));
    Serial.println(WiFi.softAPIP());
}

void WiFiConfigurator::stopSoftAP() {
    WiFi.softAPdisconnect(true);
    softAPActive = false;
}

void WiFiConfigurator::setupCaptivePortal() {
    static DNSServer dnsServer;
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

    // Call loop() on dnsServer periodically
    static TaskHandle_t dnsTaskHandle;
    xTaskCreate([](void *param) {
        while (true) {
            dnsServer.processNextRequest();
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }, "dns_loop", 4096, NULL, 1, &dnsTaskHandle);
}

void WiFiConfigurator::setupRoutes() {
    server.on("/scan", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleScan(request);
    });

    server.on("/save", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleSave(request);
    });

    server.onNotFound([this](AsyncWebServerRequest *request) {
        handleNotFound(request);
    });

    // server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html").setTemplateProcessor(
    //     [this](const String& var) {
    //         return processor(var);
    //     }
    // );

    server.begin();
    Serial.println("Web server started.");
}

void WiFiConfigurator::handleScan(AsyncWebServerRequest *request) {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; ++i) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
    }
    json += "]";
    request->send(200, "application/json", json);
}

void WiFiConfigurator::handleSave(AsyncWebServerRequest *request) {
    if (!request->hasParam("ssid", true) || !request->hasParam("password", true)) {
        request->send(400, "text/plain", "Missing ssid or password");
        return;
    }

    String ssid = request->getParam("ssid", true)->value();
    String password = request->getParam("password", true)->value();

    saveCredentials(ssid, password);

    String html = "<html><body><h2>Saved. Rebooting...</h2></body></html>";
    request->send(200, "text/html", html);
    delay(2000);
    ESP.restart();
}

void WiFiConfigurator::handleNotFound(AsyncWebServerRequest *request) {
    String path = request->url();
    if (SPIFFS.exists(path)) {
        request->send(SPIFFS, path, getContentType(path), false, [this](const String& var) {
            return processor(var);
        });
    } else {
        request->send(404, "text/plain", "Not found");
    }
}

void WiFiConfigurator::saveCredentials(const String& ssid, const String& password) {
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    Serial.printf("Saved credentials: %s / %s\n", ssid.c_str(), password.c_str());
}

String WiFiConfigurator::processor(const String& var) {
    if (var == "version") {
        return "1.0.0";  // Replace with dynamic version if needed
    }
    return String();
}

String WiFiConfigurator::getContentType(const String& filename) {
    if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) return "image/jpeg";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".json")) return "application/json";
    else if (filename.endsWith(".txt")) return "text/plain";
    return "application/octet-stream";
}
