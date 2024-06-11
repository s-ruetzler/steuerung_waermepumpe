#include <configWebserver.h>

void ConfigWebserver::SERVER(){
    pServer->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!request->authenticate(this->username.c_str(), this->password.c_str())) return request->requestAuthentication();
        // Serial.println("Seite aufgerufen");
        request->send(LittleFS, "/index.html", String(), false);
    });

    pServer->on("/style.css", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!request->authenticate(this->username.c_str(), this->password.c_str())) return request->requestAuthentication();
        request->send(LittleFS, "/style.css", "text/css");
    });

    pServer->on("/script.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!request->authenticate(this->username.c_str(), this->password.c_str())) return request->requestAuthentication();
        request->send(LittleFS, "/script.js", "text/javascript");
    });

    pServer->on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/favicon.ico", "text/plain");
    });
    
    pServer->on("/daten", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!request->authenticate(this->username.c_str(), this->password.c_str())) return request->requestAuthentication();
        // loadConfig();
        JsonDocument datenSenden;
        datenSenden["mqttBroker"] = config["mqttBroker"];
        datenSenden["mqttPort"] = config["mqttPort"];
        datenSenden["mqttTopicPumpe"] = config["mqttTopicPumpe"];
        datenSenden["mqttTopicSteuerung"] = config["mqttTopicSteuerung"];
        datenSenden["mqttTopicZustand"] = config["mqttTopicZustand"];


        String json = "";
        serializeJson(datenSenden, json);
        request->send(200, "application/json", json);

        
    });

    pServer->onRequestBody([this](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total){
        if (!request->authenticate(this->username.c_str(), this->password.c_str())) return request->requestAuthentication();
        
        if(request->url()=="/neueDaten" && request->method() == HTTP_POST){
    
            buffer += String((char*)data);

            if(index + len == total) {
                Serial.println("Daten empfangen");
                Serial.println(request->url());
                Serial.print("Länge: ");
                Serial.println(len);

                // Serial.println(buffer);

                deserializeJson(neueDaten, buffer);
                // Serial.println(neueDaten.as<String>());
                neueDatenVerarbeiten(neueDaten);
                request->send(200, "text/plain", "Daten empfangen");

                // Clear the buffer
                buffer = "";
                }
        
        }


            // if(request->url()=="/neueDaten" && request->method() == HTTP_POST){
            //     // Serial.println("Daten empfangen");
            //     // Serial.println((char*)data);
            //     deserializeJson(neueDaten, (char*)data);
            //     // Serial.println(neueDaten.as<String>());
            //     neueDatenVerarbeiten(neueDaten);
            //     request->send(200, "text/plain", "Daten empfangen");
            // }
    });

    pServer->onNotFound([this](AsyncWebServerRequest *request) {
        if (!request->authenticate(this->username.c_str(), this->password.c_str())) return request->requestAuthentication();
        request->send(404, "text/plain", "Not found");
    });

    pServer->begin();
}


void ConfigWebserver::neueDatenVerarbeiten(JsonDocument daten){
    // Serial.println(daten.as<String>());

    if(daten["changeMqttBroker"]){
        config["mqttBroker"] = daten["mqttBroker"];
    }
    if(daten["changeMqttPort"]){
        config["mqttPort"] = daten["mqttPort"];
    }
    if(daten["changeMqttBenutzername"]){
        config["mqttBenutzername"] = daten["mqttBenutzername"];
    }
    if(daten["changeMqttPasswort"]){
        config["mqttPasswort"] = daten["mqttPasswort"];
    }
    if(daten["changeMqttTopicPumpe"]){
        config["mqttTopicPumpe"] = daten["mqttTopicPumpe"];
    }
    if(daten["changeMqttTopicSteuerung"]){
        config["mqttTopicSteuerung"] = daten["mqttTopicSteuerung"];
    }
    if(daten["changeMqttTopicZustand"]){
        config["mqttTopicZustand"] = daten["mqttTopicZustand"];
    }
    if(daten["changeSsid"]){
        config["ssid"] = daten["ssid"];
    }
    if(daten["changePasswortWlan"]){
        config["passwortWlan"] = daten["passwortWlan"];
    }


    saveConfig();
    
}


void ConfigWebserver::loadConfig(){
    File configFile = dateisystem->open("/json/config.json", "r");
    if (!configFile){
        Serial.println("Failed to open config file for reading");
        config["mqttBroker"] = "sruetzler.de";
        config["mqttPort"] = 8883;
        config["mqttBenutzername"] = "";
        config["mqttPasswort"] = "";
        config["mqttTopicPumpe"] = "tele/tasmota_3094E0/SENSOR";
        config["mqttTopicSteuerung"] = "waermepumpe/steuerung";
        config["mqttTopicZustand"] = "waermepumpe/zustand";
        config["ssid"] = "";
        config["passwortWlan"] = "";
        saveConfig();

        return;
    }

    size_t size = configFile.size();
    if (size > 1024){
        Serial.println("Config file size is too large");
        return;
    }

    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);
    configFile.close();

    DeserializationError error = deserializeJson(config, buf.get());
    // DeserializationError error = deserializeJson(config, configFile);
    if (error){
        Serial.println("Failed to parse config file");
       

        return;
    }
    // Serial.println(config.as<String>());
}

void ConfigWebserver::saveConfig(){
    File configFile = dateisystem->open("/json/config.json", "w");
    if (!configFile){
        Serial.println("Failed to open config file for writing");
        return;
    }

    serializeJson(config, configFile);
    configFile.close();
    // Serial.println(config.as<String>());
}