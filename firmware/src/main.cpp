#include <Arduino.h>

// SD card libraries
#include <SPI.h>
#include "SdFat.h"

// A9G libraries
#include <SoftwareSerial.h>
#include <A9G.h>
#include <MicroNMEA.h>

// BMP libraries
#include <Wire.h>
#include "SparkFunBMP384.h"

// Servo libraries
#include <ESP32Servo.h>

// Moving average library
#include <movingAvg.h> // https://github.com/JChristensen/movingAvg

// STATE
String STATE = "INIT";
String LAST_STATE = "INIT";
int STATE_NUMBER = 1;

// SD card initialization
#define SD_CS_PIN SS
SdFat SD;
int fileCounter = 1;
String fileName = "data_0.csv";
File myFile;
String csv_header = "Time,Altitude,Latitude,Longitude,Pressure,Temp,aX,aY,aZ,gX,gY,gZ,acX,acY,acZ";

// A9G initialization
GSM gsm(1);

// GPS initialization
#define GPS_RXPIN 0  // GPIO0
#define GPS_TXPIN -1 // unused
#define GPS_BAUD 9600
SoftwareSerial gps(GPS_RXPIN, GPS_TXPIN);
char nmeaBuffer[100];
MicroNMEA nmea(nmeaBuffer, sizeof(nmeaBuffer));
long latitude, longitude;
bool gpsFix = false;

#define GPS_EN 1 // GPIO1
unsigned long SetupA9GInterval = 20000;
unsigned long LastSetupA9GTime = 0;
bool A9GRunning = false;

// Wait 20 seconds before setup (non-blocking)
unsigned long setupStartTime = 0;
bool setupInProgress = false;
bool resetInProgress = false;     // New flag for reset phase
unsigned long resetStartTime = 0; // New timestamp for reset phase

// MQTT initialization
#define BROKER_NAME "broker.hivemq.com"
#define PORT 1883
#define USER ""
#define PASS ""
#define UNIQUE_ID "roocket"
#define PUB_TOPIC "iotready/gprs"
#define SUB_TOPIC "iotready/gprs"
#define CLEAN_SEASSION 0
#define KEEP_ALIVE 120

// Sending interval
unsigned long lastSendingTime = 0;
unsigned long currentSendingTime = 0;
int sendingIntervalSlow = 1000;
int sendingIntervalFast = 50;
unsigned long sendingInterval = sendingIntervalSlow;

// BMP init

BMP384 pressureSensor;
uint8_t i2cAddress = BMP384_I2C_ADDRESS_DEFAULT; // 0x77
bmp3_data data;

#define SEALEVELPRESSURE_HPA (1013.25)
float temperature;
float pressure;
int altitude;

// Servo initialization
Servo myservo;
int openedServoPosition = 19;
int closedServoPosition = 95;
int servoPin = 2; // GPIO1

// Before the flight eject pin
int ejectPin = 7;
bool ejectPinState; // 0 = closed    1 = open

unsigned long lastMeasurementTime = 0;
unsigned long currentTime = 0;

unsigned long loopCounter = 0;

// Apagee detection
movingAvg avgAltitude(10);
int avgAltitude_result;
int maxAltitude = -10000;       // Initialize to a very low value
int minAltitude = 10000;        // Initialize to a very high value
const int descentThreshold = 3; // Threshold to detect descent

// Launch detection
int launchThreshold = 1; // Minimum altitude change to detect launch
bool launchThresholdONCE = false;
int launchAltitude = 0;

// Landing detection
#define LIST_SIZE 10
int altitudeResults[LIST_SIZE] = {0};
int currentIndex = 0;

// Battery voltage
#define BATTERY_PIN 6
#define BATTERY_VOLTAGE_DIVIDER 5.7 // Voltage divider ratio (R1 + R2) / R2
float batteryVoltage = 0.0;

// Buzzer
#define BUZZER_PIN 5
#define BUZZING_INTERVAL 1000 // Buzzer interval in milliseconds
static unsigned long lastTimeBuzzer = 0;
static unsigned long currentTimeBuzzer = 0;
static bool buzzerState = LOW;

float calculateAltitude(float seaLevel, float atmospheric_pressure)
{
    float atmospheric = atmospheric_pressure / 100.0F;
    return 44330.0 * (1.0 - pow(atmospheric / seaLevel, 0.1903));
}

void setupBuzzer()
{
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, buzzerState); // Turn off the buzzer initially
}

void buzz()
{
    currentTimeBuzzer = millis();
    if (currentTimeBuzzer - lastTimeBuzzer >= BUZZING_INTERVAL)
    {
        lastTimeBuzzer = currentTimeBuzzer;
        buzzerState = !buzzerState; // Toggle buzzer state
        digitalWrite(BUZZER_PIN, buzzerState);
    }
}

