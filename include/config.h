#ifndef CONFIG_H
#define CONFIG_H

// ==========================================
// Wi-Fi Credentials
// ==========================================
#define WIFI_SSID     "JioFiber-NbkmY"
#define WIFI_PASSWORD "eer1luB3cah3zeih"

// ==========================================
// ArduinoOTA Configuration
// ==========================================
#define OTA_HOSTNAME  "wemos-d1mini-ota"
#define OTA_PASSWORD  "" // Set password if desired (e.g. "admin"), or keep empty "" for no password
#define OTA_PORT      8266

// ==========================================
// NeoPixel Configuration (16 LED Round Ring)
// ==========================================
#if defined(ARDUINO_ESP8266_WEMOS_D1MINI) || defined(ARDUINO_D1_MINI) || defined(D5)
#define NEOPIXEL_PIN        D5    // Pin D5 (GPIO14 on Wemos D1 Mini)
#else
#define NEOPIXEL_PIN        2     // GPIO2 (ESP-01S Header Pin 2)
#endif
#define MAX_LEDS            16    // 16 LED Round Ring total LEDs
#define DEFAULT_ACTIVE_LEDS 16    // Default active LEDs on boot
#define NEOPIXEL_BRIGHTNESS 15    // Safe brightness level (15 out of 255) to prevent 3.3V power supply brownouts

#endif // CONFIG_H
