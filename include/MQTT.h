#ifndef MQTT_H
#define MQTT_H

#include "WiFi.h"
#include "PubSubClient.h"
#include "Throttle.h"
#include "Arduino.h"

extern WiFiClient espClient;
extern PubSubClient client;
extern Throttle throttle;
extern int roadNum;

void mqttSetup(String mqtt_Server, String iobNode);
void connectMQTT(String iobNode);
void setupSubscriptions();
void callback(char *topic, byte *message, unsigned int length);

#endif