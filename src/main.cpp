#include <BleMouse.h>

BleMouse bleMouse("ESP32 Mouse", "ESP32", 100);

// Onboard addressable RGB LED (WS2812 on GPIO8 of the ESP32-C3-DevKitM-1).
// Driven via the Arduino core helper rgbLedWrite(RGB_BUILTIN, r, g, b).
// Status colors (kept dim so the LED isn't blinding):
//   blue           -> booting / BLE stack starting up
//   blinking yellow -> advertising, waiting for a host to connect
//   dim green      -> connected, idle
//   cyan flash     -> actively jiggling the cursor
static void setLed(uint8_t r, uint8_t g, uint8_t b) {
    rgbLedWrite(RGB_BUILTIN, r, g, b);
}

#define LED_OFF()       setLed(0, 0, 0)
#define LED_BOOT()      setLed(0, 0, 40)    // blue
#define LED_WAITING()   setLed(40, 30, 0)   // yellow
#define LED_CONNECTED() setLed(0, 30, 0)    // dim green
#define LED_JIGGLE()    setLed(0, 40, 40)   // cyan

void setup() {
    Serial.begin(115200);
    Serial.printf("Bluetooth Jiggler Initializing");
    LED_BOOT();
    bleMouse.begin();
}

void loop() {
    if (bleMouse.isConnected()) {
        // Connected: hold dim green, then briefly flash cyan while moving.
        LED_CONNECTED();

        LED_JIGGLE();
        bleMouse.move(1, 0);  // move 1px right
        delay(50);
        bleMouse.move(-1, 0); // move 1px back to original position
        LED_CONNECTED();

        delay(10000); // repeat every 10 seconds
    } else {
        // Not connected yet: blink yellow to show we're advertising.
        LED_WAITING();
        delay(250);
        LED_OFF();
        delay(250);
    }
}
