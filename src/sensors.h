#pragma once
#include <Wire.h>
#include "devices.h"
#include "env.h"
#include "utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

bsec_virtual_sensor_t sensorList[13] = {
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_STABILIZATION_STATUS,
    BSEC_OUTPUT_RUN_IN_STATUS,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
    BSEC_OUTPUT_GAS_PERCENTAGE};

float getTemperature()
{
    return iaqSensor.temperature - 1; // Compensated ambient temperature [°C]
}

float getHumidity()
{
    return iaqSensor.humidity; // Compensated relative humidity [%]
}

float getPressure()
{
    return iaqSensor.pressure / 100; // Pressure in hPa
}

float getIAQ()
{
    return iaqSensor.iaq; // Indoor Air Quality score (0–500)
}

uint8_t getIAQAccuracy()
{
    return iaqSensor.iaqAccuracy; // 0 = Unreliable, 3 = Stable
}

float getCO2Equivalent()
{
    return iaqSensor.co2Equivalent; // Estimated CO2 [ppm]
}

float getVOC()
{
    return iaqSensor.breathVocEquivalent; // Breath VOC equivalent [ppm]
}

float getGasResistance()
{
    return iaqSensor.gasResistance; // Raw gas sensor resistance [Ohm]
}

void sensorsWatcher(void *param)
{
    while (true)
    {
        iaqSensor.run();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

unsigned long lastUpdateSensorPrint = 0;
void sensorPrint()
{

    if (!(millis() > lastUpdateSensorPrint + 1000))
    {
        return;
    }
    lastUpdateSensorPrint = millis();

    float temperature = getTemperature();
    float pressure = getPressure();
    float humidity = getHumidity();
    int airQuality = getIAQ();
    float voc = getVOC();           // new
    float co2 = getCO2Equivalent(); // new

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Temp   : ");
    display.print(temperature);
    display.println(" C");

    display.print("Humi   : ");
    display.print(humidity);
    display.println(" %");

    display.print("hPa    : ");
    display.println(pressure);

    display.print("Air Q  : ");
    display.println(airQuality);

    display.print("CO2    : ");
    display.print(co2);
    display.println(" ppm");

    display.print("VOC    : ");
    display.print(voc);
    display.println(" ppm");

    auto ctime = getCurrentDateTime();

    display.println(ctime.first);
    display.println(ctime.second);

    display.display();
}