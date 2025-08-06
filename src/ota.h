#include <ArduinoOTA.h>

#include "env.h"
#include "devices.h"

String getLoadingBar(uint8_t percentage)
{
    const uint8_t lineWidth = 21;
    const uint8_t barWidth = lineWidth - 2;

    uint8_t filled = (percentage * barWidth) / 100;
    uint8_t empty = barWidth - filled;

    String bar = "[";
    for (uint8_t i = 0; i < filled; i++)
        bar += "#";
    for (uint8_t i = 0; i < empty; i++)
        bar += ".";
    bar += "]";
    return bar;
}

float lastOta = 0.0;
void setupOTA()
{
    ArduinoOTA.setHostname(device_name);

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { float otaProgressStatus = (progress / (float)total) * 100.0;
                            if (otaProgressStatus - lastOta > 1)
                            {
                                lastOta = otaProgressStatus;
                                display.clearDisplay();
                                display.setCursor(0,0);
                                display.println("Ota update!");
                                display.println();
                                display.print(int(otaProgressStatus));
                                display.println("%");
                                display.println();
                                display.println();
                                display.println(getLoadingBar(otaProgressStatus));
                                display.display(); 
                            } });

    ArduinoOTA.begin();
}
