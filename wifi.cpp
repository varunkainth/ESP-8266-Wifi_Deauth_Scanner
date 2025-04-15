#include "wifi.h"
#include "config.h"
#include "main_menu.h"
#include "ButtonManager.h"

// External references
extern Adafruit_SSD1306 display;
extern ButtonManager buttonManager;

// Constants
const unsigned long BUTTON_CHECK_INTERVAL = 100; // ms between button checks
const unsigned long SCROLL_DELAY = 200; // ms between text scroll updates
const unsigned long ANIMATION_DELAY = 50; // ms between animation frames

// MAC Vendor lookup table (abbreviated)
const char* MAC_VENDORS[][2] = {
    {"00:11:22", "Cisco"},
    {"00:13:10", "Linksys"},
    {"00:18:4D", "Netgear"},
    {"00:1F:90", "D-Link"},
    {"00:25:9C", "Cisco-Linksys"},
    {"00:26:37", "Samsung"},
    {"00:50:BA", "D-Link"},
    {"00:90:4C", "Epigram"},
    {"08:86:3B", "Belkin"},
    {"0C:80:63", "TP-Link"},
    {"0C:D2:92", "Intel"},
    {"18:E8:29", "Ubiquiti"},
    {"1C:B7:2C", "ASUSTek"},
    {"30:AE:A4", "Espressif"},
    {"38:60:77", "Apple"},
    {"50:C7:BF", "TP-Link"},
    {"5C:CF:7F", "Espressif"},
    {"60:38:E0", "Belkin"},
    {"64:09:80", "Xiaomi"},
    {"74:DA:38", "Edimax"},
    {"94:10:3E", "Belkin"},
    {"AC:72:89", "Intel"},
    {"D0:15:4A", "TP-Link"},
    {"D8:0D:17", "TP-Link"},
    {"DC:A6:32", "Raspberry Pi"},
    {"F0:9F:C2", "Ubiquiti"}
};

// Constructor with improved initialization
WifiMenu::WifiMenu() : 
    filteredNetworkCount(0),
    filteredNetworks(nullptr),
    networkDetails(nullptr),
    deauthRunning(false),
    deauthStartTime(0),
    deauthPacketsSent(0),
    lastStatusUpdate(0),
    deauthDuration(0),
    targetAllClients(false)
{
    resetFilters();
    
    // Allocate memory for network storage
    filteredNetworks = new String[MAX_SCAN_RESULTS];
    networkDetails = new NetworkDetail[MAX_SCAN_RESULTS];
}

// Destructor to free memory
WifiMenu::~WifiMenu() {
    if (filteredNetworks) {
        delete[] filteredNetworks;
        filteredNetworks = nullptr;
    }
    
    if (networkDetails) {
        delete[] networkDetails;
        networkDetails = nullptr;
    }
}

void WifiMenu::initializeEEPROM() {
  // Always reset EEPROM on device startup
  uint8_t oldCount = EEPROM.read(EEPROM_START_ADDR);
  
  // Reset the network count to 0
  EEPROM.write(EEPROM_START_ADDR, 0);
  
  // Optionally, you can also clear the entire EEPROM area used for networks
  // This is more thorough but not strictly necessary
  for (int i = 0; i < MAX_NETWORKS * NETWORK_DATA_SIZE; i++) {
    EEPROM.write(EEPROM_START_ADDR + 1 + i, 0);
  }
  
  // Commit changes to EEPROM
  if (EEPROM.commit()) {
    Serial.print(F("EEPROM cleared on reset (was: "));
    Serial.print(oldCount);
    Serial.println(F(" networks)"));
  } else {
    Serial.println(F("ERROR: Failed to clear EEPROM on reset"));
  }
}
// Add a function to clear all saved networks
bool WifiMenu::clearAllNetworks() {
    EEPROM.write(EEPROM_START_ADDR, 0);
    if (EEPROM.commit()) {
        Serial.println(F("All networks cleared"));
        return true;
    } else {
        Serial.println(F("Failed to clear networks"));
        return false;
    }
}

// Reset filters to default values
void WifiMenu::resetFilters() {
    filterSettings.enabled = false;
    filterSettings.minSignal = -80;
    filterSettings.openOnly = false;
    filterSettings.hiddenOnly = false;
    filterSettings.channel24GHz = true;
    filterSettings.channel5GHz = true;
    filterSettings.ssidPattern = "";
    filterSettings.channelFilter = 0;
}

// Main filter method with improved flow and memory usage
void WifiMenu::filterNetworks() {
    // Store current settings to detect changes
    FilterSettings originalSettings = filterSettings;
    
    // Show the filter menu
    showFilterMenu();
    
    // Check if filter settings changed
    bool settingsChanged = (
        originalSettings.enabled != filterSettings.enabled ||
        originalSettings.minSignal != filterSettings.minSignal ||
        originalSettings.openOnly != filterSettings.openOnly ||
        originalSettings.hiddenOnly != filterSettings.hiddenOnly ||
        originalSettings.channel24GHz != filterSettings.channel24GHz ||
        originalSettings.channel5GHz != filterSettings.channel5GHz ||
        originalSettings.ssidPattern != filterSettings.ssidPattern ||
        originalSettings.channelFilter != filterSettings.channelFilter
    );
    
    // Only apply filters if settings changed or filters are enabled
    if (settingsChanged) {
        if (filterSettings.enabled) {
            // Show processing message
            display.clearDisplay();
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 0);
            display.print(F("Applying filters..."));
            display.display();
            
            // Non-blocking delay equivalent
            unsigned long startTime = millis();
            while (millis() - startTime < 200) yield();
            
            applyFilters(); // Apply the new filters
        } else {
            // If filters are disabled, just sort by signal strength
            display.clearDisplay();
            display.setCursor(0, 0);
            display.print(F("Sorting networks..."));
            display.display();
            
            sortBySignalStrength();
            
            display.clearDisplay();
            display.setCursor(0, 0);
            display.print(F("Networks sorted"));
            display.setCursor(0, 10);
            display.print(F("by signal strength"));
            display.display();
            
            // Non-blocking delay
            unsigned long startTime = millis();
            while (millis() - startTime < 1000) yield();
        }
        
        // Show the filtered/sorted results automatically
        showScannedNetworks();
    }
}

