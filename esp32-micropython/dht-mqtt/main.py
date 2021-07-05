from machine import Pin
from time import sleep
from dht import DHT22
from os import urandom
from umqtt.robust import MQTTClient

import network
import ujson

# WiFi settings
WIFI_SSID = "YOUR_WIFI_SSID"
WIFI_PASSWORD = "YOUR_WIFI_PASSWORD"

# Beeno settings
BEENO_HOST = "api.beeno.it"
BEENO_PORT = 8883

# Device settings
DEVICE_TOKEN = "YOUR_DEVICE_SECURITY_TOKEN"
DEVICE_PUBLISH_CYCLE = 5

# DHT sensor
dht = DHT22(Pin(27));

wifi = network.WLAN(network.STA_IF)
wifi.active(True)
print("WiFi connecting to:", WIFI_SSID, "...")
wifi.connect(WIFI_SSID, WIFI_PASSWORD)

while not wifi.isconnected():
    sleep(1)

print("WiFi connected to: ", WIFI_SSID)

randomNum = int.from_bytes(urandom(3), 'little')
CLIENT_ID = bytes("client_" + str(randomNum), 'utf-8')

def sub_cb(topic, payload):
  print("[Received message] Topic: " + str(topic, 'utf-8') + " | Message: " + str(payload, 'utf-8'))

mqttClient = MQTTClient(client_id=CLIENT_ID,
                    server=BEENO_HOST,
                    port=BEENO_PORT,
                    user="unused",
                    password=DEVICE_TOKEN,
                    ssl=True)

try:
    mqttClient.set_callback(sub_cb)
    mqttClient.connect()
except Exception as e:
    print(e)

while True:
    # This actually reads the data and caches the bytes in a buffer
    dht.measure()

    temperature = dht.temperature()
    humidity = dht.humidity()

    payload = {
        "timeseries": [
            {
                "sensorId": "temperature",
                "points": [
                    {
                        "value": temperature
                    }
                ]
            },
            {
                "sensorId": "humidity",
                "points": [
                    {
                        "value": humidity
                    }
                ]
            }
        ]
    }

    print("Payload:" + ujson.dumps(payload))

    try:
        mqttClient.subscribe("commands")
        mqttClient.publish("timeseries", ujson.dumps(payload))
    except KeyboardInterrupt:
            print('Ctrl-C pressed...exiting gracefully')
            mqttClient.disconnect()
    except Exception as e:
        print(e)

    sleep(DEVICE_PUBLISH_CYCLE)
