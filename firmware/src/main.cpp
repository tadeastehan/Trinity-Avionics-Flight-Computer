#include <Arduino.h>
#include "pudding/pudding.h"
#include "qwiic/qwiic.h"
#include "board/board.h"

// Moving average library
#include <movingAvg.h> // https://github.com/JChristensen/movingAvg

// Apagee detection
movingAvg avgAltitude(10);
int avgAltitude_result;

int minAltitude = 10000;  // Initialize to a very high value
int maxAltitude = -10000; // Initialize to a very low value

// Launch detection
int AltitudeNeededForLaunch = 0;

unsigned long SDCardWriteTimeLast = 0;
unsigned long SDCardWriteTimeDiff = 0;

unsigned long SerialUpdateTimeLast = 0; // Last time serial data was updated
unsigned long BatteryVoltageUpdateTimeLast = 0;
unsigned long MQTTUpdateTimeLast = 0;

// Landing detection
#define LIST_SIZE 10
int descentAltitudes[LIST_SIZE] = {0};
int currentIndex = 0;
int descentLoopCounter = 0; // Counter for descent loop

enum AVIONICS_State
{
    INIT,
    READY,
    GROUND,
    ARM,
    ASCENT,
    DEPLOY,
    DESCENT,
    LANDED
};

AVIONICS_State currentState = INIT;
AVIONICS_State lastState = INIT;

void SDCardLogging()
{
    SDCardWriteTimeDiff = millis() - SDCardWriteTimeLast; // Calculate the time difference since the last write
    if (SDCardWriteTimeDiff >= SD_CARD_WRITE_INTERVAL)
    {
        SDCardWriteTimeLast = millis(); // Update the last write time

        BMPData bmpData = getBMPData();
        sfe_lsm_data_t accelData = getLSM6DSV16XAccel();
        sfe_lsm_data_t gyroData = getLSM6DSV16XGyro();
        nmeaData gpsData = getGPSdata();

        char csv_line[166];
        //                                    Time[d_ms],Altitude[m],Latitude[°],Longitude[°],Pressure[hPa],Temp[°C],gX[°],gY[°],gZ[°],acX[ms-2],acY[ms-2],acZ[ms-2],state[x]
        snprintf(csv_line, sizeof(csv_line), "%lu,%.2f,%.6f,%.6f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d",
                 SDCardWriteTimeDiff,
                 bmpData.altitude,
                 gpsData.latitude / 1000000.0,
                 gpsData.longitude / 1000000.0,
                 bmpData.pressure,
                 bmpData.temperature,
                 gyroData.xData,
                 gyroData.yData,
                 gyroData.zData,
                 accelData.xData,
                 accelData.yData,
                 accelData.zData,
                 currentState);

        SDCardWrite(csv_line); // Write to SD card
    }
}

void update()
{
    updateGPS();
    if (millis() - BatteryVoltageUpdateTimeLast >= BATTERY_VOLTAGE_UPDATE_INTERVAL)
    {
        BatteryVoltageUpdateTimeLast = millis(); // Update the last battery voltage update time
        updateBatteryVoltage();                  // Update battery voltage
    }
}

void detectStateChange()
{
    if (currentState != lastState)
    {

        Serial.print("State changed: ");
        switch (currentState)
        {
        case INIT:
            Serial.println("INIT");
            break;
        case READY:
            Serial.println("READY");
            break;
        case GROUND:
            Serial.println("GROUND");
            break;
        case ARM:
            Serial.println("ARM");
            break;
        case ASCENT:
            Serial.println("ASCENT");
            break;
        case DEPLOY:
            Serial.println("DEPLOY");
            break;
        case DESCENT:
            Serial.println("DESCENT");
            break;
        case LANDED:
            Serial.println("LANDED");
            break;
        }
        lastState = currentState; // Update last state
    }
}