// Check if a network matches the current filters - optimized with const methods
bool WifiMenu::matchesFilters(int networkIndex) const {
    if (networkIndex < 0 || networkIndex >= filteredNetworkCount || !networkDetails) {
        return false;
    }
    
    const NetworkDetail& detail = networkDetails[networkIndex];
    
    // Check minimum signal strength
    if (detail.rssi < filterSettings.minSignal) {
        return false;
    }
    
    // Check open networks only
    if (filterSettings.openOnly && detail.encryption != "None") {
        return false;
    }
    
    // Check hidden networks
    if (filterSettings.hiddenOnly && detail.isHidden != "Yes") {
        return false;
    }
    
    // Check frequency band
    if (!filterSettings.channel24GHz && detail.band == "2.4GHz") {
        return false;
    }
    
    if (!filterSettings.channel5GHz && detail.band == "5GHz") {
        return false;
    }
    
    // Check specific channel
    if (filterSettings.channelFilter > 0 && detail.channel != filterSettings.channelFilter) {
        return false;
    }
    
    // Check SSID pattern
    if (filterSettings.ssidPattern.length() > 0) {
        // Get network SSID from filteredNetworks (stored with signal info)
        String fullNetwork = filteredNetworks[networkIndex];
        int bracketPos = fullNetwork.lastIndexOf('(');
        
        // Fixed the trim() issue - trim() modifies the string in-place and returns void
        String ssid;
        if (bracketPos > 0) {
            ssid = fullNetwork.substring(0, bracketPos);
            ssid.trim(); // Call trim() separately - it modifies the string in place
        } else {
            ssid = fullNetwork;
        }
        
        // Convert both to lowercase for case-insensitive matching
        String pattern = filterSettings.ssidPattern;
        pattern.toLowerCase();
        
        String ssidLower = ssid;
        ssidLower.toLowerCase();
        
        // Check if SSID contains the pattern
        if (ssidLower.indexOf(pattern) == -1) {
            return false;
        }
    }
    
    // If all filters passed, the network matches
    return true;
}


// Apply the current filters to the network list - optimized for performance
void WifiMenu::applyFilters() {
    if (!networkDetails || filteredNetworkCount == 0) {
        return;
    }
    
    // Create temporary arrays to store filtered results
    String* tempNetworks = new String[MAX_SCAN_RESULTS];
    NetworkDetail* tempDetails = new NetworkDetail[MAX_SCAN_RESULTS];
    int tempCount = 0;
    
    // Apply filters to each network
    for (int i = 0; i < filteredNetworkCount && tempCount < MAX_SCAN_RESULTS; i++) {
        if (matchesFilters(i)) {
            tempNetworks[tempCount] = filteredNetworks[i];
            tempDetails[tempCount] = networkDetails[i];
            tempCount++;
            
            // Update progress every 3 networks
            if (i % 3 == 0) {
                int progress = (i * 100) / filteredNetworkCount;
                drawProgressBar(10, 15, 108, 8, progress);
                display.display();
                yield(); // Allow WiFi and other tasks to run
            }
        }
    }
    
    // Update the filtered networks list
    for (int i = 0; i < tempCount; i++) {
        filteredNetworks[i] = tempNetworks[i];
        networkDetails[i] = tempDetails[i];
    }
    
    filteredNetworkCount = tempCount;
    
    // Clean up
    delete[] tempNetworks;
    delete[] tempDetails;
    
    // Sort by signal strength (strongest first)
    sortBySignalStrength();
    
    // Show results
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print(F("Filter applied"));
    display.setCursor(0, 10);
    display.print(F("Found "));
    display.print(filteredNetworkCount);
    display.print(F(" matching"));
    display.display();
    
    // Non-blocking delay
    unsigned long startTime = millis();
    while (millis() - startTime < 1500) yield();
}

