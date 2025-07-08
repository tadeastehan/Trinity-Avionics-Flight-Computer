#define SEALEVELPRESSURE_HPA (1013.25)

// MQTT initialization
#define BROKER_NAME "broker.mqtt.cool"
#define PORT 1883
#define UNIQUE_ID "roocket"
#define PUB_TOPIC "iotready/gprs"
#define SUB_TOPIC "iotready/gprs"
#define CLEAN_SEASSION 0
#define KEEP_ALIVE 120

#define RBF_PIN 7 // GPIO7

#define BUZZER_PIN 5          // GPIO5
#define BUZZING_INTERVAL 1000 // Buzzer interval in milliseconds

#define BATTERY_PIN 6                // GPIO6
#define BATTERY_VOLTAGE_DIVIDER 4.13 // Voltage divider ratio (R1 + R2) / R2

#define SERVO_PIN 2 // GPIO2

#define SERVO_OPENED_POSITION 10
#define SERVO_CLOSED_POSITION 57

#define LAUCH_THRESHOLD 5   // Minimum altitude change to detect launch
#define DESCENT_THRESHOLD 3 // Threshold to detect descent

#define SD_CARD_WRITE_INTERVAL 10            // SD card write interval in milliseconds
#define BATTERY_VOLTAGE_UPDATE_INTERVAL 5000 // Battery voltage update interval in milliseconds
#define SERIAL_UPDATE_INTERVAL 5000          // Serial update interval in milliseconds
#define MQTT_UPDATE_INTERVAL 1000            // MQTT update interval in milliseconds