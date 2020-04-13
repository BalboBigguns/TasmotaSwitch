#include <Arduino.h>
#include <ESP8266WiFi.h> // https://github.com/esp8266/Arduino
#include <ESP8266WiFiType.h>
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient
#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson
#include "switch.h"

#if defined(DEBUG_SERIAL)
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

#define STATE_ON_SIGNAL "{\'state\':\'ON\'}"
#define STATE_OFF_SIGNAL "{\'state\':\'OFF\'}"

StaticJsonDocument<256> jsonDocument;
//char jsonDocument[256] = {0};

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

Switch switchDev;

///////////////////////////////////////////////////////////////////////////
//   WiFi
///////////////////////////////////////////////////////////////////////////
/*
   Function called to handle WiFi events
*/
void handleWiFiEvent(WiFiEvent_t event)
{
    switch (event) {
    case WIFI_EVENT_STAMODE_GOT_IP:
        DEBUG_PRINTLN(F("INFO: WiFi connected"));
        DEBUG_PRINT(F("INFO: IP address: "));
        DEBUG_PRINTLN(WiFi.localIP());
        break;
    case WIFI_EVENT_STAMODE_DISCONNECTED:
        DEBUG_PRINTLN(F("ERROR: WiFi losts connection"));
        /*
         TODO: Do something smarter than rebooting the device
        */
        delay(5000);
        ESP.restart();
        break;
    default:
        DEBUG_PRINT(F("INFO: WiFi event: "));
        DEBUG_PRINTLN(event);
        break;
    }
}

/*
   Function called to setup the connection to the WiFi AP
*/
void setupWiFi()
{
    DEBUG_PRINT(F("INFO: WiFi connecting to: "));
    DEBUG_PRINTLN(WIFI_SSID);

    delay(10);

    WiFi.mode(WIFI_STA);
    WiFi.onEvent(handleWiFiEvent);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    //randomSeed(micros());
}

///////////////////////////////////////////////////////////////////////////
//   MQTT
///////////////////////////////////////////////////////////////////////////

char MQTT_CLIENT_ID[7] = {0};
#if defined(MQTT_HOME_ASSISTANT_SUPPORT)
char MQTT_CONFIG_TOPIC[sizeof(MQTT_HOME_ASSISTANT_DISCOVERY_PREFIX) + sizeof(MQTT_CLIENT_ID) + sizeof(MQTT_CONFIG_TOPIC_TEMPLATE) - 4] = {0};
#else

#endif

char MQTT_STATE_TOPIC[sizeof(MQTT_CLIENT_ID) + sizeof(MQTT_STATE_TOPIC_TEMPLATE) - 2] = {0};
char MQTT_COMMAND_TOPIC[sizeof(MQTT_CLIENT_ID) + sizeof(MQTT_COMMAND_TOPIC_TEMPLATE) - 2] = {0};
char MQTT_STATUS_TOPIC[sizeof(MQTT_CLIENT_ID) + sizeof(MQTT_STATUS_TOPIC_TEMPLATE) - 2] = {0};

volatile unsigned long lastMQTTConnection = 0;

/*
  Function called to publish to a MQTT topic with the given payload
*/
void publishToMQTT(char *p_topic,const char *p_payload)
{
    if (mqttClient.publish(p_topic, p_payload, true)) {
        DEBUG_PRINT(F("INFO: MQTT message published successfully, topic: "));
        DEBUG_PRINT(p_topic);
        DEBUG_PRINT(F(", payload: "));
        DEBUG_PRINTLN(p_payload);
    }
    else {
        DEBUG_PRINTLN(F("ERROR: MQTT message not published, either connection lost, or message too large. Topic: "));
        DEBUG_PRINT(p_topic);
        DEBUG_PRINT(F(" , payload: "));
        DEBUG_PRINTLN(p_payload);
    }
}

/*
   Function called when a MQTT message has arrived
   @param p_topic   The topic of the MQTT message
   @param p_payload The payload of the MQTT message
   @param p_length  The length of the payload
*/
void handleMQTTMessage(char *p_topic, byte *p_payload, unsigned int p_length)
{
    // concatenates the payload into a string
    String payload;
    for (uint8_t i = 0; i < p_length; i++) {
        payload.concat((char)p_payload[i]);
    }

    DEBUG_PRINTLN(F("INFO: New MQTT message received"));
    DEBUG_PRINT(F("INFO: MQTT topic: "));
    DEBUG_PRINTLN(p_topic);
    DEBUG_PRINT(F("INFO: MQTT payload: "));
    DEBUG_PRINTLN(payload);


    if (String(MQTT_COMMAND_TOPIC).equals(p_topic)) {
        DeserializationError err = deserializeJson(jsonDocument, payload);

        if (err) {
            DEBUG_PRINTLN(F("ERROR: deserializeJson() failed with code: "));
            DEBUG_PRINTLN(err.c_str());
            return;
        }

        JsonObject root = jsonDocument.as<JsonObject>();
        JsonVariant value = root["state"];
        
        if (!value.isNull()) {
            if (strcmp(value, MQTT_STATE_ON_PAYLOAD) == 0) {
                if (switchDev.setState(true)) {
                    DEBUG_PRINT(F("INFO: State changed to: "));
                    DEBUG_PRINTLN(switchDev.getState());
                    publishToMQTT(MQTT_STATE_TOPIC, STATE_ON_SIGNAL);
                }
            }
            else if (strcmp(value, MQTT_STATE_OFF_PAYLOAD) == 0) {
                // stops the possible current effect
                //switchDev.setEffect(EFFECT_NOT_DEFINED_NAME);

                if (switchDev.setState(false)) {
                    DEBUG_PRINT(F("INFO: State changed to: "));
                    DEBUG_PRINTLN(switchDev.getState());
                    publishToMQTT(MQTT_STATE_TOPIC, STATE_OFF_SIGNAL);
                }
            }
        }
    }
}