// Show the filter menu - optimized for performance and memory
void WifiMenu::showFilterMenu() {
    bool keepRunning = true;
    int selectedOption = 0;
    const int numOptions = 9; // Total number of filter options
    bool valueEditMode = false;
    unsigned long lastButtonCheckTime = 0;
    
    // Store initial filter settings to detect changes
    FilterSettings originalSettings = filterSettings;
    
    // Option labels - stored in flash memory
    const char* optionLabelsProgmem[] = {
        "Enable Filters:",
        "Min Signal:",
        "Open Only:",
        "Hidden Only:",
        "Show 2.4GHz:",
        "Show 5GHz:",
        "SSID Pattern:",
        "Channel:",
        "APPLY & EXIT"
    };
    
    while (keepRunning) {
        display.clearDisplay();
        
        // Title bar
        display.fillRect(0, 0, SCREEN_WIDTH, 12, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor((SCREEN_WIDTH - 84) / 2, 2);
        display.print(F("FILTER OPTIONS"));
        
        // Main content area
        display.setTextColor(SSD1306_WHITE);
        
        // Calculate visible range (show 4 items at once)
        int startOption = max(0, selectedOption - 1);
        startOption = min(startOption, numOptions - 4);
        int endOption = min(numOptions, startOption + 4);
        
        // Prepare option values
        String optionValues[numOptions];
        optionValues[0] = filterSettings.enabled ? F("ON") : F("OFF");
        optionValues[1] = String(filterSettings.minSignal) + F(" dBm");
        optionValues[2] = filterSettings.openOnly ? F("YES") : F("NO");
        optionValues[3] = filterSettings.hiddenOnly ? F("YES") : F("NO");
        optionValues[4] = filterSettings.channel24GHz ? F("YES") : F("NO");
        optionValues[5] = filterSettings.channel5GHz ? F("YES") : F("NO");
        optionValues[6] = filterSettings.ssidPattern.length() > 0 ? filterSettings.ssidPattern : F("[NONE]");
        optionValues[7] = filterSettings.channelFilter > 0 ? String(filterSettings.channelFilter) : F("ALL");
        optionValues[8] = "";
        
        // Draw visible options
        for (int i = 0; i < (endOption - startOption); i++) {
            int idx = startOption + i;
            int y = 16 + i * 12;
            
            // Get option label from progmem
            String optionLabel = optionLabelsProgmem[idx];
            
            // Highlight selected option
            if (idx == selectedOption) {
                if (valueEditMode && idx < numOptions - 1) { 
                    // When in edit mode, highlight the whole row with a rectangle
                    display.drawRect(0, y - 1, SCREEN_WIDTH, 12, SSD1306_WHITE);
                    // And highlight the value with inverse colors
                    int valueWidth = optionValues[idx].length() * 6 + 4;
                    display.fillRect(SCREEN_WIDTH - valueWidth, y - 1, valueWidth, 12, SSD1306_WHITE);
                    display.setTextColor(SSD1306_BLACK);
                } else {
                    // When not in edit mode, highlight entire row
                    display.fillRect(0, y - 1, SCREEN_WIDTH, 12, SSD1306_WHITE);
                    display.setTextColor(SSD1306_BLACK);
                }
            } else {
                display.setTextColor(SSD1306_WHITE);
            }
            
            // Draw option label
            display.setCursor(4, y);
            display.print(optionLabel);
            
            // For the last option (APPLY), center it
            if (idx == numOptions - 1) {
                if (idx == selectedOption) {
                    display.fillRect(0, y - 1, SCREEN_WIDTH, 12, SSD1306_WHITE);
                    display.setTextColor(SSD1306_BLACK);
                } else {
                    display.setTextColor(SSD1306_WHITE);
                }
                display.setCursor((SCREEN_WIDTH - 75) / 2, y);
                display.print(optionLabel);
            } else {
                // Draw option value
                int valueX = SCREEN_WIDTH - (optionValues[idx].length() * 6) - 4;
                display.setCursor(valueX, y);
                
                // Set appropriate color for value
                if (!(valueEditMode && idx == selectedOption)) {
                    if (idx == selectedOption) {
                        display.setTextColor(SSD1306_BLACK);
                    } else {
                        display.setTextColor(SSD1306_WHITE);
                    }
                }
                
                display.print(optionValues[idx]);
            }
        }
        
        // Scroll indicators
        display.setTextColor(SSD1306_WHITE);
        if (startOption > 0) {
            display.setCursor(SCREEN_WIDTH - 6, 13);
            display.print(F("^"));
        }
        if (endOption < numOptions) {
            display.setCursor(SCREEN_WIDTH - 6, SCREEN_HEIGHT - 8);
            display.print(F("v"));
        }
        
        // Footer with instructions
        display.drawLine(0, SCREEN_HEIGHT - 10, SCREEN_WIDTH, SCREEN_HEIGHT - 10, SSD1306_WHITE);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(2, SCREEN_HEIGHT - 8);
        
        if (valueEditMode) {
            display.print(F("UP/DN: Change   SEL: Save"));
        } else {
            display.print(F("UP/DN: Move   SEL: Edit"));
        }
        
        display.display();
        
        // Non-blocking button handling with rate limiting
        unsigned long currentTime = millis();
        if (currentTime - lastButtonCheckTime >= BUTTON_CHECK_INTERVAL) {
            lastButtonCheckTime = currentTime;
            
            Button btn = buttonManager.readButton();
            
            if (valueEditMode) {
                // Value editing mode
                switch (btn) {
                    case UP:
                        // Increase value based on the option type
                        switch (selectedOption) {
                            case 0: // Enable Filters
                                filterSettings.enabled = !filterSettings.enabled;
                                break;
                            case 1: // Min Signal
                                filterSettings.minSignal = min(-30, filterSettings.minSignal + 5);
                                break;
                            case 2: // Open Only
                                filterSettings.openOnly = !filterSettings.openOnly;
                                break;
                            case 3: // Hidden Only
                                filterSettings.hiddenOnly = !filterSettings.hiddenOnly;
                                break;
                            case 4: // Show 2.4GHz
                                filterSettings.channel24GHz = !filterSettings.channel24GHz;
                                break;
                            case 5: // Show 5GHz
                                filterSettings.channel5GHz = !filterSettings.channel5GHz;
                                break;
                            case 6: // SSID Pattern
                                inputSsidPattern();
                                valueEditMode = false;
                                break;
                            case 7: // Channel
                                filterSettings.channelFilter++;
                                if (filterSettings.channelFilter > 14) filterSettings.channelFilter = 0;
                                break;
                        }
                        break;
                        
                    case DOWN:
                        // Decrease value based on the option type
                        switch (selectedOption) {
                            case 0: // Enable Filters
                                filterSettings.enabled = !filterSettings.enabled;
                                break;
                            case 1: // Min Signal
                                filterSettings.minSignal = max(-100, filterSettings.minSignal - 5);
                                break;
                            case 2: // Open Only
                                filterSettings.openOnly = !filterSettings.openOnly;
                                break;
                            case 3: // Hidden Only
                                filterSettings.hiddenOnly = !filterSettings.hiddenOnly;
                                break;
                            case 4: // Show 2.4GHz
                                filterSettings.channel24GHz = !filterSettings.channel24GHz;
                                break;
                            case 5: // Show 5GHz
                                filterSettings.channel5GHz = !filterSettings.channel5GHz;
                                break;
                            case 6: // SSID Pattern
                                inputSsidPattern();
                                valueEditMode = false;
                                break;
                            case 7: // Channel
                                filterSettings.channelFilter--;
                                if (filterSettings.channelFilter < 0) filterSettings.channelFilter = 14;
                                break;
                        }
                        break;
                        
                    case SELECT:
                        // Exit edit mode and save the value
                        valueEditMode = false;
                        break;
                        
                    case BACK:
                        // Exit edit mode without saving changes to this option
                        // Revert the current option to its original value
                        switch (selectedOption) {
                            case 0: filterSettings.enabled = originalSettings.enabled; break;
                            case 1: filterSettings.minSignal = originalSettings.minSignal; break;
                            case 2: filterSettings.openOnly = originalSettings.openOnly; break;
                            case 3: filterSettings.hiddenOnly = originalSettings.hiddenOnly; break;
                            case 4: filterSettings.channel24GHz = originalSettings.channel24GHz; break;
                            case 5: filterSettings.channel5GHz = originalSettings.channel5GHz; break;
                            case 6: filterSettings.ssidPattern = originalSettings.ssidPattern; break;
                            case 7: filterSettings.channelFilter = originalSettings.channelFilter; break;
                        }
                        valueEditMode = false;
                        break;
                        
                    default:
                        break;
                }
            } else {
                // Option selection mode
                switch (btn) {
                    case UP:
                        if (selectedOption > 0) selectedOption--;
                        break;
                        
                    case DOWN:
                        if (selectedOption < numOptions - 1) selectedOption++;
                        break;
                        
                    case SELECT:
                        if (selectedOption == numOptions - 1) {
                            // Apply and exit
                            // Show applying message
                            display.clearDisplay();
                            display.setCursor(0, 0);
                            display.print(F("Applying filters..."));
                            display.display();
                            
                            // Non-blocking delay
                            unsigned long startTime = millis();
                            while (millis() - startTime < 500) yield();
                            
                            keepRunning = false;
                        } else {
                            // Enter edit mode for this option
                            valueEditMode = true;
                            
                            // For SSID Pattern, directly call input function
                            if (selectedOption == 6) {
                                inputSsidPattern();
                                valueEditMode = false;
                            }
                        }
                        break;
                        
                    case BACK:
                        // Restore all original settings and exit
                        filterSettings = originalSettings;
                        keepRunning = false;
                        break;
                        
                    default:
                        break;
                }
            }
        }
        
        yield(); // Allow background processes to run
    }
}

// Helper for MAC vendor lookup - now with const qualifier
String WifiMenu::getMacVendor(const String& mac) const {
    // Extract first 8 characters (XX:XX:XX) for vendor matching
    String oui = mac.substring(0, 8);
    oui.toUpperCase(); // Convert to uppercase
    
    // Check against our vendor database
    for (unsigned int i = 0; i < sizeof(MAC_VENDORS) / sizeof(MAC_VENDORS[0]); i++) {
        if (oui.startsWith(MAC_VENDORS[i][0])) {
            return MAC_VENDORS[i][1];
        }
    }
    
    return F("Unknown");
}

// Scan for WiFi networks - optimized for memory and display updates
void WifiMenu::scanNetworks() {
    // Clean up previous results
    filteredNetworkCount = 0;
    
    // Clear the display and show scanning status
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print(F("Scanning WiFi..."));
    display.display();

    // Simulate loading bar during WiFi scan
    unsigned long startTime = millis();
    const unsigned long SCAN_ANIMATION_INTERVAL = 150;
    
    for (int i = 0; i <= 100; i += 5) {
        drawProgressBar(10, 20, 108, 10, i);
        display.display();
        
        // Non-blocking delay
        unsigned long expectedEndTime = startTime + (i * SCAN_ANIMATION_INTERVAL / 10);
        while (millis() < expectedEndTime) yield();
    }

    // Start the WiFi scan
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print(F("Scanning for networks..."));
    display.display();
    
    // Perform the actual WiFi scan
    int n = WiFi.scanNetworks();
    
    // Update display with scan results
    display.clearDisplay();
    display.setCursor(0, 0);
    
    if (n == 0) {
        display.print(F("No networks found"));
        display.display();
        
        // Non-blocking delay
        unsigned long noNetworkTime = millis();
        while (millis() - noNetworkTime < 2000) yield();
        
        return;
    }
    
    display.print(F("Found "));
    display.print(n);
    display.print(F(" networks"));
    display.display();
    
    // Non-blocking delay
    unsigned long foundNetworkTime = millis();
    while (millis() - foundNetworkTime < 1000) yield();

    // Process found networks
    for (int i = 0; i < n && filteredNetworkCount < MAX_SCAN_RESULTS; i++) {
        String ssid = WiFi.SSID(i);
        int rssi = WiFi.RSSI(i);
        String network = ssid + " (" + String(rssi) + " dBm)";
        
        // Filter networks based on predefined filter criteria
        if (isNetworkValid(network)) {
            filteredNetworks[filteredNetworkCount] = network;
            
            // Basic network details
            String bssid = WiFi.BSSIDstr(i);
            int channel = WiFi.channel(i);
            uint8_t encType = WiFi.encryptionType(i);
            bool isHidden = WiFi.isHidden(i);
            
            // Store basic details
            networkDetails[filteredNetworkCount].bssid = bssid;
            networkDetails[filteredNetworkCount].rssi = rssi;
            networkDetails[filteredNetworkCount].channel = channel;
            
            // Enhanced details - encryption type and auth mode
            switch (encType) {
                case ENC_TYPE_NONE: 
                    networkDetails[filteredNetworkCount].encryption = F("None");
                    networkDetails[filteredNetworkCount].authMode = F("Open");
                    networkDetails[filteredNetworkCount].securityProtocol = F("Open");
                    break;
                case ENC_TYPE_WEP: 
                    networkDetails[filteredNetworkCount].encryption = F("WEP");
                    networkDetails[filteredNetworkCount].authMode = F("Password");
                    networkDetails[filteredNetworkCount].securityProtocol = F("WEP");
                    break;
                case ENC_TYPE_TKIP: 
                    networkDetails[filteredNetworkCount].encryption = F("WPA/TKIP");
                    networkDetails[filteredNetworkCount].authMode = F("Password");
                    networkDetails[filteredNetworkCount].securityProtocol = F("WPA-PSK (TKIP)");
                    break;
                case ENC_TYPE_CCMP: 
                    networkDetails[filteredNetworkCount].encryption = F("WPA2/CCMP");
                    networkDetails[filteredNetworkCount].authMode = F("Password");
                    networkDetails[filteredNetworkCount].securityProtocol = F("WPA2-PSK (CCMP)");
                    break;
                case ENC_TYPE_AUTO: 
                    networkDetails[filteredNetworkCount].encryption = F("WPA/WPA2");
                    networkDetails[filteredNetworkCount].authMode = F("Password");
                    networkDetails[filteredNetworkCount].securityProtocol = F("WPA/WPA2-PSK");
                    break;
                default: 
                    networkDetails[filteredNetworkCount].encryption = F("Unknown");
                    networkDetails[filteredNetworkCount].authMode = F("Unknown");
                    networkDetails[filteredNetworkCount].securityProtocol = F("Unknown");
                    break;
            }
            
            // Enhanced details - additional information
            networkDetails[filteredNetworkCount].isHidden = isHidden ? F("Yes") : F("No");
            networkDetails[filteredNetworkCount].band = (channel > 14) ? F("5GHz") : F("2.4GHz");
            
            // Convert RSSI to quality percentage
            int qualityPercentage = constrain(map(rssi, -100, -50, 0, 100), 0, 100);
            networkDetails[filteredNetworkCount].quality = String(qualityPercentage) + "%";
            
            // Get vendor from MAC address
            networkDetails[filteredNetworkCount].vendor = getMacVendor(bssid);
            
            // Store scan time info
            networkDetails[filteredNetworkCount].scanTime = F("Now");
            
            // Calculate approximate distance
            float distance = pow(10, (-69 - (float)rssi) / (10 * 2));  // In meters
            if (distance < 1) {
                networkDetails[filteredNetworkCount].distance = F("<1m");
            } else if (distance > 100) {
                networkDetails[filteredNetworkCount].distance = F(">100m");
            } else {
                networkDetails[filteredNetworkCount].distance = String((int)distance) + "m";
            }
            
            filteredNetworkCount++;
            
            // Update progress indicator every few networks
            if (i % 3 == 0) {
                display.clearDisplay();
                display.setCursor(0, 0);
                display.print(F("Processing: "));
                display.print(i + 1);
                display.print(F("/"));
                display.print(n);
                
                int progress = ((i + 1) * 100) / n;
                drawProgressBar(10, 20, 108, 10, progress);
                display.display();
            }
            
            yield(); // Allow WiFi and system tasks to run
        }
    }

    // Sort networks by signal strength
    sortBySignalStrength();
}

// Show scanned networks with smooth scrolling and better memory usage
void WifiMenu::showScannedNetworks() {
    int selectedIndex = 0;
    const int visibleItems = 3;
    bool keepRunning = true;
    int scrollOffset = 0;
    unsigned long lastScrollTime = 0;
    unsigned long lastButtonCheckTime = 0;
    const int maxNormalChars = 16; // maximum chars to show when not selected

    while (keepRunning) {
        display.clearDisplay();

        // Title Bar
        display.fillRect(0, 0, SCREEN_WIDTH, 12, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor((SCREEN_WIDTH - 72) / 2, 2);
        display.print(F("WiFi Networks"));

        // Border
        display.drawRect(0, 12, SCREEN_WIDTH, SCREEN_HEIGHT - 12, SSD1306_WHITE);

        if (filteredNetworkCount == 0) {
            display.setTextColor(SSD1306_WHITE);
            display.setCursor((SCREEN_WIDTH - 96) / 2, SCREEN_HEIGHT / 2 - 4);
            display.print(F("No networks found"));
            display.setCursor((SCREEN_WIDTH - 108) / 2, SCREEN_HEIGHT / 2 + 6);
            display.print(F("Please scan again"));
        } else {
            int startIndex = selectedIndex - visibleItems / 2;
            startIndex = max(0, min(startIndex, filteredNetworkCount - visibleItems));
            if (startIndex < 0) startIndex = 0;

            for (int i = 0; i < visibleItems && (startIndex + i) < filteredNetworkCount; i++) {
                int idx = startIndex + i;
                int y = 16 + i * 16;
                String ssid = filteredNetworks[idx];
                bool isSelected = (idx == selectedIndex);
                
                if (isSelected) {
                    display.fillRect(2, y - 1, SCREEN_WIDTH - 4, 14, SSD1306_WHITE);
                    display.setTextColor(SSD1306_BLACK);
                    
                    // Draw scrolling text for selected item
                    drawScrollableText(ssid, 6, y, SCREEN_WIDTH - 12, scrollOffset, lastScrollTime);
                } else {
                    // Shortened text for non-selected items
                    display.setTextColor(SSD1306_WHITE);
                    display.setCursor(6, y);
                    
                    if (ssid.length() > maxNormalChars) {
                        display.print(ssid.substring(0, maxNormalChars - 3));
                        display.print(F("..."));
                    } else {
                        display.print(ssid);
                    }
                }

                // Dotted line under each
                for (int x = 4; x < SCREEN_WIDTH - 4; x += 4) {
                    display.drawPixel(x, y + 12, SSD1306_WHITE);
                }
            }

            // Scroll indicators
            display.setTextColor(SSD1306_WHITE);
            if (startIndex > 0) {
                display.setCursor(SCREEN_WIDTH - 6, 13);
                display.print(F("^"));
            }
            if ((startIndex + visibleItems) < filteredNetworkCount) {
                display.setCursor(SCREEN_WIDTH - 6, SCREEN_HEIGHT - 8);
                display.print(F("v"));
            }
        }

        display.display();

        // Non-blocking button handling
        unsigned long currentTime = millis();
        if (currentTime - lastButtonCheckTime >= BUTTON_CHECK_INTERVAL) {
            lastButtonCheckTime = currentTime;
            
            Button btn = buttonManager.readButton();
            switch (btn) {
                case UP:
                    if (selectedIndex > 0) {
                        selectedIndex--;
                        scrollOffset = 0; // Reset scroll when changing selection
                    }
                    break;
                case DOWN:
                    if (selectedIndex < filteredNetworkCount - 1) {
                        selectedIndex++;
                        scrollOffset = 0;
                    }
                    break;
                case BACK:
                    keepRunning = false;
                    break;
                case SELECT:
                    if (filteredNetworkCount > 0) {
                        if (networkDetails) {
                            showNetworkDetails(selectedIndex);
                            // Reset scroll position when returning
                            scrollOffset = 0;
                            lastScrollTime = 0;
                        } else {
                            // Show error if details aren't available
                            display.clearDisplay();
                            display.setCursor(0, 0);
                            display.print(F("Error: Network details not available"));
                            display.display();
                            
                            // Non-blocking delay
                            unsigned long errorStartTime = millis();
                            while (millis() - errorStartTime < 2000) yield();
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        yield(); // Allow background tasks to run
    }
}

// Helper function to draw scrollable text
void WifiMenu::drawScrollableText(const String& text, int x, int y, int width, int& scrollOffset, 
                                unsigned long& lastScrollTime, unsigned long scrollDelay) {
    int textWidth = text.length() * 6; // approx width per char
    
    if (textWidth > width) {
        // Auto-scroll the text if it's too long
        unsigned long currentTime = millis();
        if (currentTime - lastScrollTime > scrollDelay) {
            scrollOffset++;
            if (scrollOffset > textWidth + 16) { // 16px gap before restart
                scrollOffset = 0;
            }
            lastScrollTime = currentTime;
        }
        
        // Set up clipping rectangle
        display.setTextWrap(false);
        
        // Draw scrolling text
        display.setCursor(x - scrollOffset, y);
        display.print(text);
        
        // Draw second copy for continuous scrolling
        if (scrollOffset > 0) {
            display.setCursor(x - scrollOffset + textWidth + 16, y);
            display.print(text);
        }
    } else {
        // For short text, just display centered
        display.setCursor(x, y);
        display.print(text);
        scrollOffset = 0; // Reset scroll if not needed
    }
}

// Optimized function to draw a progress bar
void WifiMenu::drawProgressBar(int x, int y, int width, int height, int percentage) {
    percentage = constrain(percentage, 0, 100);
    
    // Draw border
    display.drawRect(x, y, width, height, SSD1306_WHITE);
    
    // Calculate fill width
    int fillWidth = (percentage * (width - 4)) / 100;
    
    // Fill the progress
    display.fillRect(x + 2, y + 2, fillWidth, height - 4, SSD1306_WHITE);
    
    // Draw percentage text if tall enough
    if (height >= 10) {
        char percentText[5];
        sprintf(percentText, "%d%%", percentage);
        
        int textX = x + (width - strlen(percentText) * 6) / 2;
        int textY = y + (height - 8) / 2;
        
        // If bar is over 50% filled, use inverse colors for better readability
        if (percentage > 50) {
            // For text over the filled part (black on white)
            display.setTextColor(SSD1306_BLACK);
            display.setCursor(textX, textY);
            display.print(percentText);
        } else {
            // For text over the empty part (white on black)
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(textX, textY);
            display.print(percentText);
        }
    }
}

// Sort networks by signal strength - improved algorithm (quick sort partition)
void WifiMenu::sortBySignalStrength() {
    // Make sure we have networkDetails before sorting
    if (!networkDetails || filteredNetworkCount <= 1) {
        return;
    }

    // Implementation of quicksort for better performance than bubble sort
    // This is a simplified version using a recursive lambda
    std::function<void(int, int)> quickSort = [&](int low, int high) {
        if (low < high) {
            // Use the RSSI value directly from NetworkDetail for pivot
            int pivotRssi = networkDetails[high].rssi;
            int i = low - 1;
            
            for (int j = low; j < high; j++) {
                if (networkDetails[j].rssi >= pivotRssi) { // >= for descending order
                    i++;
                    
                    // Swap networks
                    String tempNet = filteredNetworks[i];
                    filteredNetworks[i] = filteredNetworks[j];
                    filteredNetworks[j] = tempNet;
                    
                    // Swap details
                    NetworkDetail tempDetail = networkDetails[i];
                    networkDetails[i] = networkDetails[j];
                    networkDetails[j] = tempDetail;
                }
                
                // Yield every few operations to keep ESP responsive
                if (j % 5 == 0) yield();
            }
            
            // Swap with pivot
            String tempNet = filteredNetworks[i + 1];
            filteredNetworks[i + 1] = filteredNetworks[high];
            filteredNetworks[high] = tempNet;
            
            NetworkDetail tempDetail = networkDetails[i + 1];
            networkDetails[i + 1] = networkDetails[high];
            networkDetails[high] = tempDetail;
            
            int pivot = i + 1;
            
            // Only recurse if there's enough items to sort
            if (pivot - low > 1) quickSort(low, pivot - 1);
            if (high - pivot > 1) quickSort(pivot + 1, high);
            
            yield(); // Keep ESP responsive
        }
    };
    
    // Start quicksort
    quickSort(0, filteredNetworkCount - 1);
}

// Network details screen with smooth scrolling and better memory usage
void WifiMenu::showNetworkDetails(int networkIndex) {
    // Safety check to prevent crashes
    if (networkIndex < 0 || networkIndex >= filteredNetworkCount || !networkDetails) {
        return;
    }
    
    // Parse network information from the stored string
    String ssid = filteredNetworks[networkIndex];
    
    // Fixed the trim() issue
    String ssidOnly = ssid.substring(0, ssid.lastIndexOf('('));
    ssidOnly.trim(); // Call trim() separately as it modifies the string in place
    
    // Get the stored network details
    const NetworkDetail& detail = networkDetails[networkIndex];
    
    bool keepRunning = true;
    int scrollOffset = 0;
    unsigned long lastScrollTime = 0;
    int currentDetailIndex = 0; // Which detail is currently displayed
    const int numItems = 13; // Total number of detail items
    bool inDeauthConfirm = false; // Whether we're in the confirmation screen
    
    // Array of labels and values for easier management
    const char* labels[] = {
        "SSID:", "BSSID:", "Signal:", "Quality:", "Channel:", 
        "Band:", "Encrypt:", "Security:", "Auth:", "Hidden:", 
        "Vendor:", "Distance:", "Scan:"
    };
    
    String values[numItems] = {
        ssidOnly,
        detail.bssid,
        String(detail.rssi) + " dBm",
        detail.quality,
        String(detail.channel),
        detail.band,
        detail.encryption,
        detail.securityProtocol,
        detail.authMode,
        detail.isHidden,
        detail.vendor,
        detail.distance,
        detail.scanTime
    };
    
    while (keepRunning) {
        display.clearDisplay();
        
        if (inDeauthConfirm) {
            // Show deauth confirmation screen
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(4, 10);
            display.print(F("Select for DEAUTH:"));
            
            // Draw box around network name
            display.drawRect(2, 22, SCREEN_WIDTH - 4, 16, SSD1306_WHITE);
            display.setCursor(4, 25);
            
            // Handle long SSIDs in confirmation
            if (ssidOnly.length() > 20) {
                drawScrollableText(ssidOnly, 4, 25, SCREEN_WIDTH - 8, scrollOffset, lastScrollTime);
            } else {
                display.print(ssidOnly);
            }
         
            display.setCursor(4, 42);
            display.print(F("Press SELECT to confirm"));
            display.setCursor(4, 52);
            display.print(F("Press BACK to cancel"));
        }
        else {
            // Title bar
            display.fillRect(0, 0, SCREEN_WIDTH, 12, SSD1306_WHITE);
            display.setTextColor(SSD1306_BLACK);
            display.setCursor(4, 2);
            
            // Check if SSID is too long for title bar
            if (ssidOnly.length() > 18) { 
                display.print(ssidOnly.substring(0, 15));
                display.print(F("..."));
            } else {
                display.print(ssidOnly);
            }
            
            // Border
            display.drawRect(0, 12, SCREEN_WIDTH, SCREEN_HEIGHT - 12, SSD1306_WHITE);
            
            // Display signal strength as a visual indicator
            int signalBars = map(detail.rssi, -100, -40, 1, 5); // Map RSSI to 1-5 bars
            signalBars = constrain(signalBars, 1, 5);
            
            // Draw signal bars in top-right corner
            for (int i = 0; i < 5; i++) {
                if (i < signalBars) {
                    display.fillRect(SCREEN_WIDTH - 10 + i*2, 8 - i, 1, i+1, SSD1306_BLACK);
                } else {
                    display.drawRect(SCREEN_WIDTH - 10 + i*2, 8 - i, 1, i+1, SSD1306_BLACK);
                }
            }
            
            // Show current detail (label and value)
            display.setTextColor(SSD1306_WHITE);
            
            // Display centered parameter name in a highlighted box
            display.fillRect(2, 18, SCREEN_WIDTH - 4, 12, SSD1306_WHITE);
            display.setTextColor(SSD1306_BLACK);
            
            // Center the label text
            int labelX = (SCREEN_WIDTH - strlen(labels[currentDetailIndex]) * 6) / 2;
            display.setCursor(labelX, 20);
            display.print(labels[currentDetailIndex]);
            
            // Value area
            display.setTextColor(SSD1306_WHITE);
            
            String value = values[currentDetailIndex];
            int valueY = 34; // Position for the value
            
            // For long values, implement scrolling
            if (value.length() > 20) { 
                drawScrollableText(value, 4, valueY, SCREEN_WIDTH - 8, scrollOffset, lastScrollTime);
            } else {
                // Center shorter values
                int valueX = (SCREEN_WIDTH - value.length() * 6) / 2;
                display.setCursor(valueX, valueY);
                display.print(value);
            }
            
            // Draw navigation indicators
            display.drawLine(2, 50, SCREEN_WIDTH - 2, 50, SSD1306_WHITE); // Separator line
            
            // Navigation info at bottom
            display.setCursor(4, 53);
            display.print(F("<UP"));
            
            // Page indicator in center
            String pageIndicator = String(currentDetailIndex + 1) + "/" + String(numItems);
            int pageX = (SCREEN_WIDTH - pageIndicator.length() * 6) / 2;
            display.setCursor(pageX, 53);
            display.print(pageIndicator);
            
            // Down navigation
            display.setCursor(SCREEN_WIDTH - 30, 53);
            display.print(F("DOWN>"));
        }
        
        display.display();
        
        // Non-blocking button handling
        unsigned long currentTime = millis();
        unsigned long lastButtonCheckTime = 0;
        const unsigned long BUTTON_CHECK_INTERVAL = 100;
        
        if (currentTime - lastButtonCheckTime >= BUTTON_CHECK_INTERVAL) {
            lastButtonCheckTime = currentTime;
            
            Button btn = buttonManager.readButton();
            
            if (inDeauthConfirm) {
                // In confirmation screen
                switch (btn) {
                    case SELECT: {
                        // Confirm deauth
                        saveNetworkForDeauth(networkIndex);
                        display.clearDisplay();
                        display.setTextColor(SSD1306_WHITE);
                        display.setCursor(10, 24);
                        display.print(F("Network selected"));
                        display.setCursor(10, 34);
                        display.print(F("for deauth attack"));
                        display.display();
                        
                        // Non-blocking delay
                        unsigned long confirmStartTime = millis();
                        while (millis() - confirmStartTime < 1500) yield();
                        
                        inDeauthConfirm = false; // Return to details view
                        break;
                    }
                    case BACK:
                        // Cancel and return to details view
                        inDeauthConfirm = false;
                        break;
                    default:
                        break;
                }
            } else {
                // In details view
                switch (btn) {
                    case UP:
                        // Previous detail (if not at first)
                        if (currentDetailIndex > 0) {
                            currentDetailIndex--;
                            scrollOffset = 0; // Reset scroll when changing detail
                        }
                        break;
                    case DOWN:
                        // Next detail (if not at last)
                        if (currentDetailIndex < numItems - 1) {
                            currentDetailIndex++;
                            scrollOffset = 0; // Reset scroll when changing detail
                        }
                        break;
                    case BACK:
                        keepRunning = false; // Return to network list
                        break;
                    case SELECT:
                        // Show confirmation dialog for deauth
                        inDeauthConfirm = true;
                        scrollOffset = 0; // Reset scroll for confirmation screen
                        break;
                    default:
                        break;
                }
            }
        }
        
        yield(); // Allow background processes to run
    }
}


// Simple validation function
bool WifiMenu::isNetworkValid(const String& network) const {
    // Just check that the network name is non-empty
    return network.length() > 0;
}

// Get count of filtered networks - const qualified for safety
int WifiMenu::getFilteredNetworkCount() const {
    return filteredNetworkCount;
}

 // SSID pattern input with improved memory usage and responsiveness
void WifiMenu::inputSsidPattern() {
    String pattern = filterSettings.ssidPattern;
    bool keepRunning = true;
    unsigned long lastButtonCheckTime = 0;
    
    // Available characters (stored in flash memory)
    const char* charSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.*?";
    int charSetLength = strlen(charSet);
    int selectedCharIndex = 0;
    
    // Simplified fade-in effect
    display.clearDisplay();
    display.fillRect(0, 0, SCREEN_WIDTH, 12, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor((SCREEN_WIDTH - 80) / 2, 2);
    display.print(F("SSID PATTERN"));
    display.display();
    
    // Non-blocking delay
    unsigned long fadeStartTime = millis();
    while (millis() - fadeStartTime < 300) yield();
    
    while (keepRunning) {
        display.clearDisplay();
        
        // Title bar
        display.fillRect(0, 0, SCREEN_WIDTH, 12, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor((SCREEN_WIDTH - 80) / 2, 2);
        display.print(F("SSID PATTERN"));
        
        // Pattern display area with frame
        display.drawRect(0, 14, SCREEN_WIDTH, 14, SSD1306_WHITE);
        display.setTextColor(SSD1306_WHITE);
        
        // Show current pattern
        if (pattern.length() == 0) {
            display.setCursor(4, 17);
            display.print(F("[Empty]"));
        } else {
            // If pattern too long for display, show end with ellipsis
            if (pattern.length() > 20) {
                display.setCursor(4, 17);
                display.print(F("..."));
                display.print(pattern.substring(pattern.length() - 17));
            } else {
                display.setCursor(4, 17);
                display.print(pattern);
            }
        }
        
        // Show cursor position at the end of text
        if (millis() % 1000 < 500) { // Blinking cursor
            int cursorX = 4;
            if (pattern.length() > 0) {
                int patternLen = min(20, (int)pattern.length());
                if (pattern.length() > 20) {
                    cursorX = 4 + 3 + (17 * 6); // After "..." and 17 chars
                } else {
                    cursorX = 4 + (patternLen * 6);
                }
            } else {
                cursorX = 4 + 7*6; // Position after [Empty]
            }
            
            display.drawLine(cursorX, 17, cursorX, 24, SSD1306_WHITE);
        }
        
        // Character selection area with frame
        display.drawRect(0, 32, SCREEN_WIDTH, 16, SSD1306_WHITE);
        
        // Display the characters for selection with current highlighted
        int charsToShow = min(16, charSetLength);
        int startChar = max(0, selectedCharIndex - 7);
        if (startChar > charSetLength - charsToShow) {
            startChar = charSetLength - charsToShow;
        }
        
        for (int i = 0; i < charsToShow; i++) {
            int charIndex = startChar + i;
            char c = charSet[charIndex];
            int x = 4 + (i * 7);
            
            if (charIndex == selectedCharIndex) {
                display.fillRect(x - 1, 33, 9, 14, SSD1306_WHITE);
                display.setTextColor(SSD1306_BLACK);
            } else {
                display.setTextColor(SSD1306_WHITE);
            }
            
            display.setCursor(x, 36);
            display.print(c);
        }
        
        // Scroll indicators for character selection
        if (startChar > 0) {
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(1, 36);
            display.print(F("<"));
        }
        if (startChar + charsToShow < charSetLength) {
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(SCREEN_WIDTH - 6, 36);
            display.print(F(">"));
        }
        
        // Instructions
        display.drawLine(0, SCREEN_HEIGHT - 10, SCREEN_WIDTH, SCREEN_HEIGHT - 10, SSD1306_WHITE);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(2, SCREEN_HEIGHT - 8);
        display.print(F("UP/DN:Char SEL:Add B:Done"));
        
        display.display();
        
        // Non-blocking button handling
        unsigned long currentTime = millis();
        if (currentTime - lastButtonCheckTime >= BUTTON_CHECK_INTERVAL) {
            lastButtonCheckTime = currentTime;
            
            Button btn = buttonManager.readButton();
            switch (btn) {
                case UP:
                    selectedCharIndex = (selectedCharIndex + 1) % charSetLength;
                    break;
                    
                case DOWN:
                    selectedCharIndex = (selectedCharIndex - 1 + charSetLength) % charSetLength;
                    break;
                    
                case SELECT:
                    // Add selected character to pattern
                    if (pattern.length() < 20) {  // Limit pattern length
                        pattern += charSet[selectedCharIndex];
                    } else {
                        // Flash the display to indicate max length reached
                        display.invertDisplay(true);
                        delay(100); // Short delay is acceptable for visual feedback
                        display.invertDisplay(false);
                    }
                    break;
                    
                case BACK:
                    if (pattern.length() > 0) {
                        // Remove last character
                        pattern = pattern.substring(0, pattern.length() - 1);
                    } else {
                        // Exit if pattern is empty and BACK is pressed again
                        // Show "Done" message
                        display.clearDisplay();
                        display.setCursor(0, 0);
                        display.print(F("Pattern updated"));
                        display.display();
                        
                        // Non-blocking delay
                        unsigned long doneStartTime = millis();
                        while (millis() - doneStartTime < 500) yield();
                        
                        keepRunning = false;
                    }
                    break;
                    
                default:
                    break;
            }
        }
        
        yield(); // Allow background processes to run
    }
    
    // Store the pattern
    filterSettings.ssidPattern = pattern;
}

// EEPROM functions with error checking and improved write performance
void WifiMenu::writeStringToEEPROM(int addr, const String& data) {
    if (addr < 0 || addr + data.length() + 1 >= EEPROM.length()) {
        Serial.println(F("EEPROM write error: Address out of bounds"));
        return;
    }
    
    for (unsigned int i = 0; i < data.length(); i++) {
        EEPROM.write(addr + i, data[i]);
    }
    EEPROM.write(addr + data.length(), 0); // Null terminator
    
    if (!EEPROM.commit()) {
        Serial.println(F("EEPROM commit failed"));
    }
}

String WifiMenu::readStringFromEEPROM(int addr) {
    if (addr < 0 || addr >= EEPROM.length()) {
        Serial.println(F("EEPROM read error: Address out of bounds"));
        return "";
    }
    
    String result = "";
    char c;
    unsigned int maxRead = 200; // Safety limit
    unsigned int count = 0;
    
    while (count < maxRead && addr < EEPROM.length()) {
        c = EEPROM.read(addr);
        if (c == 0) break;
        result += c;
        addr++;
        count++;
    }
    
    return result;
}

// Saved network functions - improved safety checks
int WifiMenu::getSavedNetworkCount() const {
    uint8_t count = EEPROM.read(EEPROM_START_ADDR);
    // Sanity check
    if (count > MAX_NETWORKS) {
        return 0;
    }
    return count;
}

bool WifiMenu::deleteSavedNetwork(int index) {
    // Check if index is valid
    uint8_t count = getSavedNetworkCount();
    if (index < 0 || index >= count) {
        return false;
    }
    
    // Shift all networks after this one up by one slot
    for (int i = index; i < count - 1; i++) {
        // Calculate source and destination addresses
        int destAddr = EEPROM_START_ADDR + 1 + (i * NETWORK_DATA_SIZE);
        int srcAddr = EEPROM_START_ADDR + 1 + ((i + 1) * NETWORK_DATA_SIZE);
        
        // Copy data from next slot to current slot
        for (int j = 0; j < NETWORK_DATA_SIZE; j++) {
            EEPROM.write(destAddr + j, EEPROM.read(srcAddr + j));
        }
    }
    
    // Update network count
    EEPROM.write(EEPROM_START_ADDR, count - 1);
    EEPROM.commit();
    
    return true;
}

// Implementation for saveNetworkForDeauth - this is called from showNetworkDetails
void WifiMenu::saveNetworkForDeauth(int index) {
  // Safety check
  if (index < 0 || index >= filteredNetworkCount || !networkDetails) {
    Serial.println(F("Invalid network index for deauth"));
    return;
  }
  
  // Get the network info from scan results
  String selectedNetwork = filteredNetworks[index];
  // Extract SSID
  int bracketPos = selectedNetwork.lastIndexOf('(');
  String ssid = selectedNetwork;
  if (bracketPos > 0) {
    ssid = selectedNetwork.substring(0, bracketPos);
    ssid.trim();
  }
  
  // Get the BSSID
  String bssid = networkDetails[index].bssid;
  
  Serial.println(F("Saving network for deauth:"));
  Serial.print(F("SSID: ")); Serial.println(ssid);
  Serial.print(F("BSSID: ")); Serial.println(bssid);
  
  // First, read how many networks we already have stored
  uint8_t networkCount = EEPROM.read(EEPROM_START_ADDR);
  
  // Sanity check - if count is invalid, reset it
  if (networkCount > MAX_NETWORKS) {
    networkCount = 0;
    Serial.println(F("Invalid network count, resetting to 0"));
  }
  
  Serial.print(F("Current network count: ")); Serial.println(networkCount);
  
  // Check if we're already at max capacity
  if (networkCount >= MAX_NETWORKS) {
    Serial.println(F("Maximum networks reached, replacing oldest entry"));
    // We need to shift all networks down by one, removing the oldest
    for (int i = 0; i < MAX_NETWORKS - 1; i++) {
      // Calculate source and destination addresses
      int destAddr = EEPROM_START_ADDR + 1 + (i * NETWORK_DATA_SIZE);
      int srcAddr = EEPROM_START_ADDR + 1 + ((i + 1) * NETWORK_DATA_SIZE);
      
      // Copy network data from next slot to current slot
      for (int j = 0; j < NETWORK_DATA_SIZE; j++) {
        EEPROM.write(destAddr + j, EEPROM.read(srcAddr + j));
      }
    }
    
    // Keep network count at maximum (we're replacing the oldest)
    networkCount = MAX_NETWORKS;
  } else {
    // We have room for a new network
    networkCount++;
    Serial.print(F("Increasing network count to: ")); Serial.println(networkCount);
  }
  
  // Update the network count
  EEPROM.write(EEPROM_START_ADDR, networkCount);
  
  // Calculate the address for the new network
  // Always store at the newest slot (count-1)
  int networkAddr = EEPROM_START_ADDR + 1 + ((networkCount - 1) * NETWORK_DATA_SIZE);
  
  // Write the SSID length
  int ssidLen = ssid.length();
  EEPROM.write(networkAddr, ssidLen);
  networkAddr++;
  
  Serial.print(F("SSID length: ")); Serial.println(ssidLen);
  
  // Write the SSID
  for (int i = 0; i < ssidLen; i++) {
    EEPROM.write(networkAddr + i, ssid[i]);
  }
  networkAddr += ssidLen;
  
  // Write separator
  EEPROM.write(networkAddr, ';');
  networkAddr++;
  
  // Write BSSID length
  int bssidLen = bssid.length();
  EEPROM.write(networkAddr, bssidLen);
  networkAddr++;
  
  Serial.print(F("BSSID length: ")); Serial.println(bssidLen);
  
  // Write BSSID
  for (int i = 0; i < bssidLen; i++) {
    EEPROM.write(networkAddr + i, bssid[i]);
  }
  
  // Commit changes to EEPROM
  if (EEPROM.commit()) {
    Serial.println(F("Network saved successfully!"));
  } else {
    Serial.println(F("ERROR: Failed to commit EEPROM changes!"));
  }
}

bool WifiMenu::getSavedNetwork(int index, String &ssid, String &bssid) {
    // Check if index is valid
    uint8_t count = getSavedNetworkCount();
    if (index < 0 || index >= count) {
        Serial.print(F("getSavedNetwork: Invalid index "));
        Serial.println(index);
        return false;
    }
    
    // Calculate address for this network
    int networkAddr = EEPROM_START_ADDR + 1 + (index * NETWORK_DATA_SIZE);
    
    // Check if the address is valid
    if (networkAddr < 0 || networkAddr >= EEPROM.length()) {
        Serial.print(F("getSavedNetwork: Invalid EEPROM address "));
        Serial.println(networkAddr);
        return false;
    }
    
    // Read SSID length
    int ssidLen = EEPROM.read(networkAddr);
    if (ssidLen <= 0 || ssidLen > 50) { // Sanity check
        Serial.print(F("getSavedNetwork: Invalid SSID length "));
        Serial.println(ssidLen);
        return false;
    }
    networkAddr++;
    
    // Read SSID
    ssid = "";
    for (int i = 0; i < ssidLen && networkAddr + i < EEPROM.length(); i++) {
        ssid += (char)EEPROM.read(networkAddr + i);
    }
    networkAddr += ssidLen;
    
    // Skip separator
    if (networkAddr >= EEPROM.length() || EEPROM.read(networkAddr) != ';') {
        Serial.println(F("getSavedNetwork: Invalid separator"));
        return false;  // Invalid format
    }
    networkAddr++;
    
    // Read BSSID length
    int bssidLen = EEPROM.read(networkAddr);
    if (bssidLen <= 0 || bssidLen > 20) { // Sanity check
        Serial.print(F("getSavedNetwork: Invalid BSSID length "));
        Serial.println(bssidLen);
        return false;
    }
    networkAddr++;
    
    // Read BSSID
    bssid = "";
    for (int i = 0; i < bssidLen && networkAddr + i < EEPROM.length(); i++) {
        bssid += (char)EEPROM.read(networkAddr + i);
    }
    
    Serial.print(F("getSavedNetwork: Successfully read network #"));
    Serial.println(index);
    return true;
}



