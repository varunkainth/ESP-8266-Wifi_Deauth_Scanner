#include "main_menu.h"
#include "config.h"
#include "wifi.h"
#include <EEPROM.h>

// OLED Display Object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Global instances
WifiMenu wifi;
ButtonManager buttonManager; // Renamed for consistency with class name

extern void debugEEPROM();
// Last time an animation frame was updated
unsigned long lastAnimationTime = 0;
const unsigned long ANIMATION_DELAY = 5; // ms between animation frames

// ==========================
// Initialization
// ==========================
void MainMenu::begin() {
    Serial.println(F("Initializing OLED..."));
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    
    // Non-blocking delay alternative
    unsigned long startTime = millis();
    while (millis() - startTime < 600) {
        yield(); // Allow ESP8266 to handle background tasks
    }

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        while (true) { 
            yield(); // Keep system responsive even in error state
        }
    }

    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    clear();

    Serial.println(F("OLED initialized successfully!"));
    showCenteredMessage(F("Booting..."));
    
    // Non-blocking delay alternative
    startTime = millis();
    while (millis() - startTime < 1000) {
        yield();
    }
    
    clear();
}

// ==========================
// Clear Display
// ==========================
void MainMenu::clear() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.display();
}

// ==========================
// Fade Transition Effect
// ==========================
void MainMenu::fadeTransition() {
    lastAnimationTime = 0; // Reset animation timer
    
    for (int i = 0; i < SCREEN_WIDTH; i += 4) {
        // Non-blocking animation timing
        unsigned long currentTime = millis();
        if (currentTime - lastAnimationTime >= ANIMATION_DELAY) {
            lastAnimationTime = currentTime;
            display.fillRect(i, 0, 4, SCREEN_HEIGHT, SSD1306_BLACK);
            display.display();
        }
        yield(); // Let ESP8266 handle background tasks
    }
}

