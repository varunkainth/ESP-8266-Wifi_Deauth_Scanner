#ifndef WIFI_H
#define WIFI_H

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <Adafruit_SSD1306.h>

// Memory management optimizations
#define MAX_NETWORKS 5
#define NETWORK_DATA_SIZE 150  // Max size for each network entry
#define EEPROM_START_ADDR 0
#define MAX_SCAN_RESULTS 20  // Maximum networks to store in memory

class WifiMenu {
public:
    WifiMenu();  // Constructor
    ~WifiMenu(); // Destructor to free memory

    // Core WiFi functions
    void scanNetworks();
    void showScannedNetworks();
    void filterNetworks();
    void sortBySignalStrength();
    void showFilteredNetworks();
    void saveNetworkForDeauth(int index);
    int getFilteredNetworkCount() const;
    void initializeEEPROM();
    bool clearAllNetworks();

    // Network management
    bool isNetworkValid(const String& network) const;
    void showNetworkDetails(int networkIndex);
    
    // Deauth functions
    void startDeauth(const String& targetBSSID, const String& targetSSID);
    void stopDeauth();
    void showDeauthScreen();
    bool isDeauthRunning() const;
    
    // EEPROM helpers
    String readStringFromEEPROM(int addr);
    void writeStringToEEPROM(int addr, const String& data);
    
    // Saved network functions
    int getSavedNetworkCount() const;
    bool getSavedNetwork(int index, String &ssid, String &bssid);
    bool deleteSavedNetwork(int index);

private:
    // Utility functions
    void splitString(const String& input, char delimiter, String output[]);
    String getMacVendor(const String& mac) const;
    
    // Network information structure - consolidated for better memory management
    struct NetworkDetail {
        String bssid;
        int rssi;
        int channel;
        String encryption;
        String authMode;
        String isHidden;
        String band;
        String securityProtocol;
        String quality;
        String vendor;
        String scanTime;
        String distance;
    };
    
    // Filtering system
    struct FilterSettings {
        bool enabled;
        int minSignal;
        bool openOnly;
        bool hiddenOnly;
        bool channel24GHz;
        bool channel5GHz;
        String ssidPattern;
        int channelFilter;
    };
    
    // Filter-related functions
    void showFilterMenu();
    void applyFilters();
    void resetFilters();
    bool matchesFilters(int networkIndex) const;
    void inputSsidPattern();
    
    // Deauth tracking variables
    volatile bool deauthRunning;
    String deauthBSSID;
    String deauthSSID;
    unsigned long deauthStartTime;
    unsigned long deauthPacketsSent;
    unsigned long lastStatusUpdate;
    int deauthDuration;
    bool targetAllClients;
    
    // Send deauth packet implementation
    void sendDeauthPacket(uint8_t *bssid, uint8_t *station, uint8_t reason);
    
    // Network storage
    int filteredNetworkCount;
    String* filteredNetworks;  // Dynamic array
    NetworkDetail* networkDetails;  // Dynamic array
    FilterSettings filterSettings;
    
    // UI helpers
    void drawScrollableText(const String& text, int x, int y, int width, int& scrollOffset, 
                           unsigned long& lastScrollTime, unsigned long scrollDelay = 200);
    void drawProgressBar(int x, int y, int width, int height, int percentage);
};

#endif // WIFI_H