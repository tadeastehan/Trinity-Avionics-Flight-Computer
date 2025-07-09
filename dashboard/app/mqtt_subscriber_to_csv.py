import os
import csv
import paho.mqtt.client as mqtt
from datetime import datetime, timezone
import re

# MQTT broker settings
broker = "broker.mqtt.cool"
port = 1883
topic = "iotready/gprs"

# Output CSV file (in the same data folder as app.py expects)
DATA_DIR = os.path.join(os.path.dirname(__file__), 'data')
os.makedirs(DATA_DIR, exist_ok=True)

def get_new_flight_filename():
    year = datetime.now(timezone.utc).year
    base = f"{year}_flight_"
    n = 1
    while True:
        fname = f"{base}{n}.csv"
        fpath = os.path.join(DATA_DIR, fname)
        if not os.path.exists(fpath):
            return fpath, fname
        n += 1

csv_file, csv_file_name = get_new_flight_filename()

# Define CSV headers as requested, with Time [ms] first
CSV_HEADERS = [
    'Time [ms]',
    'Altitude [m]', 'Latitude [°]', 'Longitude [°]',
    'NumSatellites[x]', 'HDOP[x]', 'valid[x]',
    'Pressure[hPa]', 'Temp[°C]',
    'Battery[V]',
    'gX[°]', 'gY[°]', 'gZ[°]', 'acX[ms^2]', 'acY[ms^2]', 'acZ[ms^2]', 'State[x]'
]

# Remove persistence, just set start_timestamp on first message
start_timestamp = None
# Store the last known values for each column
last_row = {h: '' for h in CSV_HEADERS}

def parse_mqtt_payload(payload):
    abs_pattern = r"altitude:(?P<altitude>-?\d+\.\d+),battery:(?P<battery>\d+\.\d+),state:(?P<state>\d+)"
    abs_match = re.match(abs_pattern, payload)
    if abs_match:
        d = abs_match.groupdict()
        return {
            'Altitude [m]': d.get('altitude', ''),
            'Battery[V]': d.get('battery', ''),
            'State[x]': d.get('state', ''),
        }
    gps_pattern = (
        r"navSystem:(?P<navSystem>\d+),"
        r"numSatellites:(?P<numSatellites>\d+),"
        r"hdop:(?P<hdop>\d+),"
        r"valid:(?P<valid>true|false),"
        r"latitude:(?P<latitude>-?\d+\.\d+),"
        r"longitude:(?P<longitude>-?\d+\.\d+),"
        r"altitude:(?P<gps_altitude>-?\d+\.\d+),"
        r"date:(?P<date>\d{4}-\d+-\d+),"
        r"time:(?P<time>\d{2}:\d{2}:\d{2}\.\d{2}),"
        r"speed:(?P<speed>-?\d+\.\d+),"
        r"course:(?P<course>-?\d+\.\d+)"
    )
    gps_match = re.match(gps_pattern, payload)
    if gps_match:
        d = gps_match.groupdict()
        valid_val = d.get('valid', '')
        if valid_val == 'true':
            valid_val = '1'
        elif valid_val == 'false':
            valid_val = '0'
        return {
            'AltitudeGPS[m]': d.get('gps_altitude', ''),
            'Latitude [°]': d.get('latitude', ''),
            'Longitude [°]': d.get('longitude', ''),
            'NumSatellites[x]': d.get('numSatellites', ''),
            'HDOP[x]': d.get('hdop', ''),
            'valid[x]': valid_val,
        }
    tp_pattern = r"temperature:(?P<temperature>-?\d+\.\d+),pressure:(?P<pressure>-?\d+\.\d+)"
    tp_match = re.match(tp_pattern, payload)
    if tp_match:
        d = tp_match.groupdict()
        return {
            'Pressure[hPa]': d.get('pressure', ''),
            'Temp[°C]': d.get('temperature', ''),
        }
    return None

def ensure_csv_header():
    if not os.path.exists(csv_file) or os.path.getsize(csv_file) == 0:
        with open(csv_file, 'w', newline='', encoding='utf-8') as f:
            writer = csv.DictWriter(f, fieldnames=CSV_HEADERS)
            writer.writeheader()

def remap_state(state_val):
    try:
        state_int = int(state_val)
        if state_int >= 2:
            return state_int
        return state_int + 1
    except (ValueError, TypeError):
        return ''

def on_message(client, userdata, msg):
    global start_timestamp, last_row
    epoch_ms = int(datetime.now(timezone.utc).timestamp() * 1000)
    if start_timestamp is None:
        start_timestamp = epoch_ms
    rel_time = epoch_ms - start_timestamp
    payload = msg.payload.decode()
    parsed = parse_mqtt_payload(payload)
    if parsed:
        # Update last_row with new values
        for k in parsed:
            if k == 'State[x]':
                last_row[k] = remap_state(parsed[k])
            elif k in last_row and parsed[k] != '':
                last_row[k] = parsed[k]
        last_row['Time [ms]'] = rel_time
        # Write the updated row
        with open(csv_file, 'a', newline='', encoding='utf-8') as f:
            writer = csv.DictWriter(f, fieldnames=CSV_HEADERS)
            writer.writerow(last_row)
        # Only print the received message if it contains an altitude value
        # if 'Altitude [m]' in parsed and parsed['Altitude [m]'] != '':
        #     print(f"Received: {payload}")
    else:
        print(f"[WARN] Could not parse: {payload}")

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker!")
        client.subscribe(topic)
        print(f"Subscribed to topic: {topic}")
    else:
        print(f"Failed to connect, return code {rc}")

def main():
    ensure_csv_header()
    client = mqtt.Client()
    client.on_message = on_message
    client.on_connect = on_connect
    client.connect(broker, port, 60)
    print(f"Listening to topic: {topic} and writing to {csv_file_name}")
    try:
        client.loop_forever()
    except KeyboardInterrupt:
        print("Stopped by user.")
    finally:
        client.disconnect()

if __name__ == '__main__':
    main()
