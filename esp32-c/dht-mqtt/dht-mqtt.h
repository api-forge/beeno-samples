// Copyright 2021 API FORGE S.R.L.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "config.h"

void messageReceived(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");

    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }

    Serial.println();
}

WiFiClientSecure netClient;
PubSubClient mqttClient(BEENO_HOST, BEENO_PORT, messageReceived, netClient);

void connectWifi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    Serial.print("WiFi connecting to: ");
    Serial.print(ssid);
    Serial.print("...");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }

    Serial.println();
    Serial.print("WiFi connected to: ");
    Serial.print(ssid);
    Serial.println();
    Serial.print("IP address: ");
    Serial.print(WiFi.localIP());
    Serial.println();

    netClient.setCACert(root_ca_cert);
}

void connectMqtt() {
    Serial.print("Connecting to Beeno...");

    String clientId = "Beeno-device-";
    clientId += String(random(0xffffffff), HEX);

    while(!mqttClient.connect(clientId.c_str(), "unused", device_token)) {
        Serial.print(".");
        delay(1000);
    }

    Serial.println();
    Serial.print("Connected to Beeno");
    Serial.println();
}
