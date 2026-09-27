# Evidence index

The source and setup instructions are in this repository. Photos and screenshots are kept separate so reviewers can distinguish the code from proof of a particular hardware run.

## Evidence status

- The project conversation included photos of the ESP32 breadboard/OLED and screenshots of the phone dashboard and camera image. They are not currently stored in this GitHub repository as files.
- The historical notes report a successful compile/upload and successful sensor, camera, and MQTT-control checks. The exact IDE/package/library versions and original IDE/Serial Monitor screenshots were not preserved here.
- No new compile, upload, or hardware test is claimed for this documentation update.

## Suggested evidence to add after a fresh check

Use dated filenames and include only evidence you can personally verify:

| Suggested filename | What it should show |
|---|---|
| `compile-upload-success.png` | Arduino IDE selected board and successful compile/upload result |
| `serial-monitor-startup.png` | Camera, Wi-Fi, MQTT, and sensor status at 115200 baud |
| `hardware-overview.jpg` | ESP32, camera wiring, DHT11, OLED, and RGB LED in one view where practical |
| `phone-dashboard-controls.png` | Sensor display and RGB controls; avoid account/profile details |
| `camera-result.png` | A photo command and the returned image |

Before adding evidence to this public repository, remove Wi-Fi names/passwords, private broker credentials, personal account details, and identifying images of other people unless they have agreed to public sharing. Do not edit or fabricate IDE success messages or test results. If a result cannot be reproduced, record that honestly instead of presenting it as a successful run.

If evidence is later added, link it here with the test date and the environment versions recorded in [REPRODUCIBILITY.md](REPRODUCIBILITY.md).
