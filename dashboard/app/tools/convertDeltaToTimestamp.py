import tkinter as tk
from tkinter import filedialog, simpledialog, messagebox
import pandas as pd
import os

def select_and_rename_headers_and_convert_time():
    root = tk.Tk()
    root.withdraw()
    file_path = filedialog.askopenfilename(
        title="Select CSV file",
        filetypes=[("CSV Files", "*.csv")]
    )
    if not file_path:
        messagebox.showinfo("No file selected", "No file was selected.")
        return
    df = pd.read_csv(file_path)
    headers = list(df.columns)

    # Show headers and let user select columns to rename by index
    header_str = '\n'.join([f"{i}: {col}" for i, col in enumerate(headers)])
    indices_str = simpledialog.askstring(
        "Select Columns",
        f"Headers found:\n{header_str}\n\nEnter column indices to rename (comma separated):",
        parent=root
    )
    if not indices_str:
        messagebox.showinfo("No columns selected", "No columns selected.")
        return
    try:
        indices = [int(i.strip()) for i in indices_str.split(',') if i.strip().isdigit()]
    except Exception:
        messagebox.showerror("Invalid input", "Please enter valid column indices.")
        return
    new_names = {}
    for idx in indices:
        if idx < 0 or idx >= len(headers):
            continue
        new_name = simpledialog.askstring(
            "Rename Header",
            f"Enter new name for header '{headers[idx]}':",
            parent=root
        )
        if new_name:
            new_names[headers[idx]] = new_name

    # Special handling for Time [delta_ms]
    if 'Time [delta_ms]' in headers:
        # Calculate timestamp column
        time_col = df['Time [delta_ms]'].fillna(0).astype(float)
        timestamp = time_col.cumsum()
        df.insert(0, 'Time [ms]', timestamp)
        # Remove the old delta column
        df = df.drop(columns=['Time [delta_ms]'])
    # Rename columns
    df = df.rename(columns=new_names)
    base, ext = os.path.splitext(file_path)
    new_file = base + f"_renamed{ext}"
    df.to_csv(new_file, index=False)
    messagebox.showinfo("Success", f"File saved as {new_file}")

if __name__ == "__main__":
    select_and_rename_headers_and_convert_time()
