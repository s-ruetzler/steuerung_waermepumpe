#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <vector>

class ConfigWebserver {
    public:
        DynamicJsonDocument config{1024};
        String errorMessage;
        int outputPinStatus = 0;
        int inputPinStatus = 0;

        ConfigWebserver(fs::FS* fs, String websitePasswort, String websiteUsername) : dateisystem(fs), neueDaten(1024) {
            this->username = websiteUsername;
            this->password = websitePasswort;
            pServer = new AsyncWebServer(80);
            loadConfig();
            SERVER();
        };

        ~ConfigWebserver(){
            if (pServer) {
                delete pServer;
            }
        };

    private:
        DynamicJsonDocument neueDaten;
        AsyncWebServer *pServer;
        fs::FS* dateisystem;
        String username;
        String password; 
        String buffer = "";
        void SERVER();
        void neueDatenVerarbeiten(DynamicJsonDocument neueDaten);
        void loadConfig();
        void saveConfig();
};