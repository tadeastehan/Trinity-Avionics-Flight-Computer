#include <SoftwareSerial.h>
#include "pudding.h"
#include "A9G.h"
#include <MicroNMEA.h>

#include <SPI.h>
#include "SdFat.h"

#include "board/board.h"

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

nmeaData gpsData = {
    0,     // navSystem
    0,     // numSatellites
    0,     // hdop
    false, // valid
    0,     // latitude
    0,     // longitude
    0,     // altitude
    0,     // year
    0,     // month
    0,     // day
    0,     // hour
    0,     // minute
    0,     // second
    0,     // hundredths
    0,     // speed
    0      // course
};

SoftwareSerial puddingGPS(GPS_RXPIN, GPS_TXPIN);

static PuddingState puddingState = PUDDING_INIT;
static unsigned long puddingStateStart = 0;
static bool puddingStatus = false;

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
        Serial.print("Uknown event ID: ");
        Serial.println((int)event->id);
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
        // Serial.print(c); // Print the character to Serial for debugging
        if (nmea.process(c))
        {
            gpsData.navSystem = nmea.getNavSystem();
            gpsData.numSatellites = nmea.getNumSatellites();
            gpsData.hdop = nmea.getHDOP();
            gpsData.valid = nmea.isValid();
            gpsData.latitude = nmea.getLatitude();
            gpsData.longitude = nmea.getLongitude();
            long alt;
            if (nmea.getAltitude(alt))
            {
                gpsData.altitude = alt;
            }
            gpsData.year = nmea.getYear();
            gpsData.month = nmea.getMonth();
            gpsData.day = nmea.getDay();
            gpsData.hour = nmea.getHour();
            gpsData.minute = nmea.getMinute();
            gpsData.second = nmea.getSecond();
            gpsData.hundredths = nmea.getHundredths();
            gpsData.speed = nmea.getSpeed();
            gpsData.course = nmea.getCourse();
        }
    }
}

void puddingUartFirstInit()
{
    Serial0.begin(115200);
    puddingUart.init(&Serial0);
    puddingUart.EventDispatch(A9G_eventDispatch);

    pinMode(GPS_EN, OUTPUT);
    digitalWrite(GPS_EN, HIGH);
    Serial.println("A9G: GPS Enabled");
}

void puddingStateMachineInit()
{
    puddingState = PUDDING_INIT;
    puddingStateStart = millis();
    puddingStatus = false;
}

// Call this in setup()
void puddingNonBlockingInit()
{
    puddingUartFirstInit();
    puddingStateMachineInit();
}

// Call this in loop()
void puddingProcess()
{
    static PuddingState lastState = (PuddingState)(-1);
    unsigned long now = millis();
    if (lastState != puddingState)
    {
        Serial.print("[DEBUG] Pudding state: ");
        Serial.println((int)puddingState);
        lastState = puddingState;
    }
    switch (puddingState)
    {
    case PUDDING_INIT:
        if (!SDCardInit())
        {
            Serial.println("SD Card init failed, continuing without SD card.");
            showRGBColor(255, 165, 0); // Show orange color on RGB light
        }
        puddingState = PUDDING_WAIT_20S;
        puddingStateStart = now;
        break;
    case PUDDING_WAIT_20S:
        if (now - puddingStateStart >= 20000)
        {
            puddingUart.bIsReadyNoWait();
            puddingState = PUDDING_WAIT_FOR_READY;
            puddingStateStart = now;
        }
        break;
    case PUDDING_WAIT_FOR_READY:
        if (now - puddingStateStart >= 2000)
        {
            // After 2s, assume ready and continue, or reset if needed
            // Optionally, you could check a flag or status here if you want to verify
            puddingState = PUDDING_SETUP_GPS_READY;
            puddingStateStart = now;
        }
        break;
    case PUDDING_RESETTING_LOW:
        if (now - puddingStateStart >= 5000)
        {
            digitalWrite(GPS_EN, HIGH);
            puddingState = PUDDING_RESETTING_HIGH;
            puddingStateStart = now;
        }
        break;
    case PUDDING_RESETTING_HIGH:
        if (now - puddingStateStart >= 20000)
        {
            puddingState = PUDDING_WAIT_20S;
            puddingStateStart = now;
        }
        break;
    case PUDDING_SETUP_GPS_READY:
        puddingUart.bIsReadyNoWait();
        puddingState = PUDDING_SETUP_GPS_READY_WAIT;
        puddingStateStart = now;
        break;
    case PUDDING_SETUP_GPS_READY_WAIT:
        if (now - puddingStateStart >= 2000)
        {
            puddingState = PUDDING_SETUP_GPS_UART;
            puddingStateStart = now;
        }
        break;
    case PUDDING_SETUP_GPS_UART:
        puddingUart.ReleaseGPSUartNoWait();
        puddingState = PUDDING_SETUP_GPS_UART_WAIT;
        puddingStateStart = now;
        break;
    case PUDDING_SETUP_GPS_UART_WAIT:
        if (now - puddingStateStart >= 2000)
        {
            puddingGPSInit();
            Serial.println("GPS Status S: OK");
            puddingState = PUDDING_SETUP_MQTT_READY;
            puddingStateStart = now;
        }
        break;
    case PUDDING_SETUP_MQTT_READY:
        puddingUart.bIsReadyNoWait();
        puddingState = PUDDING_SETUP_MQTT_READY_WAIT;
        puddingStateStart = now;
        break;
    case PUDDING_SETUP_MQTT_READY_WAIT:
        if (now - puddingStateStart >= 2000)
        {
            puddingState = PUDDING_SETUP_MQTT_NET;
            puddingStateStart = now;
        }
        break;
    case PUDDING_SETUP_MQTT_NET:
        puddingUart.SupportedNetworkSettingsNoWait();
        puddingState = PUDDING_SETUP_MQTT_NET_WAIT;
        puddingStateStart = now;
        break;
    case PUDDING_SETUP_MQTT_NET_WAIT:
        if (now - puddingStateStart >= 2000)
        {
            puddingUart.ReadCCIDNoWait();
            puddingState = PUDDING_SETUP_MQTT_CCID_WAIT;
            puddingStateStart = now;
        }
        break;
    case PUDDING_SETUP_MQTT_CCID_WAIT:
        if (now - puddingStateStart >= 2000)
        {
            puddingState = PUDDING_SETUP_MQTT_GPRS;
            puddingStateStart = now;
        }
        break;
    case PUDDING_SETUP_MQTT_GPRS:
        puddingUart.AttachToGPRSNoWait();
        puddingState = PUDDING_SETUP_MQTT_GPRS_WAIT;
        puddingStateStart = now;
        break;
    case PUDDING_SETUP_MQTT_GPRS_WAIT:
        if (now - puddingStateStart >= 2000)
        {
            puddingState = PUDDING_SETUP_MQTT_APN;
            puddingStateStart = now;
        }
        break;
    case PUDDING_SETUP_MQTT_APN:
        puddingUart.SetAPNNoWait("IP", "internet");
        puddingState = PUDDING_SETUP_MQTT_APN_WAIT;
        puddingStateStart = now;
        break;
    case PUDDING_SETUP_MQTT_APN_WAIT:
        if (now - puddingStateStart >= 2000)
        {
            puddingState = PUDDING_SETUP_MQTT_PDP;
            puddingStateStart = now;
        }
        break;
    case PUDDING_SETUP_MQTT_PDP:
        puddingUart.ActivatePDPNoWait();
        puddingState = PUDDING_SETUP_MQTT_PDP_WAIT;
        puddingStateStart = now;
        break;
    case PUDDING_SETUP_MQTT_PDP_WAIT:
        if (now - puddingStateStart >= 2000)
        {
            puddingState = PUDDING_SETUP_MQTT_BROKER;
            puddingStateStart = now;
        }
        break;
    case PUDDING_SETUP_MQTT_BROKER:
        puddingUart.ConnectToBrokerNoWait(BROKER_NAME, PORT, UNIQUE_ID, KEEP_ALIVE, CLEAN_SEASSION);
        puddingState = PUDDING_SETUP_MQTT_BROKER_WAIT;
        puddingStateStart = now;
        break;
    case PUDDING_SETUP_MQTT_BROKER_WAIT:
        if (now - puddingStateStart >= 2000)
        {
            puddingUart.PublishToTopic(PUB_TOPIC, "Hello IoT Started");
            Serial.println("MQTT Status: OK");
            puddingStatus = true;
            puddingState = PUDDING_SETUP_DONE;
        }
        break;
    case PUDDING_SETUP_DONE:
        // Ready for normal operation
        break;
    case PUDDING_ERROR:
        puddingStatus = false;
        // Optionally, you can restart the state machine or stay in error
        break;
    }
}

