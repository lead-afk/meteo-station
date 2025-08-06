#include <Wire.h>
#include <WiFi.h>
#include <time.h>
#include <ArduinoOTA.h>

#include <PubSubClient.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_AHTX0.h>  // For AHT20
#include <Adafruit_BMP085.h> // For BMP180 (GY-68)

#include <vector>
#include <string>

#include "env.h"
#include "utils.h"
#include "devices.h"

using namespace std;

#include "snake.h"
#include "sensors.h"
#include "chart.h"
#include "ota.h"
#include "mqtt.h"

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

    display.print("ATH-20 - ");
    if (!aht.begin())
    {
        Serial.println("AHT20 not detected. Check wiring.");
        display.println(F("Err"));
        display.display();
        while (1)
            ;
    }

    display.println(F("OK"));
    display.print("BMP180 - ");
    display.display();

    if (!bmp.begin())
    {
        Serial.println("BMP180 not detected. Check wiring.");
        display.println(F("Err"));
        display.display();
        while (1)
            ;
    }
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
}

int currentMode = 0;
bool modeFlag = true;
bool leftFlag = true;
bool rightFlag = true;
int lastMode = -1;
unsigned long lastModeChange = 0;
unsigned long lastLeft = 0;
unsigned long lastRight = 0;

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
        if (currentMode > 6)
            currentMode = 0;
    }
    if (currentMode == 6)
    {
        if (is_pressed(BUTTON_PIN_LEFT) && leftFlag && millis() > lastLeft + 50)
        {
            snakeGame.setSnakeLeftRegistered(true);
            leftFlag = false;
            lastLeft = millis();
        }
        if (is_pressed(BUTTON_PIN_RIGHT) && rightFlag && millis() > lastRight + 50)
        {
            snakeGame.setSnakeRightRegistered(true);
            rightFlag = false;
            lastRight = millis();
        }
    }

    if (millis() - lastModeChange > 20000 && lastAnimationChange + animationDelay < millis() && currentMode != 6)
    {
        lastAnimationChange = millis();
        animationDelay = (currentMode == 5) ? 10000 : 5000;
        currentMode = (currentMode + 1) % 6;
    }

    if (!is_pressed(BUTTON_PIN_MODE))
    {
        modeFlag = true;
    }
    if (!is_pressed(BUTTON_PIN_LEFT))
    {
        leftFlag = true;
    }
    if (!is_pressed(BUTTON_PIN_RIGHT))
    {
        rightFlag = true;
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
        snakeGame.game(display);
        break;
    }

    if (currentMode != lastMode)
    {
        Serial.print("Current mode: ");
        Serial.println(currentMode);
        lastMode = currentMode;
    }
}
