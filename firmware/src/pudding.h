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
    long latitude;
    long longitude;
    bool gpsFix;
};

// Event dispatch for A9G events
void A9G_eventDispatch(A9G_Event_t *event);

// Setup and initialization
bool puddingSetup();
void puddingUartInit();
bool puddingGPSSetup();
void puddingGPSInit();
void puddingUartReset();
bool puddingMQTTSetup();

// GPS data functions
nmeaData getGPSdata();
void updateGPS();
