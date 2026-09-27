#include <Bonezegei_DHT11.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_camera.h>
#include <mbedtls/base64.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "secrets.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

constexpr uint8_t DHT_PIN = 4;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr uint8_t I2C_SDA_PIN = 16;
constexpr uint8_t I2C_SCL_PIN = 17;
constexpr char MQTT_HOST[] = "mqttgo.io";
constexpr uint16_t MQTT_PORT = 1883;
constexpr char MQTT_TEMPERATURE_TOPIC[] = "1griffith/class508/temp";
constexpr char MQTT_HUMIDITY_TOPIC[] = "1griffith/class508/humi";
constexpr char MQTT_GREEN_TOPIC[] = "veric1030/class508/g";
constexpr char MQTT_BLUE_TOPIC[] = "veric1030/class508/b";
constexpr char MQTT_RED_TOPIC[] = "veric1030/class508/r";
constexpr char CAMERA_TRIGGER_TOPIC[] = "veric1030/class508/pic2";
constexpr char CAMERA_IMAGE_TOPIC[] = "veric1030/class508/pic";
constexpr uint8_t RED_PIN = 14;
constexpr uint8_t BLUE_PIN = 12;
constexpr uint8_t GREEN_PIN = 13;
// Camera XCLK uses LEDC channel 0, so RGB uses separate PWM channels.
constexpr uint8_t RED_CHANNEL = 3;
constexpr uint8_t BLUE_CHANNEL = 4;
constexpr uint8_t GREEN_CHANNEL = 5;
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t BLINK_INTERVAL_MS = 1000;
constexpr uint32_t SENSOR_INTERVAL_MS = 5000;
constexpr uint32_t DISPLAY_INTERVAL_MS = 500;
constexpr uint32_t COLOR_CYCLE_MS = 6000;
constexpr uint32_t PWM_FREQUENCY = 5000;
constexpr uint8_t PWM_RESOLUTION = 8;
constexpr float RED_GAIN = 1.00f;
constexpr float BLUE_GAIN = 0.70f;
constexpr float GREEN_GAIN = 0.70f;

Bonezegei_DHT11 dht(DHT_PIN);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
uint32_t lastBlink = 0;
uint32_t lastSensorRead = 0;
uint32_t lastDisplayUpdate = 0;
uint32_t lastMqttAttempt = 0;
bool ledState = false;
bool oledReady = false;
bool cameraReady = false;
volatile bool photoRequested = false;
float currentTemperature = NAN;
float currentHumidity = NAN;
uint8_t currentRedPwm = 0;
uint8_t currentBluePwm = 0;
uint8_t currentGreenPwm = 0;

void mqttCallback(char* topic, byte* payload, unsigned int length);
void setRgbBrightness(uint8_t red, uint8_t blue, uint8_t green);
bool initCamera();
void publishPhotoBase64();

// AI Thinker OV2640 pin map.
constexpr int8_t CAM_PWDN_GPIO = 32;
constexpr int8_t CAM_RESET_GPIO = -1;
constexpr int8_t CAM_XCLK_GPIO = 0;
constexpr int8_t CAM_SIOD_GPIO = 26;
constexpr int8_t CAM_SIOC_GPIO = 27;
constexpr int8_t CAM_D7_GPIO = 35;
constexpr int8_t CAM_D6_GPIO = 34;
constexpr int8_t CAM_D5_GPIO = 39;
constexpr int8_t CAM_D4_GPIO = 36;
constexpr int8_t CAM_D3_GPIO = 21;
constexpr int8_t CAM_D2_GPIO = 19;
constexpr int8_t CAM_D1_GPIO = 18;
constexpr int8_t CAM_D0_GPIO = 5;
constexpr int8_t CAM_VSYNC_GPIO = 25;
constexpr int8_t CAM_HREF_GPIO = 23;
constexpr int8_t CAM_PCLK_GPIO = 22;

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi connecting");
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(" connected, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(" timeout");
  }
}