// Returns true if pudding is ready
bool isPuddingReady()
{
    return puddingState == PUDDING_SETUP_DONE && puddingStatus;
}

void puddingGPSInit()
{
    puddingGPS.begin(GPS_BAUD);
}

void printDateTime()
{
    // Print current date and time
    Serial.print("Date: ");
    Serial.print(nmea.getYear());
    Serial.print("-");
    Serial.print(nmea.getMonth());
    Serial.print("-");
    Serial.print(nmea.getDay());
    Serial.print(" Time: ");
    Serial.print(nmea.getHour());
    Serial.print(":");
    Serial.print(nmea.getMinute());
    Serial.print(":");
    Serial.println(nmea.getSecond());
}

// SD card initialization
#define SD_CS_PIN SS
SdFat SD;
int fileCounter = 1;
String fileName = "data_0.csv";
File myFile;
String csv_header = "Time[d_ms],Altitude[m],Latitude[°],Longitude[°],Pressure[hPa],Temp[°C],gX[°],gY[°],gZ[°],acX[ms-2],acY[ms-2],acZ[ms-2],state[x]";
bool SDCardFileNameChanged = false;

bool SDCardInit()
{
    if (!SD.begin(SD_CS_PIN, SD_SCK_MHZ(40)))
    {
        Serial.println("SD Card initialization failed!");
        return false;
    }
    Serial.println("SD Card initialized successfully.");

    // Create a new file with a unique name
    if (!SDCardFileNameChanged)
    {
        while (SD.exists(fileName.c_str()) && fileCounter < 1000) // Limit to 1000 files
        // Increment fileCounter until a unique file name is found
        {
            fileCounter++;
            fileName = "data_" + String(fileCounter) + ".csv";
        }
        if (fileCounter >= 1000)
        {
            Serial.println("Maximum file count reached. Cannot create new file.");
            return false;
        }

        SDCardFileNameChanged = true;
    }

    Serial.print("Data will be saved to: ");
    Serial.println(fileName);

    return true;
}

static int writeCount = 0;

bool SDCardWrite(const String &data)
{
    // Open the file once
    if (!myFile)
    {
        myFile = SD.open(fileName.c_str(), FILE_WRITE);
        if (!myFile)
        {
            Serial.println("Failed to open file for writing.");
            return false;
        }
        // Optional: write CSV header
        myFile.println(csv_header);
        myFile.flush();
    }

    myFile.println(data);

    if (++writeCount >= 500)
    {
        myFile.flush(); // Commit to SD every N writes
        writeCount = 0;
    }

    return true;
}

void SDCardClose()
{
    if (myFile)
    {
        myFile.flush();
        myFile.close();
    }
}