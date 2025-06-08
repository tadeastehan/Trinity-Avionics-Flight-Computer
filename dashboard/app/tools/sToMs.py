import pandas as pd
from tkinter import filedialog, Tk, messagebox

def convert_s_to_ms():
    root = Tk()
    root.withdraw()
    file_path = filedialog.askopenfilename(
        title="Select CSV file",
        filetypes=[("CSV Files", "*.csv")]
    )
    if not file_path:
        messagebox.showinfo("No file selected", "No file was selected.")
        return

    df = pd.read_csv(file_path)
    # Find the column with 'Time [s]'
    time_s_col = None
    for col in df.columns:
        if col.strip().lower() == 'time [s]':
            time_s_col = col
            break

    if time_s_col is None:
        messagebox.showerror("Column not found", "No 'Time [s]' column found in the selected file.")
        return

    # Create new column 'Time [ms]' and remove 'Time [s]'
    df.insert(0, 'Time [ms]', df[time_s_col] * 1000)
    df = df.drop(columns=[time_s_col])

    # Save to new file
    new_path = file_path.replace('.csv', '_ms.csv')
    df.to_csv(new_path, index=False)
    messagebox.showinfo("Done", f"Converted file saved as:\n{new_path}")

if __name__ == "__main__":
    convert_s_to_ms()