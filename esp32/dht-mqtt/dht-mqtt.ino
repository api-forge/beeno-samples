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

#include <ArduinoJson.h>
#include "dht-mqtt.h"

#include <Adafruit_Sensor.h>
#include <DHT.h>

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
    Serial.begin(115200);

    dht.begin();

    connectWifi();
    connectMqtt();

    mqttClient.subscribe("commands");
}

unsigned long lastMillis = 0;

void loop() {
    mqttClient.loop();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Disconnected from WiFi");
        connectWifi();
    }

    if (!mqttClient.connected()) {
        Serial.println("Disconnected from Beeno");
        connectMqtt();
    }

    if (millis() - lastMillis > publish_cycle) {
        lastMillis = millis();

        // Create the JSON payload
        StaticJsonDocument<MQTT_MAX_PACKET_SIZE> doc;
        JsonArray timeseries = doc.createNestedArray("timeseries");

        float temperature = dht.readTemperature();
        if (!isnan(temperature)) {
            JsonObject tempObj = timeseries.createNestedObject();

            tempObj["sensorId"] = "temperature";
            tempObj["points"][0]["value"] = temperature;
        }

        float humidity = dht.readHumidity();
        if (!isnan(humidity)) {
            JsonObject tempObj = timeseries.createNestedObject();

            tempObj["sensorId"] = "humidity";
            tempObj["points"][0]["value"] = humidity;
        }

        // Only publish if sensors generated data
        if (timeseries.size() != 0) {
            String timeseriesString;
            serializeJson(doc, timeseriesString);

            mqttClient.publish("timeseries", timeseriesString.c_str());
        }
    }
}
