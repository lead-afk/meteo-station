#include <Wire.h>
#include <WiFi.h>
#include <time.h>
#include <ArduinoOTA.h>

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

unsigned long lastUpdateIPInfo = 0;
void ipinfo()
{
    if (!(millis() > lastUpdateIPInfo + 300))
    {
        return;
    }
    lastUpdateIPInfo = millis();

    display.clearDisplay();
    display.setCursor(0, 0);

    display.print("IP: ");
    display.println(WiFi.localIP());

    display.print("GW: ");
    display.println(WiFi.gatewayIP());

    display.print("DNS: ");
    display.println(WiFi.dnsIP());

    int32_t rssi = WiFi.RSSI();
    display.print("RSSI: ");
    display.println(rssi);
    display.print("Wifi C: ");
    display.println(WiFi.status());
    display.print("MQTT C: ");
    display.println(client.state());

    display.display();
}

int failedWifi = 0;
void publishToMQTT()
{

    if (WiFi.status() != WL_CONNECTED)
    {
        failedWifi++;
        if (failedWifi > 20)
        {
            ESP.restart();
        }

        WiFi.reconnect();
    }
    else
    {
        failedWifi = 0;
    }

    // Read values
    float temperature = getTemperature();
    float pressure = getPressure();
    int airQuality = getIAQ();
    float humidity = getHumidity();
    float voc = getVOC();              // new
    float co2 = getCO2Equivalent();    // new
    float gasRes = getGasResistance(); // new
    int iaqAcc = getIAQAccuracy();     // new

    // Prepare payloads
    String tempStr = String(temperature, 2); // 2 decimal places
    String pressureStr = String(pressure);
    String airStr = String(airQuality);
    String humidityStr = String(humidity);
    String vocStr = String(voc, 2);
    String co2Str = String(co2, 1);
    String gasStr = String(gasRes, 0);
    String iaqAccStr = String(iaqAcc);
    String RSSIStr = String(WiFi.RSSI());
    String IPStr = WiFi.localIP().toString();

    if (!client.connected())
    {
        client.connect(device_name, mqtt_user, mqtt_pass);
    }

    // Publish to respective topics
    client.publish((preTopicStr + "/temp").c_str(), tempStr.c_str());
    client.publish((preTopicStr + "/pressure").c_str(), pressureStr.c_str());
    client.publish((preTopicStr + "/aq").c_str(), airStr.c_str());
    client.publish((preTopicStr + "/aq_acc").c_str(), iaqAccStr.c_str()); // new
    client.publish((preTopicStr + "/humidity").c_str(), humidityStr.c_str());
    client.publish((preTopicStr + "/voc").c_str(), vocStr.c_str());   // new
    client.publish((preTopicStr + "/co2eq").c_str(), co2Str.c_str()); // new
    client.publish((preTopicStr + "/gas").c_str(), gasStr.c_str());   // new
    client.publish((preTopicStr + "/rssi").c_str(), RSSIStr.c_str());
    client.publish((preTopicStr + "/ip").c_str(), IPStr.c_str());

    //    Serial.print("MQTT status: ");
    //    Serial.println(client.state());
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

    ArduinoOTA.setHostname(device_name);
    ArduinoOTA.begin();

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

void debug()
{
    vector<float> tempHistory = loadVector<float>("tmp");
    vector<float> humiHistory = loadVector<float>("hum");
    vector<float> pressureHistory = loadVector<float>("pre");
    vector<int> aqHistory = loadVector<int>("aqh");

    printVectorDebug(tempHistory, "tempHistory");
    printVectorDebug(humiHistory, "humiHistory");
    printVectorDebug(pressureHistory, "pressureHistory");
    printVectorDebug(aqHistory, "aqHistory");
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
        if (currentMode > 7)
            currentMode = 0;
    }

    if (millis() - lastModeChange > 20000 && lastAnimationChange + animationDelay < millis() && currentMode != 6)
    {
        lastAnimationChange = millis();
        animationDelay = (currentMode == 5) ? 10000 : 5000;
        currentMode = (currentMode + 1) % 8;
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
        //        Serial.println(WiFi.localIP());
        lastMode = currentMode;
    }
}
