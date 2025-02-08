import paho.mqtt.client as mqtt
from datetime import datetime

# Define broker and topic
broker = "test.mosquitto.org"
port = 1883
topic = "iotready/gprs"

# Callback when a message is received
def on_message(client, userdata, msg):
    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S:%f')
    print(f"[{timestamp}] Received message: {msg.payload.decode()} on topic {msg.topic}")

# Create the MQTT client
client = mqtt.Client()

# Attach the message callback
client.on_message = on_message

# Connect to the broker
client.connect(broker, port, 60)

# Subscribe to the topic
client.subscribe(topic)

# Start the loop to process incoming messages
try:
    print(f"Listening to topic: {topic}")
    client.loop_forever()
except KeyboardInterrupt:
    print("Subscriber stopped.")
finally:
    client.disconnect()