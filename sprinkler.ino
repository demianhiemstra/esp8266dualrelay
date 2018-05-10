#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define MQTT_VERSION MQTT_VERSION_3_1_1

// Wifi: SSID and password
const char* WIFI_SSID = "";
const char* WIFI_PASSWORD = "";

// MQTT: ID, server IP, port, username and password
const PROGMEM char* MQTT_CLIENT_ID = "garden_sprinkler";
const PROGMEM char* MQTT_SERVER_IP = "10.0.0.10";
const PROGMEM uint16_t MQTT_SERVER_PORT = 1883;
const PROGMEM char* MQTT_USER = "homeassistant";
const PROGMEM char* MQTT_PASSWORD = "homeassistant";

// MQTT: topics
// state
const String MQTT_RELAY = "garden/sprinkler";
const String MQTT_RELAY_STATE_TOPIC = "/sprinkler/status";
const String MQTT_RELAY_COMMAND_TOPIC = "/sprinkler/switch";

// payloads by default (on/off)
const char* RELAY_ON = "ON";
const char* RELAY_OFF = "OFF";

// variables used to store the state
boolean m_relay_state[2] = {0,0};

const byte relayOneOn[] = {0xA0, 0x01, 0x01, 0xA2 };
const byte relayOneOff[] = {0xA0, 0x01, 0x00, 0xA1 };
const byte relayTwoOn[] = {0xA0, 0x02, 0x01, 0xA3 };
const byte relayTwoOff[] = {0xA0, 0x02, 0x00, 0xA2 };

// buffer used to send/receive data with MQTT
const uint8_t MSG_BUFFER_SIZE = 20;
char m_msg_buffer[MSG_BUFFER_SIZE];

WiFiClient wifiClient;
PubSubClient client(wifiClient);

char* string2char(String command){
    if(command.length()!=0){
        char *p = const_cast<char*>(command.c_str());
        return p;
    }
}

// function called to publish the state of the led (on/off)
void publishRelayState(uint8_t relay) {
  if (m_relay_state[relay]) {
    client.publish(string2char(MQTT_RELAY + relay + MQTT_RELAY_STATE_TOPIC), RELAY_ON, true);
  } else {
    client.publish(string2char(MQTT_RELAY + relay + MQTT_RELAY_STATE_TOPIC), RELAY_OFF, true);
  }
}

void commandHandler(uint8_t relay, String payload) {
  if (payload.equals(String(RELAY_ON))) {
    if (m_relay_state[relay] != true) {
      m_relay_state[relay] = true;
      switch(relay) {
        case 0:
          Serial.write(relayOneOn, sizeof(relayOneOn));
          break;
         case 1:
          Serial.write(relayTwoOn, sizeof(relayTwoOn));
          break;
      }
      publishRelayState(relay);
    }
  } else if (payload.equals(String(RELAY_OFF))) {
    if (m_relay_state[relay] != false) {
      m_relay_state[relay] = false;
      switch(relay) {
        case 0:
          Serial.write(relayOneOff, sizeof(relayOneOff));
          break;
         case 1:
          Serial.write(relayTwoOff, sizeof(relayTwoOff));
          break;
      }
      publishRelayState(relay);
    }
  } 
}

// function called when a MQTT message arrived
void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
  // concat the payload into a string
  String payload;
  for (uint8_t i = 0; i < p_length; i++) {
    payload.concat((char)p_payload[i]);
  }
  Serial.println("payload: " + payload);
  for (uint8_t relay = 0; relay < sizeof(m_relay_state); relay++) {
    // handle message topic
    if (String(MQTT_RELAY + relay + MQTT_RELAY_COMMAND_TOPIC).equals(p_topic)) {
      // test if the payload is equal to "ON" or "OFF"
      commandHandler(relay, payload);
      break;
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("INFO: Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("INFO: connected");
      
      // Once connected, publish an announcement...
      for (uint8_t relay = 0; relay < sizeof(m_relay_state); relay++) {
        // ... and resubscribe
        client.subscribe(string2char(MQTT_RELAY + relay + MQTT_RELAY_COMMAND_TOPIC));
        client.loop();

        // publish the initial values
        publishRelayState(relay);
      }
    } else {
      Serial.print("ERROR: failed, rc=");
      Serial.print(client.state());
      Serial.println("DEBUG: try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // init the serial
  Serial.begin(115200);

  // init the WiFi connection
  Serial.println();
  Serial.println();
  Serial.print("INFO: Connecting to ");
  WiFi.mode(WIFI_STA);
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("INFO: WiFi connected");
  Serial.print("INFO: IP address: ");
  Serial.println(WiFi.localIP());

  // init the MQTT connection
  client.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  client.setCallback(callback);
}

void loop() {  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
