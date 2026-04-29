/*
 * PaveSafe — Arduino MKR WiFi 1010 + NeoPixel 8-stick
 * HTTP server receives /C /P /B to control NeoPixel
 */

#include <WiFiNINA.h>
#include <Adafruit_NeoPixel.h>

const char* SSID     = "Gilang";
const char* PASSWORD = "internetmahal";
const int   NEO_PIN  = 6;
const int   NEO_NUM  = 8;

Adafruit_NeoPixel strip(NEO_NUM, NEO_PIN, NEO_GRB + NEO_KHZ800);
WiFiServer server(80);

char currentStatus = 'S';
unsigned long lastBreath = 0;
unsigned long lastBlink  = 0;
bool blinkState = false;
float breathVal = 10;
float breathDir = 1;

uint32_t COL_CYAN  = strip.Color(0,   200, 255);
uint32_t COL_GREEN = strip.Color(0,   230, 100);
uint32_t COL_AMBER = strip.Color(255, 160, 0  );
uint32_t COL_RED   = strip.Color(255, 20,  50 );
uint32_t COL_OFF   = strip.Color(0,   0,   0  );

void setup() {
  Serial.begin(115200);

  strip.begin();
  strip.setBrightness(40);
  showAll(COL_OFF);
  strip.show();

  // Boot sweep
  for (int i = 0; i < NEO_NUM; i++) {
    strip.setPixelColor(i, COL_CYAN);
    strip.show();
    delay(60);
  }

  WiFi.disconnect();
  WiFi.end();
  delay(1000);

  Serial.print("Connecting to ");
  Serial.println(SSID);
  showAll(strip.Color(10, 10, 10));

  WiFi.begin(SSID, PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to: " + String(WiFi.SSID()));
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    for (int i = 0; i < 3; i++) {
      showAll(COL_GREEN); delay(200);
      showAll(COL_OFF);   delay(200);
    }

    server.begin();
    Serial.print("Enter this IP in PaveSafe app: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi failed!");
    for (int i = 0; i < 5; i++) {
      showAll(COL_RED); delay(300);
      showAll(COL_OFF); delay(300);
    }
  }
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String request = "";
    unsigned long timeout = millis() + 1000;

    while (client.connected() && millis() < timeout) {
      if (client.available()) {
        char c = client.read();
        request += c;
        if (c == '\n' && request.length() > 10) break;
      }
    }

    Serial.println("Request: " + request.substring(0, min(50, (int)request.length())));

    // Parse first line of HTTP request
    // e.g. "GET /C HTTP/1.1"
    char cmd = 0;
    if      (request.indexOf("/C") >= 0) cmd = 'C';
    else if (request.indexOf("/P") >= 0) cmd = 'P';
    else if (request.indexOf("/B") >= 0) cmd = 'B';
    else if (request.indexOf("/S") >= 0) cmd = 'S';

    if (cmd != 0) {
      currentStatus = cmd;
      Serial.print("Status → ");
      Serial.println(currentStatus);

      // Immediately update LEDs for snappy response
      updateLEDsNow();
    }

    // CORS headers so browser/app can call from any origin
    client.println("HTTP/1.1 200 OK");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Access-Control-Allow-Methods: GET");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.print("OK:");
    client.println(currentStatus);
    delay(1);
    client.stop();
  }

  animateLEDs();
}

void updateLEDsNow() {
  // Immediate LED update without waiting for animation loop
  strip.setBrightness(40);
  switch (currentStatus) {
    case 'C': showAll(COL_GREEN); break;
    case 'P': showAll(COL_AMBER); break;
    case 'B': showAll(COL_RED);   break;
    case 'S': showAll(COL_CYAN);  break;
  }
}

void animateLEDs() {
  unsigned long now = millis();

  switch (currentStatus) {
    case 'S':
      if (now - lastBreath > 20) {
        lastBreath = now;
        breathVal += breathDir * 3;
        if (breathVal >= 50) { breathVal = 50; breathDir = -1; }
        if (breathVal <= 5)  { breathVal = 5;  breathDir =  1; }
        strip.setBrightness((int)breathVal);
        showAll(COL_CYAN);
      }
      break;

    case 'C':
      strip.setBrightness(40);
      showAll(COL_GREEN);
      break;

    case 'P':
      if (now - lastBreath > 25) {
        lastBreath = now;
        breathVal += breathDir * 2;
        if (breathVal >= 50) { breathVal = 50; breathDir = -1; }
        if (breathVal <= 15) { breathVal = 15; breathDir =  1; }
        strip.setBrightness((int)breathVal);
        showAll(COL_AMBER);
      }
      break;

    case 'B':
      if (now - lastBreath > 15) {
        lastBreath = now;
        breathVal += breathDir * 4;
        if (breathVal >= 55) { breathVal = 55; breathDir = -1; }
        if (breathVal <= 5)  { breathVal = 5;  breathDir =  1; }
        strip.setBrightness((int)breathVal);
        showAll(COL_RED);
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
