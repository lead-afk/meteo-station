#include "devices.h"
#include "sensors.h"

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
    float voc = getVOC();
    float co2 = getCO2Equivalent();
    float gasRes = getGasResistance();
    int iaqAcc = getIAQAccuracy();

    // Prepare payloads
    String tempStr = String(temperature, 2);
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
    client.publish((preTopicStr + "/aq_acc").c_str(), iaqAccStr.c_str());
    client.publish((preTopicStr + "/humidity").c_str(), humidityStr.c_str());
    client.publish((preTopicStr + "/voc").c_str(), vocStr.c_str());
    client.publish((preTopicStr + "/co2eq").c_str(), co2Str.c_str());
    client.publish((preTopicStr + "/gas").c_str(), gasStr.c_str());
    client.publish((preTopicStr + "/rssi").c_str(), RSSIStr.c_str());
    client.publish((preTopicStr + "/ip").c_str(), IPStr.c_str());

    //    Serial.print("MQTT status: ");
    //    Serial.println(client.state());
}