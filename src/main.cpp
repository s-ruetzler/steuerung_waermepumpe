#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h> 
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <configWebserver.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

String mqtt_user;
String mqtt_pass;
String mqtt_server;
int mqtt_port;
String mqttTopicPumpe;
String mqttTopicSteuerung;
String mqttTopicZustand;

unsigned long lastTime = 0;
unsigned long lastTimePumpe = 0;
unsigned int timerDelay = 3000;
bool configMode = false;
bool wlanOpen = false;
bool wlanConnected = false;
String zustand = "Off";
bool setupOTAaufgerufen = false;
JsonDocument datenPumpe;

const int OUTPUT_PIN = D6;
const int INPUT_PIN = D5;
int zustandKompressorPin = LOW;
int zustandKompressorPinAlt = LOW;
int steuerungZustand = 0;
const int prellzeit = 50;
unsigned int lastMillisPrell = 0;

String ssidWLanSelf;
String passwordWlanSelf;
String usernameWebsite;
String passwordWebsite;

ConfigWebserver *configServer = nullptr;

WiFiClientSecure espClient;
PubSubClient client(espClient);

// AsyncWebServer server(80);

void openWLAN(); //Das eigene WLAN wird erstellt
void closeWLAN(); // Das eigene WLAN wird geschlossen
void setup_wifi_mqtt(); //WLAN und MQTT einstellen
void readSecretFile(); //Passwörter auslesen
void changeZustand(String newState); //Zustand des ESPs ändern
void callback_mqtt(char* topic, byte* payload, unsigned int length); //Callback Funktion für MQTT
void reconnect_mqtt(); //Mit MQTT Broker neu verbinden
void handlePayloadPumpe(byte* payload, unsigned int length); //Payload von Pumpe verarbeiten
void handlePayloadSteuerung(byte* payload, unsigned int length); //Payload von Steuerung verarbeiten
void setupOTA(); // OTA einstellungen einrichten

void setup() {
  Serial.begin(115200);
  pinMode(OUTPUT_PIN, OUTPUT);
  pinMode(INPUT_PIN, INPUT);
  delay(2000);

  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  readSecretFile();

  configServer = new ConfigWebserver(&LittleFS, usernameWebsite, passwordWebsite);

  setup_wifi_mqtt();


}

void loop() {
  wlanConnected = WiFi.status() == WL_CONNECTED;
  zustandKompressorPin = digitalRead(INPUT_PIN);
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

  if (!setupOTAaufgerufen && wlanConnected){
    setupOTA();
    setupOTAaufgerufen = true;
  }

  //Wenn inerhalb von 20 Sekunden keine WLAN verbindung hergestellt werden kann, dann wird der ConfigMode gestartet.
  if ((!wlanConnected) && millis() > 20000){
    configMode = true;
  }

  //Wenn eine WLAN verbindung hergestellt wurde und MQTT nicht verbunden ist, dann versucht er sich mit dem MQTT Broker zu verbinden.
  if (!client.connected() && wlanConnected) {
    if (millis() - lastTime > timerDelay)
    {
      Serial.println("Neu verbinden...");
      reconnect_mqtt();
      lastTime = millis();
    }
  }

  //Wenn die Pumpe länger als 20 Sekunden nichts sendet, dann wird der Zustand auf 0 gesetzt.
  if (millis() - lastTimePumpe > 20000)
  {
    steuerungZustand = 0;
    if (zustand != "Error")
    {
      changeZustand("Error");
    }
  }

  if (zustandKompressorPin != zustandKompressorPinAlt){
    zustandKompressorPinAlt = zustandKompressorPin;
    lastMillisPrell = millis();
  }

  if (zustand == "Wait" && zustandKompressorPin == HIGH && millis() - lastMillisPrell > prellzeit)
  {
    changeZustand("On");
  }

  if (zustand == "On" && zustandKompressorPin == LOW && millis() - lastMillisPrell > prellzeit)
  {
    changeZustand("Wait");
  }

  if (steuerungZustand == 1 && zustand == "Off")
  {
    changeZustand("Error");
  }
  client.loop();
  ArduinoOTA.handle();
}


void openWLAN(){
  WiFi.mode(WIFI_AP);
  
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

  WiFi.mode(WIFI_STA);
  WiFi.begin(wlanSsid, wlanPasswort);
  delay(2000);

}

