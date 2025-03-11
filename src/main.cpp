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
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"

// Servo libraries
#include <ESP32Servo.h>

// Moving average library
#include <movingAvg.h> // https://github.com/JChristensen/movingAvg

// STATE
String STATE = "INIT";
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

// MQTT initialization
#define BROKER_NAME "test.mosquitto.org"
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
Adafruit_BMP3XX bmp;
#define SEALEVELPRESSURE_HPA (1013.25)
float temperature;
float pressure;
int altitude;
float altitude_float;

// Servo initialization
Servo myservo;
int openedServoPosition = 19;
int closedServoPosition = 95;
int servoPin = 1; // GPIO1

// Before the flight eject pin
int ejectPin = 2;
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

// Landing detection
#define LIST_SIZE 10
int altitudeResults[LIST_SIZE] = {0};
int currentIndex = 0;

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
    if (bmp.begin_I2C(BMP3XX_DEFAULT_ADDRESS, &Wire))
    { // hardware I2C mode, can pass in address & alt Wire
        Serial.println("BMP Status: OK");
        // Set up oversampling and filter initialization
        bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
        bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
        bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
        bmp.setOutputDataRate(BMP3_ODR_50_HZ);
    }
    else
    {
        Serial.println("BMP Status: FAIL");
    }

    // Spit of garbage data
    for (int i = 0; i <= 50; i++)
    {
        altitude = bmp.readAltitude(SEALEVELPRESSURE_HPA);
        if (!bmp.performReading())
        {
            return;
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

bool A9GSetup()
{
    Serial0.begin(115200); // Serial0 is the hardware serial port on the A9G
    delay(100);
    gsm.init(&Serial0);
    gsm.EventDispatch(A9G_eventDispatch);

    // Maximum of 2 seconds to wait for the A9G to be ready
    if (gsm.bIsReady())
    {
        Serial.println("A9G Status: OK");
        return true;
    }
    else
    {
        Serial.println("A9G Status: FAIL");
        return false;
    }
}

void GPSSetup()
{
    if (gsm.ReleaseGPSUart())
    {
        gps.begin(GPS_BAUD);

        Serial.println("GPS Status: OK");
    }
    else
    {
        Serial.println("GPS Status: FAIL");
    }
}

void MQTTSetup()
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
    setupSD();
    Wire.begin();
    Wire.setClock(400000); // 400khz clock
    setupBMP();

    // Wait for A9G to be start up after power up
    Serial.println("Waiting for A9G to start up...");
    while (millis() < 20000)
    {
        Serial.print(".");
        delay(1000);
    }

    if (A9GSetup())
    {
        Serial.println("***********Read CSQ ***********");
        gsm.ReadCSQ();

        MQTTSetup();

        GPSSetup();
    }
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
    temperature = bmp.temperature;
    pressure = bmp.pressure;

    altitude = bmp.readAltitude(SEALEVELPRESSURE_HPA);
    altitude_float = bmp.readAltitude(SEALEVELPRESSURE_HPA);
}

void detectState()
{
    // Read the eject pin state
    if (STATE == "INIT" || STATE == "READY")
    {
        ejectPinState = digitalRead(ejectPin);

        if (ejectPinState == 1 && STATE == "INIT")
        {
            STATE = "READY";
            STATE_NUMBER = 2;
            myservo.write(closedServoPosition);
        }
        if (ejectPinState == 0 && STATE == "READY")
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
        altitude = bmp.readAltitude(SEALEVELPRESSURE_HPA);
        launchThreshold += altitude;
        launchThresholdONCE = true;

        for (int i = 0; i <= 10; i++)
        {
            altitude = bmp.readAltitude(SEALEVELPRESSURE_HPA);
            avgAltitude.reading(altitude);
        }
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
}

void getData()
{
    getGPSdata();
    getBMPdata();
}

void sendData()
{
    currentSendingTime = millis();
    unsigned long sendingTimeDiff = currentSendingTime - lastSendingTime;
    if (sendingTimeDiff >= sendingInterval)
    {
        lastSendingTime = currentSendingTime;

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

        Serial.println(csv_header);
        Serial.println(csv_line);

        Serial.println(STATE);

        // gsm.PublishToTopicNoWait(PUB_TOPIC, csv_line);
    }
}

void loop()
{
    getData();     // Update data from sensors
    detectState(); // Detect the current state of the rocket
    sendData();    // Send data to MQTT broker
}