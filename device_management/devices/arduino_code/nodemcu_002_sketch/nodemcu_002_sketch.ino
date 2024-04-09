#include <ArduinoJson.h>
#include "DHT.h"
#include <ESP8266WiFi.h>  // Esp8266/NodeMCU Library
#include <PubSubClient.h> // MQTT Library

// Hardware
#define DHTTYPE DHT11
#define DHT_DPIN D1
#define CH1_DPIN D3
#define CH2_DPIN D5

// Device Information
#define DEVICE_SERIAL "nodemcu-002"

// Sensors Information
#define TEMPER_SENSOR_SERIAL "dht11-temper-001"
#define HUMID_SENSOR_SERIAL "dht11-humid-001"
#define RELAY_CH1_SENSOR_SERIAL "relay-ch1-001"
#define RELAY_CH2_SENSOR_SERIAL "relay-ch2-001"

// Units
#define TEMPERATURE_UNIT "celcius"
#define HUMIDITY_UNIT "percentage"
#define RELAY_STATE_UNIT "boolean"

// Sensor Commands
#define RELAY_TURN_ON "ON"
#define RELAY_TURN_OFF "OFF"

// Time between printing and publishing sensor data
#define PUBLISH_TIME_PERIOD 3000

// Will message values
#define WILL_ONLINE_MESSAGE_VALUE "online"
#define WILL_OFFLINE_MESSAGE_VALUE "offline"

// sensors initialization
DHT dht(DHT_DPIN, DHTTYPE);

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
const char *temper_topic =
    "device/" DEVICE_SERIAL "/sensor/" TEMPER_SENSOR_SERIAL;
const char *humid_topic =
    "device/" DEVICE_SERIAL "/sensor/" HUMID_SENSOR_SERIAL;
const char *relay_ch1_state_topic =
    "device/" DEVICE_SERIAL "/sensor/" RELAY_CH1_SENSOR_SERIAL;
const char *relay_ch2_state_topic =
    "device/" DEVICE_SERIAL "/sensor/" RELAY_CH2_SENSOR_SERIAL;
const char *relay_ch1_command_topic =
    "device/" DEVICE_SERIAL "/sensor/" RELAY_CH1_SENSOR_SERIAL "/command";
const char *relay_ch2_command_topic =
    "device/" DEVICE_SERIAL "/sensor/" RELAY_CH2_SENSOR_SERIAL "/command";

// serial message
char serial_message[512];
StaticJsonDocument<512> serial_message_doc;
JsonArray doc_array;
JsonObject temper_sensor_object;
JsonObject humid_sensor_object;
JsonObject relay_ch1_sensor_object;
JsonObject relay_ch2_sensor_object;

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

  // assign function for new messages on subscribed topics
  client.setCallback(callback);
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
      // subscribe to command topics
      client.subscribe(relay_ch1_command_topic);
      client.subscribe(relay_ch2_command_topic);
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

  temper_sensor_object = doc_array.createNestedObject();
  temper_sensor_object["serial"] = TEMPER_SENSOR_SERIAL;
  temper_sensor_object["unit"] = TEMPERATURE_UNIT;

  humid_sensor_object = doc_array.createNestedObject();
  humid_sensor_object["serial"] = HUMID_SENSOR_SERIAL;
  humid_sensor_object["unit"] = HUMIDITY_UNIT;

  relay_ch1_sensor_object = doc_array.createNestedObject();
  relay_ch1_sensor_object["serial"] = RELAY_CH1_SENSOR_SERIAL;
  relay_ch1_sensor_object["unit"] = RELAY_STATE_UNIT;

  relay_ch2_sensor_object = doc_array.createNestedObject();
  relay_ch2_sensor_object["serial"] = RELAY_CH2_SENSOR_SERIAL;
  relay_ch2_sensor_object["unit"] = RELAY_STATE_UNIT;
}

/**
 * @brief Update serial message with current sensor data, wifi status and mqtt
 * status.
 */
void update_serial_message(bool wifi_state, bool mqtt_state, int ch1_state,
                           int ch2_state, float temper_data, float hum_data) {

  serial_message_doc["wifi"] = wifi_state;
  serial_message_doc["mqtt"] = mqtt_state;

  relay_ch1_sensor_object["data"] = ch1_state;
  relay_ch2_sensor_object["data"] = ch2_state;
  humid_sensor_object["data"] = hum_data;
  temper_sensor_object["data"] = temper_data;

  serializeJson(serial_message_doc, serial_message);
}
/**
 * @brief Publish sensor data to MQTT topics.
 */
void publish_data(int ch1_state, int ch2_state, float temper_data,
                  float hum_data) {
  char mqtt_message[256];
  StaticJsonDocument<256> doc;

  doc["data"] = ch1_state;
  doc["unit"] = RELAY_STATE_UNIT;
  serializeJson(doc, mqtt_message);
  client.publish(relay_ch1_state_topic, mqtt_message);

  doc["data"] = ch2_state;
  doc["unit"] = RELAY_STATE_UNIT;
  serializeJson(doc, mqtt_message);
  client.publish(relay_ch2_state_topic, mqtt_message);

  doc["data"] = temper_data;
  doc["unit"] = TEMPERATURE_UNIT;
  serializeJson(doc, mqtt_message);
  client.publish(temper_topic, mqtt_message);

  doc["data"] = hum_data;
  doc["unit"] = HUMIDITY_UNIT;
  serializeJson(doc, mqtt_message);
  client.publish(humid_topic, mqtt_message);
}

/**
 * @brief Control the relay pin based on the given command.
 */
void handle_relay_command(const char *command, int pin) {
  if (strcmp(command, RELAY_TURN_ON) == 0) {
    digitalWrite(pin, HIGH);
  } else if (strcmp(command, RELAY_TURN_OFF) == 0) {
    digitalWrite(pin, LOW);
  }
}

/**
 * @brief Handle relay command.
 */
void relay_handler(const char *command, const char *topic) {
  if (strcmp(topic, relay_ch2_command_topic) == 0) {
    handle_relay_command(command, CH2_DPIN);
  } else if (strcmp(topic, relay_ch1_command_topic) == 0) {
    handle_relay_command(command, CH1_DPIN);
  }
}

/**
 * @brief Callback function for MQTT message reception.
 */
void callback(char *topic, byte *payload, unsigned int length) {
  payload[length] = '\0';
  char *cstring = (char *)payload;
  relay_handler(cstring, topic);
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
  pinMode(CH1_DPIN, OUTPUT);
  pinMode(CH2_DPIN, OUTPUT);
  dht.begin();

  // setup mqtt
  setup_mqtt();
}

void loop() {
  bool wifi_state = connect_to_wifi();
  bool mqtt_state = connect_to_mqtt();

  delay(2000);
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > PUBLISH_TIME_PERIOD) {
    lastMsg = now;

    // read temperature and humidity
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    // read relay channels state
    int ch1_state = digitalRead(CH1_DPIN);
    int ch2_state = digitalRead(CH2_DPIN);

    // update serial_message with new values
    update_serial_message(wifi_state, mqtt_state, ch1_state, ch2_state,
                          temperature, humidity);

    // print data to serial output
    Serial.println(serial_message);

    // publish data with mqtt
    publish_data(ch1_state, ch2_state, temperature, humidity);
  }
}