void detectState()
{
    switch (currentState)
    {
    case INIT:
        if (digitalRead(RBF_PIN) == LOW) // RBF pressed
        {
            currentState = READY; // Transition to ARM state if RBF is pressed
        }
        break;
    case READY:
        if (digitalRead(RBF_PIN) == HIGH) // RBF released
        {
            currentState = GROUND; // Transition to ARM state if RBF is released
        }
        break;
    case GROUND:
        for (int i = 0; i < 10; i++)
        {
            updateBMP384(); // Update BMP384 sensor data
            avgAltitude_result = avgAltitude.reading((int)getBMPData().altitude);
            delay(40); // 25Hz sampling rate
        }

        AltitudeNeededForLaunch = avgAltitude_result + LAUCH_THRESHOLD; // Set the altitude needed for launch
        Serial.print("Altitude needed for launch: ");
        Serial.println(AltitudeNeededForLaunch);
        currentState = ARM; // Transition to ARM state
        break;
    case ARM:
        avgAltitude_result = avgAltitude.reading((int)getBMPData().altitude);
        if (avgAltitude_result >= AltitudeNeededForLaunch)
        {
            currentState = ASCENT; // Transition to ASCENT state if altitude is sufficient
        }
        SDCardLogging(); // Log data to SD card
        break;
    case ASCENT:
        avgAltitude_result = avgAltitude.reading((int)getBMPData().altitude);
        if (avgAltitude_result > maxAltitude)
        {
            maxAltitude = avgAltitude_result; // Update maximum altitude during ascent
        }
        if (avgAltitude_result <= (maxAltitude - DESCENT_THRESHOLD))
        {
            currentState = DEPLOY; // Transition to DEPLOY state if descent is detected
        }
        SDCardLogging(); // Log data to SD card
        break;
    case DEPLOY:
        avgAltitude_result = avgAltitude.reading((int)getBMPData().altitude);
        ServoOpen();            // Open the servo to deploy the payload
        currentState = DESCENT; // Transition to DESCENT state
        break;
    case DESCENT:
        avgAltitude_result = avgAltitude.reading((int)getBMPData().altitude);
        buzz();          // Activate buzzer during descent
        SDCardLogging(); // Log data to SD card
        descentLoopCounter++;
        if (avgAltitude_result < minAltitude)
        {
            minAltitude = avgAltitude_result; // Update minimum altitude during descent
        }
        if (descentLoopCounter >= 100) // Check every 100 loops (about 4 seconds at 25Hz)
        {
            descentLoopCounter = 0;                              // Reset the loop counter every 100 iterations
            descentAltitudes[currentIndex] = avgAltitude_result; // Store the current altitude in the list
            currentIndex = (currentIndex + 1) % LIST_SIZE;       // Update index for next altitude

            bool allSame = true;
            for (int i = 1; i < LIST_SIZE; i++)
            {
                if (descentAltitudes[i] != descentAltitudes[0])
                {
                    allSame = false; // Check if all values in the list are the same
                    break;
                }
            }
            if (allSame)
            {
                currentState = LANDED; // Transition to LANDED state if all values are the same
                SDCardClose();         // Close the SD card file
            }
        }
        break;
    case LANDED:
        SDCardLogging(); // Log data to SD card
        // In LANDED state, you can perform any final actions or wait for a reset
        if (digitalRead(RBF_PIN) == HIGH) // RBF released
        {
            buzz(); // Activate buzzer
        }
        break;

    default:
        break;
    }
}

void setup()
{
    Serial.begin(115200);
    delay(2000);

    if (QWIICsetup())
    {
        Serial.println("QWIIC: OK");
    }
    else
    {
        Serial.println("QWIIC: FAIL");
    }
    puddingNonBlockingInit(); // Non-blocking pudding init
    setupBoard();             // Initialize board pins and peripherals
    avgAltitude.begin();
}

// Timers for MQTT messages
unsigned long gpsMQTTLastSent = 0;
unsigned long altBattStateLastSent = 0;
unsigned long tempPressLastSent = 0;

// Helper function to check if in ASCENT or DEPLOY
bool isAscentOrDeploy()
{
    return currentState == ASCENT || currentState == DEPLOY;
}

// Helper function to check if in ASCENT, DEPLOY, or DESCENT
bool isAscentDeployDescent()
{
    return currentState == ASCENT || currentState == DEPLOY || currentState == DESCENT;
}

