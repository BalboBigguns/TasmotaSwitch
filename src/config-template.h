///////////////////////////////////////////////////////////////////////////
//   AVAILABLE PINS
///////////////////////////////////////////////////////////////////////////
#define RELAY_PIN   12
#define LED_PIN     13
#define BUTTON_PIN  0

///////////////////////////////////////////////////////////////////////////
//   MISC
///////////////////////////////////////////////////////////////////////////
#define FRIENDLY_NAME "MQTT Sonoff Switch"

///////////////////////////////////////////////////////////////////////////
//   WiFi
///////////////////////////////////////////////////////////////////////////
#define WIFI_SSID     "***"
#define WIFI_PASSWORD "***"

///////////////////////////////////////////////////////////////////////////
//   MQTT
///////////////////////////////////////////////////////////////////////////
#define MQTT_USERNAME     NULL
#define MQTT_PASSWORD     NULL
#define MQTT_SERVER       "192.168.8.***"
#define MQTT_SERVER_PORT  1883


#define MQTT_STATE_TOPIC_TEMPLATE   "%s/***/***"
#define MQTT_COMMAND_TOPIC_TEMPLATE "%s/***/***/set"
#define MQTT_STATUS_TOPIC_TEMPLATE  "%s/***/***/status" // MQTT connection: alive/dead

#define MQTT_HOME_ASSISTANT_DISCOVERY_PREFIX  "homeassistant"
#define MQTT_STATE_ON_PAYLOAD   "ON"
#define MQTT_STATE_OFF_PAYLOAD  "OFF"

#define MQTT_CONNECTION_TIMEOUT 5000

///////////////////////////////////////////////////////////////////////////
//   DEBUG
///////////////////////////////////////////////////////////////////////////
#define DEBUG_SERIAL
