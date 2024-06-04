#include <WiFi.h>
#include <MQTTClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

#define DHT22_PIN  4 // ESP32 pin GPIO21 connected to DHT22 sensor
#define CLIENT_ID "ESP32-001"
#define PUBLISH_TOPIC "kon324"
#define SUBSCRIBE_TOPIC "esp32-001/send"


String URL = "http://api.openweathermap.org/data/2.5/weather?";
String ApiKey = "****";
String lat = "41.104153";
String lon = "29.025075";

DHT dht22(DHT22_PIN, DHT22);

const char WIFI_SSID[] = "***";
const char WIFI_PASSWORD[] = "****";
const char MQTT_BROKER_ADRRESS[] = "*****";
const int MQTT_PORT = 1884;
const char MQTT_USERNAME[] = "****";
const char MQTT_PASSWORD[] = "****";

#define PUBLISH_INTERVAL 10000  // 30 seconds

WiFiClient network;
MQTTClient mqtt(256);

unsigned long lastPublishTime = 0;

// Function declarations
void connectToMQTT();

void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  dht22.begin();

  Serial.println("ESP32 - Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");

  connectToMQTT();
}

void loop() {
  float humi = dht22.readHumidity();
  float tempC = dht22.readTemperature();
  float webTemp = 0;

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(URL + "lat=" + lat + "&lon=" + lon + "&units=metric&appid=" + ApiKey);
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println(payload);

      DynamicJsonDocument doc(2048);
      deserializeJson(doc, payload);

      webTemp = doc["main"]["temp"].as<float>(); // Extract temperature from JSON
      Serial.print("Web Temperature: ");
      Serial.println(webTemp);
    }
    http.end();
  }

  mqtt.loop();

  if (millis() - lastPublishTime > PUBLISH_INTERVAL) {
    StaticJsonDocument<200> message;
    message["groupno"] = 12;
    message["sensortemp"] = tempC;
    message["webtemp"] = webTemp;
    char messageBuffer[512];
    serializeJson(message, messageBuffer);
    mqtt.publish(PUBLISH_TOPIC, messageBuffer);
   Serial.println("ESP32 - sent to MQTT:");
    Serial.print("- topic: ");
    Serial.println(PUBLISH_TOPIC);
    Serial.print("- payload:");
    Serial.println(messageBuffer);
    lastPublishTime = millis();
  }

  
}

void connectToMQTT() {
 // Connect to the MQTT broker
  mqtt.begin(MQTT_BROKER_ADRRESS, MQTT_PORT, network);

  // Create a handler for incoming messages
  mqtt.onMessage(messageHandler);

  Serial.print("ESP32 - Connecting to MQTT broker");

  while (!mqtt.connect(CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.print("N");
    delay(100);
  }
  Serial.println();

  if (!mqtt.connected()) {
    Serial.println("ESP32 - MQTT broker Timeout!");
    return;
  }

  // Subscribe to a topic, the incoming messages are processed by messageHandler() function
  if (mqtt.subscribe(SUBSCRIBE_TOPIC))
    Serial.print("ESP32 - Subscribed to the topic: ");
  else
    Serial.print("ESP32 - Failed to subscribe to the topic: ");

  Serial.println(SUBSCRIBE_TOPIC);
  Serial.println("ESP32  - MQTT broker Connected!");
}

void messageHandler(String &topic, String &payload) {
  Serial.println("ESP32 - received from MQTT:");
  Serial.println("- topic: " + topic);
  Serial.println("- payload:");
  Serial.println(payload);  
}
