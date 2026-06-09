#include <BleMouse.h>

BleMouse bleMouse("ESP32 Mouse", "ESP32", 100);

void setup() {
    Serial.begin(115200);
    Serial.printf("Bluetooth Jiggler Initializing");
    bleMouse.begin();
}

void loop() {
    if (bleMouse.isConnected()) {
        bleMouse.move(1, 0); // move 1px right
        delay(50);
        bleMouse.move(-1, 0); // move 1px back to original position
    }

    delay(10000); // repeat every 10 seconds
}