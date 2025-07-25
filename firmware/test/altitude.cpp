#include <Wire.h>

#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BMP3XX bmp;

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;
    Serial.println("Adafruit BMP388 / BMP390 test");

    Wire.begin();
    Wire.setClock(400000); // 400khz clock

    if (!bmp.begin_I2C(BMP3XX_DEFAULT_ADDRESS, &Wire))
    { // hardware I2C mode, can pass in address & alt Wire
        // if (! bmp.begin_SPI(BMP_CS)) {  // hardware SPI mode
        // if (! bmp.begin_SPI(BMP_CS, BMP_SCK, BMP_MISO, BMP_MOSI)) {  // software SPI mode
        Serial.println("Could not find a valid BMP3 sensor, check wiring!");
        while (1)
            ;
    }

    // Set up oversampling and filter initialization
    bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
    bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
    bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
    bmp.setOutputDataRate(BMP3_ODR_50_HZ);
}

void loop()
{
    if (!bmp.performReading())
    {
        Serial.println("Failed to perform reading :(");
        return;
    }
    Serial.print("Temperature = ");
    Serial.print(bmp.temperature);
    Serial.println(" *C");

    Serial.print("Pressure = ");
    Serial.print(bmp.pressure / 100.0);
    Serial.println(" hPa");

    Serial.print("Approx. Altitude = ");
    Serial.print(bmp.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(" m");

    Serial.println();
    delay(2000);
}