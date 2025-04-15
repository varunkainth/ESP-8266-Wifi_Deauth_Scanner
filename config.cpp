#include "config.h"

const char* menuItems[] = {
    "WiFi Scan",
    "Deauth",
    "Settings",
    "Show Saved Networks"
};

const int menuItemCount = sizeof(menuItems) / sizeof(menuItems[0]);

const char* wifiSubMenuItems[] = {
    "Scan",
    "Show Networks",
    "Filter",
    "Go Back"
};

const int wifiSubMenuItemCount = sizeof(wifiSubMenuItems) / sizeof(wifiSubMenuItems[0]);

const char* deauthSubMenuItems[] = {
    "Select AP",
    "Select Client",
    "Attack Type",
    "Packet Count",
    "Start Attack",
    "Go Back"
};

const int deauthSubMenuItemCount = sizeof(deauthSubMenuItems) / sizeof(deauthSubMenuItems[0]);

const char* settingSubMenuItems[] = {
    "Buzzer Toggle",
    "Display Brightness",
    "TimeOut Settings",
    "Firmware Info",
    "Go Back"
};

const int settingSubMenuItemCount = sizeof(settingSubMenuItems) / sizeof(settingSubMenuItems[0]); 

const char* filterOptions[] = {
    "Only Open Networks",
    "Only WPA/WPA2",
    "Strong Signal (> -70dBm)",
    "Sort by Signal Strength",
    "Show All Again",
    "Go Back"
};

const int filterOptionCount = sizeof(filterOptions) / sizeof(filterOptions[0]);

bool isBuzzerEnabled = true; // Default buzzer state