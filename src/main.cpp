#include <Arduino.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#include "credentials.h"

// Edit credentials.h

const char* mqtt_server = "your_mqtt_broker_ip_address";
const char* mqtt_user = "your_mqtt_username";
const char* mqtt_password = "your_mqtt_password";
const char* mqtt_topic = "your_mqtt_topic";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// cc1101
const uint8_t byteArrSize = 61;

#ifdef VERBOSE
// one minute mark
#define MARK
#define INTERVAL_1MIN (1 * 60 * 1000L)
unsigned long lastMillis = 0L;
uint32_t countMsg = 0;
#endif

// supplementary functions
#ifdef MARK
void printMARK()
{
  if (countMsg == 0)
  {
    Serial.println(F("> [MARK] Starting... OK"));
    countMsg++;
  }
  if (millis() - lastMillis >= INTERVAL_1MIN)
  {
    Serial.print(F("> [MARK] Uptime: "));
    Serial.print(countMsg);
    Serial.println(F(" min"));
    countMsg++;
    lastMillis += INTERVAL_1MIN;
  }
}
#endif

// Last 4 digits of ChipID
int getUniqueID()
{
  int uid = 0;
  // read EEPROM serial number
  int address = 13;
  int serialNumber;
  if (EEPROM.read(address) != 255)
  {
    EEPROM.get(address, serialNumber);
    uid = serialNumber;
  }
  return uid;
}

void setup()
{
  Serial.begin(9600);
  delay(10);
#ifdef VERBOSE
  delay(5000);
#endif
  // Start Boot
  Serial.println(F("> "));
  Serial.println(F("> "));
  Serial.print(F("> Booting... Compiled: "));
  Serial.println(GIT_VERSION);
  Serial.print(F("> Node ID: "));
  Serial.println(String(getUniqueID(), HEX));
#ifdef VERBOSE
  Serial.print(("> Mode: "));
#ifdef GD0
  Serial.print(F("GD0 "));
#endif
  Serial.print(F("VERBOSE "));
#ifdef DEBUG
  Serial.print(F("DEBUG"));
#endif
  Serial.println();
#endif

  // Start CC1101
  Serial.print(F("> [CC1101] Initializing... "));
  int cc_state = ELECHOUSE_cc1101.getCC1101();
  if (cc_state)
  {
    Serial.println(F("OK"));
    ELECHOUSE_cc1101.Init(); // must be set to initialize the cc1101!
#ifdef GD0
    ELECHOUSE_cc1101.setGDO0(GD0); // set lib internal gdo pin (gdo0). Gdo2 not use for this example.
#endif
    ELECHOUSE_cc1101.setCCMode(1);
    ELECHOUSE_cc1101.setModulation(0);
    ELECHOUSE_cc1101.setMHZ(CC_FREQ);
    // ELECHOUSE_cc1101.setPA(CC_POWER);
    ELECHOUSE_cc1101.setSyncMode(2); 
    ELECHOUSE_cc1101.setCrc(1);
    ELECHOUSE_cc1101.setCRC_AF(1); 
    // ELECHOUSE_cc1101.setAdrChk(1);
    // ELECHOUSE_cc1101.setAddr(0);
  }
  else
  {
    Serial.print(F("ERR "));
    Serial.println(cc_state);
    while (true)
      ;
  }
  // Start WiFi
  Serial.print(F("> [WiFi] Initializing... "));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("OK"));
  }
  // Start MQTT
  Serial.print(F("> [MQTT] Initializing... "));
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(mqttCallback);
  while (!mqttClient.connected()) {
    if (mqttClient.connect("ESP32Client", mqtt_user, mqtt_password)) {
      mqttClient.subscribe(mqtt_topic);
    }
    Serial.println(F("OK"));
  }
}

void loop()
{
#ifdef MARK
  printMARK();
#endif

#ifdef GD0
  if (ELECHOUSE_cc1101.CheckReceiveFlag())
#else
  if (ELECHOUSE_cc1101.CheckRxFifo(CC_DELAY))
#endif
  {
    byte byteArr[byteArrSize] = {0};
#ifdef DEBUG
    Serial.print(F("> [CC1101] Receive... "));
#endif
    if (ELECHOUSE_cc1101.CheckCRC())
    {
#ifdef VERBOSE
#ifndef DEBUG
      Serial.print(F("> [CC1101] Receive... "));
#endif
#endif
#ifdef VERBOSE
      Serial.print(F("CRC "));
#endif
      int byteArrLen = ELECHOUSE_cc1101.ReceiveData(byteArr);
      byteArr[byteArrLen] = '\0'; // 0, \0
#ifdef VERBOSE
      Serial.println(F("OK"));
      Serial.print(F("> [CC1101] Length: "));
      Serial.println(byteArrLen);
#endif
      StaticJsonDocument<200> doc;
      doc["data"] = rx_buf;
      String jsonString;
      serializeJson(doc, jsonString);
      Serial.println(jsonString);
      mqttClient.publish(mqtt_topic, jsonString.c_str());
      
      for (uint8_t i = 0; i < byteArrLen; i++)
      {
        // Filter [0-9A-Za-z,:]
        if ((byteArr[i] >= '0' && byteArr[i] <= '9') ||
            (byteArr[i] >= 'A' && byteArr[i] <= 'Z') ||
            (byteArr[i] >= 'a' && byteArr[i] <= 'z') ||
            byteArr[i] == ',' || byteArr[i] == ':' || byteArr[i] == '-')
        {
          Serial.print((char)byteArr[i]);
        }
      }
      Serial.print(F(",RSSI:"));
      Serial.print(ELECHOUSE_cc1101.getRssi());
      Serial.print(F(",LQI:"));
      Serial.print(ELECHOUSE_cc1101.getLqi());
      Serial.print(F(",RN:"));
      Serial.println(String(getUniqueID(), HEX));
#ifdef VERBOSE_FW
      Serial.print(F(",RF:"));
      Serial.println(String(GIT_VERSION_SHORT));
#endif
    }
#ifdef DEBUG
    else
    {
#ifdef DEBUG_CRC
      for (uint8_t i = 0; i < byteArrSize; i++)
      {
        Serial.print((char)byteArr[i]);
      }
      Serial.print(F(",RSSI:"));
      Serial.print(ELECHOUSE_cc1101.getRssi());
      Serial.print(F(",LQI:"));
      Serial.print(ELECHOUSE_cc1101.getLqi());
      Serial.print(F(",RN:"));
      Serial.print(String(getUniqueID(), HEX));
      Serial.print(F(",RF:"));
      Serial.println(String(GIT_VERSION_SHORT));
#endif
    }
#endif
  }
  mqttClient.loop();
}
