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

---

## 📷 Wiring Diagram<img width="627" height="456" alt="1" src="https://github.com/user-attachments/assets/72d11b79-a55a-4148-9351-ab4a53e538d5" />

---

## 📷 Node-RED flow graph<img width="693" height="485" alt="2" src="https://github.com/user-attachments/assets/9a9191d8-a617-4388-950d-59073a403f74" />

---

## 📷 Node-RED Dashboard<img width="1327" height="587" alt="3" src="https://github.com/user-attachments/assets/76eb386a-c0e8-42e4-abe0-b2a9c009fe56" />


---



