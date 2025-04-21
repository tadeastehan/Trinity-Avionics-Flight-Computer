import paho.mqtt.client as mqtt
import time

# Define broker and topic
broker = "broker.hivemq.com"
port = 1883  # Default MQTT port
topic = "iotready/gprs"

# Create the MQTT client
client = mqtt.Client()

# Connect to the broker
client.connect(broker, port, 60)

# Publish messages
try:
    while True:
        message = "Hello from the rocket!"
        print(f"Publishing: {message}")
        client.publish(topic, message)
        time.sleep(2)  # Publish every 2 seconds
except KeyboardInterrupt:
    print("Publishing stopped.")
finally:
    client.disconnect()
