# Medi Box

## Overview
The **Medi Box** is medication reminder system developed using the **ESP32 microcontroller**. It leverages **MQTT** for efficient communication between devices and integrates with **Node-RED** for real-time data visualization and control. The project aims to provide an accessible platform for health monitoring.

- [Wokwi Simulation](https://wokwi.com/projects/398155012491341825)
- [Wokwi IOT Part](https://wokwi.com/projects/398057780678452225)
- [Node Red Interface]()

## Features
- **MQTT Communication**: Enables efficient data transfer between devices.
- **Node-RED Integration**: Provides a user-friendly interface for data visualization and interaction.
- **Multiple Sensor Support**: Utilizes various sensors to get environmental conditions.

## Libraries Used
The following libraries are used in this project:
- **Adafruit GFX Library**: For graphical display functions.
- **Adafruit SSD1306**: For controlling OLED displays.
- **DHT Sensor Library for ESPx**: For temperature and humidity measurement.
- **PubSubClient**: For MQTT communication.
- **NTPClient**: For time synchronization.
- **ESP32Servo**: For controlling servo motors.

## Getting Started

### Prerequisites
- **ESP32 Microcontroller**
- **Arduino IDE** or any other compatible IDE
- **MQTT Broker** (e.g., Mosquitto)

### Installation
1. **Clone the Repository**:
   ```bash
   git clone https://github.com/username/medi-box.git
   cd medi-box
