#ifndef CONFIG_H
#define CONFIG_H

// ==========================================
// Wi-Fi Access Point Settings
// ==========================================
#define AP_SSID       "ESP8266-NeoPixel"
// WPA2 password: use at least eight characters.
#define AP_PASSWORD   "change-me"

// ==========================================
// ArduinoOTA Configuration
// ==========================================
#define OTA_HOSTNAME  "wemos-d1mini-ota"
#define OTA_PASSWORD  "" // Set password if desired (e.g. "admin"), or keep empty "" for no password
#define OTA_PORT      8266

// ==========================================
// NeoPixel Configuration
// Both supported displays have 16 LEDs. Set NEOPIXEL_LAYOUT to override the
// automatic board default, for example via a PlatformIO build flag.
#define NEOPIXEL_LAYOUT_RING       0
#define NEOPIXEL_LAYOUT_MATRIX_4X4 1

#ifndef NEOPIXEL_LAYOUT
#if defined(ARDUINO_ESP8266_WEMOS_D1MINI) || defined(ARDUINO_D1_MINI) || defined(D5)
#define NEOPIXEL_LAYOUT NEOPIXEL_LAYOUT_RING
#else
#define NEOPIXEL_LAYOUT NEOPIXEL_LAYOUT_MATRIX_4X4
#endif
#endif

#if NEOPIXEL_LAYOUT == NEOPIXEL_LAYOUT_RING
#define NEOPIXEL_LAYOUT_NAME "16-LED Ring"
#elif NEOPIXEL_LAYOUT == NEOPIXEL_LAYOUT_MATRIX_4X4
#define NEOPIXEL_LAYOUT_NAME "4x4 LED Matrix"
#else
#error "NEOPIXEL_LAYOUT must be NEOPIXEL_LAYOUT_RING or NEOPIXEL_LAYOUT_MATRIX_4X4"
#endif
// ==========================================
#if defined(ARDUINO_ESP8266_WEMOS_D1MINI) || defined(ARDUINO_D1_MINI) || defined(D5)
#define NEOPIXEL_PIN        D5    // Pin D5 (GPIO14 on Wemos D1 Mini)
#else
#define NEOPIXEL_PIN        2     // GPIO2 (ESP-01S Header Pin 2)
#endif
#define MAX_LEDS            16    // Ring and 4x4 matrix both use 16 LEDs
#define DEFAULT_ACTIVE_LEDS 16    // Default active LEDs on boot
#define NEOPIXEL_BRIGHTNESS 15    // Safe brightness level (15 out of 255) to prevent 3.3V power supply brownouts

#endif // CONFIG_H
