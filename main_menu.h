#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "ButtonManager.h"
#include <Wire.h>

#define MAX_NETWORKS 5 

class MainMenu {
public:
    void begin();
    void clear();
    void showMenu(const char* options[], int count, int selectedIndex);
    void showSubMenu(const char* title, const char* options[], int count, int selectedIndex);
    void showMessage(const String& message);
    void showCenteredMessage(const String& message, int yOffset = 0);
    void fadeTransition();
    void renderBoxMenu(const char* title, const char* options[], int count, int selectedIndex, bool useTransition);
    void showLoadingBar(int percentage);
    void showSavedNetworks();
    int showNetworkOptionsMenu(const String& ssid);
    bool confirmDelete(const String& ssid);
    void showNetworkDetails(const String& ssid, const String& bssid);
};

// Global objects accessible from any file that includes main_menu.h
extern Adafruit_SSD1306 display;
extern ButtonManager buttonManager;

#endif