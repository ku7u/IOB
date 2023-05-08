#ifndef MQTT_H
#define MQTT_H


void mqttSetup(String mqtt_Server, String iobNode);
void connectMQTT(String iobNode);
void setupSubscriptions();
void callback(char *topic, byte *message, unsigned int length);

#endif