#include <BleMouse.h>

BleMouse bleMouse("ESP32 Mouse", "ESP32", 100);

// The ESP32-C3 SuperMini has a single blue LED on GPIO8, wired ACTIVE-LOW
// (driving the pin LOW turns the LED on). There is no addressable RGB LED on
// this board, so status is shown via blink patterns rather than colors:
//   solid on        -> booting / BLE stack starting up
//   slow blink      -> advertising, waiting for a host to connect
//   solid on        -> connected, idle
//   quick double wink -> a jiggle just happened
#define LED_PIN 8

static void ledOn()  { digitalWrite(LED_PIN, LOW); }   // active-low
static void ledOff() { digitalWrite(LED_PIN, HIGH); }

void setup() {
    Serial.begin(115200);
    Serial.printf("Bluetooth Jiggler Initializing");
    pinMode(LED_PIN, OUTPUT);
    ledOn(); // solid on while the BLE stack starts up
    bleMouse.begin();
}

void loop() {
    if (bleMouse.isConnected()) {
        // Connected: hold the LED on, then a quick double-wink on each jiggle.
        ledOn();

        bleMouse.move(1, 0);  // move 1px right
        delay(50);
        bleMouse.move(-1, 0); // move 1px back to original position

        // Double wink to signal "I just jiggled".
        for (int i = 0; i < 2; i++) {
            ledOff();
            delay(80);
            ledOn();
            delay(80);
        }

        delay(10000); // repeat every 10 seconds
    } else {
        // Not connected yet: slow blink to show we're advertising.
        ledOn();
        delay(250);
        ledOff();
        delay(250);
    }
}