void setupBattery()
{
    pinMode(BATTERY_PIN, INPUT);
}

void setupSD()
{
    if (SD.begin(SD_CS_PIN))
    {
        while (SD.exists(fileName))
        {
            fileName = "data_" + String(fileCounter) + ".csv";
            fileCounter++;
            Serial.println(fileName);
        }
        myFile = SD.open(fileName, FILE_WRITE);
        if (myFile)
        {
            myFile.print(csv_header);
            myFile.print("\n");
            myFile.close();

            Serial.println("SD Status: OK");
        }
        else
        {
            Serial.println("SD Status: FAIL");
        }
    }
    else
    {
        Serial.println("SD Status: FAIL");
    }
}

void setupBMP()
{
    if (pressureSensor.beginI2C(i2cAddress) == BMP3_OK)
    { // hardware I2C mode, can pass in address & alt Wire
        Serial.println("BMP Status: OK");

        int8_t err = BMP3_OK;

        bmp3_odr_filter_settings osrMultipliers =
            {
                .press_os = BMP3_OVERSAMPLING_4X,
                .temp_os = BMP3_OVERSAMPLING_8X,
                .iir_filter = BMP3_IIR_FILTER_COEFF_3,
            };

        err = pressureSensor.setOSRMultipliers(osrMultipliers);
        if (err)
        {
            // Setting OSR failed, most likely an invalid multiplier (code -3)
            Serial.print("Error setting OSR! Error code: ");
            Serial.println(err);
        }
    }
    else
    {
        Serial.println("BMP Status: FAIL");
    }

    // Spit of garbage data
    for (int i = 0; i <= 50; i++)
    {
        int8_t err = pressureSensor.getSensorData(&data);

        // Check whether data was acquired successfully
        if (err == BMP3_OK)
        {
            altitude = calculateAltitude(SEALEVELPRESSURE_HPA, data.pressure);
        }
        else
        {
            // Acquisition failed, most likely a communication error (code -2)
            Serial.print("Error getting data from sensor! Error code: ");
            Serial.println(err);
        }
    }

    avgAltitude.begin();
}

void A9G_eventDispatch(A9G_Event_t *event)
{
    switch (event->id)
    {
    case EVENT_MQTTPUBLISH:
        Serial.print("Topic: ");
        Serial.println(event->topic);
        Serial.printf("message: %s\n", event->message);
        break;

    case EVENT_NEW_SMS_RECEIVED:
        Serial.print("Number: ");
        Serial.println(event->number);
        Serial.print("Message: ");
        Serial.println(event->message);
        Serial.print("Date & Time: ");
        Serial.println(event->date_time);
        break;

    case EVENT_CSQ:
        Serial.print("CSQ: ");
        Serial.println(event->param1);
        break;

    case EVENT_IMEI:
        Serial.print("IMEI: ");
        Serial.println(event->param2);
        break;

    case EVENT_CCID:
        Serial.print("CCID: ");
        Serial.println(event->param2);
        break;

    case EVENT_CME:
        Serial.print("CME ERROR Message:");
        gsm.errorPrintCME(event->error);
        break;

    case EVENT_CMS:
        Serial.print("CMS ERROR Message:");
        gsm.errorPrintCMS(event->error);
        break;

    default:
        break;
    }
}

void A9GInitialSetup()
{
    Serial0.begin(115200); // Serial0 is the hardware serial port on the A9G
    gsm.init(&Serial0);
    gsm.EventDispatch(A9G_eventDispatch);

    pinMode(GPS_EN, OUTPUT);
    digitalWrite(GPS_EN, HIGH);
}

bool GPSSetup()
{
    if (gsm.ReleaseGPSUart())
    {
        gps.begin(GPS_BAUD);

        Serial.println("GPS Status: OK");
        return true;
    }
    else
    {
        Serial.println("GPS Status: FAIL");
        return false;
    }
}

bool MQTTSetup()
{
    bool status = false;
    if (gsm.AttachToGPRS())
    {
        if (gsm.SetAPN("IP", "internet"))
        {
            if (gsm.ActivatePDP())
            {
                if (gsm.ConnectToBroker(BROKER_NAME, PORT, UNIQUE_ID, KEEP_ALIVE, CLEAN_SEASSION))
                {
                    if (gsm.PublishToTopic(PUB_TOPIC, "Hello IoT Started"))
                    {
                        status = true;
                        Serial.println("MQTT Status: OK");
                    }
                }
                else
                {
                    Serial.println("Broker Connect Fail");
                }
            }
            else
            {
                Serial.println("Activate PDP Fail");
            }
        }
        else
        {
            Serial.println("APN Set Fail");
        }
    }
    else
    {
        Serial.println("GPRS Attach Fail");
    }
    if (!status)
    {
        Serial.println("MQTT Status: FAIL");
    }
    return status;
}

