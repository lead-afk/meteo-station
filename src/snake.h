#pragma once
#include <vector>
#include <Wire.h>

#include "utils.h"
#include "devices.h"

using namespace std;

class Snake
{
public:
    void game(Adafruit_SSD1306 &display)
    {
        if (millis() < deathTime + 2000 && dead)
        {
            deathScreen(display);
            return;
        }
        dead = false;
        snake(display);
    }

    void setSnakeLeftRegistered(bool flag)
    {
        snakeLeftRegistered = flag;
    }

    void setSnakeRightRegistered(bool flag)
    {
        snakeRightRegistered = flag;
    }

private:
    bool snakeSetup = true;
    vector<pair<int, int>> directions = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
    int currentDirection = 0;
    pair<int, int> applePosition = {1, 0};
    vector<pair<int, int>> snakeBody = {{64, 32}};
    unsigned long lastUpdateSnake = 0;
    unsigned long appleBlink = 0;
    bool appleFlag = true;
    bool snakeLeftRegistered = false;
    bool snakeRightRegistered = false;
    bool dead = false;
    unsigned long deathTime = 0;
    void deathScreen(Adafruit_SSD1306 &display)
    {
        display.clearDisplay();
        display.setCursor(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        display.println("You died!");
        display.display();
    }
    void resetSnake()
    {
        directions = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
        currentDirection = 0;
        applePosition.first = random(0, SCREEN_WIDTH);
        applePosition.second = random(0, SCREEN_HEIGHT);
        snakeBody = {{64, 32}, {63, 32}, {62, 32}};
        lastUpdateSnake = 0;
        dead = false;
        deathTime = 0;
        snakeSetup = false;
    }
    void deadSnake()
    {
        resetSnake();
        dead = true;
        deathTime = millis();
    }
    void snake(Adafruit_SSD1306 &display)
    {
        if (snakeSetup)
            resetSnake();

        if (lastUpdateSnake + 300 < millis())
        {
            lastUpdateSnake = millis();

            if (snakeRightRegistered)
            {

                currentDirection++;
                if (currentDirection > 3)
                {
                    currentDirection = 0;
                }
            }
            else if (snakeLeftRegistered)
            {
                currentDirection--;
                if (currentDirection < 0)
                {
                    currentDirection = 3;
                }
            }
            snakeRightRegistered = false;
            snakeLeftRegistered = false;

            pair<int, int> newHead = snakeBody[0] + directions[currentDirection];
            for (size_t i = 0; i < snakeBody.size(); i++)
            {
                if (equalPair(newHead, snakeBody[i]))
                {
                    deadSnake();
                    return;
                }
            }

            if (newHead.first >= SCREEN_WIDTH || newHead.first < 0 || newHead.second >= SCREEN_HEIGHT || newHead.second < 0)
            {
                deadSnake();
                return;
            }

            snakeBody.insert(snakeBody.begin(), newHead);
            if (!equalPair(applePosition, newHead))
            {
                snakeBody.pop_back();
            }
            else
            {
                applePosition.first = random(0, SCREEN_WIDTH);
                applePosition.second = random(0, SCREEN_HEIGHT);
            }
        }

        display.clearDisplay();

        for (size_t i = 0; i < snakeBody.size(); i++)
        {
            display.drawPixel(snakeBody[i].first, snakeBody[i].second, SSD1306_WHITE);
        }

        if (appleBlink + 200 < millis())
        {
            appleFlag = !appleFlag;
            appleBlink = millis();
        }

        if (appleFlag)
        {
            display.drawPixel(applePosition.first, applePosition.second, SSD1306_WHITE);
        }

        display.display();
    }
};