// ==========================
// Render Box Menu (Helper)
// ==========================
void MainMenu::renderBoxMenu(const char* title, const char* options[], int count, int selectedIndex, bool useTransition) {
    if (useTransition) {
        fadeTransition();
    }
    clear();

    const int itemHeight = 10;
    const int visibleItems = (SCREEN_HEIGHT - 16) / itemHeight; // 16 for title and border

    // Calculate start index (with boundary checking)
    int startIndex = max(0, min(selectedIndex - visibleItems / 2, count - visibleItems));
    if (startIndex < 0) startIndex = 0; // In case count < visibleItems

    // Draw title bar
    display.fillRect(0, 0, SCREEN_WIDTH, 12, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor((SCREEN_WIDTH - (strlen(title) * 6)) / 2, 2);
    display.print(title);

    // Draw menu box border
    display.drawRect(0, 12, SCREEN_WIDTH, SCREEN_HEIGHT - 12, SSD1306_WHITE);

    // Draw menu items
    for (int i = 0; i < visibleItems && (startIndex + i) < count; i++) {
        int menuItemIndex = startIndex + i;
        int y = 16 + i * itemHeight;

        // Highlight selected item
        if (menuItemIndex == selectedIndex) {
            display.fillRect(2, y - 1, SCREEN_WIDTH - 4, itemHeight, SSD1306_WHITE);
            display.setTextColor(SSD1306_BLACK);
        } else {
            display.setTextColor(SSD1306_WHITE);
        }

        display.setCursor(6, y);
        display.print(options[menuItemIndex]);
    }

    // Draw scroll indicators if needed
    if (startIndex > 0) {
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(SCREEN_WIDTH - 6, 13);
        display.print(F("^"));
    }
    if (startIndex + visibleItems < count) {
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(SCREEN_WIDTH - 6, SCREEN_HEIGHT - 8);
        display.print(F("v"));
    }

    display.display();
}

// ==========================
// Show Main Menu (Home)
// ==========================
void MainMenu::showMenu(const char* options[], int count, int selectedIndex) {
    renderBoxMenu("Home", options, count, selectedIndex, false);
}

// ==========================
// Show Sub Menu (Custom Title)
// ==========================
void MainMenu::showSubMenu(const char* title, const char* options[], int count, int selectedIndex) {
    renderBoxMenu(title, options, count, selectedIndex, false);
}

// ==========================
// Show Single Message
// ==========================
void MainMenu::showMessage(const String& message) {
    fadeTransition();
    clear();
    display.setCursor(0, 0);
    display.print(message);
    display.display();
}

//=============================
// Show Saved Networks
//=============================
void MainMenu::showSavedNetworks() {
    int networkCount = wifi.getSavedNetworkCount();
    int selectedIndex = 0;
    bool exitMenu = false;
    unsigned long lastButtonCheckTime = 0;
    const unsigned long BUTTON_CHECK_INTERVAL = 100; // ms between button checks
    
    // Cache for network data to avoid repeated EEPROM reads
    struct NetworkCache {
        String ssid;
        String bssid;
        bool valid;
    };
    NetworkCache* networkCache = new NetworkCache[MAX_NETWORKS];
    
    // Initialize cache as invalid
    for (int i = 0; i < MAX_NETWORKS; i++) {
        networkCache[i].valid = false;
    }
    
    Serial.print(F("showSavedNetworks: Found "));
    Serial.print(networkCount);
    Serial.println(F(" networks"));
    
    while (!exitMenu) {
        display.clearDisplay();
        
        // Title bar
        display.fillRect(0, 0, 128, 12, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setTextSize(1);
        display.setCursor(16, 2);
        display.print("SAVED NETWORKS");
        
        // Content area
        display.setTextColor(SSD1306_WHITE);
        
        if (networkCount == 0) {
            // No saved networks
            display.setCursor(10, 24);
            display.print("No networks saved");
            display.setCursor(15, 36);
            display.print("Scan and save");
            display.setCursor(8, 48);
            display.print("networks first");
        } else {
            // Show how many networks we have
            display.setCursor(0, 14);
            display.print("Networks: ");
            display.print(networkCount);
            display.print("/");
            display.print(MAX_NETWORKS);
            
            // Calculate visible networks (up to 3 at once)
            int startIdx = max(0, min(selectedIndex - 1, networkCount - 3));
            int endIdx = min(startIdx + 3, networkCount);
            
            // Draw list of networks
            for (int i = 0; i < (endIdx - startIdx); i++) {
                int networkIdx = startIdx + i;
                int y = 26 + i * 12;
                
                // Check if we need to load this network into cache
                if (!networkCache[networkIdx].valid) {
                    String ssid, bssid;
                    bool success = wifi.getSavedNetwork(networkIdx, ssid, bssid);
                    
                    if (success) {
                        networkCache[networkIdx].ssid = ssid;
                        networkCache[networkIdx].bssid = bssid;
                        networkCache[networkIdx].valid = true;
                    }
                }
                
                // Highlight selected network
                if (networkIdx == selectedIndex) {
                    display.fillRect(0, y, 128, 12, SSD1306_WHITE);
                    display.setTextColor(SSD1306_BLACK);
                } else {
                    display.setTextColor(SSD1306_WHITE);
                }
                
                if (networkCache[networkIdx].valid) {
                    String ssid = networkCache[networkIdx].ssid;
                    
                    // Extract SSID from saved network (if it contains signal strength info)
                    int bracketPos = ssid.lastIndexOf("(");
                    if (bracketPos > 0) {
                        ssid = ssid.substring(0, bracketPos);
                        ssid.trim();
                    }
                    
                    // Truncate if too long
                    if (ssid.length() > 18) {
                        ssid = ssid.substring(0, 15) + "...";
                    }
                    
                    // Display SSID
                    display.setCursor(2, y + 2);
                    display.print(ssid);
                } else {
                    // Error reading network
                    display.setCursor(2, y + 2);
                    display.print("[Read Error]");
                }
            }
            
            // Scroll indicators
            if (startIdx > 0) {
                display.setTextColor(SSD1306_WHITE);
                display.setCursor(120, 15);
                display.print("^");
            }
            if (endIdx < networkCount) {
                display.setTextColor(SSD1306_WHITE);
                display.setCursor(120, 56);
                display.print("v");
            }
            
            // Footer with instructions
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 56);
            display.print("SEL:Options  BACK:Exit");
        }
        
        display.display();
        
        // Non-blocking button handling with rate limiting
        unsigned long currentTime = millis();
        if (currentTime - lastButtonCheckTime >= BUTTON_CHECK_INTERVAL) {
            lastButtonCheckTime = currentTime;
            
            // Handle button input
            Button btn = buttonManager.readButton();
            
            if (btn == UP) {
                if (networkCount > 0) {
                    selectedIndex = (selectedIndex > 0) ? selectedIndex - 1 : networkCount - 1;
                }
            } 
            else if (btn == DOWN) {
                if (networkCount > 0) {
                    selectedIndex = (selectedIndex < networkCount - 1) ? selectedIndex + 1 : 0;
                }
            }
            else if (btn == SELECT) {
                if (networkCount > 0 && networkCache[selectedIndex].valid) {
                    String ssid = networkCache[selectedIndex].ssid;
                    String bssid = networkCache[selectedIndex].bssid;
                    
                    // Extract clean SSID from saved network
                    String displaySSID = ssid;
                    int bracketPos = ssid.lastIndexOf("(");
                    if (bracketPos > 0) {
                        displaySSID = ssid.substring(0, bracketPos);
                        displaySSID.trim();
                    }
                    
                    // Show options menu and handle the result
                    int optionResult = showNetworkOptionsMenu(displaySSID);
                    
                    if (optionResult == 1) { // View Details
                        showNetworkDetails(ssid, bssid);
                    } 
                    else if (optionResult == 2) { // Delete Network
                        if (confirmDelete(displaySSID)) {
                            wifi.deleteSavedNetwork(selectedIndex);
                            networkCount = wifi.getSavedNetworkCount();
                            
                            // Invalidate all cache entries
                            for (int i = 0; i < MAX_NETWORKS; i++) {
                                networkCache[i].valid = false;
                            }
                            
                            // Adjust selected index if needed
                            if (selectedIndex >= networkCount) {
                                selectedIndex = max(0, networkCount - 1);
                            }
                            
                            // Show confirmation
                            display.clearDisplay();
                            display.setTextColor(SSD1306_WHITE);
                            display.setCursor(10, 24);
                            display.print("Network deleted");
                            display.display();
                            
                            // Non-blocking delay
                            unsigned long deleteConfirmStart = millis();
                            while (millis() - deleteConfirmStart < 1500) yield();
                        }
                    } 
                    else if (optionResult == 3) { // Use for Deauth
                        // Select this network for deauth attack
                        display.clearDisplay();
                        display.setTextColor(SSD1306_WHITE);
                        display.setCursor(10, 24);
                        display.print("Network selected");
                        display.setCursor(10, 34);
                        display.print("for deauth attack");
                        display.display();
                        
                        // Non-blocking delay
                        unsigned long confirmStart = millis();
                        while (millis() - confirmStart < 1500) yield();
                        
                        // Flag this network for deauth
                        // (You might need to implement this storage mechanism)
                        
                        // Exit to the main menu
                        exitMenu = true;
                    }
                    // Option 0 or 4 (Cancel) just returns to the network list
                }
            }
            else if (btn == BACK) {
                exitMenu = true;
            }
        }
        
        yield(); // Allow background processes to run
    }
    
    // Clean up
    delete[] networkCache;
}

// Network options menu implementation - works with 4 buttons (UP, DOWN, SELECT, BACK)
int MainMenu::showNetworkOptionsMenu(const String& ssid) {
    const int optionCount = 4; // Number of options
    const char* options[optionCount] = {
        "View Details",
        "Delete Network",
        "Use for Deauth",
        "Cancel"
    };
    
    int selectedOption = 0;
    bool menuActive = true;
    unsigned long lastButtonCheckTime = 0;
    const unsigned long BUTTON_CHECK_INTERVAL = 100;
    
    while (menuActive) {
        display.clearDisplay();
        
        // Title bar
        display.fillRect(0, 0, SCREEN_WIDTH, 12, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(20, 2);
        display.print(F("NETWORK OPTIONS"));
        
        // Network name
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 14);
        // Truncate SSID if too long
        String displayName = ssid;
        if (displayName.length() > 21) {
            displayName = displayName.substring(0, 18) + "...";
        }
        display.print(displayName);
        
        // Draw options
        for (int i = 0; i < optionCount; i++) {
            int y = 26 + (i * 10);
            
            // Highlight selected option
            if (i == selectedOption) {
                display.fillRect(0, y - 1, SCREEN_WIDTH, 10, SSD1306_WHITE);
                display.setTextColor(SSD1306_BLACK);
            } else {
                display.setTextColor(SSD1306_WHITE);
            }
            
            display.setCursor(2, y);
            display.print(options[i]);
        }
        
        display.display();
        
        // Handle button input
        unsigned long currentTime = millis();
        if (currentTime - lastButtonCheckTime >= BUTTON_CHECK_INTERVAL) {
            lastButtonCheckTime = currentTime;
            
            Button btn = buttonManager.readButton();
            
            switch (btn) {
                case UP:
                    selectedOption = (selectedOption > 0) ? selectedOption - 1 : optionCount - 1;
                    break;
                    
                case DOWN:
                    selectedOption = (selectedOption < optionCount - 1) ? selectedOption + 1 : 0;
                    break;
                    
                case SELECT:
                    if (selectedOption == optionCount - 1) { // Cancel
                        menuActive = false;
                        return 0; // No action
                    } else {
                        menuActive = false;
                        return selectedOption + 1; // Return option index (1-based)
                    }
                    break;
                    
                case BACK:
                    menuActive = false;
                    return 0; // No action, same as Cancel
                    
                default:
                    break;
            }
        }
        
        yield(); // Allow background processes to run
    }
    
    return 0; // Default: no action
}



// Confirmation dialog for network deletion - works with 4 buttons
bool MainMenu::confirmDelete(const String& ssid) {
    bool confirmed = false; // Start with NO selected
    bool dialogActive = true;
    unsigned long lastButtonCheckTime = 0;
    const unsigned long BUTTON_CHECK_INTERVAL = 100;
    
    while (dialogActive) {
        display.clearDisplay();
        
        // Title
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(10, 8);
        display.print(F("Confirm Delete:"));
        
        // Network name
        display.setCursor(5, 22);
        // Truncate SSID if too long
        String displayName = ssid;
        if (displayName.length() > 20) {
            displayName = displayName.substring(0, 17) + "...";
        }
        display.print(displayName);
        
        // Options - highlight the selected option
        display.fillRect(5, 35, 50, 14, confirmed ? SSD1306_WHITE : SSD1306_BLACK);
        display.fillRect(73, 35, 50, 14, confirmed ? SSD1306_BLACK : SSD1306_WHITE);
        
        display.setTextColor(confirmed ? SSD1306_BLACK : SSD1306_WHITE);
        display.setCursor(19, 39);
        display.print(F("YES"));
        
        display.setTextColor(confirmed ? SSD1306_WHITE : SSD1306_BLACK);
        display.setCursor(87, 39);
        display.print(F("NO"));
        
        // Instructions for 4-button navigation
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(2, 55);
        display.print(F("UP/DN:Toggle SEL:Confirm"));
        
        display.display();
        
        // Handle button input
        unsigned long currentTime = millis();
        if (currentTime - lastButtonCheckTime >= BUTTON_CHECK_INTERVAL) {
            lastButtonCheckTime = currentTime;
            
            Button btn = buttonManager.readButton();
            
            switch (btn) {
                case UP:
                case DOWN:
                    confirmed = !confirmed; // Toggle between YES/NO with UP or DOWN
                    break;
                    
                case SELECT:
                    dialogActive = false;
                    return confirmed; // Return YES/NO selection
                    
                case BACK:
                    dialogActive = false;
                    return false; // Cancel = NO
                    
                default:
                    break;
            }
        }
        
        yield(); // Allow background processes to run
    }
    
    return false; // Default: No
}

// Network details viewer - works with 4 buttons
void MainMenu::showNetworkDetails(const String& ssid, const String& bssid) {
    bool viewActive = true;
    unsigned long lastButtonCheckTime = 0;
    const unsigned long BUTTON_CHECK_INTERVAL = 100;
    
    // Extract clean SSID
    String displaySSID = ssid;
    int bracketPos = ssid.lastIndexOf("(");
    if (bracketPos > 0) {
        displaySSID = ssid.substring(0, bracketPos);
        displaySSID.trim();
    }
    
    while (viewActive) {
        display.clearDisplay();
        
        // Title bar
        display.fillRect(0, 0, SCREEN_WIDTH, 12, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(12, 2);
        display.print(F("NETWORK DETAILS"));
        
        // Network details
        display.setTextColor(SSD1306_WHITE);
        
        // SSID
        display.setCursor(0, 16);
        display.print(F("SSID:"));
        
        // Show SSID, handling long names
        if (displaySSID.length() > 16) {
            // Show scrolling text or truncated name
            display.setCursor(0, 26);
            display.print(displaySSID.substring(0, 20));
            if (displaySSID.length() > 20) {
                display.print("...");
            }
        } else {
            display.setCursor(40, 16);
            display.print(displaySSID);
        }
        
        // BSSID
        display.setCursor(0, 36);
        display.print(F("BSSID:"));
        display.setCursor(40, 36);
        display.print(bssid);
        
        // Status
        display.setCursor(0, 46);
        display.print(F("Status: Saved for deauth"));
        
        // Footer
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(12, 56);
        display.print(F("Press BACK to return"));
        
        display.display();
        
        // Handle button input - only respond to BACK button
        unsigned long currentTime = millis();
        if (currentTime - lastButtonCheckTime >= BUTTON_CHECK_INTERVAL) {
            lastButtonCheckTime = currentTime;
            
            Button btn = buttonManager.readButton();
            if (btn == BACK) {
                viewActive = false;
            }
        }
        
        yield(); // Allow background processes to run
    }
}

// Show centered message
void MainMenu::showCenteredMessage(const String& message, int yOffset) {
    fadeTransition();
    clear();
    
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(message, 0, 0, &x1, &y1, &w, &h);
    int x = (SCREEN_WIDTH - w) / 2;
    int y = (SCREEN_HEIGHT - h) / 2 + yOffset;

    display.setCursor(x, y);
    display.print(message);
    display.display();
}

// Show loading bar
void MainMenu::showLoadingBar(int percentage) {
    percentage = constrain(percentage, 0, 100);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Centered "Loading..."
    display.setCursor((128 - 60) / 2, 18);
    display.print(F("Loading..."));
    
    // Draw loading bar border
    const int barX = 10, barY = 40, barWidth = 108, barHeight = 10;
    display.drawRect(barX, barY, barWidth, barHeight, SSD1306_WHITE);

    // Fill the bar according to the percentage
    int fillWidth = (int)(104.0 * percentage / 100.0);
    display.fillRect(barX + 2, barY + 2, fillWidth, barHeight - 4, SSD1306_WHITE);

    // Draw percentage text
    display.setCursor((128 - 30) / 2, 55);
    display.print(percentage);
    display.print(F("%"));

    display.display();
}