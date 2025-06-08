import csv
import time
import random
from datetime import datetime

CSV_PATH = 'data/2025_MQTT.csv'

# Example CSV header and some simulated data fields
def get_header():
    return [
        'Time [ms]', 'Altitude [m]', 'Latitude [°]', 'Longitude [°]', 'State', 'acc_x'
    ]

def generate_row(t):
    # Simulate some data
    altitude = 100 + 10 * random.random() + t * 0.5
    lat = 49.2389 + random.uniform(-0.0005, 0.0005)
    lon = 16.5546 + random.uniform(-0.0005, 0.0005)
    state = min(7, int(t // 10) + 1)  # Simulate stage progression
    acc_x = random.uniform(-2, 2)
    return [
        round(t, 2),
        round(altitude, 2),
        round(lat, 7),
        round(lon, 7),
        state,
        round(acc_x, 3)
    ]

def main():
    
    with open(CSV_PATH, 'w', newline='', encoding="UTF-8") as f:
        writer = csv.writer(f)
        writer.writerow(get_header())

    t = 0
    while True:
        with open(CSV_PATH, 'a', newline='', encoding="UTF-8") as f:
            writer = csv.writer(f)
            writer.writerow(generate_row(t))
        t += random.randint(30,45)
        time.sleep(t/1000)

if __name__ == '__main__':
    main()
