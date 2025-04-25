#include <configWebserver.h>
#include <ArduinoJson.h>

void ConfigWebserver::SERVER(){
    pServer->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!request->authenticate(this->username.c_str(), this->password.c_str())) {
            return request->requestAuthentication();
        }

        if (!LittleFS.exists("/index.html")) {
            Serial.println("Error: /index.html not found on LittleFS");
            request->send(500, "text/plain", "Internal Server Error: /index.html not found");
            return;
        }

        // // Debugging: Check free heap memory
        // Serial.print("Free heap before sending /index.html: ");
        // Serial.println(ESP.getFreeHeap());

        request->send(LittleFS, "/index.html", String(), false);

        // // Debugging: Check free heap memory after sending
        // Serial.print("Free heap after sending /index.html: ");
        // Serial.println(ESP.getFreeHeap());
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
        request->send(LittleFS, "/favicon.svg", "text/plain");
    });
    
    pServer->on("/daten", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!request->authenticate(this->username.c_str(), this->password.c_str())) return request->requestAuthentication();
        
        DynamicJsonDocument datenSenden(1024); // Corrected to DynamicJsonDocument
        datenSenden["mqttBroker"] = config["mqttBroker"];
        datenSenden["mqttPort"] = config["mqttPort"];
        datenSenden["mqttTopicPumpe"] = config["mqttTopicPumpe"];
        datenSenden["intervalPumpe"] = config["intervalPumpe"];
        datenSenden["mqttTopicSteuerung"] = config["mqttTopicSteuerung"];
        datenSenden["intervalSteuerung"] = config["intervalSteuerung"];
        datenSenden["mqttTopicZustand"] = config["mqttTopicZustand"];
        datenSenden["mqttTopicErrorMessage"] = config["mqttTopicErrorMessage"];
        datenSenden["mqttTopicPinStates"] = config["mqttTopicPinStates"];
        datenSenden["errorMessage"] = errorMessage;
        datenSenden["outputPinStatus"] = outputPinStatus;
        datenSenden["inputPinStatus"] = inputPinStatus;
        datenSenden["ipConfig"] = config["ipConfig"];
        datenSenden["ipAddress"] = config["ipAddress"];
        datenSenden["subnetMask"] = config["subnetMask"];
        datenSenden["gateway"] = config["gateway"];
        datenSenden["ssid"] = config["ssid"];

        String json = "";
        serializeJson(datenSenden, json);
        // // Debugging: Check free heap memory
        // Serial.print("Free heap before sending /daten: ");
        // Serial.println(ESP.getFreeHeap());
        request->send(200, "application/json", json);
    });

    pServer->on("/restart", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!request->authenticate(this->username.c_str(), this->password.c_str())) return request->requestAuthentication();
        request->send(200, "text/plain", "Restarting...");
        Serial.println("Restarting...");
        delay(1000);
        ESP.restart();
    });

    pServer->onRequestBody([this](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total){
        if (!request->authenticate(this->username.c_str(), this->password.c_str())) return request->requestAuthentication();
        
        if(request->url()=="/neueDaten" && request->method() == HTTP_POST){
            request->send(200, "text/plain", "Daten empfangen");
            buffer += String((char*)data);

            if(index + len == total) {
                Serial.println("Daten empfangen");
                Serial.println(request->url());
                Serial.print("Länge: ");
                Serial.println(len);

                DeserializationError error = deserializeJson(neueDaten, buffer);
                if (error) {
                    Serial.print("Deserialization failed: ");
                    Serial.println(error.c_str());
                    request->send(400, "text/plain", "Invalid JSON");
                    buffer = ""; // Clear the buffer
                    return;
                }

                Serial.println(neueDaten.as<String>());
                neueDatenVerarbeiten(neueDaten);
                

                buffer = ""; // Clear the buffer
            }
        }
    });

    pServer->onNotFound([this](AsyncWebServerRequest *request) {
        if (!request->authenticate(this->username.c_str(), this->password.c_str())) return request->requestAuthentication();
        request->send(404, "text/plain", "Not found");
    });

    pServer->begin();
}


void ConfigWebserver::neueDatenVerarbeiten(DynamicJsonDocument daten){
    if (daten.containsKey("benutzername")) {
        File secretFile = dateisystem->open("/json/secret.json", "w");
        if (!secretFile) {
            Serial.println("Failed to open secret file for writing");
            return;
        }

        DynamicJsonDocument secretJson(1024);
        secretJson["usernameWebsite"] = daten["benutzername"];
        secretJson["passwordWebsite"] = daten["neuesPasswort"];

        serializeJson(secretJson, secretFile);
        secretFile.close();
        Serial.println("Secret file updated");
        return;
    }

    for (JsonPair kv : daten.as<JsonObject>()) {
        const char* key = kv.key().c_str();
        Serial.println(key);
        config[key] = daten[key];
    }

    saveConfig();
}


void ConfigWebserver::loadConfig(){
    File configFile = dateisystem->open("/json/config.json", "r");
    if (!configFile){
        Serial.println("Failed to open config file for reading");
        config["mqttBroker"] = "sruetzler.de";
        config["mqttPort"] = 8883;
        config["mqttBenutzername"] = "waermepumpe";
        config["mqttPasswort"] = "";
        config["mqttTopicPumpe"] = "tele/tasmota_test/SENSOR";
        config["intervalPumpe"] = 20000;
        config["mqttTopicSteuerung"] = "waermepumpeTest/enable";
        config["intervalSteuerung"] = 40000;
        config["mqttTopicZustand"] = "waermepumpeTest/state";
        config["ssid"] = "";
        config["passwortWlan"] = "";
        config["ipConfig"] = "dhcp";
        config["ipAddress"] = "";
        config["subnetMask"] = "255.255.255.0";
        config["gateway"] = "192.168.178.1";
        config["mqttTopicErrorMessage"] = "waermepumpeTest/error";
        config["mqttTopicPinStates"] = "waermepumpeTest/pinStates";
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
    Serial.println(config.as<String>());
}

void ConfigWebserver::saveConfig(){
    File configFile = dateisystem->open("/json/config.json", "w");
    if (!configFile){
        Serial.println("Failed to open config file for writing");
        return;
    }

    serializeJson(config, configFile);
    configFile.close();
    Serial.println(config.as<String>());
}