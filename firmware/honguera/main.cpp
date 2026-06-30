#include <Arduino.h>
#include "HongueraNode.h"

HongueraNode* node = nullptr;

void setup() {
    node = new HongueraNode();
    if (!node->initialize()) {
        Serial.println("FATAL: HongueraNode init failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }
}

void loop() {
    if (node) node->update();
    delay(1);
}
