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

unsigned long lastTimeMQTT= 0;
unsigned long lastTimePumpe = 0;
unsigned int timeOutPumpe = 0;
unsigned long lastTimeSteuerung = 0;
unsigned long lastTimeChangeZustand = 0;
unsigned int timeOutSteuerung = 0;
unsigned int timerDelay = 3000;
bool configMode = false;
bool wlanOpen = false;
bool wlanConnected = false;
String zustand = "Off"; //Off = 0, On = 1, Wait = 2, Error = 3
int zustandNR = 0; //0 = Off, 1 = On, 2 = Wait, 3 = Error
bool setupOTAaufgerufen = false;
JsonDocument datenPumpe;
int powerPumpe = 0;

const int OUTPUT_PIN = D6;
const int INPUT_PIN = D5;
int zustandKompressorPin = LOW;
int zustandKompressorPinAlt = LOW;
bool heatingEnabled = false;
const int prellzeit = 50;
unsigned long lastMillisPrell = 0;
int intervalPumpe = 20000; //Wie oft, wird von der Poolpumpe neue Daten erwartet
int intervalSteuerung = 40000; // Wie oft, wird von der Steuerung neue Daten erwartet

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
void logikChangeZustand(); //Logik des Programms
bool fehlerVorhanden(); //Fehler überprüfen

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
  configServer->errorMessage = "Keine Fehlermeldung vorhanden";

  setup_wifi_mqtt();


}

void loop() {
  wlanConnected = WiFi.status() == WL_CONNECTED;
  zustandKompressorPin = digitalRead(INPUT_PIN);
  configServer->inputPinStatus = zustandKompressorPin;
  configServer->outputPinStatus = digitalRead(OUTPUT_PIN);

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

  //WLAN Verbindung hergestellt, aber noch keine Verbindung zum MQTT Broker
  if (!client.connected() && wlanConnected) {
    if (millis() - lastTimeMQTT> timerDelay)
    {
      Serial.println("Neu verbinden...");
      reconnect_mqtt();
      lastTimeMQTT= millis();
    }
  }

  //Signal entprellen
  if (zustandKompressorPin != zustandKompressorPinAlt){
    zustandKompressorPinAlt = zustandKompressorPin;
    lastMillisPrell = millis();
  }

  logikChangeZustand();
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

  lastTimeChangeZustand = millis();

  if (zustand == "Error" && newState != "Error")
  {
    configServer->errorMessage = "Keine Fehlermeldung vorhanden";
  }

  zustand = newState;

  if (zustand == "Off"){
    zustandNR = 0;
    heatingEnabled = false;
    digitalWrite(OUTPUT_PIN, LOW);

  } else if (zustand == "On"){
    zustandNR = 1;
    heatingEnabled = true;
    lastTimePumpe = millis();
    digitalWrite(OUTPUT_PIN, HIGH);

  } else if (zustand == "Wait"){
    zustandNR = 2;
    heatingEnabled = true;
    digitalWrite(OUTPUT_PIN, HIGH);

  } else{
    zustandNR = 3;
    digitalWrite(OUTPUT_PIN, LOW);

  }

  Serial.println("Zustand: " + zustand);
  client.publish(mqttTopicZustand.c_str(), zustand.c_str(), true);
  
}

