#include "SparkFun_LSM6DSV16X.h"
#include <Wire.h>
#include "SparkFunBMP384.h"

// LSM6DSV16X sensor
SparkFun_LSM6DSV16X myLSM;
sfe_lsm_data_t accelData;
sfe_lsm_data_t gyroData;

// BMP384 sensor
BMP384 pressureSensor;
uint8_t i2cAddress = BMP384_I2C_ADDRESS_DEFAULT; // 0x77

void setup()
{
    Wire.begin();
    Serial.begin(115200);
    while (!Serial)
    {
    }
    Serial.println("LSM6DSV16X & BMP384 Qwiic Test");

    // LSM6DSV16X init
    if (!myLSM.begin())
    {
        Serial.println("LSM6DSV16X not found!");
        while (1)
            ;
    }
    myLSM.deviceReset();
    while (!myLSM.getDeviceReset())
    {
        delay(1);
    }
    myLSM.enableBlockDataUpdate();
    myLSM.setAccelDataRate(LSM6DSV16X_ODR_AT_7Hz5);
    myLSM.setAccelFullScale(LSM6DSV16X_16g);
    myLSM.setGyroDataRate(LSM6DSV16X_ODR_AT_15Hz);
    myLSM.setGyroFullScale(LSM6DSV16X_2000dps);
    myLSM.enableFilterSettling();
    myLSM.enableAccelLP2Filter();
    myLSM.setAccelLP2Bandwidth(LSM6DSV16X_XL_STRONG);
    myLSM.enableGyroLP1Filter();
    myLSM.setGyroLP1Bandwidth(LSM6DSV16X_GY_ULTRA_LIGHT);
    Serial.println("LSM6DSV16X Ready.");

    // BMP384 init
    Serial.println("BMP384 Example1 begin!");
    while (pressureSensor.beginI2C(i2cAddress) != BMP3_OK)
    {
        Serial.println("Error: BMP384 not connected, check wiring and I2C address!");
        delay(1000);
    }
    Serial.println("BMP384 connected!");
}

void loop()
{
    // LSM6DSV16X: Read and print accelerometer (G) and gyroscope (deg/s) data
    if (myLSM.checkStatus())
    {
        myLSM.getAccel(&accelData);
        myLSM.getGyro(&gyroData);
        Serial.print("Accelerometer [G]: X: ");
        Serial.print(accelData.xData / 1000.0, 3);
        Serial.print(" Y: ");
        Serial.print(accelData.yData / 1000.0, 3);
        Serial.print(" Z: ");
        Serial.println(accelData.zData / 1000.0, 3);
        Serial.print("Gyroscope [deg/s]: X: ");
        Serial.print(gyroData.xData / 100.0, 2);
        Serial.print(" Y: ");
        Serial.print(gyroData.yData / 100.0, 2);
        Serial.print(" Z: ");
        Serial.println(gyroData.zData / 100.0, 2);
    }

    // BMP384: Read and print temperature and pressure
    bmp3_data data;
    int8_t err = pressureSensor.getSensorData(&data);
    if (err == BMP3_OK)
    {
        Serial.print("Temperature (C): ");
        Serial.print(data.temperature);
        Serial.print("\tPressure (Pa): ");
        Serial.println(data.pressure);
    }
    else
    {
        Serial.print("Error getting data from BMP384! Error code: ");
        Serial.println(err);
    }
    delay(1000);
}