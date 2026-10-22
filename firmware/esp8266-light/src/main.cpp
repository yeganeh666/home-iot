#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"

// ---------- تنظیمات سخت‌افزار ----------
#define RELAY_PIN        D1     // GPIO5
#define RELAY_ACTIVE_LOW true   // بیشتر ماژول‌های رله با LOW روشن می‌شوند؛ اگر برعکس بود false کن

// ---------- تنظیمات دستگاه ----------
static const char* DEVICE_ID   = "room_light_1";
static const char* DEVICE_NAME = "Room Light";

static char topicCmd[64], topicState[64], topicAvail[64], topicDisc[96];

WiFiClient espClient;
PubSubClient mqtt(espClient);
bool lightOn = false;

void applyRelay(bool on) {
  lightOn = on;
  bool level = RELAY_ACTIVE_LOW ? !on : on;
  digitalWrite(RELAY_PIN, level ? HIGH : LOW);
}

void publishState() {
  mqtt.publish(topicState, lightOn ? "ON" : "OFF", true);
}

void publishDiscovery() {
  // Home Assistant با این پیام خودکار دستگاه را پیدا می‌کند
  char payload[512];
  snprintf(payload, sizeof(payload),
    "{\"name\":\"%s\",\"unique_id\":\"%s\","
    "\"command_topic\":\"%s\",\"state_topic\":\"%s\","
    "\"availability_topic\":\"%s\","
    "\"payload_on\":\"ON\",\"payload_off\":\"OFF\","
    "\"device\":{\"identifiers\":[\"%s\"],\"name\":\"%s\","
    "\"manufacturer\":\"DIY\",\"model\":\"ESP8266 + Relay\"}}",
    DEVICE_NAME, DEVICE_ID, topicCmd, topicState, topicAvail, DEVICE_ID, DEVICE_NAME);
  mqtt.publish(topicDisc, payload, true);
}

void onMessage(char* topic, byte* payload, unsigned int len) {
  String msg;
  for (unsigned int i = 0; i < len; i++) msg += (char)payload[i];
  msg.trim();
  msg.toUpperCase();
  if (msg == "ON")  applyRelay(true);
  if (msg == "OFF") applyRelay(false);
  publishState();
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.print("\nIP: "); Serial.println(WiFi.localIP());
}

void connectMqtt() {
  while (!mqtt.connected()) {
    Serial.print("MQTT... ");
    // LWT: اگر دستگاه قطع شود broker خودکار offline را اعلام می‌کند
    if (mqtt.connect(DEVICE_ID, MQTT_USER, MQTT_PASSWORD, topicAvail, 0, true, "offline")) {
      Serial.println("ok");
      mqtt.publish(topicAvail, "online", true);
      mqtt.subscribe(topicCmd);
      publishDiscovery();
      publishState();
    } else {
      Serial.printf("fail rc=%d\n", mqtt.state());
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  // اول سطح امن را بگذار، بعد پایه را خروجی کن تا رله هنگام بوت پَر نزند
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
  pinMode(RELAY_PIN, OUTPUT);
  applyRelay(false);

  snprintf(topicCmd,   sizeof(topicCmd),   "home/%s/set", DEVICE_ID);
  snprintf(topicState, sizeof(topicState), "home/%s/state", DEVICE_ID);
  snprintf(topicAvail, sizeof(topicAvail), "home/%s/availability", DEVICE_ID);
  snprintf(topicDisc,  sizeof(topicDisc),  "homeassistant/light/%s/config", DEVICE_ID);

  connectWifi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setBufferSize(768);
  mqtt.setCallback(onMessage);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWifi();
  if (!mqtt.connected()) connectMqtt();
  mqtt.loop();
}
