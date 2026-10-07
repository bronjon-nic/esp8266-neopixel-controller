# ESP8266 NeoPixel Controller & Web Dashboard

An ESP8266-powered NeoPixel (WS2812B) controller featuring an embedded responsive web dashboard, real-time animation effects, multi-zone control, and ArduinoOTA over-the-air updates.

Designed for both **Wemos D1 Mini** (16-LED Circular Ring) and **ESP-01S** (4x4 Matrix / 16 NeoPixels).

---

## ✨ Features

- **Embedded Web Dashboard**: Modern, responsive UI served directly from ESP8266 memory on port 80.
- **Dynamic Lighting Animations**:
  - Comet Spinner
  - Rainbow Cycle
  - Breathing / Pulse
  - Solid Color & Custom Hex Picker
- **Zone Controls**: Switch between `All`, `Halves`, `Quarters`, and `Cross` LED segments.
- **Over-The-Air (OTA) Updates**: Flash new firmware over Wi-Fi without USB cables using `ArduinoOTA`.
- **Dual Hardware Support**: Pre-configured PlatformIO environments for Wemos D1 Mini and ESP-01S (1MB).
- **Power Brownout Protection**: Built-in safe brightness defaults and Wi-Fi modem sleep handling to prevent 3.3V reset loops.

---

## 🛠️ Hardware & Pin Configuration

| Board | NeoPixel Data Pin | Recommended Setup | Default Env |
|---|---|---|---|
| **Wemos D1 Mini** | `D5` (GPIO14) | 16-LED Circular Ring | `d1_mini` |
| **ESP-01S (1MB)** | `GPIO2` (Pin 2) | 4x4 LED Matrix / 16 LEDs | `esp01_1m_serial` / `esp01_1m_ota` |

> [!IMPORTANT]
> **ESP-01S Power & Bootloader Requirements**:
> - ESP-01S requires a stable 3.3V supply (peak Wi-Fi TX + LED current can draw >400mA).
> - Place a **100µF – 470µF electrolytic capacitor** across `3.3V` and `GND`.
> - Ensure **CH_PD / EN** is pulled HIGH to 3.3V with a 10kΩ resistor.
> - **GPIO2** must stay HIGH at boot to avoid triggering bootloader programming mode.

---

## 🚀 Getting Started

### 1. Prerequisites
- [PlatformIO Core or PlatformIO IDE](https://platformio.org/)
- VS Code or your preferred editor

### 2. Configure Wi-Fi & Settings
Edit `include/config.h`:
```cpp
// Wi-Fi Credentials
#define WIFI_SSID     "Your-WiFi-SSID"
#define WIFI_PASSWORD "Your-WiFi-Password"

// OTA Configuration
#define OTA_HOSTNAME  "wemos-d1mini-ota"
#define OTA_PORT      8266

// NeoPixel Configuration
#define MAX_LEDS            16
#define NEOPIXEL_BRIGHTNESS 15  // Safe brightness (0-255)
```

### 3. Build & Upload Firmware

#### For Wemos D1 Mini (Serial):
```bash
pio run -e d1_mini -t upload
```

#### For ESP-01S (Serial via USB-UART adapter):
```bash
pio run -e esp01_1m_serial -t upload
```

#### For ESP-01S (Over-The-Air):
```bash
pio run -e esp01_1m_ota -t upload
```

---

## 🌐 Web Dashboard Usage

1. Open your serial monitor at **115200 baud** after booting (`pio device monitor`).
2. Note the assigned local IP address (e.g. `http://192.168.1.100`) or mDNS hostname.
3. Open the IP address in your browser to access the control panel.
4. Toggle power, change animations, select custom colors, and set brightness on the fly.

---

## 📁 Project Structure

```
├── include/
│   └── config.h         # Wi-Fi credentials, pinouts, and LED constants
├── src/
│   └── main.cpp         # Web server, OTA handler, animation logic & dashboard HTML
├── platformio.ini       # Multi-environment PlatformIO build configuration
├── AGENTS.md            # Hardware troubleshooting & project guidelines
├── .gitignore           # Git ignore rules for PlatformIO & caches
└── README.md            # Project documentation
```

---

## 📄 License
MIT License. Feel free to use and adapt in your own IoT projects!