void connectMqtt() {
  if (WiFi.status() != WL_CONNECTED || mqttClient.connected()) {
    return;
  }
  const uint32_t now = millis();
  if (now - lastMqttAttempt < 5000) {
    return;
  }
  lastMqttAttempt = now;
  Serial.print("MQTT connecting to ");
  Serial.print(MQTT_HOST);
  Serial.print(":");
  Serial.println(MQTT_PORT);
  char mqttClientId[32];
  snprintf(mqttClientId, sizeof(mqttClientId), "ESP32_%08lX",
           static_cast<unsigned long>(esp_random()));
  if (mqttClient.connect(mqttClientId)) {
    Serial.println("MQTT connected");
    mqttClient.subscribe(MQTT_GREEN_TOPIC);
    mqttClient.subscribe(MQTT_BLUE_TOPIC);
    mqttClient.subscribe(MQTT_RED_TOPIC);
    mqttClient.subscribe(CAMERA_TRIGGER_TOPIC);
    Serial.println("RGB and camera control topics subscribed");
  } else {
    Serial.print("MQTT failed, state: ");
    Serial.println(mqttClient.state());
  }
}

void publishSensorData(uint32_t now) {
  if (!mqttClient.connected() || isnan(currentTemperature) || isnan(currentHumidity)) {
    return;
  }
  char temperaturePayload[16];
  char humidityPayload[16];
  snprintf(temperaturePayload, sizeof(temperaturePayload), "%.1f", currentTemperature);
  snprintf(humidityPayload, sizeof(humidityPayload), "%.1f", currentHumidity);
  const bool tempSent = mqttClient.publish(MQTT_TEMPERATURE_TOPIC, temperaturePayload, true);
  const bool humiditySent = mqttClient.publish(MQTT_HUMIDITY_TOPIC, humidityPayload, true);
  if (tempSent && humiditySent) {
    Serial.printf("MQTT published: temp=%s, humi=%s\n", temperaturePayload, humidityPayload);
  } else {
    Serial.println("MQTT publish failed");
  }
}

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = CAM_D0_GPIO;
  config.pin_d1 = CAM_D1_GPIO;
  config.pin_d2 = CAM_D2_GPIO;
  config.pin_d3 = CAM_D3_GPIO;
  config.pin_d4 = CAM_D4_GPIO;
  config.pin_d5 = CAM_D5_GPIO;
  config.pin_d6 = CAM_D6_GPIO;
  config.pin_d7 = CAM_D7_GPIO;
  config.pin_xclk = CAM_XCLK_GPIO;
  config.pin_pclk = CAM_PCLK_GPIO;
  config.pin_vsync = CAM_VSYNC_GPIO;
  config.pin_href = CAM_HREF_GPIO;
  config.pin_sscb_sda = CAM_SIOD_GPIO;
  config.pin_sscb_scl = CAM_SIOC_GPIO;
  config.pin_pwdn = CAM_PWDN_GPIO;
  config.pin_reset = CAM_RESET_GPIO;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QQVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;

  const esp_err_t result = esp_camera_init(&config);
  if (result != ESP_OK) {
    Serial.printf("Camera init failed: 0x%lx\n", static_cast<unsigned long>(result));
    return false;
  }
  Serial.println("OV2640 camera ready");
  return true;
}

void publishPhotoBase64() {
  if (!cameraReady) {
    Serial.println("Photo skipped: camera is not ready");
    return;
  }
  if (!mqttClient.connected()) {
    Serial.println("Photo skipped: MQTT not connected");
    return;
  }

  camera_fb_t* frame = esp_camera_fb_get();
  if (frame == nullptr) {
    Serial.println("Photo capture failed");
    return;
  }

  const size_t encodedCapacity = 4 * ((frame->len + 2) / 3) + 1;
  char* encoded = static_cast<char*>(malloc(encodedCapacity));
  if (encoded == nullptr) {
    Serial.println("Photo skipped: Base64 buffer allocation failed");
    esp_camera_fb_return(frame);
    return;
  }

  size_t encodedLength = 0;
  const int result = mbedtls_base64_encode(
      reinterpret_cast<unsigned char*>(encoded), encodedCapacity, &encodedLength,
      frame->buf, frame->len);

  bool published = false;
  if (result == 0 && encodedLength + 64 < mqttClient.getBufferSize()) {
    // retain=false: the broker does not keep the photo after this publish.
    published = mqttClient.publish(CAMERA_IMAGE_TOPIC,
                                   reinterpret_cast<const uint8_t*>(encoded),
                                   encodedLength, false);
  } else if (result == 0) {
    Serial.printf("Photo skipped: Base64 size %u exceeds MQTT buffer\n",
                  static_cast<unsigned int>(encodedLength));
  }

  Serial.printf("Photo %s: JPEG %u bytes, Base64 %u bytes\n",
                published ? "published" : "publish failed",
                static_cast<unsigned int>(frame->len),
                static_cast<unsigned int>(encodedLength));
  free(encoded);
  esp_camera_fb_return(frame);
}

