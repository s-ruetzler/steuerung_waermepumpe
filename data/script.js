function datenSenden(){
    var mqttBroker = document.getElementById("mqttBroker").value;
    var changeMqttBroker = document.getElementById("changeMqttBroker").checked;
    var mqttPort = document.getElementById("mqttPort").value;
    var changeMqttPort = document.getElementById("changeMqttPort").checked;
    var mqttBenutzername = document.getElementById("mqttBenutzername").value;
    var changeMqttBenutzername = document.getElementById("changeBenutzername").checked;
    var mqttPasswort = document.getElementById("mqttPasswort").value;
    var changeMqttPasswort = document.getElementById("changePasswort").checked;
    var mqttTopic = document.getElementById("mqttTopic").value;
    var changeMqttTopic = document.getElementById("changeMqttTopic").checked;
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
        "mqttTopic": mqttTopic,
        "changeMqttTopic": changeMqttTopic,
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