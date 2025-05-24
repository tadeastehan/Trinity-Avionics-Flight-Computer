#pragma once

#include <Wire.h>
#include "SparkFun_LSM6DSV16X.h"
#include "SparkFunBMP384.h"

// Struct for BMP384 sensor data
struct BMPData
{
    float temperature;
    float pressure;
    float altitude;
};

// Extern declarations for sensors and data structures
extern BMP384 pressureSensor;
extern uint8_t i2cAddress;
extern int8_t err;

// Function prototype for getting BMP384 sensor data as a struct
BMPData getBMPData();

// Function prototype for updating sensor data
bool updateBMP384();

// Function prototype for setting up BMP384
bool setupBMP384();

extern SparkFun_LSM6DSV16X myLSM;
extern sfe_lsm_data_t accelData;
extern sfe_lsm_data_t gyroData;

// Function prototype for getting LSM6DSV16X sensor data
sfe_lsm_data_t getLSM6DSV16XAccel();
sfe_lsm_data_t getLSM6DSV16XGyro();
// Function prototype for updating LSM6DSV16X sensor data
bool updateLSM6DSV16X();
// Function prototype for setting up LSM6DSV16X
bool setupLSM6DSV16X();

// Function prototype for setting up QWIIC connected devices
bool QWIICsetup();