void readSecretFile(){
  File file = LittleFS.open("/json/secret.json", "r");
  if (!file) {
    Serial.println("Datei konnte nicht geöffnet werden");
    return;
  }

  size_t size = file.size();
  std::unique_ptr<char[]> buf(new char[size]);
  file.readBytes(buf.get(), size);
  file.close();

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, buf.get());
  if (error) {
    Serial.println("Fehler beim Parsen der JSON Datei");
    return;
  }

  ssidWLanSelf = doc["ssidWLanSelf"].as<String>();
  passwordWlanSelf = doc["passwordWlanSelf"].as<String>();
  usernameWebsite = doc["usernameWebsite"].as<String>();
  passwordWebsite = doc["passwordWebsite"].as<String>();
}

void changeZustand(String newState){

  zustand = newState;

  if (zustand == "Off"){
    digitalWrite(OUTPUT_PIN, LOW);

  } else if (zustand == "On"){
    digitalWrite(OUTPUT_PIN, HIGH);

  } else if (zustand == "Wait"){
    digitalWrite(OUTPUT_PIN, HIGH);

  } else if (zustand == "Error"){
    digitalWrite(OUTPUT_PIN, LOW);

  } else{
    digitalWrite(OUTPUT_PIN, LOW);

  }

  // Serial.println("Zustand: " + zustand);
  client.publish(mqttTopicZustand.c_str(), zustand.c_str(), true);
  
}


void callback_mqtt(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");

  String topicString = String(topic);
  if (topicString == mqttTopicPumpe) {
    // Pass the payload to a function for further processing
    handlePayloadPumpe(payload, length);
  } else if (topicString == mqttTopicSteuerung) {
    handlePayloadSteuerung(payload, length);
  } else {
    Serial.print("Unknown topic: ");
    Serial.println(topicString);
  }

}


void reconnect_mqtt() {

  client.setServer(mqtt_server.c_str(), mqtt_port);
  client.setBufferSize(2048);
  client.setCallback(callback_mqtt);

  // Loop until we're reconnected
  String clientId = "ESP8266Client";
  clientId += String(random(0xffff), HEX);
  espClient.setInsecure();

  if (client.connect(clientId.c_str(), mqtt_user.c_str(), mqtt_pass.c_str())) {
    Serial.println("Connectet");
    // Once connected, publish an announcement...
    client.publish(mqttTopicZustand.c_str(), String(zustand).c_str(), true);
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

void handlePayloadPumpe(byte* payload, unsigned int length){
  DeserializationError error = deserializeJson(datenPumpe, payload, length);
  if (error) {
    Serial.print("Failed to parse JSON payload: ");
    Serial.println(error.c_str());
    changeZustand("Off");
    return;
  }
  lastTimePumpe = millis();


  int value = datenPumpe["StatusSNS"]["ENERGY"]["Power"];
  // Serial.println(value);

  if (value <= 10 && zustand == "On")
  {
    changeZustand("Off");
  } else if (zustand == "Error" && steuerungZustand == 1 && value > 10){
    changeZustand("Wait");
  }

}

void handlePayloadSteuerung(byte* payload, unsigned int length){


  String payloadString = "";
  for (unsigned int i = 0; i < length; i++) {
    payloadString += (char)payload[i];
  }
  
  if (zustand == "Error"){
    if (payloadString == "true"){
      steuerungZustand = 1;
    } else{
      steuerungZustand = 0;
    }

  }else if (payloadString == "true")
  {
    steuerungZustand = 1;
    if (zustand =! "Wait")
    {
      changeZustand("Wait");
    }

  } else{
    steuerungZustand = 0;
    if (zustand =! "Off")
    {
      changeZustand("Off");
    }
  }
  
  // Serial.println(payloadString);

}


void setupOTA(){
  // ArduinoOTA.setHostname("ESP8266");
  // ArduinoOTA.setPassword("admin");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Starte OTA Update: " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnde");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Fehler[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Authentifizierung fehlgeschlagen");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Fehler beim Starten");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Fehler beim Verbinden");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Fehler beim Empfangen");
    } else if (error == OTA_END_ERROR) {
      Serial.println("Fehler beim Ende");
    }
  });

  ArduinoOTA.begin();
  Serial.println("OTA bereit");

}