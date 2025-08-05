#include <Wire.h>
#include <WiFi.h>
#include <time.h>

#include <PubSubClient.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <stdio.h>
#include <vector>
#include <string>

#include "env.h"
#include "utils.h"
#include "devices.h"

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

using namespace std;

#include "snake.h"
#include "sensors.h"
#include "chart.h"
#include "Arduino.h"
#include "ota.h"
#include "mqtt.h"

void checkIaqSensorStatus()
{
    Serial.println("Checking IAQ sensor status...");

    if (iaqSensor.bsecStatus != BSEC_OK)
    {
        if (iaqSensor.bsecStatus < BSEC_OK)
        {
            Serial.println("BSEC error code: " + String(iaqSensor.bsecStatus));
        }
        else
        {
            Serial.println("BSEC warning code: " + String(iaqSensor.bsecStatus));
        }
    }

    if (iaqSensor.bme68xStatus != BME68X_OK)
    {
        if (iaqSensor.bme68xStatus < BME68X_OK)
        {
            Serial.println("BME68X error code: " + String(iaqSensor.bme68xStatus));
        }
        else
        {
            Serial.println("BME68X warning code: " + String(iaqSensor.bme68xStatus));
        }
    }
    Serial.println("IAQ sensor status check complete.");
}

void SerialSetup()
{
    Serial.begin(115200);
    Serial.println("Booting...");
    scanI2C();
    pinMode(BUTTON_PIN_MODE, INPUT_PULLUP);
    pinMode(BUTTON_PIN_RIGHT, INPUT_PULLUP);
    pinMode(BUTTON_PIN_LEFT, INPUT_PULLUP);
    Wire.begin(SDA_PIN, SCL_PIN);
}

unsigned long lastMQTTPublish = 0;
ChartHandler chartHandler;
void setup()
{
    delay(250);
    SerialSetup();

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println(F("SSD1306 allocation failed"));
        while (1)
            ;
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("Display ready!"));
    display.display();

    display.print("BME680 - ");
    display.display();

    iaqSensor.begin(BME68X_I2C_ADDR_HIGH, Wire);
    iaqSensor.updateSubscription(sensorList, 13, BSEC_SAMPLE_RATE_LP);
    checkIaqSensorStatus();

    display.println(F("OK"));
    display.display();

    delay(1000);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Wifi: ");
    display.println(ssid);
    int wifiTries = 0;
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        if (wifiTries >= 15)
        {
            break;
        }

        delay(1000);
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Wifi: ");
        display.println(ssid);
        Serial.println(WiFi.status());
        display.println(WiFi.status());
        display.display();
        wifiTries++;
    }
    display.println("ok");
    display.display();
    delay(300);

    setupOTA();

    client.setServer(mqtt_server, mqtt_port);
    client.connect(device_name, mqtt_user, mqtt_pass);
    client.setKeepAlive(1200);
    publishToMQTT();
    ipinfo();
    delay(2000);

    lastMQTTPublish = millis();
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    auto ctime = getCurrentDateTime();
    client.publish((preTopicStr + "/lastboot").c_str(), (ctime.first + " " + ctime.second).c_str());

    chartHandler.load();

    xTaskCreatePinnedToCore(
        sensorsWatcher,
        "WorkerThread",
        2048,
        NULL,
        1,
        NULL,
        1);

    iaqSensor.run();
    while (getPressure() < 10.0)
    {
        iaqSensor.run();
        delay(50);
    }
}

int currentMode = 0;
bool modeFlag = true;
int lastMode = -1;
unsigned long lastModeChange = 0;

unsigned long animationDelay = 0;
unsigned long lastAnimationChange = 20000;

Snake snakeGame;

void loop()
{
    ArduinoOTA.handle();
    client.loop();
    chartHandler.refreshData();

    if (millis() > lastMQTTPublish + 15000)
    {
        publishToMQTT();
        lastMQTTPublish = millis();
    }

    if (is_pressed(BUTTON_PIN_MODE) && modeFlag && millis() > lastModeChange + 100)
    {
        lastModeChange = millis();
        currentMode++;
        modeFlag = false;
        if (currentMode > 7)
            currentMode = 0;
    }

    if (millis() - lastModeChange > 20000 && lastAnimationChange + animationDelay < millis())
    {
        lastAnimationChange = millis();
        animationDelay = (currentMode == 5) ? 10000 : 5000;
        currentMode = (currentMode + 1) % 8;
    }

    if (!is_pressed(BUTTON_PIN_MODE))
    {
        modeFlag = true;
    }

    switch (currentMode)
    {
    case 0:
        sensorPrint();
        break;
    case 1:
        ipinfo();
        break;
    case 2:
        chartHandler.displayChart(0, display);
        break;
    case 3:
        chartHandler.displayChart(1, display);
        break;
    case 4:
        chartHandler.displayChart(2, display);
        break;
    case 5:
        chartHandler.displayChart(3, display);
        break;
    case 6:
        chartHandler.displayChart(4, display);
        break;
    case 7:
        chartHandler.displayChart(5, display);
        break;
    }

    if (currentMode != lastMode)
    {
        Serial.print("Current mode: ");
        Serial.println(currentMode);
        lastMode = currentMode;
    }
}