void logikChangeZustand(){


  if (zustand != "Error")
  {
    fehlerVorhanden(); // Es wird einmal überprüft, ob ein Fehler vorhanden ist
  }
  
  switch (zustandNR)
  {
  case 0: // Off
    //Wenn das Heizen aktiviert ist und der Zustand aus ist
    if (heatingEnabled == true)
    {
      // changeZustand("Error");
      Serial.println("Info: Heizen aktiviert, aber Zustand ist aus");
      // configServer->errorMessage = "Heizen aktiviert, aber Zustand ist aus";
    }
    break;

  case 1: // On
    //Wenn der Kompressor sich ausschaltet, dann wird der Zustand auf "Off" gesetzt.
    if (zustandKompressorPin == LOW && millis() - lastMillisPrell > prellzeit)
    {
      changeZustand("Wait");
    }


    //Wenn die Poolpumpe weniger als 10 Watt verbraucht, dann wird der Zustand auf "Off" gesetzt.
    if (powerPumpe <= 10 && millis() - lastTimeChangeZustand > intervalPumpe) //Hier ist noch ien fehler in der Logik
    {
      changeZustand("Off");
      // Serial.println("Eror: Die PoolPumpe geht nicht an!");
      // configServer->errorMessage = "Die Poolpumpe geht nicht an!";
    }

    break;

  case 2: // Wait
    //Wenn der Kompressor sich einschaltet, dann wird der Zustand auf "On" gesetzt.
    if (zustandKompressorPin == HIGH && millis() - lastMillisPrell > prellzeit)
    {
      changeZustand("On");
    }  

    break;

  case 3: // Error

    //Wenn weiterhin ein Fehler vorhanden ist, dann passiert nichts
    if (fehlerVorhanden())
    {
      break;
    }

    // Wenn das Heizen aktiviert ist und die Poolpumpe mehr als 10 Watt verbraucht, dann wird der Zustand auf "Wait" gesetzt.
    // if (heatingEnabled == true && powerPumpe < 10){
    //   break;
    // }

    //Wenn der Code bis hier hin kommt, bedeutet es, dass keine Fehler mehr vorhanden sind.
    //Je nach Statuscode wird dann der Zustand geändert.
    if (heatingEnabled)
    {
      changeZustand("Wait");
    } else{
      changeZustand("Off");
    }

    break;
  
  default:
    break;
  }

 


}

//Hier wird überprüft ob ein Fehler vorhanden ist

bool fehlerVorhanden(){

  //Keine Verbindung zum MQTT Broker oder zum WLAN
  if (!client.connected() || !wlanConnected){
    if (zustand != "Error")
    {
      changeZustand("Error");
      Serial.println("Error: Keine Verbindung zum MQTT Broker oder zum WLAN");
      configServer->errorMessage = "Es konnte keine Verbindung zum MQTT Broker oder zum WLAN hergestellt werden";

    }
    return true;
  }

  //Wenn die Pumpe länger als 20 Sekunden keine Daten sendet
  if (millis() - lastTimePumpe > intervalPumpe || timeOutPumpe > intervalPumpe) {
    if (zustand != "Error")
    {
      changeZustand("Error");
      Serial.println("Error: Pumpe sendet keine Daten");
      configServer->errorMessage = "Die Poolpumpe sendet keine Daten";

    }
    return true;
  }

  //Wenn die Steuerung länger als 40 Sekunden keine sendet
  if (millis() - lastTimeSteuerung > intervalSteuerung || timeOutSteuerung > intervalSteuerung) {
    if (zustand != "Error")
    {
      changeZustand("Error");
      Serial.println("Error: Steuerung sendet keine Daten");
      configServer->errorMessage = "Die Steuerung sendet keine Daten";

    }
    return true;
  }

  return false;
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
    configServer->errorMessage = "Keine Fehlermeldung vorhanden";
    changeZustand("Off");
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
  timeOutPumpe = millis() - lastTimePumpe;
  lastTimePumpe = millis();


  powerPumpe = datenPumpe["StatusSNS"]["ENERGY"]["Power"];
  // Serial.println(value);

  
}

void handlePayloadSteuerung(byte* payload, unsigned int length){
  String payloadString = "";
  for (unsigned int i = 0; i < length; i++) {
    payloadString += (char)payload[i];
  }
  timeOutSteuerung = millis() - lastTimeSteuerung;
  lastTimeSteuerung = millis();
  
  if (zustand == "Error"){
    if (payloadString == "true"){
      heatingEnabled = true;
    } else{
      heatingEnabled = false;
    }

  }else if (payloadString == "true")
  {
    if (zustand != "Wait" && zustand != "On")
    {
      changeZustand("Wait");
    }

  } else{
    if (zustand != "Off")
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