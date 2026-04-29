/*
 * PaveSafe — Arduino MKR WiFi 1010 + NeoPixel 8-stick
 * Auto-reconnects to WiFi hotspot if connection drops
 */

#include <WiFiNINA.h>
#include <Adafruit_NeoPixel.h>

// ── Config ────────────────────────────────────────────────────────────────────
const char* SSID     = "Gilang";
const char* PASSWORD = "internetmahal";
const int   NEO_PIN  = 6;
const int   NEO_NUM  = 8;

// ── NeoPixel ──────────────────────────────────────────────────────────────────
Adafruit_NeoPixel strip(NEO_NUM, NEO_PIN, NEO_GRB + NEO_KHZ800);

// ── HTTP Server ───────────────────────────────────────────────────────────────
WiFiServer server(80);

// ── State ─────────────────────────────────────────────────────────────────────
char currentStatus = 'S';
unsigned long lastBlink  = 0;
unsigned long lastBreath = 0;
bool blinkState = false;
float breathVal = 10;
float breathDir = 1;

uint32_t COL_CYAN;
uint32_t COL_GREEN;
uint32_t COL_AMBER;
uint32_t COL_RED;
uint32_t COL_OFF;

// ── Connect WiFi — keeps retrying until connected ─────────────────────────────
void connectWiFi() {
  Serial.print("Connecting to ");
  Serial.println(SSID);

  // Amber blink while attempting
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(SSID, PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");

      // Alternate amber/off while waiting
      if (attempts % 2 == 0) showAll(COL_AMBER);
      else                    showAll(COL_OFF);

      attempts++;
    }

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("\nFailed — retrying in 3s...");
      // 3 red flashes then retry
      for (int i = 0; i < 3; i++) {
        showAll(COL_RED); delay(300);
        showAll(COL_OFF); delay(300);
      }
      delay(1000);
      WiFi.disconnect();
      delay(500);
    }
  }

  // Connected!
  Serial.println("\nConnected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // 3 green flashes = connected
  for (int i = 0; i < 3; i++) {
    showAll(COL_GREEN); delay(200);
    showAll(COL_OFF);   delay(200);
  }

  server.begin();
  Serial.println("HTTP server started");
  Serial.print("Enter this IP in PaveSafe app: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);

  strip.begin();
  strip.setBrightness(40);
  strip.show();

  // Define colours after strip.begin()
  COL_CYAN  = strip.Color(0,   200, 255);
  COL_GREEN = strip.Color(0,   230, 100);
  COL_AMBER = strip.Color(255, 160, 0  );
  COL_RED   = strip.Color(255, 20,  50 );
  COL_OFF   = strip.Color(0,   0,   0  );

  // Boot sweep
  for (int i = 0; i < NEO_NUM; i++) {
    strip.setPixelColor(i, COL_CYAN);
    strip.show();
    delay(60);
  }

  // Connect — will keep trying until successful
  connectWiFi();
}

void loop() {
  // ── Auto-reconnect if WiFi drops ──────────────────────────────────────────
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost — reconnecting...");
    currentStatus = 'S'; // back to scanning while reconnecting
    connectWiFi();
  }

  // ── Handle HTTP requests ──────────────────────────────────────────────────
  WiFiClient client = server.available();
  if (client) {
    String request = "";
    unsigned long timeout = millis() + 500;

    while (client.connected() && millis() < timeout) {
      if (client.available()) {
        char c = client.read();
        request += c;
        if (request.endsWith("\r\n\r\n")) break;
      }
    }

    // Parse command from URL: GET /C, /P, /B, /S
    if      (request.indexOf("GET /C") >= 0) currentStatus = 'C';
    else if (request.indexOf("GET /P") >= 0) currentStatus = 'P';
    else if (request.indexOf("GET /B") >= 0) currentStatus = 'B';
    else if (request.indexOf("GET /S") >= 0) currentStatus = 'S';

    // CORS response
    client.println("HTTP/1.1 200 OK");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("OK");
    client.stop();

    Serial.print("Status: ");
    Serial.println(currentStatus);
  }

  // ── Animate LEDs ──────────────────────────────────────────────────────────
  animateLEDs();
}

void animateLEDs() {
  unsigned long now = millis();

  switch (currentStatus) {
    case 'S': // Scanning — cyan breathing
      if (now - lastBreath > 20) {
        lastBreath = now;
        breathVal += breathDir * 3;
        if (breathVal >= 50) { breathVal = 50; breathDir = -1; }
        if (breathVal <= 5)  { breathVal = 5;  breathDir =  1; }
        strip.setBrightness((int)breathVal);
        showAll(COL_CYAN);
      }
      break;

    case 'C': // Clear — steady green
      strip.setBrightness(40);
      showAll(COL_GREEN);
      break;

    case 'P': // Partial — amber slow pulse
      if (now - lastBreath > 25) {
        lastBreath = now;
        breathVal += breathDir * 2;
        if (breathVal >= 50) { breathVal = 50; breathDir = -1; }
        if (breathVal <= 15) { breathVal = 15; breathDir =  1; }
        strip.setBrightness((int)breathVal);
        showAll(COL_AMBER);
      }
      break;

    case 'B': // Blocked — red fast blink
      if (now - lastBlink > 200) {
        lastBlink = now;
        blinkState = !blinkState;
        strip.setBrightness(40);
        showAll(blinkState ? COL_RED : COL_OFF);
      }
      break;
  }
}

void showAll(uint32_t color) {
  for (int i = 0; i < NEO_NUM; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}