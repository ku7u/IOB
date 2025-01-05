#pragma once

#include "Arduino.h"

void mqttSetup(String mqtt_Server, String iobNode);
void connectMQTT(String iobNode);
void setupSubscriptions();
void callback(char *topic, byte *message, unsigned int length);
