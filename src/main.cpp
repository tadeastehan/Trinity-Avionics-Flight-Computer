#include <Arduino.h>

// SD card libraries
#include <SPI.h>
#include "SdFat.h"

// A9G libraries
#include <SoftwareSerial.h>
#include <A9G.h>
#include <MicroNMEA.h>

unsigned long previousMillis = 0;
const long interval = 1000;

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

void setup()
{
    Serial.begin(115200);

    delay(2000);

    setupSD();

    // Wait for A9G to be start up after power up
    while (millis() < 20000)
    {
        yield();
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

void loop()
{
    getGPSdata();

    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis > interval)
    {
        Serial.print("Latitude: ");
        Serial.println(latitude / 1000000., 6);
        Serial.print("Longitude: ");
        Serial.print(longitude / 1000000., 6);
        Serial.print("GPS Fix: ");
        Serial.print(gpsFix ? "OK" : "NO FIX");
        Serial.println();

        previousMillis = currentMillis;
    }
}