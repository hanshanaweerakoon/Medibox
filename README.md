# Medibox – Smart Medicine Storage System

This project is part of the *EN2853 Embedded Systems and Applications*.
It enhances a Medibox with *light and temperature monitoring, an **automatic servo-controlled window, and a **Node-RED dashboard* for remote monitoring and control.

---

## Features
- 📟 *OLED Display* – Shows time, temperature, and light levels.
- 🌡 *DHT22 Sensor* – Monitors temperature inside the Medibox.
- 💡 *LDR Sensor* – Measures light intensity (0–1 scale).
- ⚙ *Servo Motor* – Adjusts a sliding window to control light.
- 📢 *Buzzer* – Alerts for medication schedule.
- 📊 *Node-RED Dashboard* – Displays data and allows parameter adjustments via MQTT.
- 🌐 *WiFi + MQTT* – Uses test.mosquitto.org broker for communication.

---

## Hardware Used
- ESP32 Dev Board
- DHT22 Temperature & Humidity Sensor
- Light Dependent Resistor (LDR)
- SG90 Servo Motor
- Passive Buzzer
- OLED Display (SSD1306)


![Breadboard Diagram](<img width="627" height="456" alt="hardwarecircuit-diagram png" src="https://github.com/user-attachments/assets/57b3c5d8-65d0-4ca6-a663-5f90e8d44449" />
)

---



