# ESP32 IoT Learning Project

An ESP32 project that reads temperature and humidity, sends readings to an MQTT broker, controls an RGB LED from a phone dashboard, and returns a photo from an AI Thinker OV2640 camera.

## Features

- Reads DHT11 temperature and humidity every five seconds.
- Displays sensor readings and device status on a 128×64 SSD1306 OLED.
- Publishes temperature and humidity as separate MQTT messages.
- Accepts phone controls for the red, green, and blue LED brightness.
- Captures a low-resolution JPEG when the phone sends a photo command, converts it to Base64, and publishes it without MQTT retain.
- Uses a generated MQTT client ID when connecting.

## Hardware connections

| Component | ESP32 connection |
|---|---|
| DHT11 DATA | GPIO4 |
| DHT11 VCC / GND | 3.3V / GND |
| OLED SDA / SCL | GPIO16 / GPIO17 |
| OLED power | 3.3V / GND |
| RGB red / blue / green | GPIO14 / GPIO12 / GPIO13 |
| OV2640 camera | AI Thinker pin map in the sketch |

Use a current-limiting resistor (typically 220–330 Ω) for each RGB channel. The sketch assumes a common-cathode LED. GPIO12 is a boot-strapping pin; avoid external wiring that pulls it to an unsuitable level during startup. For a bare DHT11 sensor, add a 4.7–10 kΩ pull-up resistor between DATA and 3.3V.

## MQTT topics

| Purpose | Topic | Payload |
|---|---|---|
| Temperature | `1griffith/class508/temp` | Numeric value, e.g. `24.5` |
| Humidity | `1griffith/class508/humi` | Numeric value, e.g. `51.0` |
| Green LED | `veric1030/class508/g` | `0.00` to `1.00` |
| Blue LED | `veric1030/class508/b` | `0.00` to `1.00` |
| Red LED | `veric1030/class508/r` | `0.00` to `1.00` |
| Photo command | `veric1030/class508/pic2` | Send `1` to capture |
| Photo result | `veric1030/class508/pic` | Base64-encoded JPEG |

The latest sensor readings are published with MQTT retain enabled. Photo messages use retain disabled. The photo is captured at QQVGA resolution to reduce its size; publication can still fail if the encoded image exceeds the MQTT packet buffer.

## Wi-Fi configuration

1. Copy `secrets.example.h` to `secrets.h` in this sketch folder.
2. Enter your own Wi-Fi name and password in `secrets.h`.
3. Keep `secrets.h` local. It is excluded by `.gitignore` and must not be committed.

The public test broker `mqttgo.io:1883` does not provide encrypted transport in this sketch. Do not send private photos or sensitive information through it.

## Build and upload

1. In Arduino IDE, install the Espressif **esp32** board package and select **ESP32 Dev Module**. This sketch uses the classic ESP32 camera driver and AI Thinker OV2640 pin map; confirm your board and camera wiring match that hardware before uploading.
2. Install `Bonezegei_DHT11`, `PubSubClient`, `Adafruit GFX Library`, and `Adafruit SSD1306` from Library Manager.
3. Create the local `secrets.h` file as described above.
4. Open `esp32-iot-learning-project.ino`, compile, then upload to the ESP32.
5. Open Serial Monitor at **115200 baud**. Check the startup messages, Wi-Fi/MQTT status, camera initialization, and sensor readings.

Exact Arduino IDE, ESP32 board-package, and library versions were not preserved with this project. They are intentionally marked **unknown** rather than inferred from unrelated build files. See [reproducibility notes](docs/REPRODUCIBILITY.md) for the known setup and a verification sequence.

## Project records

- [Reproducibility and environment record](docs/REPRODUCIBILITY.md): known requirements, missing version information, and step-by-step checks.
- [Evidence index](docs/EVIDENCE.md): what evidence is available, what is not in this repository, and how to add it safely.

The ESP32 camera and mbedTLS Base64 headers are supplied by the ESP32 platform package.