void setupServo()
{
    // Allow allocation of all timers
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    myservo.setPeriodHertz(50);          // standard 50 hz servo
    myservo.attach(servoPin, 500, 2500); // 500us to 2500us

    myservo.write(openedServoPosition);
}

void setup()
{
    Serial.begin(115200);

    pinMode(ejectPin, INPUT_PULLUP);

    delay(2000);

    setupServo();
    setupBattery();
    setupBuzzer();
    setupSD();
    Wire.begin();
    Wire.setClock(400000); // 400khz clock
    setupBMP();

    A9GInitialSetup();
}

void getGPSdata()
{
    while (gps.available())
    {
        char c = gps.read();
        nmea.process(c);
    }

    latitude = nmea.getLatitude();
    longitude = nmea.getLongitude();
    gpsFix = nmea.isValid();
}

void getBMPdata()
{
    pressureSensor.getSensorData(&data);
    temperature = data.temperature;
    pressure = data.pressure;

    altitude = calculateAltitude(SEALEVELPRESSURE_HPA, data.pressure);
}

void getBatteryVoltage()
{
    uint32_t Vbatt = 0;
    for (int i = 0; i < 16; i++)
    {
        Vbatt += analogReadMilliVolts(BATTERY_PIN); // Read and accumulate ADC voltage
    }

    batteryVoltage = BATTERY_VOLTAGE_DIVIDER * (Vbatt / 16.0) / 1000.0; // Convert to voltage
}

void detectState()
{
    // Read the eject pin state
    if (STATE == "INIT" || STATE == "READY")
    {
        ejectPinState = digitalRead(ejectPin);

        if (ejectPinState == 0 && STATE == "INIT")
        {
            STATE = "READY";
            STATE_NUMBER = 2;
            myservo.write(closedServoPosition);
        }
        if (ejectPinState == 1 && STATE == "READY")
        {
            STATE = "ARM";
            STATE_NUMBER = 3;
        }
    }

    // When the rocket is armed, the launch threshold is set
    // to the current altitude and the average altitude is calculated
    if (STATE == "ARM" && !launchThresholdONCE)
    {
        Serial.println("ARMED - position zeroed");
        pressureSensor.getSensorData(&data);
        altitude = calculateAltitude(SEALEVELPRESSURE_HPA, data.pressure);
        launchThreshold += altitude;
        launchThresholdONCE = true;

        for (int i = 0; i <= 10; i++)
        {
            pressureSensor.getSensorData(&data);
            altitude = calculateAltitude(SEALEVELPRESSURE_HPA, data.pressure);
            avgAltitude.reading(altitude);
        }

        launchAltitude = avgAltitude.reading(altitude);
    }

    if (STATE == "ARM" && avgAltitude_result >= launchThreshold)
    {
        STATE = "ASCENT";
        STATE_NUMBER = 4;
    }

    if (STATE == "DEPLOY")
    {
        myservo.write(openedServoPosition);
        STATE = "DESCENT";
        STATE_NUMBER = 6;
    }

    if (STATE == "DESCENT")
    {
        loopCounter++;
    }

    if (STATE != "READY" || STATE != "INIT")
    {
        avgAltitude_result = avgAltitude.reading(altitude);
    }

    if (STATE == "DESCENT" && loopCounter >= 100)
    {
        loopCounter = 0;

        // Add the current avgAltitude_result to the list
        altitudeResults[currentIndex] = avgAltitude_result;
        currentIndex = (currentIndex + 1) % LIST_SIZE;

        // Check if all values in the list are the same
        bool allSame = true;
        for (int i = 1; i < LIST_SIZE; i++)
        {
            if (altitudeResults[i] != altitudeResults[0])
            {
                allSame = false;
                break;
            }
        }

        // Print a message if all values are the same
        if (allSame)
        {
            STATE = "LANDED";
            STATE_NUMBER = 7;
        }
    }

    // Update maximum altitude during ascent
    if (STATE == "ASCENT" && avgAltitude_result > maxAltitude)
    {
        maxAltitude = avgAltitude_result;
    }

    // Check for transition from ascent to descent
    if (STATE == "ASCENT" && avgAltitude_result <= (maxAltitude - descentThreshold))
    {
        STATE = "DEPLOY";
        STATE_NUMBER = 5;
    }

    if (STATE == "DESCENT" && avgAltitude_result < minAltitude)
    {
        minAltitude = avgAltitude_result;
    }

    if (STATE == "DESCENT" or STATE == "LANDED")
    {
        buzz();
    }

    if (STATE != LAST_STATE)
    {
        if (STATE == "ASCENT" or STATE == "DESCENT" or STATE == "DEPLOY")
        {
            sendingInterval = sendingIntervalFast;
        }
        else
        {
            sendingInterval = sendingIntervalSlow; // READY, LANDED
        }

        Serial.print("STATE changed: ");
        Serial.println(STATE);
        LAST_STATE = STATE;
    }
}

