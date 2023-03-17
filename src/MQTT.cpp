#include "Arduino.h"
#include "SerialCommand.h"
#include "RBot.h"
#include "Function.h"
#include "MQTT.h"
#include "PubSubClient.h"
#include "Throttle.h"

extern PubSubClient client;
extern Throttle throttle;




/*****************************************************************************/
// defines the connection parameters, the callback, the subscriptions and then connects
void mqttSetup(String mqtt_Server, String iobNode)
{
  char mqtt_server[mqtt_Server.length() + 1]; // converting from string to char array required for client parameter
  strcpy(mqtt_server, mqtt_Server.c_str());
  Serial.print("mqtt_server ");
  Serial.println(mqtt_server);
  uint8_t ip[4];
  // int ip[4];
  sscanf(mqtt_server, "%u.%u.%u.%u", &ip[0], &ip[1], &ip[2], &ip[3]);
  client.setServer(ip, 1883); // 1883 is the default port on mosquitto server
  // client.setKeepAlive(60);    // this is probaably not necessary, just use the default
  client.setCallback(callback);
  connectMQTT(iobNode);
  setupSubscriptions();
}

/*****************************************************************************/
// connects to the MQTT server
void connectMQTT(String nodeName)
{
  // bool flasher = false;

  char mqtt_node[nodeName.length() + 1];
  strcpy(mqtt_node, nodeName.c_str());

  // uint32_t now = millis();

  // Loop until we're reconnected
  while (!client.connect(mqtt_node))
  {
    pinMode(2, OUTPUT);
    Serial.println("Failed to connect to mqtt server");
    Serial.print(" Response was ");
    Serial.println(client.state());
    Serial.println("Retrying...");

    delay(1000);
    // todo should make a technique to notify op about this, he can't see the serial
  }
  Serial.println("connected to MQTT server");
}

/*****************************************************************************/
void setupSubscriptions()
{
  // subscribe to 'IOB/roadnum/command/#'

  char subscription[100];
  
  int roadNum = throttle.getRoadNumber();

  // String prefix = "IOB/command/";
  String prefix = "IOB/";
  prefix.concat(String(roadNum));
  prefix.concat(String("/command/#"));
  strcpy(subscription, prefix.c_str());
  int ret = client.subscribe(subscription, 1);

  if (ret == 0)
    Serial.println("Main subscribe failed");
  else
  {
    Serial.print("Main topic subscription set as: ");
    Serial.println(subscription);
  }
}

/*****************************************************************************/
// this is a callback from the mqtt object, made when a subscribed message comes in
// we expect the messages to be filtered on the first part of the topic
// so we need to examine the last component of topic to know the command
// also must get the value sent
void callback(char *topic, byte *message, unsigned int length)
{

  char messChars[50];
  String topicString = String(topic);

  String partString = topicString.substring(topicString.indexOf('/') + 1); // gets rid of first field (IOB)
  partString = partString.substring(partString.indexOf('/') + 1);          // gets rid of second field (roadnum)
  partString = partString.substring(partString.indexOf('/') + 1);          // gets rid of third field (command)

  // Serial.print("topic string "); Serial.println(topicString);
  // Serial.print("last part "); Serial.println(partString);

  for (int i = 0; i < length; i++)
    messChars[i] = (char)message[i];

  messChars[length] = '\0';
  int msgVal = atoi(messChars);

  // if (partString == "startstop")
  // {
  //   startStop(msgVal); // TBD this is clumsy as hell
  //   if (msgVal)
  //     throttle.startPM();
  //   else
  //     throttle.stopPM();
  // }
  if (partString == "startstop")
    throttle.pmOnOff(msgVal);
  else if (partString == "bell")
    throttle.bell(msgVal);
  else if (partString == "horn")
    throttle.horn(msgVal);
  else if (partString == "headlight")
    throttle.headlight(msgVal);
  else if (partString == "throttleLever")
    throttle.setThrottleLever(msgVal);
  else if (partString == "notchUp")
    throttle.manualNotch(true);
  else if (partString == "notchDown")
    throttle.manualNotch(false);
  else if (partString == "direction")
    throttle.setDirection(msgVal);
  else if (partString == "reverser")
    throttle.setDirection(msgVal);
  else if (partString == "ibrake")
    throttle.setIBrake(msgVal);
  else if (partString == "tbrake")
    throttle.setTBrake(msgVal);
  else if (partString == "tonnage")
    throttle.setMass(msgVal);
  else if (partString.substring(0, 1) == "f")
  {
    partString = partString.substring(partString.lastIndexOf('/') + 1);
    setFunction(atoi(partString.c_str()), (bool)msgVal);
  }
  else if (partString == "test")
    Serial.println("received test message");
  return;
}
