#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

class ConfigWebserver {
    public:
        JsonDocument config;

        ConfigWebserver(fs::FS* fs, String websitePasswort, String websiteUsername) : dateisystem(fs){
            this->username = websiteUsername;
            this->password = websitePasswort;
            Serial.println(username);
            Serial.println(password);
            pServer = new AsyncWebServer(80);
            SERVER();
            loadConfig();


        };
        

        ~ConfigWebserver(){
            delete pServer;
        };


    private:
        JsonDocument neueDaten;
        // JsonDocument config;
        // String zustandString;
        // JsonDocument details;
        AsyncWebServer *pServer;
        fs::FS* dateisystem;
        String username;
        String password; 
        void SERVER();
        void neueDatenVerarbeiten(JsonDocument neueDaten);
        void loadConfig();
        void saveConfig();

};