# Smart Home with K8s and Microservices | Device Management Service

## Overview

This repository is part of my thesis project, "Enhancing Integration Process and Manageability of a Microservices-Based Home Automation Application with Kubernetes", and contains the device management microservice of the system. It allows for the management of devices, rooms, sensors, and provides an API endpoint for flashing Arduino devices over a serial connection. Additionally, it includes an API view for retrieving device information based on serial communication and an API view for sending commands to sensors.

## Features

- **Device Management**: CRUD operations for devices, rooms, and sensors that include creating rooms, assign rooms to devices, assign desired names to devices or sensors, etc.

- **Flash Serial Device API Endpoint**: An API endpoint for flashing Arduino devices over a serial connection with the appropriate WiFi credentials (device connected to server via USB).

- **Get Serial Device API Endpoint**: An API endpoint for retrieving device information based on the serial communication (device connected to server via USB).

- **Send Command To Sensor API Endpoint**: An API endpoint for sending commands to sensors over MQTT using the appropriate topics.

- **Device Status Updater**: A Django management command that starts an MQTT listener and updates device statuses on database based on the related to status MQTT messages sent by devices.

- **Data Population**: A Django management command that populates the database with device data from a JSON file. This command is for emulating the project's functionality as a product shipment, where predefined devices are shipped with a preconfigured JSON file.

## Technical Details

### Arduino Integration
- Supports NodeMCU ESP8266 devices
- Implements DHT11 temperature and humidity sensors
- Dual relay control capabilities
- Automatic WiFi configuration during flashing
- Serial communication for device status and sensor data

### MQTT Communication
- Topics Structure:
  - Device Status: `device/<device_serial>/status`
  - Sensor Data: `device/<device_serial>/sensor/<sensor_serial>`
  - Sensor Commands: `device/<device_serial>/sensor/<sensor_serial>/command`
- Will Message Support:
  - Online Status: `{"status": "online"}`
  - Offline Status: `{"status": "offline"}`
- QoS Level: 0
- Retained Messages: Enabled for device status

### Sensor Types
- Temperature (DHT11)
  - Unit: Celsius
  - Update Frequency: 1 second
- Humidity (DHT11)
  - Unit: Percentage
  - Update Frequency: 1 second
- Relays
  - Unit: Boolean
  - Commands: "ON", "OFF"
  - State Persistence: Yes


## API Documentation

### Device Management

- **Rooms API**
  - Endpoint: `/api/rooms/`
  - Methods: GET, POST, PUT, PATCH
  - Description: Manage room configurations

- **Devices API**
  - Endpoint: `/api/devices/`
  - Methods: GET, POST, PUT, PATCH
  - Description: Manage device configurations

- **Sensors API**
  - Endpoint: `/api/sensors/`
  - Methods: GET, POST, PUT, PATCH
  - Description: Manage sensor configurations

### Device Operations

- **Flash Serial Device**
  - Endpoint: `/api/flash/<int:id>/`
  - Method: POST
  - Description: Flash Arduino devices with WiFi credentials

- **Get Serial Device Info**
  - Endpoint: `/api/serial/`
  - Method: GET
  - Description: Retrieve device information via serial communication

- **Send Sensor Command**
  - Endpoint: `/api/command/`
  - Method: POST
  - Description: Send commands to sensors via MQTT

## Management Commands

- **Device Status Updater**
  ```bash
  python manage.py device_status_updater
  ```
  Starts an MQTT listener to update device statuses based on MQTT messages.

- **Data Population**
  ```bash
  python manage.py populate_devices <json_file>
  ```
  Populates the database with device data from a JSON file.
