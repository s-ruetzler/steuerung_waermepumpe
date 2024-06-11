var mqttBroker;
var changeMqttBroker;
var mqttPort;
var changeMqttPort;
var mqttBenutzername;
var changeMqttBenutzername;
var mqttPasswort;
var changeMqttPasswort;

//Mqtt Topics
var mqttTopicPumpe;
var changeMqttTopicPumpe;
var mqttTopicSteuerung;
var changeMqttTopicSteuerung;
var mqttTopicZustand;
var changeMqttTopicZustand;

//Wlan Daten
var ssid;
var changeSsid;
var passwortWlan;
var changePasswortWlan;



function datenSenden(){

    //Mqtt Daten

    var data = {
        "mqttBroker": mqttBroker.value,
        "changeMqttBroker": changeMqttBroker.checked,
        "mqttPort": mqttPort.value,
        "changeMqttPort": changeMqttPort.checked,
        "mqttBenutzername": mqttBenutzername.value,
        "changeMqttBenutzername": changeMqttBenutzername.checked,
        "mqttPasswort": mqttPasswort.value,
        "changeMqttPasswort": changeMqttPasswort.checked,
        "mqttTopicPumpe": mqttTopicPumpe.value,
        "changeMqttTopicPumpe": changeMqttTopicPumpe.checked,
        "mqttTopicSteuerung": mqttTopicSteuerung.value,
        "changeMqttTopicSteuerung": changeMqttTopicSteuerung.checked,
        "mqttTopicZustand": mqttTopicZustand.value,
        "changeMqttTopicZustand": changeMqttTopicZustand.checked,
        "ssid": ssid.value,
        "changeSsid": changeSsid.checked,
        "passwortWlan": passwortWlan.value,
        "changePasswortWlan": changePasswortWlan.checked
    };

    var xhr = new XMLHttpRequest();
    xhr.open("POST", "/neueDaten", true);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send(JSON.stringify(data));
    xhr.onreadystatechange = function() {
        if (xhr.readyState == 4 && xhr.status == 200) {
            console.log(xhr.responseText);
        }
    }
    
}

function datenLaden(){

    mqttBroker = document.getElementById("mqttBroker");
    changeMqttBroker = document.getElementById("changeMqttBroker");
    mqttPort = document.getElementById("mqttPort");
    changeMqttPort = document.getElementById("changeMqttPort");
    mqttBenutzername = document.getElementById("mqttBenutzername");
    changeMqttBenutzername = document.getElementById("changeBenutzername");
    mqttPasswort = document.getElementById("mqttPasswort");
    changeMqttPasswort = document.getElementById("changePasswort");

    //Mqtt Topics
    mqttTopicPumpe = document.getElementById("mqttHeartBeat");
    changeMqttTopicPumpe = document.getElementById("changeMqttHearbeat");
    mqttTopicSteuerung = document.getElementById("mqttSteuerung");
    changeMqttTopicSteuerung = document.getElementById("changeMqttSteuerung");
    mqttTopicZustand = document.getElementById("mqttZustand");
    changeMqttTopicZustand = document.getElementById("changeMqttZustand");

    //Wlan Daten
    ssid = document.getElementById("ssid");
    changeSsid = document.getElementById("changeSsid");
    passwortWlan = document.getElementById("passwortWlan");
    changePasswortWlan = document.getElementById("changePasswortWlan");


    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/daten", true);
    xhr.onreadystatechange = function() {
        if (xhr.readyState == 4 && xhr.status == 200) {
            var response = JSON.parse(xhr.responseText);
            mqttBroker.setAttribute("value", response["mqttBroker"]);
            mqttPort.setAttribute("value", response["mqttPort"]);
            mqttTopicPumpe.setAttribute("value", response["mqttTopicPumpe"]);
            mqttTopicSteuerung.setAttribute("value", response["mqttTopicSteuerung"]);
            mqttTopicZustand.setAttribute("value", response["mqttTopicZustand"]);
            
            // Hier kannst du die erhaltenen Daten weiterverarbeiten
            console.log(response);
        }
    }
    xhr.send();
}

window.addEventListener('load', datenLaden);