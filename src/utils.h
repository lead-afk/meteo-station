#pragma once
#include <utility>
#include <Wire.h>
#include <time.h>
#include <PubSubClient.h>

template <typename T1, typename T2>
std::pair<T1, T2> operator+(const std::pair<T1, T2> &a, const std::pair<T1, T2> &b)
{
    return {a.first + b.first, a.second + b.second};
}

bool equalPair(const std::pair<int, int> &a, const std::pair<int, int> &b)
{
    return (a.first == b.first && a.second == b.second);
}

bool is_pressed(int pin)
{
    return digitalRead(pin) == LOW;
}

std::pair<String, String> getCurrentDateTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return {"--/--/----", "--:--:--"};
    }

    char dateStr[11]; // DD/MM/YYYY
    char timeStr[9];  // HH:MM:SS

    snprintf(dateStr, sizeof(dateStr), "%02d/%02d/%04d",
             timeinfo.tm_mday,
             timeinfo.tm_mon + 1,
             timeinfo.tm_year + 1900);

    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec);

    return {String(dateStr), String(timeStr)};
}

void scanI2C(uint8_t sda = 21, uint8_t scl = 22)
{
    Wire.begin(sda, scl);
    Serial.println("Scanning I2C bus...");

    for (uint8_t address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0)
        {
            Serial.print("Found I2C device at 0x");
            Serial.println(address, HEX);
        }
    }

    Serial.println("Scan complete.");
}

template <typename T>
void printVectorDebug(const vector<T> &vec, const char *name)
{
    Serial.print(name);
    Serial.print(": size=");
    Serial.print(vec.size());
    if (vec.empty())
    {
        Serial.println(" (empty)");
        return;
    }
    Serial.print(", values=[");
    size_t count = min(vec.size(), size_t(5)); // print up to first 5 elements
    for (size_t i = 0; i < count; i++)
    {
        Serial.print(vec[i]);
        if (i < count - 1)
            Serial.print(", ");
    }
    if (vec.size() > 5)
        Serial.print(", ...");
    Serial.println("]");
}
