# graph_maker.py
import os
import re
import matplotlib.pyplot as plt
import pandas as pd

# File paths
DATA_DIR = os.path.join(os.path.dirname(__file__), 'data')
GRAPH_DIR = os.path.join(os.path.dirname(__file__), 'graphs')
FILES = [
    ('Dualsky520mAh_1C.xls', 500),
    ('Dualsky520mAh_2C.xls', 490),
    ('Dualsky520mAh_3C.xls', 480),
]

THEORETICAL_CAPACITY = 520  # mAh

# Helper to parse the custom .xls format
def parse_dualsky_file(filepath):
    with open(filepath, encoding='utf-16') as f:
        lines = f.readlines()

    # Find where the data starts
    data_start = None
    for i, line in enumerate(lines):
        if line.strip().lower().startswith('no.'):
            data_start = i + 1
            break
    # if data_start is None:
    #     raise ValueError(f"Could not find data header ('No.') in file: {filepath}")
    # Read data
    data = []
    for line in lines[data_start:]:
        parts = line.strip().split('\t')
        if len(parts) == 3:
            try:
                idx, t, v = parts
                v = float(v.replace(',', '.'))
                data.append(v)
            except Exception:
                continue
    return data

# Plot settings for each curve
PLOT_STYLES = [
    {  # 1C
        'label': '1C',
        'color': '#00847e',
        'linewidth': 3,
    },
    {  # 2C
        'label': '2C',
        'color': 'hotpink',
        'linewidth': 3,
    },
    {  # 3C
        'label': '3C',
        'color': 'royalblue',
        'linewidth': 3,
    },
]

plt.figure(figsize=(10, 6), dpi=200)

for (fname, measured_capacity), style in zip(FILES, PLOT_STYLES):
    fpath = os.path.join(DATA_DIR, fname)
    voltages = parse_dualsky_file(fpath)
    # Find the first drop in voltage
    start_idx = 0
    for i in range(1, len(voltages)):
        if voltages[i] < voltages[i-1]:
            start_idx = i - 1
            break
    # Crop at first drop and at minimum voltage (before voltage rises)
    min_idx = voltages.index(min(voltages))
    voltages = voltages[start_idx:min_idx+1]
    n = len(voltages)
    # Adjust percent so the last point is (measured_capacity / THEORETICAL_CAPACITY) * 100
    percent = [i / (n - 1) * (measured_capacity / THEORETICAL_CAPACITY) * 100 for i in range(n)]
    plt.plot(
        percent,
        voltages,
        label=style['label'],
        color=style['color'],
        linewidth=style['linewidth'],
    )

plt.ylim(6.3, 8.5)
plt.yticks([6.4, 7.0, 7.5, 8.0, 8.4])
plt.xlabel('Capacity Used [%]')
plt.ylabel('Voltage [V]')
plt.title('Dualsky 520mAh 2S Discharge Curves')
plt.legend()
plt.grid(True)
plt.tight_layout()

# Save high quality image
os.makedirs(GRAPH_DIR, exist_ok=True)
plt.savefig(os.path.join(GRAPH_DIR, 'dualsky_520mah_discharge.png'), dpi=300)
plt.show()  # Show the plot interactively
plt.close()

print('Graph saved to graphs/dualsky_520mah_discharge.png')
