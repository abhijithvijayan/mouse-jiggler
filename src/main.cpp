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
static constexpr int BOX_X = 60;          // max horizontal offset from origin (px)
static constexpr int BOX_Y = 40;          // max vertical offset from origin (px)
static constexpr int MIN_REST_MS = 25000; // shortest gap between jiggles
static constexpr int MAX_REST_MS = 75000; // longest gap (well under app idle timeouts)

// Net offset from the start point, so movement stays bounded inside the box.
static int netX = 0;
static int netY = 0;

// Glide smoothly to a fresh random spot inside the box, at a human-ish speed.
static void glideToRandomSpot() {
    const int targetX = random(-BOX_X, BOX_X + 1);
    const int targetY = random(-BOX_Y, BOX_Y + 1);
    const uint32_t durationMs = random(180, 650); // a natural flick takes a fraction of a second
    bleMouse.moveTo(targetX - netX, targetY - netY, durationMs);
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
    Serial.printf("Bluetooth Jiggler Initializing");
    pinMode(LED_PIN, OUTPUT);
    ledOn(); // solid on while the BLE stack starts up
    randomSeed(esp_random()); // hardware RNG so the wander differs every boot
    bleMouse.begin();
}

void loop() {
    // isPaired() is true only once the host has fully authenticated the link;
    // sending movement before then has no effect.
    if (bleMouse.isPaired()) {
        ledOn(); // solid on while idle between jiggles

        jiggleLikeAHuman();

        // Double wink to signal "I just jiggled".
        for (int i = 0; i < 2; i++) {
            ledOff();
            delay(80);
            ledOn();
            delay(80);
        }

        // Randomized rest so it doesn't tick like a metronome, but always short
        // enough to keep presence-aware apps (Slack, Teams, ...) marking you active.
        delay(random(MIN_REST_MS, MAX_REST_MS + 1));
    } else {
        // Not paired yet: slow blink to show we're advertising.
        ledOn();
        delay(250);
        ledOff();
        delay(250);
    }
}