void loop()
{
    updateBMP384();      // Update BMP384 sensor data
    detectState();       // Detect the current state of the rocket
    detectStateChange(); // Check if the state has changed
    updateLSM6DSV16X();

    update(); // Update GPS and battery voltage
    puddingProcess();

    unsigned long now = millis();

    // 1. GPS MQTT message every 5s except ASCENT, DEPLOY
    if (!isAscentOrDeploy() && now - gpsMQTTLastSent >= 5000)
    {
        gpsMQTTLastSent = now;
        nmeaData gps = getGPSdata();
        char mqttPayload[300];
        snprintf(mqttPayload, sizeof(mqttPayload),
                 "navSystem:%d,numSatellites:%d,hdop:%d,valid:%s,latitude:%.6f,longitude:%.6f,altitude:%.3f,date:%04d-%02d-%02d,time:%02d:%02d:%02d.%02d,speed:%.3f,course:%.3f",
                 gps.navSystem,
                 gps.numSatellites,
                 gps.hdop,
                 gps.valid ? "true" : "false",
                 gps.latitude / 1000000.0,
                 gps.longitude / 1000000.0,
                 gps.altitude / 1000.0,
                 gps.year,
                 gps.month,
                 gps.day,
                 gps.hour,
                 gps.minute,
                 gps.second,
                 gps.hundredths,
                 gps.speed / 1000.0,
                 gps.course / 1000.0);
        puddingUart.PublishToTopicNoWait(PUB_TOPIC, mqttPayload);
    }

    // 2. Altitude, battery voltage, state
    unsigned long altBattStateInterval = isAscentDeployDescent() ? 500 : 1000; // 500ms or 1s
    if (now - altBattStateLastSent >= altBattStateInterval)
    {
        altBattStateLastSent = now;
        float altitude = getBMPData().altitude;
        float battery = getBatteryVoltage();
        char msg[128];
        snprintf(msg, sizeof(msg), "altitude:%.2f,battery:%.2f,state:%d", altitude, battery, currentState);
        puddingUart.PublishToTopicNoWait(PUB_TOPIC, msg);
    }

    // 3. Temperature and pressure every 10s except ASCENT, DEPLOY
    if (!isAscentOrDeploy() && now - tempPressLastSent >= 10000)
    {
        tempPressLastSent = now;
        BMPData bmp = getBMPData();
        char msg[128];
        snprintf(msg, sizeof(msg), "temperature:%.2f,pressure:%.2f", bmp.temperature, bmp.pressure);
        puddingUart.PublishToTopicNoWait(PUB_TOPIC, msg);
    }

    // if (millis() - SerialUpdateTimeLast >= SERIAL_UPDATE_INTERVAL)
    // {
    //     SerialUpdateTimeLast = millis(); // Update the last serial update time

    //     nmeaData gps = getGPSdata();
    //     Serial.println("GPS Data:");
    //     Serial.print("  navSystem: ");
    //     Serial.println(gps.navSystem);
    //     Serial.print("  numSatellites: ");
    //     Serial.println(gps.numSatellites);
    //     Serial.print("  hdop: ");
    //     Serial.println(gps.hdop);
    //     Serial.print("  valid: ");
    //     Serial.println(gps.valid ? "true" : "false");
    //     Serial.print("  latitude: ");
    //     Serial.println(gps.latitude / 1000000.0, 6);
    //     Serial.print("  longitude: ");
    //     Serial.println(gps.longitude / 1000000.0, 6);
    //     Serial.print("  altitude: ");
    //     Serial.print(gps.altitude / 1000.0, 3);
    //     Serial.println(" m");
    //     Serial.print("  date: ");
    //     Serial.print(gps.year);
    //     Serial.print('-');
    //     Serial.print(gps.month);
    //     Serial.print('-');
    //     Serial.println(gps.day);
    //     Serial.print("  time: ");
    //     Serial.print(gps.hour);
    //     Serial.print(':');
    //     Serial.print(gps.minute);
    //     Serial.print(':');
    //     Serial.print(gps.second);
    //     Serial.print('.');
    //     Serial.println(gps.hundredths);
    //     Serial.print("  speed: ");
    //     Serial.print(gps.speed / 1000.0, 3);
    //     Serial.println(" knots");
    //     Serial.print("  course: ");
    //     Serial.print(gps.course / 1000.0, 3);
    //     Serial.println(" deg");
    // }
}
