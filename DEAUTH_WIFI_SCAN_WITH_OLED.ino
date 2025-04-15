#include "main_menu.h"
#include "config.h"
#include "ButtonManager.h"
#include <Wire.h>
#include <ESP8266WiFi.h>
#include "wifi.h" // Include the WiFi functionalities

// Enum to keep track of the current screen
enum Screen {
    MAIN_MENU,
    WIFI_MENU,
    DEAUTH_MENU,
    SETTINGS_MENU,
    SHOW_SAVED_NETWORKS
};

Screen currentScreen = MAIN_MENU; // Start at the main menu
int currentMenuIndex = 0;         // Current selected index
MainMenu OledDisplay;             // OLED display object
ButtonManager buttons;            // Button manager for button presses

WifiMenu wifiMenu;  // Create WifiMenu object to handle WiFi functionalities

// Forward declarations of functions
void showCurrentScreen(bool withTransition = false);
void handleSelection(int selectedIndex);
void handleWiFiSelection(int selectedIndex);
int getCurrentMenuCount();

// ==========================
// Initialization
// ==========================
void setup() {
    Serial.begin(115200);
    EEPROM.begin(512);
    wifiMenu.initializeEEPROM();
    debugEEPROM();
    Serial.println("Starting.....");
    OledDisplay.begin();  // Initialize OLED
    buttons.begin();      // Initialize button manager

    // Show the main menu on the screen
    showCurrentScreen(true);
}

// ==========================
// Debug EEPROM
// ==========================
// Add this to your setup() function or call it from a menu option:
void debugEEPROM() {
  Serial.println(F("======= EEPROM DEBUG ======="));
  
  // Check the network count first
  uint8_t count = EEPROM.read(EEPROM_START_ADDR);
  Serial.print(F("Saved network count: "));
  Serial.println(count);
  
  if (count > MAX_NETWORKS) {
    Serial.println(F("WARNING: Count exceeds maximum! Possible corruption."));
  }
  
  // Try to read each network
  for (int i = 0; i < count && i < MAX_NETWORKS; i++) {
    Serial.print(F("Network #"));
    Serial.print(i);
    Serial.println(F(":"));
    
    String ssid, bssid;
    bool success = wifiMenu.getSavedNetwork(i, ssid, bssid);
    
    if (success) {
      Serial.print(F("  SSID: "));
      Serial.println(ssid);
      Serial.print(F("  BSSID: "));
      Serial.println(bssid);
    } else {
      Serial.println(F("  ERROR: Failed to read network!"));
    }
  }
  
  Serial.println(F("============================"));
}

// ==========================
// Show the Current Screen
// ==========================
void showCurrentScreen(bool withTransition) {
    switch (currentScreen) {
        case MAIN_MENU:
            OledDisplay.showMenu(menuItems, menuItemCount, currentMenuIndex);
            break;

        case WIFI_MENU:
            OledDisplay.showSubMenu("WiFi Scan", wifiSubMenuItems, wifiSubMenuItemCount, currentMenuIndex);
            break;

        case DEAUTH_MENU:
            OledDisplay.showSubMenu("Deauth", deauthSubMenuItems, deauthSubMenuItemCount, currentMenuIndex);
            break;

        case SETTINGS_MENU:
            OledDisplay.showSubMenu("Settings", settingSubMenuItems, settingSubMenuItemCount, currentMenuIndex);
            break;
        case SHOW_SAVED_NETWORKS:
            OledDisplay.showSavedNetworks();
            break;
    }
}

// ==========================
// Main Loop
// ==========================
void loop() {
    Button btn = buttons.readButton();  // Read button press

    switch (btn) {
        case UP:
            currentMenuIndex--;
            if (currentMenuIndex < 0) {
                currentMenuIndex = getCurrentMenuCount() - 1;
            }
            showCurrentScreen(true);  // Update screen with transition
            break;

        case DOWN:
            currentMenuIndex++;
            if (currentMenuIndex >= getCurrentMenuCount()) {
                currentMenuIndex = 0;
            }
            showCurrentScreen(true);  // Update screen with transition
            break;

        case SELECT:
            handleSelection(currentMenuIndex);  // Handle selection action
            break;

        case BACK:
            // Go back to the main menu from any submenu
            if (currentScreen != MAIN_MENU) {
                currentScreen = MAIN_MENU;
                currentMenuIndex = 0;  // Reset to the first menu item in main menu
                showCurrentScreen(true);  // Update screen with transition
            }
            break;

        case NONE:
        default:
            // Do nothing if no button is pressed
            break;
    }
}

// ==========================
// Handle Menu Selection
// ==========================
void handleSelection(int selectedIndex) {
    switch (currentScreen) {
        case MAIN_MENU:
            switch (selectedIndex) {
                case 0:  // "WiFi Scan"
                    currentScreen = WIFI_MENU;
                    break;
                case 1:  // "Deauth"
                    currentScreen = DEAUTH_MENU;
                    break;
                case 2:  // "Settings"
                    currentScreen = SETTINGS_MENU;
                    break;
                case 3: // "Show Saved Networks"
                    currentScreen = SHOW_SAVED_NETWORKS;
                    break;
                default:
                    break;
            }
            break;

       case WIFI_MENU:
    if (selectedIndex == getCurrentMenuCount() - 1) {
        currentScreen = MAIN_MENU;
    } else {
        handleWiFiSelection(selectedIndex);  // <-- Trigger the WiFi actions!
    }
    break;

case DEAUTH_MENU:
    if (selectedIndex == getCurrentMenuCount() - 1) {
        currentScreen = MAIN_MENU;
    } else {
        // handleDeauthSelection(selectedIndex); // if implemented
    }
    break;

case SHOW_SAVED_NETWORKS:
    if(selectedIndex == getCurrentMenuCount() - 1){
        currentScreen = MAIN_MENU;
    }
    else{
        
    }
    break;

case SETTINGS_MENU:
    if (selectedIndex == getCurrentMenuCount() - 1) {
        currentScreen = MAIN_MENU;
    } else {
        // handleSettingsSelection(selectedIndex); // if implemented
    }
    break;

    }
    showCurrentScreen(true);  // Show the updated screen with transition
}

// ==========================
// Get the Current Menu Count
// ==========================
int getCurrentMenuCount() {
    switch (currentScreen) {
        case MAIN_MENU: return menuItemCount;
        case WIFI_MENU: return wifiSubMenuItemCount;
        case DEAUTH_MENU: return deauthSubMenuItemCount;
        case SETTINGS_MENU: return settingSubMenuItemCount;
        case SHOW_SAVED_NETWORKS: return 1;
        default: return 0;
    }
}

// ==========================
// Handle WiFi Submenu Selection
// ==========================
void handleWiFiSelection(int selectedIndex) {
    switch (selectedIndex) {
        case 0:  // WiFi Scan
            wifiMenu.scanNetworks();  // Perform network scan
            break;
        case 1:  // Show Networks
            wifiMenu.showScannedNetworks();  // Show list of scanned networks
            break;
        case 2:  // Filter Networks
            wifiMenu.filterNetworks();  // Filter networks based on criteria
            break;
        default:
            break;
    }
}