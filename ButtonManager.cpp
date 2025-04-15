#include "ButtonManager.h"
#include "config.h"

void ButtonManager::begin() {
    pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
    pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
    pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP);
    pinMode(BUTTON_BACK_PIN, INPUT_PULLUP);

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

Button ButtonManager::readButton() {
    unsigned long currentTime = millis();

    if (currentTime - lastDebounceTime < debounceDelay) {
        return NONE;
    }

    if (digitalRead(BUTTON_UP_PIN) == LOW) {
        lastDebounceTime = currentTime;
        handleBuzzer();
        return UP;
    }

    if (digitalRead(BUTTON_DOWN_PIN) == LOW) {
        lastDebounceTime = currentTime;
        handleBuzzer();
        return DOWN;
    }

    if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
        lastDebounceTime = currentTime;
        handleBuzzer();
        return SELECT;
    }

    if (digitalRead(BUTTON_BACK_PIN) == LOW) {
        lastDebounceTime = currentTime;
        handleBuzzer();
        return BACK;
    }

    return NONE;
}

void ButtonManager::handleBuzzer() {
    if (!isBuzzerEnabled) return;
    digitalWrite(BUZZER_PIN, HIGH);
    delay(20);
    digitalWrite(BUZZER_PIN, LOW);
}