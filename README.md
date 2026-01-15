# BMP-SPO2-Monitoring-using-MAX30102

A dual-mode Arduino-based pulse oximeter that measures Heart Rate (BPM) and Blood Oxygen Saturation (SpO2). It features a touch-sensitive interface to toggle between modes and an OLED display for real-time feedback.

## 🚀 Features
* **BPM Mode:** Instant heart rate detection with moving average filtering.
* **SpO2 Mode:** Advanced blood oxygen calculation using the Maxim algorithm.
* **Touch Interface:** Toggle modes using a TTP223 touch sensor.
* **Finger Detection:** Automatic sensing to start/stop measurements.
* **OLED Feedback:** Visual progress bars and large-text results.

## 🛠 Hardware Required
| Component | Quantity | Description |
| :--- | :--- | :--- |
| MAX30102 / MAX30105 | 1 | Pulse Oximetry Sensor |
| SSD1306 OLED | 1 | 128x64 I2C Display |
| TTP223 | 1 | Capacitive Touch Sensor |
| ESP32 / Arduino | 1 | Microcontroller (Code pinout is for ESP32) |

## 🔌 Wiring Diagram
| Sensor Pin | Microcontroller Pin |
| :--- | :--- |
| VCC | 3.3V |
| GND | GND |
| SCL | SCL (GPIO 22 on ESP32) |
| SDA | SDA (GPIO 21 on ESP32) |
| Touch SIG | GPIO 27 |



## 📦 Libraries Needed
To run this code, install the following via the Arduino Library Manager:
1.  `MAX30105` by SparkFun
2.  `Adafruit SSD1306` by Adafruit
3.  `Adafruit GFX Library` by Adafruit

## 📖 How it Works
1.  **BPM Mode:** The sensor detects the "plethysmogram" (the change in light absorption caused by blood flow). The code calculates the time between peaks to find your heart rate.
2.  **SpO2 Mode:** The sensor emits both Red and Infrared light. Oxygenated hemoglobin absorbs more IR light, while deoxygenated hemoglobin absorbs more Red light. The algorithm compares these ratios over 100 samples to determine SpO2 levels.



## ⚠️ Important Note
This project is for educational purposes only. It is not a medical-grade device. Accuracy can be affected by ambient light, finger pressure, and skin temperature.

## 📜 License
This project is licensed under the MIT License - see the LICENSE file for details.
