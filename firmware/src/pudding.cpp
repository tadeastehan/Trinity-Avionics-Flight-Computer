#include <SoftwareSerial.h>
#include "pudding.h"
#include "A9G.h"
#include <MicroNMEA.h>

#include <env.h>

// Pudding dev module
#define GPS_EN 1 // GPIO1
// A9G initialization
GSM puddingUart(1);

// GPS initialization
#define GPS_RXPIN 0  // GPIO0
#define GPS_TXPIN -1 // unused
#define GPS_BAUD 9600
char nmeaBuffer[100];
MicroNMEA nmea(nmeaBuffer, sizeof(nmeaBuffer));

nmeaData gpsData = {0, 0, false};

SoftwareSerial puddingGPS(GPS_RXPIN, GPS_TXPIN);

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
        Serial.println(event->error);
        break;

    case EVENT_CMS:
        Serial.print("CMS ERROR Message:");
        Serial.println(event->error);
        break;

    default:
        break;
    }
}

nmeaData getGPSdata()
{
    return gpsData;
}

void updateGPS()
{
    if (puddingGPS.available())
    {
        char c = puddingGPS.read();
        if (nmea.process(c))
        {
            gpsData.latitude = nmea.getLatitude();
            gpsData.longitude = nmea.getLongitude();
            gpsData.gpsFix = nmea.isValid();
        }
    }
}

void puddingUartInit()
{
    Serial0.begin(115200);
    puddingUart.init(&Serial0);
    puddingUart.EventDispatch(A9G_eventDispatch);

    pinMode(GPS_EN, OUTPUT);
    digitalWrite(GPS_EN, HIGH);
    Serial.println("A9G: GPS Enabled");
    puddingUartReset();
}

void puddingUartReset()
{
    Serial.println("A9G: Resetting...");
    digitalWrite(GPS_EN, LOW);  // Pull GPS_EN LOW
    delay(5000);                // Wait for 1 second
    digitalWrite(GPS_EN, HIGH); // Pull GPS_EN HIGH
    delay(20000);               // Wait for 20 seconds to allow A9G to reset and initialize
    Serial.println("A9G: Reset Complete");
}

bool puddingMQTTSetup()
{
    Serial.println("-------MQTT Setup-------");
    unsigned long startTime = millis();
    while (1)
    {
        if (puddingUart.bIsReady())
        {
            Serial.println("A9G is now ready");
            break;
        }
        else
        {
            delay(5);
            if (millis() - startTime > 2000)
            {
                Serial.println("A9G: FAIL");
                return false;
            }
        }
    }
    startTime = millis();
    while (1)
    {
        if (puddingUart.SupportedNetworkSettings())
        {
            Serial.println("A9G: Supported Network Settings");
            break;
        }
        else
        {
            delay(5);
            if (millis() - startTime > 2000)
            {
                Serial.println("A9G: FAIL");
                return false;
            }
        }
    }
    puddingUart.ReadCCID();

    startTime = millis();
    while (1)
    {
        if (puddingUart.AttachToGPRS())
        {
            Serial.println("A9G: Attach to GPRS");
            break;
        }
        else
        {
            delay(5);
            if (millis() - startTime > 2000)
            {
                Serial.println("A9G: FAIL");
                return false;
            }
        }
    }
    startTime = millis();
    while (1)
    {
        if (puddingUart.SetAPN("IP", "internet"))
        {
            Serial.println("A9G: Set APN");
            break;
        }
        else
        {
            delay(5);
            if (millis() - startTime > 2000)
            {
                Serial.println("A9G: FAIL");
                return false;
            }
        }
    }
    startTime = millis();
    while (1)
    {
        if (puddingUart.ActivatePDP())
        {
            Serial.println("A9G: Activate PDP");
            break;
        }
        else
        {
            delay(5);
            if (millis() - startTime > 2000)
            {
                Serial.println("A9G: FAIL");
                return false;
            }
        }
    }
    startTime = millis();
    while (1)
    {
        if (puddingUart.ConnectToBroker(BROKER_NAME, PORT, UNIQUE_ID, KEEP_ALIVE, CLEAN_SEASSION))
        {
            Serial.println("A9G: Connect to Broker");
            break;
        }
        else
        {
            delay(5);
            if (millis() - startTime > 2000)
            {
                Serial.println("A9G: FAIL");
                return false;
            }
        }
    }
    puddingUart.PublishToTopic(PUB_TOPIC, "Hello IoT Started");
    return true;
}

bool puddingGPSSetup()
{
    unsigned long startTime = millis();
    while (1)
    {
        if (puddingUart.bIsReady())
        {
            Serial.println("A9G is now ready");
            break;
        }
        else
        {
            delay(5);
            if (millis() - startTime > 2000)
            {
                Serial.println("A9G: FAIL");
                return false;
            }
        }
    }
    startTime = millis();
    while (1)
    {
        if (puddingUart.ReleaseGPSUart())
        {
            puddingGPSInit();
            Serial.println("GPS Status S: OK");
            break;
        }
        else
        {
            delay(5);
            if (millis() - startTime > 2000)
            {
                Serial.println("GPS Status S: FAIL");
                return false;
            }
        }
    }
    return true;
}

void puddingGPSInit()
{
    puddingGPS.begin(GPS_BAUD);
}

bool puddingSetup()
{
    puddingUartInit();
    bool status = true;
    if (puddingGPSSetup())
    {
        Serial.println("GPS Status: OK");
    }
    else
    {
        Serial.println("GPS Status: FAIL");
        status = false;
    }
    if (puddingMQTTSetup())
    {
        Serial.println("MQTT Status: OK");
    }
    else
    {
        Serial.println("MQTT Status: FAIL");
        status = false;
    }
    return status;
}