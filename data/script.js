function datenSenden(){

    //Mqtt Daten
    var mqttBroker = document.getElementById("mqttBroker").value;
    var changeMqttBroker = document.getElementById("changeMqttBroker").checked;
    var mqttPort = document.getElementById("mqttPort").value;
    var changeMqttPort = document.getElementById("changeMqttPort").checked;
    var mqttBenutzername = document.getElementById("mqttBenutzername").value;
    var changeMqttBenutzername = document.getElementById("changeBenutzername").checked;
    var mqttPasswort = document.getElementById("mqttPasswort").value;
    var changeMqttPasswort = document.getElementById("changePasswort").checked;

    //Mqtt Topics
    var mqttTopicPumpe = document.getElementById("mqttHeartBeat").value;
    var changeMqttTopicPumpe = document.getElementById("changeMqttHearbeat").checked;
    var mqttTopicSteuerung = document.getElementById("mqttSteuerung").value;
    var changeMqttTopicSteuerung = document.getElementById("changeMqttSteuerung").checked;
    var mqttTopicZustand = document.getElementById("mqttZustand").value;
    var changeMqttTopicZustand = document.getElementById("changeMqttZustand").checked;
    
    //Wlan Daten
    var ssid = document.getElementById("ssid").value;
    var changeSsid = document.getElementById("changeSsid").checked;
    var passwortWlan = document.getElementById("passwortWlan").value;
    var changePasswortWlan = document.getElementById("changePasswortWlan").checked;



    var data = {
        "mqttBroker": mqttBroker,
        "changeMqttBroker": changeMqttBroker,
        "mqttPort": mqttPort,
        "changeMqttPort": changeMqttPort,
        "mqttBenutzername": mqttBenutzername,
        "changeMqttBenutzername": changeMqttBenutzername,
        "mqttPasswort": mqttPasswort,
        "changeMqttPasswort": changeMqttPasswort,
        "mqttTopicPumpe": mqttTopicPumpe,
        "changeMqttTopicPumpe": changeMqttTopicPumpe,
        "mqttTopicSteuerung": mqttTopicSteuerung,
        "changeMqttTopicSteuerung": changeMqttTopicSteuerung,
        "mqttTopicZustand": mqttTopicZustand,
        "changeMqttTopicZustand": changeMqttTopicZustand,
        "ssid": ssid,
        "changeSsid": changeSsid,
        "passwortWlan": passwortWlan,
        "changePasswortWlan": changePasswortWlan
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