/*
  Function called to subscribe to a MQTT topic
*/
void subscribeToMQTT(char *p_topic)
{
    if (mqttClient.subscribe(p_topic)) {
        DEBUG_PRINT(F("INFO: Sending the MQTT subscribe succeeded for topic: "));
        DEBUG_PRINTLN(p_topic);
    }
    else {
        DEBUG_PRINT(F("ERROR: Sending the MQTT subscribe failed for topic: "));
        DEBUG_PRINTLN(p_topic);
    }
}

/*
  Function called to connect/reconnect to the MQTT broker
*/
void connectToMQTT()
{
    if (!mqttClient.connected()) {
        Serial.print("Last connection: ");
        Serial.println(lastMQTTConnection);
        Serial.print("Timeout: ");
        Serial.println(MQTT_CONNECTION_TIMEOUT);
        Serial.print("Millis: ");
        Serial.println(millis());

        if (lastMQTTConnection + MQTT_CONNECTION_TIMEOUT < millis()) {
            DEBUG_PRINTLN(F("INFO: Trying to establish connection with the MQTT broker..."));
            if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_STATUS_TOPIC, 0, 1, "dead" /*, MQTT_USERNAME, MQTT_PASSWORD, MQTT_STATUS_TOPIC, 0, 1, "dead"*/)) {
                DEBUG_PRINTLN(F("INFO: The client is successfully connected to the MQTT broker"));
                publishToMQTT(MQTT_STATUS_TOPIC, "alive");

#if defined(MQTT_HOME_ASSISTANT_SUPPORT)
                // MQTT discovery for Home Assistant
                JsonObject &root = staticJsonBuffer.createObject();
                root["name"] = FRIENDLY_NAME;
                root["platform"] = "mqtt_json";
                root["state_topic"] = MQTT_STATE_TOPIC;
                root["command_topic"] = MQTT_COMMAND_TOPIC;
                root["brightness"] = true;
                root["rgb"] = true;
                root["color_temp"] = true;
                root["effect"] = true;
                root["effect_list"] = EFFECT_LIST;
                root.printTo(jsonBuffer, sizeof(jsonBuffer));
                publishToMQTT(MQTT_CONFIG_TOPIC, jsonBuffer);
#endif

                subscribeToMQTT(MQTT_COMMAND_TOPIC);
            }
            else {
                DEBUG_PRINTLN(F("ERROR: The connection to the MQTT broker failed"));
                DEBUG_PRINT(F("INFO: MQTT username: "));
                DEBUG_PRINTLN(MQTT_USERNAME);
                DEBUG_PRINT(F("INFO: MQTT password: "));
                DEBUG_PRINTLN(MQTT_PASSWORD);
                DEBUG_PRINT(F("INFO: MQTT broker: "));
                DEBUG_PRINTLN(MQTT_SERVER);
            }
            lastMQTTConnection = millis();
        }
    }
}

///////////////////////////////////////////////////////////////////////////
//  SETUP() AND LOOP()
///////////////////////////////////////////////////////////////////////////

void setup()
{
#if defined(DEBUG_SERIAL)
    Serial.begin(9600);
#elif defined(DEBUG_TELNET)
    telnetServer.begin();
    telnetServer.setNoDelay(true);
#endif

    setupWiFi();

    sprintf(MQTT_CLIENT_ID, "%06X", ESP.getChipId());
#if defined(MQTT_HOME_ASSISTANT_SUPPORT)
    sprintf(MQTT_CONFIG_TOPIC, MQTT_CONFIG_TOPIC_TEMPLATE, MQTT_HOME_ASSISTANT_DISCOVERY_PREFIX, MQTT_CLIENT_ID);
    DEBUG_PRINT(F("INFO: MQTT config topic: "));
    DEBUG_PRINTLN(MQTT_CONFIG_TOPIC);
#else

#endif

    sprintf(MQTT_STATE_TOPIC, MQTT_STATE_TOPIC_TEMPLATE, MQTT_CLIENT_ID);
    sprintf(MQTT_COMMAND_TOPIC, MQTT_COMMAND_TOPIC_TEMPLATE, MQTT_CLIENT_ID);
    sprintf(MQTT_STATUS_TOPIC, MQTT_STATUS_TOPIC_TEMPLATE, MQTT_CLIENT_ID);

    DEBUG_PRINT(F("INFO: MQTT state topic: "));
    DEBUG_PRINTLN(MQTT_STATE_TOPIC);
    DEBUG_PRINT(F("INFO: MQTT command topic: "));
    DEBUG_PRINTLN(MQTT_COMMAND_TOPIC);
    DEBUG_PRINT(F("INFO: MQTT status topic: "));
    DEBUG_PRINTLN(MQTT_STATUS_TOPIC);

    mqttClient.setServer(MQTT_SERVER, MQTT_SERVER_PORT);
    mqttClient.setCallback(handleMQTTMessage);

    connectToMQTT();

    switchDev.init();
}

void loop()
{
#if defined(DEBUG_TELNET)
    // handle the Telnet connection
    handleTelnet();
#endif

    yield();

    connectToMQTT();
    mqttClient.loop();

    yield();
    if (switchDev.loop()) {
        if (switchDev.getState()) {
            publishToMQTT(MQTT_STATE_TOPIC, STATE_ON_SIGNAL);
        }
        else {
            publishToMQTT(MQTT_STATE_TOPIC, STATE_OFF_SIGNAL);
        }
    }

}