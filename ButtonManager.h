#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>

enum Button {
    NONE,
    UP,
    DOWN,
    SELECT,
    BACK
};

class ButtonManager {
public:
    void begin();
    Button readButton();
    void handleBuzzer();

private:
    unsigned long lastDebounceTime = 0;
    const unsigned long debounceDelay = 200; // milliseconds
};

#endif