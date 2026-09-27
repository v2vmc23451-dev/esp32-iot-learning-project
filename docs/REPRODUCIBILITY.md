# Reproducibility and environment record

This record separates details visible in the source from environment details that were not saved. It is intended to help a reviewer reproduce the main functions without claiming an exact historical setup that cannot be verified.

## Known from the project

| Item | Recorded value |
|---|---|
| Board selection documented for the sketch | ESP32 Dev Module |
| Camera wiring profile in source | AI Thinker OV2640 pin map |
| Required Arduino libraries | Bonezegei_DHT11, PubSubClient, Adafruit GFX Library, Adafruit SSD1306 |
| Serial Monitor speed | 115200 baud |
| Wi-Fi secrets file | Local `secrets.h`, excluded from Git |
| MQTT broker configured in source | `mqttgo.io:1883` (public, unencrypted test broker) |

## Not preserved

The exact Arduino IDE version, Espressif ESP32 board-package version, and individual library versions were not saved with this project. The original setup record is incomplete, so those versions are **unknown**. Do not treat versions found in unrelated or stale build folders as the versions used for this sketch.

The sketch calls ESP32 camera and LEDC APIs supplied by the board package. Because the package version is unknown and relevant APIs can differ between releases, a fresh compile is required to establish which installed version works with this source. This repository does not claim compatibility with every board-package release.

To record a future reproduction, fill in the versions shown by Arduino IDE Boards Manager and Library Manager:

| Item | Version used for this reproduction |
|---|---|
| Arduino IDE | To be recorded |
| Espressif ESP32 board package | To be recorded |
| Bonezegei_DHT11 | To be recorded |
| PubSubClient | To be recorded |
| Adafruit GFX Library | To be recorded |
| Adafruit SSD1306 | To be recorded |
| Board marking / camera module | To be recorded |
| Test date | To be recorded |

## Reproduction and verification sequence

1. Install the board package and libraries listed in the README. Select **ESP32 Dev Module** only if the physical board is a compatible classic ESP32 and the OV2640 is wired to the AI Thinker pin map in the sketch.
2. Copy `secrets.example.h` to `secrets.h` beside the sketch and enter credentials for a 2.4 GHz Wi-Fi network. Do not commit or screenshot the real credentials.
3. Check wiring against the README before powering the board. In particular, keep the camera pin map, DHT11 on GPIO4, OLED I²C on GPIO16/17, and RGB channels on GPIO14/12/13 consistent with the actual hardware. Use current-limiting resistors for the RGB LED.
4. Compile and upload. Record whether compilation and upload succeeded, the selected board, the installed package/library versions, and any error text needed to diagnose a failure.
5. Open Serial Monitor at 115200 baud. Confirm Wi-Fi and MQTT connection, `OV2640 camera ready`, and changing temperature/humidity readings. If a subsystem fails, capture the corresponding error before changing one thing at a time.
6. Using the phone dashboard or an MQTT client connected to the same broker, publish `0.00`, `0.50`, and `1.00` to each RGB topic in the README. Confirm that each channel changes independently.
7. Publish `1` to the photo-command topic. Confirm `Photo request received`, then a successful photo publication in Serial Monitor and a returned image in the dashboard/client. Photo transport depends on the public broker and its message-size limit.
8. Save screenshots of the IDE result, Serial Monitor, board wiring, dashboard controls, and received image. Redact Wi-Fi details, personal account information, and bystanders before sharing.

## Expected checks

| Function | Evidence to observe |
|---|---|
| Compile and upload | Arduino IDE reports success for both operations |
| Sensor | Serial output shows temperature and humidity values; MQTT topics receive numeric values |
| OLED | Displays sensor readings and device status |
| RGB control | Each color responds to its corresponding 0–1 MQTT value |
| Camera | Startup reports initialization; a `1` command requests capture and a JPEG result is received |

This repository describes how to repeat the checks; it does not claim a new compile or hardware run was performed while these documentation updates were prepared.
