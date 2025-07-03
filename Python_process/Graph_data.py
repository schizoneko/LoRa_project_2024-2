import pandas as pd
import matplotlib.pyplot as plt

# Đọc file
df = pd.read_csv(r"D:\Esp-idf\LoRa_project\Data_text\Bridge\DenCuaSong.csv")

# Xử lý tên cột tránh lỗi (loại bỏ khoảng trắng nếu có)
df.columns = df.columns.str.strip()

# Chuyển cột Time về kiểu số nguyên (milisecond)
df["Time"] = pd.to_numeric(df["Time"], errors='coerce')  # sẽ là 37322, 39722, v.v.

# Vẽ biểu đồ
plt.figure(figsize=(15, 8))

# RSSI
plt.subplot(3, 1, 1)
plt.plot(df["Time"], df["RSSI"], color="red", label="RSSI")
plt.ylabel("RSSI (dBm)")
plt.grid(True)
plt.legend()

# SNR
plt.subplot(3, 1, 2)
plt.plot(df["Time"], df["SNR"], color="blue", label="SNR")
plt.ylabel("SNR (dB)")
plt.grid(True)
plt.legend()

# Bytes
plt.subplot(3, 1, 3)
plt.plot(df["Time"], df["Bytes"], color="green", label="Bytes Received")
plt.xlabel("Time (ms)")
plt.ylabel("Bytes")
plt.grid(True)
plt.legend()

plt.tight_layout()
plt.show()