void setRgbBrightness(uint8_t red, uint8_t blue, uint8_t green) {
  currentRedPwm = red;
  currentBluePwm = blue;
  currentGreenPwm = green;
  ledcWrite(RED_CHANNEL, red);
  ledcWrite(BLUE_CHANNEL, blue);
  ledcWrite(GREEN_CHANNEL, green);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char valueText[24];
  const unsigned int copyLength = min(length, sizeof(valueText) - 1);
  memcpy(valueText, payload, copyLength);
  valueText[copyLength] = '\0';

  if (strcmp(topic, CAMERA_TRIGGER_TOPIC) == 0) {
    if (strcmp(valueText, "1") == 0) {
      photoRequested = true;
      Serial.println("Photo request received");
    }
    return;
  }

  float value = atof(valueText);
  value = constrain(value, 0.0f, 1.0f);
  const uint8_t pwm = static_cast<uint8_t>(roundf(value * 255.0f));

  if (strcmp(topic, MQTT_GREEN_TOPIC) == 0) {
    setRgbBrightness(currentRedPwm, currentBluePwm, pwm);
  } else if (strcmp(topic, MQTT_BLUE_TOPIC) == 0) {
    setRgbBrightness(currentRedPwm, pwm, currentGreenPwm);
  } else if (strcmp(topic, MQTT_RED_TOPIC) == 0) {
    setRgbBrightness(pwm, currentBluePwm, currentGreenPwm);
  } else {
    return;
  }

  Serial.printf("RGB control: %s = %.2f, PWM = %u\n", topic, value, pwm);
}

uint8_t sineBrightness(uint32_t now, float phaseOffset, float gain) {
  const float cyclePhase = (static_cast<float>(now % COLOR_CYCLE_MS) /
                            static_cast<float>(COLOR_CYCLE_MS)) * 2.0f * PI;
  const float wave = (sinf(cyclePhase + phaseOffset) + 1.0f) * 0.5f;
  return static_cast<uint8_t>(constrain(wave * gain * 255.0f, 0.0f, 255.0f));
}

void updateRgbSineWave(uint32_t now) {
  // Three simultaneous sine waves at 2π/3, 3π/3 and 4π/3.
  const uint8_t red = sineBrightness(now, 2.0f * PI / 3.0f, RED_GAIN);
  const uint8_t blue = sineBrightness(now, 3.0f * PI / 3.0f, BLUE_GAIN);
  const uint8_t green = sineBrightness(now, 4.0f * PI / 3.0f, GREEN_GAIN);
  setRgbBrightness(red, blue, green);
}

void drawRobotIcon(uint32_t now) {
  const int16_t x = 111;
  const int16_t y = 25;
  const int16_t bob = (now / 500) % 2;

  display.drawLine(x, y - 18 + bob, x, y - 14 + bob, SSD1306_WHITE);
  display.fillCircle(x, y - 20 + bob, 2, SSD1306_WHITE);
  display.drawRoundRect(x - 14, y - 13 + bob, 28, 24, 5, SSD1306_WHITE);
  display.fillCircle(x - 6, y - 4 + bob, 2, SSD1306_WHITE);
  display.fillCircle(x + 6, y - 4 + bob, 2, SSD1306_WHITE);
  display.drawLine(x - 6, y + 6 + bob, x + 6, y + 6 + bob, SSD1306_WHITE);
  display.drawLine(x - 14, y - 4 + bob, x - 19, y + bob, SSD1306_WHITE);
  display.drawLine(x + 14, y - 4 + bob, x + 19, y + bob, SSD1306_WHITE);
}