void getData()
{
    getGPSdata();
    getBMPdata();
    getBatteryVoltage();
}

void sendData()
{
    currentSendingTime = millis();
    unsigned long sendingTimeDiff = currentSendingTime - lastSendingTime;
    if (sendingTimeDiff >= sendingInterval)
    {
        lastSendingTime = currentSendingTime;
        // Serial.print("Battery Voltage: ");
        // Serial.println(batteryVoltage);
        // Serial.print("Latitude: ");
        // Serial.print(latitude / 1000000., 6);
        // Serial.print(" Longitude: ");
        // Serial.print(longitude / 1000000., 6);
        // Serial.print(" GPS Fix: ");
        // Serial.print(gpsFix ? "OK" : "NO FIX");
        // Serial.print(" Temperature: ");
        // Serial.print(temperature);
        // Serial.print(" Pressure: ");
        // Serial.print(pressure);
        // Serial.print(" Altitude: ");
        // Serial.print(altitude);
        // Serial.println();

        char csv_line[200];
        sprintf(csv_line, "%lu,%d,%.6f,%.6f,%.2f,%.2f,0,0,0,0,0,0,0,0,0", sendingTimeDiff, altitude, latitude / 1000000., longitude / 1000000., pressure, temperature);

        Serial.println(">Time:" + String(sendingTimeDiff) + "," + "Altitude:" + String(altitude) + "," + "Latitude:" + String(latitude / 1000000., 6) + "," + "Longitude:" + String(longitude / 1000000., 6) + "," + "Pressure:" + String(pressure) + "," + "Temperature:" + String(temperature) + "," + "State:" + STATE + "," + "State Number:" + String(STATE_NUMBER));
        // Serial.println(csv_header);
        // Serial.println(csv_line);
        gsm.PublishToTopicNoWait(PUB_TOPIC, csv_line);
    }
}

void checkA9G()
{
    currentTime = millis();
    // Check if it's time to check the A9G status
    if (currentTime - LastSetupA9GTime > SetupA9GInterval && !resetInProgress && !setupInProgress)
    {
        LastSetupA9GTime = currentTime;
        A9GRunning = gsm.bIsReady();
    }

    // If A9G is not running, start the reset and setup phases
    if (!A9GRunning)
    {
        // If we're not in reset or setup phase, start the reset phase
        if (!resetInProgress && !setupInProgress)
        {
            Serial.println("A9G is not running, starting reset procedure...");
            digitalWrite(GPS_EN, LOW); // Pull GPS_EN LOW
            resetStartTime = millis();
            resetInProgress = true;
        }

        // If we're in reset phase and 5 seconds have passed, start setup phase
        if (resetInProgress && (millis() - resetStartTime >= 5000))
        {
            Serial.println("Reset phase complete, pulling GPS_EN HIGH");
            digitalWrite(GPS_EN, HIGH); // Pull GPS_EN HIGH
            resetInProgress = false;
            setupStartTime = millis();
            setupInProgress = true;
            Serial.println("A9G setup starting, waiting 20 seconds...");
        }
    }

    // Check if 20 seconds have passed since setup began
    if (setupInProgress && (millis() - setupStartTime >= 20000))
    {
        Serial.println("Setting up A9G after wait...");

        // Reset the flag for next time
        setupInProgress = false;

        // Try to initialize A9G
        if (gsm.bIsReady())
        {
            Serial.println("A9G is now ready");
            Serial.println("***********Read CSQ ***********");
            gsm.ReadCSQ();

            // Setup MQTT and GPS
            if (MQTTSetup() && GPSSetup())
            {
                Serial.println("MQTT and GPS setup complete");
                A9GRunning = true;
            }
            else
            {
                Serial.println("A9G setup failed - MQTT or GPS setup failed");
            }
        }
        else
        {
            Serial.println("A9G setup failed");
        }
    }
}

void loop()
{
    getData();     // Update data from sensors
    detectState(); // Detect the current state of the rocket
    sendData();    // Send data to MQTT broker
    checkA9G();
}