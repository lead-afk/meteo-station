#pragma once
#include <Wire.h>
#include "devices.h"
#include "env.h"
#include "utils.h"

int getAirQuality()
{
    return analogRead(MQ135_PIN);
}

float getTemperature()
{
    float temperature = bmp.readTemperature();
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);

    return ((temp.temperature + temperature) / 2) - 2;
}

float getPressure()
{
    float toReturn = bmp.readPressure();
    return toReturn / 100;
}

float getHumidity()
{
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);

    return humidity.relative_humidity;
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
    int airQuality = getAirQuality();

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

    auto ctime = getCurrentDateTime();

    display.println("");
    display.println("");
    display.println(ctime.first);
    display.println(ctime.second);

    display.display();

    // Serial.print("Temp AHT20: "); Serial.println(temperature);
    // Serial.print("Humidity: "); Serial.println(humidity);
    // Serial.print("Temp BMP180: "); Serial.println(temperature);
    // Serial.print("Pressure (hPa): "); Serial.println(pressure);
    // Serial.print("Air Quality (raw): "); Serial.println(airQuality);
    // Serial.println();
}