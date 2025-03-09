#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <vector>

class ConfigWebserver {
    public:
        JsonDocument config;
        String errorMessage;
        int outputPinStatus = 0;
        int inputPinStatus = 0;

        ConfigWebserver(fs::FS* fs, String websitePasswort, String websiteUsername) : dateisystem(fs){
            this->username = websiteUsername;
            this->password = websitePasswort;
            pServer = new AsyncWebServer(80);
            loadConfig();
            SERVER();


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
        String buffer = "";
        void SERVER();
        void neueDatenVerarbeiten(JsonDocument neueDaten);
        void loadConfig();
        void saveConfig();

};