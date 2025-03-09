// const { stat } = require("fs");

//Mqtt Daten
var mqttBroker;
var mqttPort;
var mqttBenutzername;
var mqttPasswort;

//Mqtt Topics
var mqttTopicPumpe;
var intervalPumpe;
var mqttTopicSteuerung;
var intervalSeuerung;
var mqttTopicZustand;

//Wlan Daten
var ssid;
var passwortWlan;
var radioDCHP;
var radioSTATIC;
var staticIpConfig;
var ipAddress;
var subnetMask;
var gateway;

//Website Daten
var benutzernameWenbsite;
var passwortWebsite;
var widerholenPasswortWebsite;

function datenSenden(formId) {
    const form = document.getElementById(formId);
    const formData = new FormData(form);
    const data = {};

    // Überprüfen, ob alle Felder ausgefüllt sind
    for (let [key, value] of formData.entries()) {
        if (!value) {
            if (formId === 'form-wlan' && document.getElementById('dhcp').checked && (key === 'ipAddress' || key === 'subnetMask' || key === 'gateway')) {
                continue; // Überspringe die Überprüfung dieser Felder, wenn DHCP ausgewählt ist
            }
            alert(`Bitte füllen Sie das Feld ${key} aus.`);
            return;
        }
    }
    
    // Überprüfen, ob das Passwort-Wiederholen-Feld mit dem neuen Passwort übereinstimmt
    if (formId === 'form-benutzer') {
        const neuesPasswort = formData.get('neuesPasswort');
        const passwortWiederholen = formData.get('passwortWiederholen');
        if (neuesPasswort !== passwortWiederholen) {
            alert('Die Passwörter stimmen nicht überein.');
            return;
        }
    }

    formData.forEach((value, key) => {
        data[key] = value;
    });

    console.log(data);

    var xhr = new XMLHttpRequest();
    xhr.open("POST", "/neueDaten", true);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send(JSON.stringify(data));
    xhr.onreadystatechange = function() {
        if (xhr.readyState == 4 && xhr.status == 200) {
            console.log(xhr.responseText);
            alert('Daten wurden erfolgreich gespeichert. Um diese Änderungen zu übernehmen, starten Sie das System neu.');
        }
    }
}

function datenLaden() {
    mqttBroker = document.getElementById("mqttBroker");
    mqttPort = document.getElementById("mqttPort");
    mqttBenutzername = document.getElementById("mqttBenutzername");
    mqttPasswort = document.getElementById("mqttPasswort");

    //Mqtt Topics
    mqttTopicPumpe = document.getElementById("mqttTopicPumpe");
    mqttTopicSteuerung = document.getElementById("mqttTopicSteuerung");
    mqttTopicZustand = document.getElementById("mqttTopicZustand");

    intervalPumpe = document.getElementById("intervalPumpe");
    intervalSeuerung = document.getElementById("intervalSteuerung");

    //Wlan Daten
    ssid = document.getElementById("ssid");
    passwortWlan = document.getElementById("passwortWlan");
    radioDCHP = document.getElementById('dhcp')
    radioDCHP.addEventListener('change', toggleIpConfig);
    radioSTATIC = document.getElementById('staticIp')
    radioSTATIC.addEventListener('change', toggleIpConfig);
    ipAddress = document.getElementById("ipAddress");
    subnetMask = document.getElementById("subnetMask");
    gateway = document.getElementById("gateway");

    //Website Daten
    benutzernameWenbsite = document.getElementById("benutzernameWebsite");
    passwortWebsite = document.getElementById("passwortWebsite");
    widerholenPasswortWebsite = document.getElementById("wiederholenPasswortWebsite");
   
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/daten", true);
    xhr.onreadystatechange = function() {
        if (xhr.readyState == 4 && xhr.status == 200) {
            var response = JSON.parse(xhr.responseText);
            mqttBroker.setAttribute("value", response["mqttBroker"]);
            mqttPort.setAttribute("value", response["mqttPort"]);
            mqttTopicPumpe.setAttribute("value", response["mqttTopicPumpe"]);
            intervalPumpe.setAttribute("value", response["intervalPumpe"]);
            mqttTopicSteuerung.setAttribute("value", response["mqttTopicSteuerung"]);
            intervalSeuerung.setAttribute("value", response["intervalSeuerung"]);
            mqttTopicZustand.setAttribute("value", response["mqttTopicZustand"]);
            updateErrorMessage(response["errorMessage"]);
            updatePinStatus(response["outputPinStatus"], response["inputPinStatus"]);
            ipAddress.setAttribute("value", response["ipAddress"]);
            subnetMask.setAttribute("value", response["subnetMask"]);
            gateway.setAttribute("value", response["gateway"]);
            ssid.setAttribute("value", response["ssid"]);

            if (response["ipConfig"] === "dhcp") {
                radioDCHP.checked = true;
                toggleIpConfig();
            }
            else {
                radioSTATIC.checked = true;
                toggleIpConfig();
            }
            
            // Hier kannst du die erhaltenen Daten weiterverarbeiten
            console.log(response);
        }
    }
    xhr.send();
}

function toggleIpConfig() {
    const staticIpConfig = document.getElementById('staticIpConfig');
    if (document.getElementById('staticIp').checked) {
        staticIpConfig.style.display = 'block';
    } else {
        staticIpConfig.style.display = 'none';
    }
}

function updateErrorMessage(message) {
    const errorMessagesDiv = document.getElementById('error-messages');
    errorMessagesDiv.innerHTML = ''; // Vorherige Fehlermeldungen löschen
    const p = document.createElement('p');
    p.textContent = message;
    errorMessagesDiv.appendChild(p);
}

// Beispielaufruf
// updateErrorMessages(['Fehler 1: Verbindung verloren', 'Fehler 2: Sensor nicht gefunden']);

function updatePinStatus(outputStatus, inputStatus) {
    document.getElementById('output-pin-status').textContent = outputStatus;
    document.getElementById('input-pin-status').textContent = inputStatus;
}

// Beispielaufruf
// updatePinStatus('HIGH', 'LOW');

function togglePasswordVisibility(fieldId) {
    const field = document.getElementById(fieldId);
    if (field.type === 'password') {
        field.type = 'text';
    } else {
        field.type = 'password';
    }
}

function confirmRestart() {
    if (confirm('Sind Sie sicher, dass Sie das System neu starten möchten?')) {
        var xhr = new XMLHttpRequest();
        xhr.open("POST", "/restart", true);
        xhr.setRequestHeader('Content-Type', 'application/json');
        xhr.send();
        xhr.onreadystatechange = function() {
            if (xhr.readyState == 4 && xhr.status == 200) {
                alert('Das System wird neu gestartet.');
            }
        }
    }
}

window.addEventListener('load', datenLaden);