void drawSineGraphic(uint32_t now) {
  const int16_t baseline = 59;
  for (int16_t x = 0; x < OLED_WIDTH; ++x) {
    const float phase = (static_cast<float>(x + now / 40) / 18.0f) * PI;
    const int16_t y = baseline - static_cast<int16_t>((sinf(phase) + 1.0f) * 2.0f);
    display.drawPixel(x, y, SSD1306_WHITE);
  }
}

void updateDisplay(uint32_t now) {
  if (!oledReady || now - lastDisplayUpdate < DISPLAY_INTERVAL_MS) {
    return;
  }
  lastDisplayUpdate = now;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ESP32 SENSOR");
  display.println("OLED ONLINE");

  display.print("Temp: ");
  if (isnan(currentTemperature)) {
    display.println("WAIT DHT");
  } else {
    display.print(currentTemperature, 1);
    display.println(" C");
  }

  display.print("Hum : ");
  if (isnan(currentHumidity)) {
    display.println("WAIT DHT");
  } else {
    display.print(currentHumidity, 1);
    display.println(" %");
  }

  display.print("Uptime: ");
  display.print(now / 1000UL);
  display.println(" s");
  display.println("DHT11 GPIO17  I2C OK");
  display.println("RGB MQTT CONTROL");
  drawRobotIcon(now);
  drawSineGraphic(now);
  display.display();
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(SERIAL_BAUD);
  delay(300);
  dht.begin();

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  oledReady = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  if (oledReady) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("ESP32 SENSOR NODE");
    display.println("OLED online");
    display.display();
  } else {
    Serial.println("OLED not found at I2C address 0x3C");
  }

  delay(1000);

  ledcSetup(RED_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcSetup(BLUE_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcSetup(GREEN_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(RED_PIN, RED_CHANNEL);
  ledcAttachPin(BLUE_PIN, BLUE_CHANNEL);
  ledcAttachPin(GREEN_PIN, GREEN_CHANNEL);
  setRgbBrightness(0, 0, 0);

  Serial.println();
  Serial.println("ESP32 DHT11 project started");
  Serial.println("DHT11 data pin: GPIO4");
  Serial.println("RGB MQTT control: RED14, BLUE12, GREEN13");

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setKeepAlive(30);
  mqttClient.setBufferSize(32768);
  mqttClient.setCallback(mqttCallback);
  setRgbBrightness(0, 0, 0);
  cameraReady = initCamera();
  connectWiFi();
}

void loop() {
  const uint32_t now = millis();
  updateDisplay(now);

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  connectMqtt();
  mqttClient.loop();

  if (photoRequested) {
    photoRequested = false;
    publishPhotoBase64();
  }

  if (now - lastBlink >= BLINK_INTERVAL_MS) {
    lastBlink = now;
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
    Serial.printf("LED: %s | uptime: %lu ms\n",
                  ledState ? "ON" : "OFF",
                  static_cast<unsigned long>(now));
  }

  if (now - lastSensorRead >= SENSOR_INTERVAL_MS) {
    lastSensorRead = now;

    if (!dht.getData()) {
      Serial.println("DHT11 read failed; check wiring and power.");
      return;
    }

    const float humidity = dht.getHumidity();
    const float temperature = dht.getTemperature();
    if (humidity <= 0.0f || humidity > 100.0f || temperature <= -40.0f) {
      Serial.println("DHT11 returned invalid data; check DATA pull-up and wiring.");
      return;
    }

    currentHumidity = humidity;
    currentTemperature = temperature;
    publishSensorData(now);

    Serial.printf("Temperature: %.1f C | Humidity: %.1f %%\n",
                  currentTemperature, currentHumidity);
  }
}
