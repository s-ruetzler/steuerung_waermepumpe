#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h> 
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <configWebserver.h>


unsigned long lastTime = 0;
int timerDelay = 5000;
bool configMode = false;
bool wlanOpen = false;
bool wlanConnected = false;
String ssidWLanSelf;
String passwordWlanSelf;
String usernameWebsite;
String passwordWebsite;

ConfigWebserver *configServer = nullptr;
// AsyncWebServer server(80);

void openWLAN();
void closeWLAN();
void connectToWLAN();
void passwordsIntern();

void setup() {
  Serial.begin(115200);
  delay(2000);

  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  passwordsIntern();

  configServer = new ConfigWebserver(&LittleFS, usernameWebsite, passwordWebsite);

  openWLAN();
  connectToWLAN();


  // SERVER();

  

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

  if (WiFi.status() != WL_CONNECTED && millis() > 10000){
    configMode = true;
  }



  
}


void openWLAN(){
  WiFi.mode(WIFI_AP_STA);
  
  WiFi.softAP(ssidWLanSelf, passwordWlanSelf);

  IPAddress ip = WiFi.softAPIP();
  
  Serial.println("WLAN göffnet");
  wlanOpen = true;
}

void closeWLAN(){
  WiFi.mode(WIFI_STA);
  Serial.println("WLAN geschlossen");
  wlanOpen = false;
}

void connectToWLAN(){
  String wlanSsid = configServer->config["ssid"];
  String wlanPasswort = configServer->config["passwortWlan"];
  WiFi.begin(wlanSsid, wlanPasswort);

  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println("WLAN verbunden");
  // Serial.println(WiFi.localIP());
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
