# ESP32 Test & Prototype Firmware

> Prototyping sketches and test code used during development of the Smart Language Monitoring System's ESP32 hardware.

---

## 📁 Contents

| Folder | Description |
|--------|-------------|
| `combine/` | **Main combined firmware** — WiFi + OLED + I2S speaker + web server + HTTP status reporting (final version) |
| `buttonpython/` | Button-triggered Python speech recognition tests |
| `esp32 communication handler/` | Test code for ESP32 ↔ PHP server communication |
| `esp32+result from python(inovation)/` | ESP32 receiving classification results back from Python |
| `esp32libraryaudiofiles/` | Audio file library for I2S speaker playback |
| `esp32shortcut/` | Simplified/shortcut firmware sketches |
| `pushbuttontest/` | Basic push button input testing |

---

## 🎯 Main Firmware: `combine/combine.ino`

The final combined sketch that integrates all features:

- **WiFi connection** → connects to configured network
- **Web server** (port 80) → receives commands from PHP backend
- **OLED display** (SSD1306, 128×64) → shows status and results
- **I2S audio playback** → plays `good_job.h` or `wrong_answer.h` sounds
- **HTTP status reporting** → sends heartbeat to server every 60 seconds

### Dependencies (Arduino Libraries)

- `WiFi.h` — ESP32 WiFi
- `WebServer.h` — HTTP server
- `HTTPClient.h` — outgoing HTTP requests
- `Adafruit_GFX.h` + `Adafruit_SSD1306.h` — OLED display
- `driver/i2s.h` — I2S audio output

### Audio Data

Audio stored as C header arrays in PROGMEM:
- `good_job.h` — "Good job!" audio (≈230 KB raw)
- `wrong_answer.h` — "Wrong answer" audio (≈614 KB raw)
- Format: 16-bit PCM, 16 kHz sample rate, mono

---

## ⚠️ Notes

- Update WiFi SSID/password in each sketch before flashing
- Update `statusServerUrl` to point to your PHP server's IP
- `apiKey` and `deviceID` must match the values in `esp_communication_handler.php`
