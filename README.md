# Noisy Refrigerator 🧊

An IoT-based temperature monitoring system designed to track refrigerator health and trigger smart home automation based on thermal thresholds.

## 📺 Demo
https://github.com/user-attachments/assets/5a061f05-cb68-45ef-92af-3ba1d9105525



## 📌 Overview
The **Noisy Refrigerator** project monitors the internal temperature of a fridge using an Arduino-based sensor node. When specific temperature conditions are met, the system communicates with a **Philips Hue Bridge** to trigger a **Hue Plug**, allowing for automated alerts or power management.

## 🛠 Hardware Stack
*   **Microcontroller:** Arduino Nano 33 IoT (WiFi enabled)
*   **Sensor:** DS18B20 Waterproof Temperature Sensor
*   **Actuator:** Philips Hue Plug
*   **Gateway:** Philips Hue Bridge (API integration)

## ⚙️ How it Works

### Communication Flow
```text
  [PIR Sensor] 
       |
       v (Digital)
+-------------------+           +-----------------------+            +-----------------+
|   Motion Node     |    BLE    |   Controller Hub      |    WiFi    |   Hue Bridge    |
| (Arduino Nano 33) | --------> |       (ESP32)         | ---------> |   (API Gateway) |
+-------------------+           +-----------------------+            +-----------------+
                                          ^       ^                            |
                                          |       |                        (Zigbee)
                                    [DS18B20] [Web Dashboard]                  |
                                      Sensor      (HTTP)                       v
                                                                      +----------------+
                                                                      |    Hue Plug    |
                                                                      +----------------+
```

1.  **Sensing:** The DS18B20 sensor provides high-precision temperature readings, and the Motion Node monitors movement via a PIR sensor.
2.  **Processing:** The ESP32 Controller Hub aggregates the BLE motion data and local temperature readings, monitoring for threshold breaches.
3.  **Action:** Upon triggering, the Hub sends an HTTP request to the **Hue Bridge API**, which toggles the state of the connected Hue Plug.

## 🚀 Key Features
*   **Waterproof Monitoring:** Industrial-grade sensing for high-humidity environments.
*   **Cloud-to-Local Control:** Seamless integration between Arduino IoT and the Philips Hue ecosystem.
*   **Low Power/Small Footprint:** Compact hardware setup for easy installation inside or around the appliance.
