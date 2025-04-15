#ifndef DEAUTH_H
#define DEAUTH_H
#include "config.h"
#include "main_menu.h"
#include "ButtonManager.h"
#include "wifi.h"


class DeauthAttack {
public:
    // Main functions
    void scanForAPs();
    void scanForClients(String targetBSSID);
    void selectAP(int index);
    void selectClient(int index);
    void setAttackType(int type);
    void setPacketRate(int packetsPerSec);
    void setDuration(int seconds);
    void startAttack();
    void stopAttack();
    bool isRunning();
    
    // UI functions
    void showAPSelectionMenu();
    void showClientSelectionMenu();
    void showAttackTypeMenu();
    void showAttackConfigMenu();
    void showAttackStatus();
    
private:
    // Attack parameters
    int attackType;      // 0=Deauth, 1=Beacon, 2=Probe
    int packetRate;      // Packets per second
    int duration;        // Seconds (0=continuous)
    bool isAttackRunning;
    
    // Target information
    struct Target {
        String bssid;
        String ssid;
        int channel;
        bool selected;
    };
    Target targets[10];  // Store up to 10 targets
    int targetCount;
    
    // Client information
    struct Client {
        String mac;
        bool selected;
    };
    Client clients[10];  // Store up to 10 clients
    int clientCount;
    
    // Packet crafting functions
    void sendDeauthPacket(uint8_t* ap, uint8_t* client);
    void sendBeaconPacket(String ssid, uint8_t* bssid);
    void sendProbePacket(uint8_t* client);
};

#endif