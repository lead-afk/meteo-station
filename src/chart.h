#pragma once
#include <vector>
#include <string>
#include "devices.h"
#include "sensors.h"
#include "utils.h"
#include <PubSubClient.h>
#include "saveVector.h"

void postError(string error)
{
    client.publish((preTopicStr + "/error").c_str(), (error).c_str());
}

using namespace std;

class ChartHandler
{
public:
    ChartHandler()
    {
        tempHistory.reserve(areaWidth);
        humiHistory.reserve(areaWidth);
        pressureHistory.reserve(areaWidth);
        aqHistory.reserve(areaWidth);

        tempHistoryRecent.reserve(10);
        humiHistoryRecent.reserve(10);
        pressureHistoryRecent.reserve(10);
        aqHistoryRecent.reserve(10);
    }

    void displayChart(int id, Adafruit_SSD1306 &display)
    {
        if (!(millis() > lastChartRefresh + updateDelay || lastID != id))
        {
            return;
        }
        lastChartRefresh = millis();
        lastID = id;

        switch (id)
        {
        case 0:
        {
            auto data = tempHistory.empty() ? tempHistoryRecent : tempHistory;
            if (!data.empty())
                data.back() = getTemperature();
            showChart(data, "Temperature", "C", display);
            break;
        }
        case 1:
        {
            auto data = humiHistory.empty() ? humiHistoryRecent : humiHistory;
            if (!data.empty())
                data.back() = getHumidity();
            showChart(data, "Humidity", "%", display);
            break;
        }
        case 2:
        {
            auto data = pressureHistory.empty() ? pressureHistoryRecent : pressureHistory;
            if (!data.empty())
                data.back() = getPressure();
            showChart(data, "Pressure", "hPa", display);
            break;
        }
        case 3:
        {
            auto data = aqHistory.empty() ? aqHistoryRecent : aqHistory;
            if (!data.empty())
                data.back() = getIAQ();
            showChart(data, "Air Quality", "Raw", display);
            break;
        }
        case 4:
        {
            auto data = co2History.empty() ? co2HistoryRecent : co2History;
            if (!data.empty())
                data.back() = getIAQ();
            showChart(data, "CO2", "ppm", display);
            break;
        }
        case 5:
        {
            auto data = vocHistory.empty() ? vocHistoryRecent : vocHistory;
            if (!data.empty())
                data.back() = getIAQ();
            showChart(data, "VOC", "ppm", display);
            break;
        }
        }
    }

    void refreshData()
    {
        if (millis() > lastRecent + updateDelay)
        {
            tempHistoryRecent.push_back(getTemperature());
            humiHistoryRecent.push_back(getHumidity());
            pressureHistoryRecent.push_back(getPressure());
            aqHistoryRecent.push_back(getIAQ());
            co2HistoryRecent.push_back(getCO2Equivalent());
            vocHistoryRecent.push_back(getVOC());

            lastRecent = millis();
        }
        if (tempHistoryRecent.size() * updateDelay > timeBetweenPoints)
        {
            addAverageToHistory(tempHistoryRecent, tempHistory);
            addAverageToHistory(humiHistoryRecent, humiHistory);
            addAverageToHistory(pressureHistoryRecent, pressureHistory);
            addAverageToHistory(aqHistoryRecent, aqHistory);
            addAverageToHistory(co2HistoryRecent, co2History);
            addAverageToHistory(vocHistoryRecent, vocHistory);

            save();
        }
    }

    void load()
    {
        try
        {
            tempHistory = loadVector<float>("tmp");
            humiHistory = loadVector<float>("hum");
            pressureHistory = loadVector<float>("pre");
            aqHistory = loadVector<int>("aqh");
            co2History = loadVector<float>("cot");
            vocHistory = loadVector<float>("voc");
        }
        catch (const std::exception &e)
        {
            Serial.println(e.what());
            postError(e.what());
        }
    }

private:
    vector<float> tempHistory;
    vector<float> humiHistory;
    vector<float> pressureHistory;
    vector<int> aqHistory;
    vector<float> co2History;
    vector<float> vocHistory;

    vector<float> tempHistoryRecent;
    vector<float> humiHistoryRecent;
    vector<float> pressureHistoryRecent;
    vector<int> aqHistoryRecent;
    vector<float> co2HistoryRecent;
    vector<float> vocHistoryRecent;

    unsigned long lastRecent = 0;

    const unsigned long updateDelay = 2000;

    const int areaHeight = SCREEN_HEIGHT - 18;
    const int areaWidth = SCREEN_WIDTH - 1;
    const unsigned long timeBetweenPoints = (10 * 60 * 60 * 1000) / areaWidth;

    int lastID = -1;
    unsigned long lastChartRefresh = 0;

    template <typename T>
    void showChart(vector<T> history, string name, string unit, Adafruit_SSD1306 &display)
    {
        if (history.empty())
        {
            display.clearDisplay();
            display.setCursor(0, 0);
            display.println("History empty!");
            display.display();
            return;
        }

        T min = history[0];
        T max = history[0];

        for (size_t i = 1; i < history.size(); ++i)
        {
            if (history[i] < min)
            {
                min = history[i];
            }
            if (history[i] > max)
            {
                max = history[i];
            }
        }

        float minF = min;
        float maxF = max;
        if (maxF == minF)
            maxF += 0.01f;
        display.clearDisplay();
        display.setCursor(0, 48);
        display.print(name.c_str());
        display.print(": ");
        display.print(history.back());
        display.print(" ");
        display.println(unit.c_str());
        display.print("Min: ");
        display.print(min, 1);
        display.print(" Max: ");
        display.print(max, 1);
        display.drawLine(0, 0, 0, areaHeight + 1, SSD1306_WHITE);
        display.drawLine(0, areaHeight + 1, areaWidth, areaHeight + 1, SSD1306_WHITE);

        if (history.size() > 1)
        {
            float step = SCREEN_WIDTH / float(history.size() - 1);

            for (size_t i = 1; i < history.size(); i++)
            {
                const float valuePre = history[i - 1];
                const float valueThis = history[i];

                const float scaledPreRatio = (valuePre - minF) / (maxF - minF);
                const float scaledThisRatio = (valueThis - minF) / (maxF - minF);

                const int xp = (i - 1) * step + 1;
                const int yp = areaHeight - areaHeight * scaledPreRatio;
                const int xn = (i == history.size() - 1) ? areaWidth : i * step + 1;
                const int yn = areaHeight - areaHeight * scaledThisRatio;

                display.drawLine(xp, yp, xn, yn, SSD1306_WHITE);
            }
        }
        else
        {
            display.drawLine(0, areaHeight / 2, areaWidth, areaHeight / 2, SSD1306_WHITE);
        }

        display.display();
    }

    template <typename T>
    void addAverageToHistory(vector<T> &recent, vector<T> &history)
    {
        double sum = 0;
        for (const auto &v : recent)
            sum += v;

        T avg = static_cast<T>(sum / recent.size());

        if (history.size() >= areaWidth)
            history.erase(history.begin());

        history.push_back(avg);
        recent.clear();
    }

    void save()
    {
        try
        {
            saveVector("tmp", tempHistory);
            saveVector("hum", humiHistory);
            saveVector("pre", pressureHistory);
            saveVector("aqh", aqHistory);
            saveVector("cot", co2History);
            saveVector("voc", vocHistory);
        }
        catch (const std::exception &e)
        {
            Serial.println(e.what());
            postError(e.what());
        }
    }
};
