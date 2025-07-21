#pragma once
#include <string>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define SDA_PIN 21
#define SCL_PIN 22

#define MQ135_PIN 34

#define BUTTON_PIN_MODE 13
#define BUTTON_PIN_RIGHT 14
#define BUTTON_PIN_LEFT 23

using namespace std;

const char *mqtt_server = "x.x.x.x";
const int mqtt_port = 1883;
const char *mqtt_user = "mqtt brocker username";
const char *mqtt_pass = "mqtt brocker password";

const char *ssid = "your wifi name/ssid";
const char *password = "your wifi password";

const char *device_name = "meteo0";
const string preTopicStr = string("meteo/") + device_name;
const char *preTopic = preTopicStr.c_str();

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
