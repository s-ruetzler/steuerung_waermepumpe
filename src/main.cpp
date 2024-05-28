#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h> 
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <configWebserver.h>

#include <PubSubClient.h>

String mqtt_user;
String mqtt_pass;
String mqtt_server;
int mqtt_port;
String mqttTopicPumpe;
String mqttTopicSteuerung;
String mqttTopicZustand;

unsigned long lastTime = 0;
unsigned int timerDelay = 3000;
bool configMode = false;
bool wlanOpen = false;
bool wlanConnected = false;
int zustand = 0;
const int pinEinschalten = A0;


String ssidWLanSelf;
String passwordWlanSelf;
String usernameWebsite;
String passwordWebsite;

ConfigWebserver *configServer = nullptr;

WiFiClientSecure espClient;
PubSubClient client(espClient);

// AsyncWebServer server(80);

void openWLAN();
void closeWLAN();
void setup_wifi_mqtt();
void passwordsIntern();
void changeZustand(int newNR);
void callback_mqtt(char* topic, byte* payload, unsigned int length);
void reconnect_mqtt();

void setup() {
  Serial.begin(115200);
  pinMode(pinEinschalten, OUTPUT);
  digitalWrite(pinEinschalten, LOW);
  delay(2000);

  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  passwordsIntern();

  configServer = new ConfigWebserver(&LittleFS, usernameWebsite, passwordWebsite);

  openWLAN();
  setup_wifi_mqtt();


  

}

void loop() {
  wlanConnected = WiFi.status() == WL_CONNECTED;
  if (configMode){
    if (!wlanOpen){
      openWLAN();
    }
  }
  else{
    if (wlanOpen){
      closeWLAN();
    }
  }

  //Wenn inerhalb von 10 Sekunden keine WLAN verbindung hergestellt werden kann, dann wird der ConfigMode gestartet.
  if ((!wlanConnected) && millis() > 10000){
    configMode = true;
  }

  //Wenn WLAN verbunden ist und MQTT nicht verbunden ist, dann versucht er zu verbinden.
  if (!client.connected() && wlanConnected) {
    if (millis() - lastTime > timerDelay)
    {
      Serial.println("Neu verbinden...");
      reconnect_mqtt();
      lastTime = millis();
    }
  }
  client.loop();
}


void openWLAN(){
  WiFi.mode(WIFI_AP_STA);
  
  WiFi.softAP(ssidWLanSelf, passwordWlanSelf);

  // IPAddress ip = WiFi.softAPIP();

  WiFi.softAPIP();
  
  Serial.println("WLAN göffnet");
  wlanOpen = true;
}

void closeWLAN(){
  WiFi.mode(WIFI_STA);
  Serial.println("WLAN geschlossen");
  wlanOpen = false;
}

void setup_wifi_mqtt(){
  String wlanSsid = configServer->config["ssid"];
  String wlanPasswort = configServer->config["passwortWlan"];

  String tmpU = configServer->config["mqttBenutzername"];
  String tmpP = configServer->config["mqttPasswort"];
  String tmpS = configServer->config["mqttBroker"];
  int tmpPo = configServer->config["mqttPort"];
  String tmpTP = configServer->config["mqttTopicPumpe"];
  String tmpTS = configServer->config["mqttTopicSteuerung"];
  String tmpTZ = configServer->config["mqttTopicZustand"];

  mqtt_user = tmpU;
  mqtt_pass = tmpP;
  mqtt_server = tmpS;
  mqtt_port = tmpPo;
  mqttTopicPumpe = tmpTP;
  mqttTopicSteuerung = tmpTS;
  mqttTopicZustand = tmpTZ;

  WiFi.begin(wlanSsid, wlanPasswort);

}

void passwordsIntern(){
  File file = LittleFS.open("/secret.txt", "r");
  if (!file) {
    Serial.println("Datei konnte nicht geöffnet werden");
    return;
  }
  ssidWLanSelf = file.readStringUntil('\n');
  passwordWlanSelf = file.readStringUntil('\n');
  usernameWebsite = file.readStringUntil('\n');
  passwordWebsite = file.readStringUntil('\n');

  file.close();
}

void changeZustand(int newNR){

  zustand = newNR;

  switch (zustand)
  {
  case 0:
    digitalWrite(pinEinschalten, LOW);
    break;
  
  case 1:
    digitalWrite(pinEinschalten, HIGH);
    break;

  case 2:
    digitalWrite(pinEinschalten, HIGH);
    break;

  case 3:
    digitalWrite(pinEinschalten, LOW);
    break;
  
  default:
    digitalWrite(pinEinschalten, LOW);
    break;
  }
  
}


void callback_mqtt(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  String payloadString = "";
  for (unsigned int i = 0; i < length; i++) {
    payloadString += (char)payload[i];
  }
  Serial.println(payloadString);

  // DeserializationError error = deserializeJson(daten, payload);
  // if (error) {
  //   Serial.print(F("Parsing failed: "));
  //   Serial.println(error.c_str());
  //   return;
  // }
  // veraenderung();

  // Serial.println(daten.as<String>());

}


void reconnect_mqtt() {

  client.setServer(mqtt_server.c_str(), mqtt_port);
  // client.setBufferSize(2048);
  client.setCallback(callback_mqtt);

  // Loop until we're reconnected
  String clientId = "ESP8266Client";
  clientId += String(random(0xffff), HEX);
  espClient.setInsecure();

  if (client.connect(clientId.c_str(), mqtt_user.c_str(), mqtt_pass.c_str())) {
    Serial.println("Connectet");
    // Once connected, publish an announcement...
    client.publish(mqttTopicZustand.c_str(), String(zustand).c_str());
    // ... and resubscribe
    client.subscribe(mqttTopicPumpe.c_str());
    client.subscribe(mqttTopicSteuerung.c_str());
  } else {
    // Wait 5 seconds before retrying
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
  }
}