import csv
import time
import random
import os
import math
from datetime import datetime

# Get absolute path to data folder relative to this script
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CSV_PATH = os.path.join(BASE_DIR, 'data', '2025_MQTT.csv')

# Ensure the data directory exists
os.makedirs(os.path.dirname(CSV_PATH), exist_ok=True)

# Example CSV header and some simulated data fields
def get_header():
    return [
        'Time [ms]', 'Altitude [m]', 'Latitude [°]', 'Longitude [°]', 'State[x]', 'acc_x'
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
    base_lat = 49.2389
    base_lon = 16.5546

    with open(CSV_PATH, 'w', newline='', encoding="UTF-8") as f:
        writer = csv.writer(f)
        writer.writerow(get_header())

    t = 0
    altitude = 100
    state = 1
    stage_times = [1, 2, 3, 10, 1, 20]  # seconds for stages 1-6
    stage_durations = [s * 1000 for s in stage_times]  # ms
    stage_ends = [sum(stage_durations[:i+1]) for i in range(len(stage_durations))]
    stage = 1
    start_time = time.time()
    last_time = 0
    # Trajectory parameters
    line_speed = 0.0003  # degrees per second (approx 30m/s)
    direction_angle = math.radians(45)  # northeast
    landed_lat = None
    landed_lon = None
    while True:
        elapsed = int((time.time() - start_time) * 1000)  # ms
        # Determine stage
        if elapsed < stage_ends[0]:
            stage = 1
            altitude = 100
        elif elapsed < stage_ends[1]:
            stage = 2
            altitude = 100
        elif elapsed < stage_ends[2]:
            stage = 3
            altitude = 100
        elif elapsed < stage_ends[3]:
            stage = 4
            # Ascent: 600m over 10s
            ascent_time = elapsed - stage_ends[2]
            altitude = 100 + (600 * (ascent_time / stage_durations[3]))
        elif elapsed < stage_ends[4]:
            stage = 5
            altitude = 700
        elif elapsed < stage_ends[5]:
            stage = 6
            # Descent: 700 -> 100 over 20s
            descent_time = elapsed - stage_ends[4]
            altitude = 700 - (600 * (descent_time / stage_durations[5]))
        else:
            stage = 7
            altitude = 100
        # Clamp altitude
        if stage == 4:
            altitude = min(700, altitude)
        if stage == 6:
            altitude = max(100, altitude)
        # Simulate time step
        t = elapsed
        # Position logic by stage
        if stage in [1, 2, 3, 4, 5]:
            # Stay at the base position until descent
            lat = base_lat
            lon = base_lon
            landed_lat = None
            landed_lon = None
        elif stage == 6:
            # Descent: move in a straight line from the base position
            descent_time = elapsed - stage_ends[4]  # ms since descent started
            distance = (descent_time / 1000) * line_speed  # degrees
            lat = base_lat + distance * math.cos(direction_angle)
            lon = base_lon + distance * math.sin(direction_angle)
            landed_lat = lat
            landed_lon = lon
        elif stage >= 7:
            # Landed: hold last descent position
            if landed_lat is not None and landed_lon is not None:
                lat = landed_lat
                lon = landed_lon
            else:
                lat = base_lat
                lon = base_lon
        row = [
            round(t, 2),
            round(altitude, 2),
            round(lat, 7),
            round(lon, 7),
            stage,
            round(random.uniform(-2, 2), 3)
        ]
        with open(CSV_PATH, 'a', newline='', encoding="UTF-8") as f:
            writer = csv.writer(f)
            writer.writerow(row)
        # Sleep for 30-45 ms to simulate data rate
        time.sleep(random.randint(30, 45) / 1000)

if __name__ == '__main__':
    main()
