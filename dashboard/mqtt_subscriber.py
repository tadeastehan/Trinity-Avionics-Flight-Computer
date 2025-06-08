import paho.mqtt.client as mqtt
from datetime import datetime
import re

# Define broker and topic
broker = "broker.mqtt.cool"
port = 1883
topic = "iotready/gprs"

# Function to parse the MQTT payload string into a Python dictionary
def parse_mqtt_payload(payload):
    pattern = (
        r"navSystem:(?P<navSystem>\d+),"
        r"numSatellites:(?P<numSatellites>\d+),"
        r"hdop:(?P<hdop>\d+),"
        r"valid:(?P<valid>true|false),"
        r"latitude:(?P<latitude>-?\d+\.\d+),"
        r"longitude:(?P<longitude>-?\d+\.\d+),"
        r"altitude:(?P<altitude>-?\d+\.\d+),"
        r"date:(?P<year>\d{4})-(?P<month>\d{2})-(?P<day>\d{2}),"
        r"time:(?P<hour>\d{2}):(?P<minute>\d{2}):(?P<second>\d{2})\.(?P<hundredths>\d{2}),"
        r"speed:(?P<speed>-?\d+\.\d+),"
        r"course:(?P<course>-?\d+\.\d+)"
    )
    match = re.match(pattern, payload)
    if not match:
        return None
    d = match.groupdict()
    # Convert types
    d['navSystem'] = int(d['navSystem'])
    d['numSatellites'] = int(d['numSatellites'])
    d['hdop'] = int(d['hdop'])/ 10
    d['valid'] = d['valid'] == 'true'
    d['latitude'] = float(d['latitude'])
    d['longitude'] = float(d['longitude'])
    d['altitude'] = float(d['altitude'])
    d['year'] = int(d['year'])
    d['month'] = int(d['month'])
    d['day'] = int(d['day'])
    d['hour'] = int(d['hour'])
    d['minute'] = int(d['minute'])
    d['second'] = int(d['second'])
    d['hundredths'] = int(d['hundredths'])
    d['speed'] = float(d['speed'])
    d['course'] = float(d['course'])
    return d

# Callback when a message is received
def on_message(client, userdata, msg):
    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S:%f')
    payload = msg.payload.decode()
    parsed = parse_mqtt_payload(payload)
    print(f"[{timestamp}] Received message: {payload} on topic {msg.topic}")
    if parsed:
        print("Parsed data:")
        for k, v in parsed.items():
            print(f"  {k}: {v}")
    else:
        print("Failed to parse payload!")

# Callback when the client connects to the broker
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker!")
        client.subscribe(topic)
        print(f"Subscribed to topic: {topic}")
    else:
        print(f"Failed to connect, return code {rc}")

# Create the MQTT client
client = mqtt.Client()

# Attach the message and connect callbacks
client.on_message = on_message
client.on_connect = on_connect

# Connect to the broker
client.connect(broker, port, 60)

# Start the loop to process incoming messages
try:
    print(f"Listening to topic: {topic}")
    client.loop_forever()
except KeyboardInterrupt:
    print("Subscriber stopped.")
finally:
    client.disconnect()