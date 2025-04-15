#ifndef CONFIG_H
#define CONFIG_H

// ===================== Wi-Fi Configuration =====================
#define WIFI_SSID       "Your Wifi Name"
#define WIFI_PASSWORD   "Wifi Password"
#define WIFI_SCAN_TIMEOUT 10     // In seconds
#define MAX_SCAN_RESULTS 20

// ===================== Button Pins =====================
#define BUTTON_UP_PIN      D6
#define BUTTON_DOWN_PIN    D3
#define BUTTON_SELECT_PIN  D4
#define BUTTON_BACK_PIN    D5

// ===================== LED & Buzzer =====================
#define LED_PIN            D8
#define BUZZER_PIN         D7

// ===================== OLED Display =====================
#define OLED_SDA_PIN       D2
#define OLED_SCL_PIN       D1
#define SCREEN_WIDTH       128
#define SCREEN_HEIGHT      64
#define OLED_RESET         -1
#define OLED_ADDRESS       0x3C

// ===================== Menu Configuration =====================
#define MAX_MENU_ITEMS        10
#define MAX_SUB_MENU_ITEMS    10

// ===================== Menu Items =====================
extern const char* menuItems[];
extern const int menuItemCount;

extern const char* wifiSubMenuItems[];
extern const int wifiSubMenuItemCount;

extern const char* deauthSubMenuItems[];
extern const int deauthSubMenuItemCount;

extern const char* settingSubMenuItems[];
extern const int settingSubMenuItemCount;

extern const char* filterOptions[];
extern const int filterOptionCount;

// =============== Buzzer State ======================
extern bool isBuzzerEnabled; 

#endif