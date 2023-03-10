// 
const char* mqtt_password = "your_mqtt_password";
const char* mqtt_topic = "your_mqtt_topic";
const char* mqtt_server = "your_mqtt_broker_ip_address";
const char* mqtt_user = "your_mqtt_username";

// OUTPUT
#define VERBOSE
// #define DEBUG
// #define GD0 2 // Disable for new method without GD0
#define CC_FREQ 868.32
#define CC_POWER 12
#define CC_DELAY 100 // [100]

// Type
#define SEND_CHAR
// #define SEND_BYTE

// Sensor pins
#define SENSOR_PIN_SDA 0
#define SENSOR_PIN_SDC 2
#ifdef SENSOR_TYPE_ds18b20
#define SENSOR_PIN_OW 3
#endif
#ifdef SENSOR_TYPE_pir
#define SENSOR_PIN_PIR 3
#endif

// GIT
#ifndef GIT_VERSION
#define GIT_VERSION "unknown"
#endif