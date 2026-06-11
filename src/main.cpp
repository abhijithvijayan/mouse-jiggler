#include <Arduino.h>
#include <HijelHID_BLEMouse.h>

HijelBLEMouse bleMouse("ESP32 Mouse", "ESP32", 100);

// The ESP32-C3 SuperMini has a single blue LED on GPIO8, wired ACTIVE-LOW
// (driving the pin LOW turns the LED on). There is no addressable RGB LED on
// this board, so status is shown via blink patterns rather than colors:
//   solid on          -> booting / BLE stack starting up
//   slow blink        -> advertising, waiting for a host to pair
//   solid on          -> paired, idle between jiggles
//   quick double wink  -> a jiggle just happened
#define LED_PIN 8

static void ledOn()  { digitalWrite(LED_PIN, LOW); }   // active-low
static void ledOff() { digitalWrite(LED_PIN, HIGH); }

// Human-like wander parameters. We keep the cursor roaming inside a small box
// around wherever it started, so over hours it never drifts into a corner.
static constexpr int BOX_X = 100;         // max horizontal offset from origin (px)
static constexpr int BOX_Y = 70;          // max vertical offset from origin (px)
static constexpr int MIN_TRAVEL = 90;     // each glide must cover at least this (px, manhattan) so it's clearly visible
static constexpr int MIN_REST_MS = 25000; // shortest gap between jiggles (~30s avg)
static constexpr int MAX_REST_MS = 35000; // longest gap; keep < your app's idle timeout (here: 2 min)

// Net offset from the start point, so movement stays bounded inside the box.
static int netX = 0;
static int netY = 0;

// Glide smoothly to a fresh random spot inside the box, at a human-ish speed.
static void glideToRandomSpot() {
    // Pick a fresh spot in the box, but keep trying until it's far enough from
    // where we are now -- otherwise consecutive glides can be tiny and barely visible.
    int targetX, targetY, dx, dy, tries = 0;
    do {
        targetX = random(-BOX_X, BOX_X + 1);
        targetY = random(-BOX_Y, BOX_Y + 1);
        dx = targetX - netX;
        dy = targetY - netY;
    } while (abs(dx) + abs(dy) < MIN_TRAVEL && ++tries < 10);

    const uint32_t durationMs = random(180, 650); // a natural flick takes a fraction of a second
    Serial.printf("[jiggle] glide d=(%d,%d) over %lums -> box pos (%d,%d)\n",
                  dx, dy, (unsigned long)durationMs, targetX, targetY);
    bleMouse.moveTo(dx, dy, durationMs);
    netX = targetX;
    netY = targetY;
}

static void jiggleLikeAHuman() {
    glideToRandomSpot();
    // Sometimes follow up with a second small adjustment, the way a hand
    // rarely lands a cursor in exactly one motion.
    if (random(3) == 0) {
        delay(random(120, 400));
        glideToRandomSpot();
    }
}

void setup() {
    Serial.begin(115200);
    // Give the USB CDC port a moment to enumerate so we don't miss boot logs.
    const uint32_t start = millis();
    while (!Serial && millis() - start < 3000) {
        delay(10);
    }
    Serial.println("\n=== Bluetooth Jiggler booting ===");
    pinMode(LED_PIN, OUTPUT);
    ledOn(); // solid on while the BLE stack starts up
    randomSeed(esp_random()); // hardware RNG so the wander differs every boot
    Serial.println("[ble] starting advertising as \"ESP32 Mouse\" ...");
    bleMouse.begin();
    Serial.println("[ble] begin() returned; waiting for a host to pair");
}

void loop() {
    // Log connection/pairing transitions. isConnected() = a host linked up;
    // isPaired() = the link is also authenticated/bonded (required before any
    // movement is accepted). On macOS/Windows you can see "connected but not
    // paired" if bonding fails -- that distinction is the key diagnostic.
    static bool wasConnected = false;
    static bool wasPaired = false;
    const bool connected = bleMouse.isConnected();
    const bool paired = bleMouse.isPaired();
    if (connected != wasConnected) {
        Serial.printf("[ble] connected = %s\n", connected ? "true" : "false");
        wasConnected = connected;
    }
    if (paired != wasPaired) {
        Serial.printf("[ble] paired = %s\n", paired ? "true" : "false");
        wasPaired = paired;
    }

    // isPaired() is true only once the host has fully authenticated the link;
    // sending movement before then has no effect.
    if (paired) {
        ledOn(); // solid on while idle between jiggles

        Serial.println("[jiggle] start");
        jiggleLikeAHuman();
        Serial.println("[jiggle] done");

        // Double wink to signal "I just jiggled".
        for (int i = 0; i < 2; i++) {
            ledOff();
            delay(80);
            ledOn();
            delay(80);
        }

        // Randomized rest so it doesn't tick like a metronome, but always short
        // enough to keep presence-aware apps (Slack, Teams, ...) marking you active.
        const int restMs = random(MIN_REST_MS, MAX_REST_MS + 1);
        Serial.printf("[jiggle] resting %dms until next move\n", restMs);
        delay(restMs);
    } else {
        // Not paired yet: slow blink to show we're advertising. Emit a
        // heartbeat every ~5s so the serial log proves the loop is alive.
        static uint8_t ticks = 0;
        if (++ticks >= 10) {
            ticks = 0;
            Serial.printf("[ble] still advertising, waiting to pair (connected=%s)\n",
                          connected ? "true" : "false");
        }
        ledOn();
        delay(250);
        ledOff();
        delay(250);
    }
}
