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

const mqtt = require('mqtt');
const sensor = require ('node-dht-sensor');

// The default value for publishing cycles
const DEFAULT_CYCLE_TIME = 5000 * 1; // 1s

// The initial backoff time after a disconnection occurs, in seconds.
const MINIMUM_BACKOFF_TIME = 1000; // 1s

// The maximum backoff time before giving up, in seconds.
const MAXIMUM_BACKOFF_TIME = 30000; // 30s

// The connection URL to Beeno
const API_URL = 'mqtts://api.beeno.it';

// The device key to authenticate the device with Beeno
const DEVICE_SECURITY_KEY = '...';

// Whether to wait with exponential backoff before publishing.
let shouldBackoff = false;

// The current backoff time.
let backoffTime = 1000;

const client  = mqtt.connect(API_URL, {
    username: 'unused',
    password: DEVICE_SECURITY_KEY,
    qos: 1
});

async function loopPublish() {
    // If we backed off too many times, stop.
    if (backoffTime >= MAXIMUM_BACKOFF_TIME) {
        console.log('Backoff time is too high. Closing connection.');
        client.end();
        return;
    }

    // Publish and schedule the next publish.
    let publishDelay = DEFAULT_CYCLE_TIME;

    if (shouldBackoff) {
        publishDelay = 1000 * (backoffTime + Math.random());
        backoffTime *= 2;
        console.log(`Backing off for ${publishDelay}ms before publishing.`);
    }

    const sensorData = await sensor.read(22, 4);
    const readTime = new Date();
    const payload = JSON.stringify({
        timeseries: [
            {
                sensorId: 'temperature',
                points: [
                    {
                        // `time` is optional
                        time: readTime,
                        value: sensorData.temperature
                    }
                ],
            },
            {
                sensorId: 'humidity',
                points: [
                    {
                        // `time` is optional
                        time: readTime,
                        value: sensorData.humidity,
                    }
                ],
            },
        ]
    });

    // Publish "payload" to Beeno. qos=1 means at least once delivery.
    // Beeno also supports qos=0 for at most once delivery.
    client.publish('timeseries', payload, { qos: 1 }, error => {
        if (!error) {
            shouldBackoff = false;
            backoffTime = MINIMUM_BACKOFF_TIME;
        }

        setTimeout(loopPublish, publishDelay);
    });
}

client.on('connect', (success) => {
    if (!success) {
        console.log('Client not connected');
        return;
    }

    loopPublish();
    console.log('Device monitoring started');
});

client.on('close', () => {
    console.log('Client connection closed');

    // We assume server closed the connection and
    // we should start applying the backoff strategy
    shouldBackoff = true;
});

client.on('error', error => {
    console.log('Client error', error);
});

client.on('message', function (topic, message) {
    console.log('Client received command', Buffer.from(message).toString('ascii'));
});

client.on('packetsend', (packet) => {
    // Log the sent data to make sure everything is working as expected
    console.log('Updated sensor data:', packet.payload);
});


