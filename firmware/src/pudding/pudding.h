#pragma once
#include <SoftwareSerial.h>
#include "A9G.h"
#include <MicroNMEA.h>
#include <env.h>

// GPS initialization
#define GPS_RXPIN 0  // GPIO0
#define GPS_TXPIN -1 // unused
#define GPS_BAUD 9600

extern SoftwareSerial puddingGPS;
extern GSM puddingUart;

struct nmeaData
{
    char navSystem;        // Navigation system (N, P, L, A)
    uint8_t numSatellites; // Number of satellites in use
    uint8_t hdop;          // Horizontal Dilution of Precision (HDOP)
    bool valid;            // GPS fix validity
    long latitude;         // Latitude in millionths of a degree
    long longitude;        // Longitude in millionths of a degree
    long altitude;         // Altitude in millimeters
    uint16_t year;         // Year of the fix
    uint8_t month;         // Month of the fix (1-12)
    uint8_t day;           // Day of the fix (1-31)
    uint8_t hour;          // Hour of the fix (0-23)
    uint8_t minute;        // Minute of the fix (0-59)
    uint8_t second;        // Second of the fix (0-59)
    uint8_t hundredths;    // Hundredths of a second (0-99)
    long speed;            // Speed in thousandths of a knot
    long course;           // Course in thousandths of a degree
};

void puddingGPSInit();          // Call in setup()
void puddingStateMachineInit(); // Call in setup()
// State machine for pudding dev module
enum PuddingState
{
    PUDDING_INIT,
    PUDDING_WAIT_20S,
    PUDDING_WAIT_FOR_READY,
    PUDDING_RESETTING_LOW,
    PUDDING_RESETTING_HIGH,
    PUDDING_SETUP_GPS_READY,
    PUDDING_SETUP_GPS_READY_WAIT,
    PUDDING_SETUP_GPS_UART,
    PUDDING_SETUP_GPS_UART_WAIT,
    PUDDING_SETUP_MQTT_READY,
    PUDDING_SETUP_MQTT_READY_WAIT,
    PUDDING_SETUP_MQTT_NET,
    PUDDING_SETUP_MQTT_NET_WAIT,
    PUDDING_SETUP_MQTT_CCID_WAIT,
    PUDDING_SETUP_MQTT_GPRS,
    PUDDING_SETUP_MQTT_GPRS_WAIT,
    PUDDING_SETUP_MQTT_APN,
    PUDDING_SETUP_MQTT_APN_WAIT,
    PUDDING_SETUP_MQTT_PDP,
    PUDDING_SETUP_MQTT_PDP_WAIT,
    PUDDING_SETUP_MQTT_BROKER,
    PUDDING_SETUP_MQTT_BROKER_WAIT,
    PUDDING_SETUP_DONE,
    PUDDING_ERROR
};

// Non-blocking pudding state machine API
void puddingNonBlockingInit(); // Call in setup()
void puddingProcess();         // Call in loop()
bool isPuddingReady();         // Returns true if pudding is ready

void puddingUartFirstInit(); // (still used internally)
void printDateTime();

// GPS data functions
nmeaData getGPSdata();
void updateGPS();

// SD card functions
bool SDCardInit();                    // Call in puddingSdcInit()
bool SDCardWrite(const String &data); // Call in puddingSdcWrite()
void SDCardClose();                   // Call in puddingSdcClose()