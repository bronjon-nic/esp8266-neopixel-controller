#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

// Both supported layouts contain 16 NeoPixels; NEOPIXEL_LAYOUT records their
// physical arrangement for the dashboard and configuration.
Adafruit_NeoPixel strip(MAX_LEDS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Initialize Web Server on Port 80
ESP8266WebServer server(80);

// Control State
bool powerOn = true;
uint16_t activeNumLeds = DEFAULT_ACTIVE_LEDS;
String activeZone = "all"; // Options: "all", "quarter", "cross", "halves"
uint8_t currentBrightness = NEOPIXEL_BRIGHTNESS;
uint32_t currentColor = Adafruit_NeoPixel::Color(0, 240, 255); // Default Cyan
String currentMode = "spinner"; // Default mode: Comet Spinner

// Flags and timing
bool stripDirty = true;
bool otaInitialized = false;
bool webServerInitialized = false;
bool accessPointStarted = false;

unsigned long lastAnimationUpdate = 0;
unsigned long lastStatusPrint = 0;
bool otaInProgress = false;
uint16_t pixelHue = 0;

// Embedded HTML/CSS/JS Dashboard
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP8266 NeoPixel Dashboard</title>
  <style>
    :root {
      --bg-color: #0b132b;
      --card-bg: #1c2541;
      --accent: #64dfdf;
      --accent-glow: rgba(100, 223, 223, 0.4);
      --text: #f8fafc;
      --text-sub: #94a3b8;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: system-ui, -apple-system, sans-serif; }
    body { background: var(--bg-color); color: var(--text); display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 16px; }
    .container { background: var(--card-bg); width: 100%; max-width: 480px; border-radius: 24px; padding: 24px; box-shadow: 0 20px 50px rgba(0,0,0,0.6); border: 1px solid rgba(255,255,255,0.08); }
    .header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px; }
    h1 { font-size: 1.35rem; font-weight: 700; background: linear-gradient(135deg, #64dfdf, #72efdd, #48bfe3); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }
    .badge { background: #10b98122; color: #10b981; font-size: 0.75rem; font-weight: 600; padding: 4px 12px; border-radius: 12px; border: 1px solid #10b98144; }
    .section { margin-bottom: 20px; }
    label { display: block; font-size: 0.8rem; font-weight: 600; color: var(--text-sub); margin-bottom: 10px; text-transform: uppercase; letter-spacing: 0.8px; }
    .btn-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 8px; }
    .btn-grid-4 { display: grid; grid-template-columns: repeat(4, 1fr); gap: 8px; }
    .btn { background: #3a506b; color: var(--text); border: none; padding: 12px 8px; border-radius: 12px; font-weight: 600; cursor: pointer; transition: all 0.2s; font-size: 0.85rem; text-align: center; }
    .btn:hover { background: #5c6b73; transform: translateY(-1px); }
    .btn.active { background: var(--accent); color: #0b132b; font-weight: 700; box-shadow: 0 0 16px var(--accent-glow); }
    .power-btn { width: 100%; padding: 14px; font-size: 1rem; border-radius: 16px; margin-bottom: 18px; font-weight: 700; letter-spacing: 0.5px; border: none; cursor: pointer; transition: all 0.2s; }
    .power-on { background: linear-gradient(135deg, #f43f5e, #e11d48); color: #fff; box-shadow: 0 4px 15px rgba(244,63,94,0.4); }
    .power-off { background: linear-gradient(135deg, #10b981, #059669); color: #fff; box-shadow: 0 4px 15px rgba(16,185,129,0.4); }
    input[type="range"] { width: 100%; accent-color: var(--accent); height: 6px; background: #3a506b; border-radius: 3px; cursor: pointer; }
    input[type="color"] { width: 100%; height: 44px; border: none; border-radius: 12px; cursor: pointer; background: transparent; }
    .preset-colors { display: flex; gap: 8px; margin-top: 10px; justify-content: space-between; }
    .color-dot { width: 34px; height: 34px; border-radius: 50%; cursor: pointer; border: 2px solid rgba(255,255,255,0.2); transition: transform 0.15s; }
    .color-dot:hover { transform: scale(1.15); }
    .val-disp { float: right; color: var(--accent); font-weight: 700; }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>16-LED Dashboard</h1>
      <span class="badge" id="statusBadge">ONLINE</span>
    </div>

    <button class="btn power-btn power-off" id="powerBtn" onclick="togglePower()">POWER ON</button>

    <div class="section">
      <label>LED Zones</label>
      <div class="btn-grid-4">
        <button class="btn active" id="zone-all" onclick="setZone('all')">Full Display</button>
        <button class="btn" id="zone-quarter" onclick="setZone('quarter')">Every 4th</button>
        <button class="btn" id="zone-cross" onclick="setZone('cross')">8 Cross</button>
        <button class="btn" id="zone-halves" onclick="setZone('halves')">First Half</button>
      </div>
    </div>

    <div class="section">
      <label>Active LED Limit <span class="val-disp" id="numLedsVal">16</span> / 16</label>
      <input type="range" id="numLedsSlider" min="1" max="16" value="16" oninput="updateNumLeds(this.value)">
    </div>

    <div class="section">
      <label>Animation Effects</label>
      <div class="btn-grid">
        <button class="btn active" id="mode-spinner" onclick="setMode('spinner')">🌀 Comet Spinner</button>
        <button class="btn" id="mode-orbit" onclick="setMode('orbit')">☯️ Dual Orbit</button>
        <button class="btn" id="mode-sweep" onclick="setMode('sweep')">⏱️ Clock Sweep</button>
        <button class="btn" id="mode-wave" onclick="setMode('wave')">🌊 Sine Wave</button>
        <button class="btn" id="mode-rainbow" onclick="setMode('rainbow')">🌈 Rainbow Wheel</button>
        <button class="btn" id="mode-breathing" onclick="setMode('breathing')">🫁 Breathing Halo</button>
        <button class="btn" id="mode-fireflies" onclick="setMode('fireflies')">✨ Star Sparkle</button>
        <button class="btn" id="mode-heartbeat" onclick="setMode('heartbeat')">🎯 Heartbeat</button>
        <button class="btn" id="mode-split" onclick="setMode('split')">🌓 Dual Split</button>
        <button class="btn" id="mode-solid" onclick="setMode('solid')">🎨 Solid Color</button>
      </div>
    </div>

    <div class="section">
      <label>Solid Color Picker</label>
      <input type="color" id="colorPicker" value="#00f0ff" onchange="setColor(this.value)">
      <div class="preset-colors">
        <div class="color-dot" style="background:#00f0ff" onclick="setColor('#00f0ff')"></div>
        <div class="color-dot" style="background:#ff007f" onclick="setColor('#ff007f')"></div>
        <div class="color-dot" style="background:#00ff66" onclick="setColor('#00ff66')"></div>
        <div class="color-dot" style="background:#ffb703" onclick="setColor('#ffb703')"></div>
        <div class="color-dot" style="background:#9d4edd" onclick="setColor('#9d4edd')"></div>
        <div class="color-dot" style="background:#ffffff" onclick="setColor('#ffffff')"></div>
      </div>
    </div>

    <div class="section">
      <label>Brightness <span class="val-disp" id="brightVal">15</span></label>
      <div class="val-disp" id="layoutLabel"></div>
      <input type="range" id="brightnessSlider" min="0" max="255" value="15" oninput="updateBrightness(this.value)">
    </div>
  </div>

  <script>
    let state = { power: true, mode: 'spinner', zone: 'all', brightness: 15, num_leds: 16, color: '00f0ff' };

    function fetchStatus() {
      fetch('/api/status')
        .then(res => res.json())
        .then(data => {
          state = data;
          updateUI();
        }).catch(err => console.log(err));
    }

    function updateUI() {
      const pBtn = document.getElementById('powerBtn');
      pBtn.innerText = state.power ? 'TURN OFF' : 'TURN ON';
      pBtn.className = 'btn power-btn ' + (state.power ? 'power-on' : 'power-off');

      document.querySelectorAll('.btn-grid .btn').forEach(b => b.classList.remove('active'));
      const activeBtn = document.getElementById('mode-' + state.mode);
      if(activeBtn) activeBtn.classList.add('active');

      document.querySelectorAll('.btn-grid-4 .btn').forEach(b => b.classList.remove('active'));
      const zoneBtn = document.getElementById('zone-' + (state.zone || 'all'));
      if(zoneBtn) zoneBtn.classList.add('active');

      document.getElementById('numLedsSlider').value = state.num_leds;
      document.getElementById('numLedsVal').innerText = state.num_leds;

      document.getElementById('brightnessSlider').value = state.brightness;
      document.getElementById('brightVal').innerText = state.brightness;
      document.getElementById('colorPicker').value = '#' + state.color;
      document.getElementById('layoutLabel').innerText = state.layout || '';
    }

    function sendCmd(params) {
      fetch('/api/set?' + params)
        .then(res => res.json())
        .then(data => { state = data; updateUI(); });
    }

    function togglePower() { sendCmd('power=' + (state.power ? '0' : '1')); }
    function setMode(m) { sendCmd('mode=' + m); }
    function setZone(z) { sendCmd('zone=' + z); }
    function updateNumLeds(n) { document.getElementById('numLedsVal').innerText = n; sendCmd('num_leds=' + n); }
    function updateBrightness(v) { document.getElementById('brightVal').innerText = v; sendCmd('brightness=' + v); }
    function setColor(hex) { sendCmd('color=' + hex.replace('#','')); }

    window.onload = fetchStatus;
  </script>
</body>
</html>
)rawliteral";

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

uint32_t dimColor(uint32_t color, float factor) {
  uint8_t r = (uint8_t)(((color >> 16) & 0xFF) * factor);
  uint8_t g = (uint8_t)(((color >> 8) & 0xFF) * factor);
  uint8_t b = (uint8_t)((color & 0xFF) * factor);
  return strip.Color(r, g, b);
}

bool isPixelInActiveZone(int i) {
  if (i >= activeNumLeds) return false;
  if (activeZone == "quarter") {
    return (i % 4 == 0); // 0, 4, 8, 12
  }
  if (activeZone == "cross") {
    return (i % 2 == 0); // 0, 2, 4, 6, 8, 10, 12, 14
  }
  if (activeZone == "halves") {
    return (i < 8); // Top half: 0..7
  }
  return true; // "all"
}

void applyWifiStatusOverlay() {
    if (!accessPointStarted) {
    float pulseFactor = 0.15f + 0.85f * ((sin(millis() * 0.005f) + 1.0f) / 2.0f);
    uint8_t redVal = (uint8_t)(180 * pulseFactor);

    // Pulse red indicators on cardinal points 0, 4, 8, 12
    const int CARDINALS[] = {0, 4, 8, 12};
    for (int idx : CARDINALS) {
      if (isPixelInActiveZone(idx)) {
        strip.setPixelColor(idx, strip.Color(redVal, 0, 0));
      }
    }
  }
}

void handleRoot() {
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleStatus() {
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");

  char hexColor[7];
  uint8_t r = (currentColor >> 16) & 0xFF;
  uint8_t g = (currentColor >> 8) & 0xFF;
  uint8_t b = currentColor & 0xFF;
  sprintf(hexColor, "%02x%02x%02x", r, g, b);

  char jsonBuf[256];
  snprintf(jsonBuf, sizeof(jsonBuf),
           "{\"power\":%s,\"mode\":\"%s\",\"zone\":\"%s\",\"num_leds\":%u,\"max_leds\":%u,\"brightness\":%u,\"color\":\"%s\",\"layout\":\"%s\"}",
           powerOn ? "true" : "false",
           currentMode.c_str(),
           activeZone.c_str(),
           activeNumLeds,
           MAX_LEDS,
           currentBrightness,
           hexColor,
           NEOPIXEL_LAYOUT_NAME);

  server.send(200, "application/json", jsonBuf);
}

void handleSet() {
  if (server.hasArg("power")) {
    powerOn = (server.arg("power") == "1");
    stripDirty = true;
  }
  if (server.hasArg("mode")) {
    currentMode = server.arg("mode");
    stripDirty = true;
  }
  if (server.hasArg("zone")) {
    activeZone = server.arg("zone");
    stripDirty = true;
  }
  if (server.hasArg("num_leds")) {
    int n = server.arg("num_leds").toInt();
    if (n >= 1 && n <= MAX_LEDS) {
      activeNumLeds = n;
      stripDirty = true;
    }
  }
  if (server.hasArg("brightness")) {
    currentBrightness = server.arg("brightness").toInt();
    strip.setBrightness(currentBrightness);
    stripDirty = true;
  }
  if (server.hasArg("color")) {
    String hex = server.arg("color");
    long number = strtol(hex.c_str(), NULL, 16);
    long r = number >> 16;
    long g = (number >> 8) & 0xFF;
    long b = number & 0xFF;
    currentColor = strip.Color(r, g, b);
    currentMode = "solid";
    stripDirty = true;
  }

  handleStatus();
}

void setupAccessPoint() {
  Serial.println(F("\n=================================================="));
  Serial.println(F("          STARTING ESP8266 ACCESS POINT           "));
  Serial.println(F("=================================================="));
  Serial.printf("Access point SSID: %s\n", AP_SSID);

  strip.clear();
  strip.setPixelColor(0, strip.Color(120, 0, 0));
  strip.show();

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  accessPointStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);

  if (accessPointStarted) {
    Serial.println(F("[SUCCESS] Access point started."));
    Serial.printf("  - Connect to  : %s\n", AP_SSID);
    Serial.printf("  - Dashboard   : http://%s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("  - OTA Address : %s:%u\n", WiFi.softAPIP().toString().c_str(), OTA_PORT);
    strip.clear();
    strip.show();
  } else {
    Serial.println(F("[ERROR] Unable to start access point."));
  }
  Serial.println(F("==================================================\n"));
}

void setupOTA() {
  if (otaInitialized) return;
  ArduinoOTA.setPort(OTA_PORT);
  ArduinoOTA.setHostname(OTA_HOSTNAME);

  if (strlen(OTA_PASSWORD) > 0) {
    ArduinoOTA.setPassword(OTA_PASSWORD);
  }

  ArduinoOTA.onStart([]() {
    otaInProgress = true;
    for (int i = 0; i < MAX_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(80, 0, 80));
    }
    strip.show();
  });

  ArduinoOTA.onEnd([]() {
    otaInProgress = false;
    for (int i = 0; i < MAX_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(0, 150, 0));
    }
    strip.show();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    int ledsLit = map(progress, 0, total, 0, MAX_LEDS);
    for (int i = 0; i < MAX_LEDS; i++) {
      if (i <= ledsLit) {
        strip.setPixelColor(i, strip.Color(0, 0, 150));
      } else {
        strip.setPixelColor(i, strip.Color(10, 0, 10));
      }
    }
    strip.show();
  });

  ArduinoOTA.onError([](ota_error_t error) {
    otaInProgress = false;
    for (int i = 0; i < MAX_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(150, 0, 0));
    }
    strip.show();
  });

  ArduinoOTA.begin();
  otaInitialized = true;
  Serial.println(F("[OTA] Service initialized."));
}

void setupWebServer() {
  if (webServerInitialized) return;
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/set", HTTP_GET, handleSet);
  server.begin();
  webServerInitialized = true;
  Serial.println(F("[WEB] Web Server started on port 80."));
}

void animateLEDs() {
  if (!powerOn) {
    if (!accessPointStarted || stripDirty) {
      stripDirty = false;
      strip.clear();
      applyWifiStatusOverlay();
      strip.show();
    }
    return;
  }

  // 1. Solid Color Mode
  if (currentMode == "solid") {
    if (stripDirty || !accessPointStarted) {
      stripDirty = false;
      for (int i = 0; i < MAX_LEDS; i++) {
        if (isPixelInActiveZone(i)) {
          strip.setPixelColor(i, currentColor);
        } else {
          strip.setPixelColor(i, 0);
        }
      }
      applyWifiStatusOverlay();
      strip.show();
    }
  }
  // 2. 🌀 Comet Spinner (Single head with fading tail)
  else if (currentMode == "spinner") {
    if (millis() - lastAnimationUpdate >= 45) {
      lastAnimationUpdate = millis();
      static int headPos = 0;

      strip.clear();
      for (int t = 0; t < 5; t++) {
        int idx = (headPos - t + MAX_LEDS) % MAX_LEDS;
        if (isPixelInActiveZone(idx)) {
          float factor = 1.0f - (t * 0.22f);
          strip.setPixelColor(idx, dimColor(currentColor, factor));
        }
      }
      headPos = (headPos + 1) % MAX_LEDS;

      applyWifiStatusOverlay();
      strip.show();
    }
  }
  // 3. ☯️ Dual Orbit (Two counter-rotating comets)
  else if (currentMode == "orbit") {
    if (millis() - lastAnimationUpdate >= 50) {
      lastAnimationUpdate = millis();
      static int posCW = 0;
      static int posCCW = 15;

      strip.clear();

      uint32_t color1 = currentColor;
      uint32_t color2 = strip.Color(((currentColor >> 8) & 0xFF), ((currentColor) & 0xFF), ((currentColor >> 16) & 0xFF));
      if (color2 == 0) color2 = strip.Color(255, 0, 200); // Fallback magenta

      // Clockwise comet
      for (int t = 0; t < 4; t++) {
        int idx = (posCW - t + MAX_LEDS) % MAX_LEDS;
        if (isPixelInActiveZone(idx)) {
          strip.setPixelColor(idx, dimColor(color1, 1.0f - t * 0.25f));
        }
      }

      // Counter-clockwise comet
      for (int t = 0; t < 4; t++) {
        int idx = (posCCW + t) % MAX_LEDS;
        if (isPixelInActiveZone(idx)) {
          uint32_t existing = strip.getPixelColor(idx);
          if (existing > 0) {
            strip.setPixelColor(idx, strip.Color(255, 255, 255)); // White pulse where they cross!
          } else {
            strip.setPixelColor(idx, dimColor(color2, 1.0f - t * 0.25f));
          }
        }
      }

      posCW = (posCW + 1) % MAX_LEDS;
      posCCW = (posCCW - 1 + MAX_LEDS) % MAX_LEDS;

      applyWifiStatusOverlay();
      strip.show();
    }
  }
  // 4. ⏱️ Clock Sweep / Circular Arc Wipe
  else if (currentMode == "sweep") {
    if (millis() - lastAnimationUpdate >= 60) {
      lastAnimationUpdate = millis();
      static int fillLength = 1;
      static bool filling = true;

      strip.clear();
      for (int i = 0; i < fillLength; i++) {
        if (isPixelInActiveZone(i)) {
          strip.setPixelColor(i, currentColor);
        }
      }

      if (filling) {
        fillLength++;
        if (fillLength > MAX_LEDS) {
          filling = false;
          fillLength = MAX_LEDS - 1;
        }
      } else {
        fillLength--;
        if (fillLength < 0) {
          filling = true;
          fillLength = 1;
        }
      }

      applyWifiStatusOverlay();
      strip.show();
    }
  }
  // 5. 🌊 Sine Wave Arc (Angular wave around the ring)
  else if (currentMode == "wave") {
    if (millis() - lastAnimationUpdate >= 30) {
      lastAnimationUpdate = millis();
      static float phase = 0.0;
      phase += 0.06f;

      for (int i = 0; i < MAX_LEDS; i++) {
        if (isPixelInActiveZone(i)) {
          float angle = i * (2.0f * 3.14159f / 16.0f);
          float val = (sin(phase + angle * 2.0f) + 1.0f) / 2.0f;
          strip.setPixelColor(i, dimColor(currentColor, val));
        } else {
          strip.setPixelColor(i, 0);
        }
      }

      applyWifiStatusOverlay();
      strip.show();
    }
  }
  // 6. 🌈 Rainbow Wheel (Continuous 360° Hue Spectrum)
  else if (currentMode == "rainbow") {
    if (millis() - lastAnimationUpdate >= 25) {
      lastAnimationUpdate = millis();
      for (int i = 0; i < MAX_LEDS; i++) {
        if (isPixelInActiveZone(i)) {
          byte huePos = ((i * 256 / MAX_LEDS) + pixelHue) & 255;
          strip.setPixelColor(i, Wheel(huePos));
        } else {
          strip.setPixelColor(i, 0);
        }
      }
      pixelHue = (pixelHue + 2) & 255;

      applyWifiStatusOverlay();
      strip.show();
    }
  }
  // 7. 🫁 Breathing Halo (Pulsing brightness)
  else if (currentMode == "breathing") {
    if (millis() - lastAnimationUpdate >= 30) {
      lastAnimationUpdate = millis();
      static float angle = 0.0f;
      angle += 0.05f;
      if (angle >= 6.28318f) angle = 0.0f;

      float factor = (sin(angle) + 1.0f) / 2.0f;
      uint32_t dimmed = dimColor(currentColor, factor);

      for (int i = 0; i < MAX_LEDS; i++) {
        if (isPixelInActiveZone(i)) {
          strip.setPixelColor(i, dimmed);
        } else {
          strip.setPixelColor(i, 0);
        }
      }

      applyWifiStatusOverlay();
      strip.show();
    }
  }
  // 8. ✨ Star Sparkle (Random twinkling fireflies)
  else if (currentMode == "fireflies") {
    if (millis() - lastAnimationUpdate >= 45) {
      lastAnimationUpdate = millis();
      strip.clear();

      int sparkleCount = random(2, 5);
      for (int k = 0; k < sparkleCount; k++) {
        int idx = random(0, MAX_LEDS);
        if (isPixelInActiveZone(idx)) {
          uint32_t spColor = (random(100) < 50) ? currentColor : Wheel(random(0, 256));
          strip.setPixelColor(idx, spColor);
        }
      }

      applyWifiStatusOverlay();
      strip.show();
    }
  }
  // 9. 🎯 Heartbeat (Double pulse rhythm)
  else if (currentMode == "heartbeat") {
    if (millis() - lastAnimationUpdate >= 20) {
      lastAnimationUpdate = millis();
      static unsigned long cycleStart = 0;
      unsigned long elapsed = millis() - cycleStart;

      if (elapsed > 1200) {
        cycleStart = millis();
        elapsed = 0;
      }

      float factor = 0.05f;
      if (elapsed < 150) {
        factor = sin(elapsed * 3.14159f / 150.0f);
      } else if (elapsed >= 250 && elapsed < 400) {
        factor = 0.8f * sin((elapsed - 250) * 3.14159f / 150.0f);
      }

      uint32_t pulsed = dimColor(currentColor, factor);
      for (int i = 0; i < MAX_LEDS; i++) {
        if (isPixelInActiveZone(i)) {
          strip.setPixelColor(i, pulsed);
        } else {
          strip.setPixelColor(i, 0);
        }
      }

      applyWifiStatusOverlay();
      strip.show();
    }
  }
  // 10. 🌓 Dual Split (Top half vs Bottom half complement)
  else if (currentMode == "split") {
    if (millis() - lastAnimationUpdate >= 50) {
      lastAnimationUpdate = millis();
      static int offset = 0;

      uint32_t c1 = currentColor;
      uint32_t c2 = strip.Color(((currentColor >> 8) & 0xFF), ((currentColor) & 0xFF), ((currentColor >> 16) & 0xFF));
      if (c2 == 0) c2 = strip.Color(255, 0, 150);

      for (int i = 0; i < MAX_LEDS; i++) {
        int pos = (i + offset) % MAX_LEDS;
        if (isPixelInActiveZone(pos)) {
          if (i < 8) {
            strip.setPixelColor(pos, c1);
          } else {
            strip.setPixelColor(pos, c2);
          }
        } else {
          strip.setPixelColor(pos, 0);
        }
      }
      offset = (offset + 1) % MAX_LEDS;

      applyWifiStatusOverlay();
      strip.show();
    }
  }
  yield();
}

void setup() {
  Serial.begin(115200);

  randomSeed(micros());

  strip.begin();
  strip.setBrightness(NEOPIXEL_BRIGHTNESS);

  delay(500);

  setupAccessPoint();

  if (accessPointStarted) {
    setupOTA();
    setupWebServer();
  }
}

void loop() {
  if (otaInitialized) {
    ArduinoOTA.handle();
  }
  if (webServerInitialized) {
    server.handleClient();
  }

  animateLEDs();

  // Give 1ms to the ESP8266 background scheduler to prevent Watchdog resets
  delay(1);
}
