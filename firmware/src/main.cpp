#include <Arduino.h>
#include <qwiic.h>
#include "pudding.h"

unsigned long lastUpdateTime = 0;
unsigned long updateInterval = 1000; // Update interval in milliseconds

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
    if (puddingSetup())
    {
        Serial.println("Pudding: OK");
    }
    else
    {
        Serial.println("Pudding: FAIL");
    }
}

void loop()
{
    // Update sensor data
    updateBMP384();
    updateLSM6DSV16X();
    updateGPS();

    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime >= updateInterval)
    {
        BMPData bmpData = getBMPData();
        sfe_lsm_data_t accelData = getLSM6DSV16XAccel();
        sfe_lsm_data_t gyroData = getLSM6DSV16XGyro();
        nmeaData gpsData = getGPSdata();

        lastUpdateTime = currentTime;

        // Print sensor data
        Serial.print("BMP384: ");
        Serial.print(bmpData.temperature, 2);
        Serial.print(" C, ");
        Serial.print(bmpData.pressure, 2);
        Serial.print(" hPa, ");
        Serial.print(bmpData.altitude, 2);
        Serial.println(" m");

        Serial.print("LSM6DSV16X Accel: ");
        Serial.print(accelData.xData, 2);
        Serial.print(", ");
        Serial.print(accelData.yData, 2);
        Serial.print(", ");
        Serial.println(accelData.zData, 2);

        Serial.print("LSM6DSV16X Gyro: ");
        Serial.print(gyroData.xData, 2);
        Serial.print(", ");
        Serial.print(gyroData.yData, 2);
        Serial.print(", ");
        Serial.println(gyroData.zData, 2);

        Serial.print("GPS: Fix: ");
        Serial.print(gpsData.gpsFix ? "OK" : "NO FIX");
        Serial.print(", Lat: ");
        Serial.print(gpsData.latitude / 1000000.0, 6);
        Serial.print(", Lon: ");
        Serial.println(gpsData.longitude / 1000000.0, 6);

        puddingUart.PublishToTopic(PUB_TOPIC, "Hello");
    }
}