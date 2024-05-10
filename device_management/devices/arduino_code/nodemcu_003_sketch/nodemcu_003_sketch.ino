#include <ArduinoJson.h>
#include <ESP8266WiFi.h>  // Esp8266/NodeMCU Library
#include <PubSubClient.h> // MQTT Library
#include <SPI.h>

// Hardware
#define PIR_PIN 1

// Device Information
#define DEVICE_SERIAL "nodemcu-003"

// Sensors Information
#define MOTION_DETECTOR_SENSOR_SERIAL "hc-srR501-001"

// Units
#define MOTION_DETECTOR_UNIT "boolean"

// Time between printing and publishing sensor data
#define PUBLISH_TIME_PERIOD 1000

// Will message values
#define WILL_ONLINE_MESSAGE_VALUE "online"
#define WILL_OFFLINE_MESSAGE_VALUE "offline"

// wifi
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
WiFiClient wifiClient;

// mqtt broker credentials
const char *mqtt_server = SERVER_IP;
const int mqtt_port = 30883;

// mqtt client
const char *clientID = DEVICE_SERIAL;
PubSubClient client(wifiClient);

// mqtt will configuration
const char *willTopic = "device/" DEVICE_SERIAL "/status";
char will_online_message[256];
char will_offline_message[256];

// mqtt sensor topics
const char *motion_detector_topic =
    "device/" DEVICE_SERIAL "/sensor/" MOTION_DETECTOR_SENSOR_SERIAL;

// serial message
char serial_message[512];
StaticJsonDocument<512> serial_message_doc;
JsonArray doc_array;
JsonObject motion_detector_sensor_object;

// last message counter
unsigned long lastMsg = 0;

/**
 * @brief Set up MQTT client and configurations.
 */
void setup_mqtt() {
  // configure mqtt broker
  client.setServer(mqtt_server, mqtt_port);

  // setup will message payload
  StaticJsonDocument<256> doc;
  doc["status"] = WILL_ONLINE_MESSAGE_VALUE;
  serializeJson(doc, will_online_message);
  doc["status"] = WILL_OFFLINE_MESSAGE_VALUE;
  serializeJson(doc, will_offline_message);
}

/**
 * @brief Connect to MQTT broker.
 *
 * @return true if connected, false otherwise.
 */
boolean connect_to_mqtt() {
  int willQoS = 0;
  boolean willRetain = true;

  if (!client.connected()) {
    if (client.connect(clientID, "", "", willTopic, willQoS, willRetain,
                       will_offline_message, true)) {
      // publish succesful connection message
      client.publish(willTopic, will_online_message, true);
    } else {
      return false;
    }
  }
  return true;
}

/**
 * @brief Connect to WiFi network.
 *
 * @return true if connected, false otherwise.
 */
boolean connect_to_wifi() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    delay(5000);
  }
  return WiFi.status() == WL_CONNECTED;
}

/**
 * @brief Set up initial serial message structure.
 */
void setup_serial_message() {

  serial_message_doc["serial"] = DEVICE_SERIAL;

  doc_array = serial_message_doc.createNestedArray("sensors");

  motion_detector_sensor_object = doc_array.createNestedObject();
  motion_detector_sensor_object["serial"] = MOTION_DETECTOR_SENSOR_SERIAL;
  motion_detector_sensor_object["unit"] = MOTION_DETECTOR_UNIT;
}

/**
 * @brief Update serial message with current sensor data, wifi status and mqtt
 * status.
 */
void update_serial_message(bool wifi_state, bool mqtt_state, int motion_status) {

  serial_message_doc["wifi"] = wifi_state;
  serial_message_doc["mqtt"] = mqtt_state;

  motion_detector_sensor_object["data"] = motion_status;

  serializeJson(serial_message_doc, serial_message);
}
/**
 * @brief Publish sensor data to MQTT topics.
 */
void publish_data(int motion_status) {
  char mqtt_message[256];
  StaticJsonDocument<256> doc;

  doc["data"] = motion_status;
  doc["unit"] = MOTION_DETECTOR_UNIT;
  serializeJson(doc, mqtt_message);
  client.publish(motion_detector_topic, mqtt_message);
}

void setup() {

  // setup serial communication
  Serial.begin(9600);

  // setup serial message
  setup_serial_message();

  delay(1000);

  // setup wifi
  WiFi.mode(WIFI_STA);

  // setup sensor pins
  pinMode(PIR_PIN, INPUT);

  // setup mqtt
  setup_mqtt();
}

void loop() {
  bool wifi_state = connect_to_wifi();
  bool mqtt_state = connect_to_mqtt();

  delay(1000);
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > PUBLISH_TIME_PERIOD) {
    lastMsg = now;
    int motion_detector_status = digitalRead(PIR_PIN);

    // update serial_message with new values
    update_serial_message(wifi_state, mqtt_state, motion_detector_status);

    // print data to serial output
    Serial.println(serial_message);

    // publish data with mqtt
    publish_data(motion_detector_status);
  }
}