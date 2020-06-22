#define SerialAT mySerial

#define SerialMon Serial
#define TINY_GSM_MODEM_SIM800

#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>        // Include the Wi-Fi library
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <TinyGsmClient.h>

//Create software serial object to communicate with SIM800L
SoftwareSerial mySerial(D1, D2); //SIM800L Tx & Rx is connected to ESP8266 D1 & D2
WiFiClient espClient;
PubSubClient mqtt(espClient);
TinyGsm modem(mySerial);
const char* ssid     = "";     // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "";     // The password of the Wi-Fi network
const char* broker = "";     // ip / domain your mqtt broker

const char* topicSMS = "";    // name MQTTtopic for SMS
const char* topicReport = ""; // name MQTTtopic for reporting
const char* topicBalance = ""; // name MQTTtopic for check balance
const char* deviceName = "";      // name of your device
StaticJsonDocument<250> wrapper;

boolean res;
boolean mqttConnect() {
  char buffer[256];
  SerialMon.print("Connecting to ");
  SerialMon.print(broker);
  wrapper["deviceName"] = deviceName;

  // Connect to MQTT Broker
  boolean status = mqtt.connect(deviceName);

  if (status == false) {
    SerialMon.println("fail");
    return false;
  }
  SerialMon.println("success");
  mqtt.subscribe(topicBalance);
  mqtt.subscribe(topicSMS);
  wrapper["kind"] = "connected";
  wrapper["status"] = true;
  size_t n = serializeJson(wrapper,buffer);
  mqtt.publish(topicReport,buffer,n);
  return mqtt.connected();
}

void callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<200> doc;
  char buffer[256];
  deserializeJson(doc,payload,length);
  wrapper["id"] = doc["id"];
  wrapper["deviceName"] = deviceName;
  wrapper["kind"] = topic;

  if(strcmp(topic, topicBalance) == 0){
    String ussd = modem.sendUSSD("*123#");
    wrapper["payload"] = (char*) ussd.c_str();
    size_t n = serializeJson(wrapper,buffer);
    mqtt.publish(topicReport,buffer,n);
  
  }else if(strcmp(topic, topicSMS) == 0){
    const char* phoneNumber = doc["phoneNumber"]; 
    const char* messagePayload = doc["message"]; 
    res = modem.sendSMS(phoneNumber,messagePayload);
    wrapper["status"] = res;
    size_t n = serializeJson(wrapper,buffer);
    mqtt.publish(topicReport,buffer,n);
  }

}


void setup()
{
  SerialMon.begin(9600);
  mySerial.begin(9600);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
        delay(1000);
        Serial.print('*');
  }

  modem.init();

  mqtt.setServer(broker, 1883); // connect to mqtt broker with port (default : 1883)
  mqtt.setCallback(callback);

}

void loop()
{
  if (!mqtt.connected()) {
      SerialMon.println("Trying Connecting to mqtt broker");
    if(mqttConnect()){
      SerialMon.println("MQTT Connected");
    }
  }


  mqtt.loop();

}

