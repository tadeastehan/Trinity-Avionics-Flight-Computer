
#include "qwiic.h"

#include <Wire.h>
#include "SparkFun_LSM6DSV16X.h"
#include "SparkFunBMP384.h"

#include "env.h"

// BMP384 sensor
BMP384 pressureSensor;
uint8_t i2cAddress = BMP384_I2C_ADDRESS_DEFAULT; // 0x77
int8_t err;

// Global variable for BMP384 sensor data
static BMPData bmpData = {0.0f, 0.0f, 0.0f};

// Calculate altitude from pressure
float calculateAltitude(float pressure, float seaLevel)
{
    // Equation taken from BMP180 datasheet (page 16):
    //  http://www.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf

    // Note that using the equation from wikipedia can give bad results
    // at high altitude. See this thread for more information:
    //  http://forums.adafruit.com/viewtopic.php?f=22&t=58064

    float atmospheric = pressure / 100.0F;
    return 44330.0 * (1.0 - pow(atmospheric / seaLevel, 0.1903));
}

// Setup the BMP384 sensor
bool setupBMP384()
{
    // Initialize the sensor
    unsigned long startTime = millis();
    while (1)
    {
        if (pressureSensor.beginI2C(i2cAddress) == BMP3_OK)
        {
            Serial.println("BMP384: OK");
            break;
        }
        else
        {
            // Wait a bit to see if connection is established
            delay(5);

            // Timeout after 2 seconds (2000 ms)
            if (millis() - startTime > 2000)
            {
                Serial.println("BMP384: FAIL");
                return false;
            }
        }
    }

    err = BMP3_OK;
    err = pressureSensor.setFilterCoefficient(BMP3_IIR_FILTER_COEFF_3);
    if (err)
    {
        Serial.print("BMP384_COEFF: FAIL! Error code: ");
        Serial.println(err);
        return false;
    }
    else
    {
        Serial.println("BMP384_COEFF: OK");
    }

    err = BMP3_OK;
    bmp3_odr_filter_settings osrMultipliers =
        {
            .press_os = BMP3_OVERSAMPLING_4X,
            .temp_os = BMP3_OVERSAMPLING_8X,
        };
    err = pressureSensor.setOSRMultipliers(osrMultipliers);
    if (err)
    {
        Serial.print("BMP384_OSR: FAIL! Error code: ");
        Serial.println(err);
        return false;
    }
    else
    {
        Serial.println("BMP384_OSR: OK");
    }

    uint8_t odr = 0;
    err = pressureSensor.getODRFrequency(&odr);
    if (err)
    {
        Serial.print("BMP384_ODR: FAIL! Error code: ");
        Serial.println(err);
        return false;
    }
    else
    {
        Serial.println("BMP384_ODR: OK");
    }

    Serial.print("BMP384 Sampling Frequency: ");
    Serial.print(200 / pow(2, odr));
    Serial.println("Hz");

    return true;
}

// Function to get the BMP384 sensor data as a struct
BMPData getBMPData()
{
    return bmpData;
}

// Update the BMP384 sensor and store results in bmpData struct
bool updateBMP384()
{
    bmp3_data data;
    err = pressureSensor.getSensorData(&data);

    if (err == BMP3_OK)
    {
        bmpData.temperature = data.temperature;
        bmpData.pressure = data.pressure;
        bmpData.altitude = calculateAltitude(bmpData.pressure, SEALEVELPRESSURE_HPA);
        return true;
    }
    else
    {
        Serial.print("BMP384: FAIL! Error code: ");
        Serial.println(err);
        return false;
    }
}

// LSM6DSV16X sensor
SparkFun_LSM6DSV16X myLSM;
sfe_lsm_data_t accelData;
sfe_lsm_data_t gyroData;

// Function to get the LSM6DSV16X sensor data
sfe_lsm_data_t getLSM6DSV16XAccel()
{
    return accelData;
}

sfe_lsm_data_t getLSM6DSV16XGyro()
{
    return gyroData;
}

bool setupLSM6DSV16X()
{
    unsigned long startTime = millis();
    while (1)
    {
        if (myLSM.begin())
        {
            break;
        }
        else
        {
            delay(5);
            if (millis() - startTime > 2000)
            {
                Serial.println("LSM6DSV16X: FAIL");
                return false;
            }
        }
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
    Serial.println("LSM6DSV16X: OK");
    return true;
}

bool updateLSM6DSV16X()
{
    if (myLSM.checkStatus())
    {
        myLSM.getAccel(&accelData);
        myLSM.getGyro(&gyroData);
        return true;
    }
    else
    {
        return false;
    }
}

// Setup the QWIIC connected devices
bool QWIICsetup()
{
    Serial.println("------QWIIC setup--------");
    Wire.begin();

    if (!setupBMP384())
    {
        Serial.println("BMP384 setup failed!");
        return false;
    }
    else
    {
        Serial.println("BMP384 setup complete.");
    }
    if (!setupLSM6DSV16X())
    {
        Serial.println("LSM6DSV16X setup failed!");
        return false;
    }
    else
    {
        Serial.println("LSM6DSV16X setup complete.");
    }
    return true;
}
