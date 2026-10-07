# ESP8266 & ESP-01S Project Status & Guidelines

## 🚀 Project Overview & Milestones

### 1. Wemos D1 Mini
- **Display Setup**: 16-LED Circular Ring (Round Display).
- **Status**: Running smooth and stable.
- **Wi-Fi Stability**: WiFi Modem Sleep disabled (`WiFi.setSleepMode(WIFI_NONE_SLEEP)`) to prevent packet drop and 0ms web dashboard response.
- **OTA Status**: **Not Enabled** yet (planned for future update).

### 2. ESP-01S
- **Display Setup**: 4x4 LED Matrix / Layout (16 NeoPixels total).
- **Status**: Functional web dashboard, but experiencing periodic resets / bootloops under load.
- **OTA Status**: **Enabled** (flashed via `pio run -e esp01_1m_ota -t upload`).
- **Pin Assignment**: Uses **GPIO2** as `NEOPIXEL_PIN` (defined automatically via `config.h`).
- **Flash Mode**: 1MB SPI Flash (`board_build.flash_mode = dout`).

---

## ⚠️ Known Issue: ESP-01S Periodic Resets & Root Causes

### 1. Power Supply Brownout (Primary Cause - 80% of resets)
The ESP-01S module lacks onboard voltage regulators and bulk capacitors. 
When Wi-Fi RF power spikes (up to 300mA-450mA peak) alongside NeoPixel brightness updates, the 3.3V line drops below ~2.8V, triggering a hardware **Power-On Reset (POR)** or **Brownout Reset**.

### 2. Forced Modem Sleep Mode (`WIFI_NONE_SLEEP`) Power Draw
On Wemos D1 Mini (which has a 500mA onboard LDO regulator), disabling Wi-Fi sleep (`WIFI_NONE_SLEEP`) runs fine. On ESP-01S powered by a weak USB-TTL programmer, constant max Wi-Fi TX power overwhelms the power source.

### 3. GPIO Boot Pin States (GPIO0 / GPIO2 / CH_PD)
- **GPIO2** (NeoPixel Data Pin) must be HIGH at boot. Any low pull at startup can force bootloader failure.
- **CH_PD / EN** must be tied firmly to 3.3V.

---

## 🛠️ Solutions to Fix ESP-01S Resets

### A. Hardware Fixes (Recommended)
1. **Bulk Decoupling Capacitor**: Add a **100µF to 470µF electrolytic capacitor** directly across **3.3V and GND** on the ESP-01S headers.
2. **Dedicated 3.3V Power Supply**: Do not power the ESP-01S and 16 NeoPixels directly from the 3.3V pin of a cheap USB-to-TTL adapter. Use a dedicated 3.3V regulator (e.g., AMS1117 3.3V or external 5V-to-3.3V step-down module capable of delivering **at least 500mA - 1A**).
3. **Pull-Up Resistors**:
   - Ensure **CH_PD (EN)** is connected to 3.3V with a 10kΩ resistor (or directly to 3.3V).
   - Ensure **RST** is connected to 3.3V with a 10kΩ resistor.

### B. Software Adjustments
1. **Enable Light Modem Sleep on ESP-01S** or lower default NeoPixel brightness (`NEOPIXEL_BRIGHTNESS = 15` or lower).
2. **Prevent WDT Timeouts**: Maintain regular `yield()` and `delay(1)` calls in animation loop execution.
