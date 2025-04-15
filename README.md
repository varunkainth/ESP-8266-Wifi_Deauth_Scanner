# ESP8266 WiFi Scanner and Deauthenticator with OLED Display

A portable WiFi scanner and deauthenticator tool built with ESP8266 and an OLED display. This project allows you to scan for nearby networks, filter results, save networks for later use, and perform deauthentication attacks for educational and network testing purposes.

## ⚠️ Legal Disclaimer

This device is intended for **EDUCATIONAL PURPOSES ONLY**. Unauthorized deauthentication attacks against networks you don't own or have permission to test is illegal in most jurisdictions and unethical. Always:

- Only use on your own networks or with explicit permission
- Follow local laws regarding wireless communications
- Use responsibly for security testing and learning

## Hardware Requirements

- ESP8266 board (NodeMCU, Wemos D1 Mini, etc.)
- 0.96" or 1.3" I2C OLED Display (SSD1306 or SH1106)
- 4 push buttons for navigation (UP, DOWN, SELECT, BACK)
- Buzzer (for audio feedback)
- Battery (optional, for portable use)
- Project enclosure (optional)

## Pin Connections

| Component | ESP8266 Pin | Notes |
|-----------|-------------|-------|
| OLED Display SDA | D2 (GPIO4) | I2C Data |
| OLED Display SCL | D1 (GPIO5) | I2C Clock |
| UP Button | D5 (GPIO14) | Connect to GND with 10K pull-up resistor |
| DOWN Button | D6 (GPIO12) | Connect to GND with 10K pull-up resistor |
| SELECT Button | D7 (GPIO13) | Connect to GND with 10K pull-up resistor |
| BACK Button | D8 (GPIO15) | Connect to GND with 10K pull-up resistor |
| Buzzer | D3 (GPIO0) | Connect positive to pin, negative to GND |
| 3.3V | 3.3V | Power for OLED |
| GND | GND | Common ground |

*Note: You may need to adjust pin assignments in the code if using different connections.*

## Features

- **WiFi Scanning**: Discover all nearby WiFi networks
- **Network Filtering**: Filter networks by various criteria
- **Network Management**: Save networks for later deauthentication
- **EEPROM Storage**: Save selected networks (clears on reset)
- **User Interface**: Easy navigation with 4-button control
- **OLED Display**: Clear visual feedback and menu system
- **Audio Feedback**: Buzzer provides sound notifications for actions

## Installation

### Prerequisites

1. Arduino IDE (1.8.x or newer)
2. ESP8266 board support: Add `http://arduino.esp8266.com/stable/package_esp8266com_index.json` to Board Manager URLs
3. Required libraries:
   - Wire (built-in)
   - SPI (built-in)
   - Adafruit GFX
   - Adafruit SSD1306
   - ESP8266WiFi

### Setup

1. Clone this repository:
   ```
   git clone https://github.com/varunkainth/ESP-8266-Wifi_Deauth_Scanner.git
   ```

2. Open `DEAUTH_WIFI_SCAN_WITH_OLED.ino` in Arduino IDE

3. Install required libraries via Library Manager

4. Select your ESP8266 board from Tools > Board menu

5. Compile and upload to your ESP8266

## Usage

1. Power on the device
2. Navigate using the four buttons:
   - **UP**: Move selection up
   - **DOWN**: Move selection down
   - **SELECT**: Confirm selection
   - **BACK**: Return to previous menu

3. Main Menu Options:
   - **WiFi Scan**: Scan and interact with networks
   - **Deauth**: Deauthentication tools
   - **Settings**: Configure device settings
   - **Show Saved Networks**: View and manage saved networks

### WiFi Scanning

1. Select "WiFi Scan" from the main menu
2. Choose "WiFi Scan" to perform a network scan
3. View scan results with "Show Networks"
4. Filter results with "Filter Networks"

### Saving Networks for Deauth

1. From the network list, navigate to a network
2. Press SELECT to open the options menu
3. Choose "Save for Deauth"
4. Networks will be saved until device reset (EEPROM clears on boot)

### Viewing Saved Networks

1. Select "Show Saved Networks" from the main menu
2. Browse saved networks using UP/DOWN
3. Press SELECT for network options:
   - View Details: Shows SSID and BSSID
   - Delete Network: Removes from saved list
   - Use for Deauth: Selects for deauth attack

### Audio Feedback

The buzzer provides audio cues for various actions:
- Button presses
- Menu navigation
- Network found during scanning
- Operation completed
- Error notifications

## Project Structure

- **main_menu.h/cpp**: OLED display handling and menu system
- **ButtonManager.h/cpp**: Button input detection
- **wifi.h/cpp**: WiFi scanning and deauthentication functionality
- **config.h**: Constants and configuration

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Author

- **Varun Kainth** ([@varunkainth](https://github.com/varunkainth))

## Last Updated

2025-04-15

## License
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

This project is licensed under the MIT License - see the LICENSE file for details.This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- ESP8266 community
- Adafruit for the excellent display libraries
- All contributors to this project
