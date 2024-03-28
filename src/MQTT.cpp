#include "Arduino.h"
#include "Preferences.h"
#include "Function.h"
#include "MQTT.h"
#include "PubSubClient.h"
#include "Throttle.h"
#include "Adafruit_NeoPixel.h"
#include "defines.h"

extern PubSubClient client;
extern Throttle throttle;
extern Adafruit_NeoPixel strip;

/*****************************************************************************/
// defines the connection parameters, the callback, the subscriptions and then connects
void mqttSetup(String mqtt_Server, String iobNode)
{

  char mqtt_server[mqtt_Server.length() + 1]; // converting from string to char array required for client parameter
  strcpy(mqtt_server, mqtt_Server.c_str());
#ifdef SERIAL_ON
  Serial.print("mqtt_server ");
  Serial.println(mqtt_server);
#endif
  uint8_t ip[4];
  sscanf(mqtt_server, "%u.%u.%u.%u", &ip[0], &ip[1], &ip[2], &ip[3]);
  client.setBufferSize(1024); // defaults to 256
  client.setServer(ip, 1883); // 1883 is the default port on mosquitto server
  client.setKeepAlive(60);    // this is probaably not necessary, just use the default
  client.setCallback(callback);
  // connectMQTT(iobNode);
  // setupSubscriptions();
}

/*****************************************************************************/
// connects to the MQTT server
void connectMQTT(String nodeName)
{
  // bool flasher = false;

  char mqtt_node[nodeName.length() + 1];
  strcpy(mqtt_node, nodeName.c_str());

  // Loop until we're reconnected
  while (!client.connect(mqtt_node))
  {
    // pinMode(2, OUTPUT);
#ifdef SERIAL_ON
    Serial.println("Failed to connect to mqtt server");
    Serial.print(" Response was ");
    Serial.println(client.state());
    Serial.println("Retrying...");
#endif

    delay(5000);
  }
#ifdef SERIAL_ON
  Serial.println("connected to MQTT server");
#endif
}

/*****************************************************************************/
void setupSubscriptions()
{
  // subscribe to 'IOB/roadnum/command/#'

  char subscription[100];
  Preferences myPrefs;

  myPrefs.begin("loco", true);
  String myRoadnum = String(myPrefs.getInt("roadnum", 3));
  String myLocoID = myPrefs.getString("locoid", "none");
  myPrefs.end();

  myPrefs.begin("general", true);
  String prefix = myPrefs.getString("commandtopic", "cmd/ols/");
  myPrefs.end();

  String prefixGlobal = prefix;

  prefix.concat(String(myLocoID + "/#"));
  strcpy(subscription, prefix.c_str());
  client.subscribe(subscription, 1);

  prefixGlobal.concat(String("0/#"));
  strcpy(subscription, prefixGlobal.c_str());
  client.subscribe(subscription, 1);

  throttle.muSubscribe();
}

/*****************************************************************************/
// this is a callback from the mqtt object, made when a subscribed message comes in
// we expect the messages to be filtered on the first part of the topic
// so we need to examine the last component of topic to know the command
// also must get the value sent
void callback(char *topic, byte *message, unsigned int length)
{
  char messChars[100];
  String topicString = String(topic);

  String partString = topicString.substring(topicString.indexOf('/') + 1); // gets rid of first field (cmd)
  partString = partString.substring(partString.indexOf('/') + 1);          // gets rid of second field (ols)
  partString = partString.substring(partString.indexOf('/') + 1);          // gets rid of third field (roadnum)
  // the above scheme also works for mu messaging
  // we only see cmd messages addressed to us or tlm messages sourced from our lead
  // if mid or trailing we will see messages like tlm/ols/leadID/status but only from leadID as subscribed
  // so can ignore first three fields as above and search for 'status'

  for (int i = 0; i < length; i++)
    messChars[i] = (char)message[i];

  messChars[length] = '\0';
  int msgVal = atoi(messChars);

  if (partString == "report")
    throttle.report();
  else if (partString == "startstop")
    throttle.pmOnOff(msgVal);
  else if (partString == "stop")
    throttle.panicStop();
  else if (partString == "bell")
    throttle.bell(msgVal);
  else if (partString == "horn")
    throttle.horn(msgVal);
  else if (partString == "headlight")
    throttle.headlight(msgVal);
  else if (partString == "rearlight")
    throttle.rearlight(msgVal);
  else if (partString == "throttlelever")
    throttle.setThrottleLever(msgVal);
  else if (partString == "notch")
    throttle.manualNotch(msgVal);
  else if (partString == "longpress")
    throttle.longPress(msgVal);
  else if (partString == "direction")
    throttle.setDirection(msgVal);
  else if (partString == "reverser")
    throttle.setDirection(msgVal);
  else if (partString == "brake")
  {
    throttle.setIBrake(msgVal);
    // throttle.setTBrake(msgVal);  TBD setTbrake must be changed such that msgVal relates to a psi reduction, and sensitive to airline connect status
  }
  else if (partString == "ibrake")
    throttle.setIBrake(msgVal);
  else if (partString == "tbrake")
    throttle.setTBrake(msgVal);
  else if (partString == "trainline")
    throttle.trainline(msgVal);
  else if (partString == "carcount")
    throttle.setCarCount(msgVal);
  else if (partString == "setmustate")
    throttle.setMuState(messChars);
  else if (partString == "muperformance")
    throttle.setMuPerformance(messChars);
  else if (partString == "status")
    throttle.setMuSpeed(messChars);
  else if (partString == "sendstatus")
  {
    throttle.reportCondition();
    throttle.reportStatus(); 
    throttle.setAirGauge();
  }
  else if (partString == "reportlabels")
  {
    throttle.reportFunctionLabels();
  }
  else if (partString == "calibrate")
    throttle.calibrate(msgVal);
  else if (partString == "function")
    throttle.setFunction(messChars);

  return;
}
