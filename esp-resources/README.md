# ESP Resources

> ESP32 drivers, MySQL integration code, and member-specific firmware resources.

---

## 📁 Contents

| Folder | Description |
|--------|-------------|
| `ESP32 DRIVER/` | CP2102 USB-to-serial driver files for Windows (required for ESP32 programming & serial communication) |
| `ESP32MYSQL/` | ESP32 sketches and resources for direct MySQL database integration |
| `Sam_ESP/` | Sam's ESP32 firmware — standalone version with WiFi, OLED, I2S speaker, and result handling |

---

## 📌 Sam_ESP — Standalone Firmware

`Sam_ESP/Sam_ESP.ino` is a self-contained ESP32 sketch that:

- Connects to WiFi
- Runs an HTTP server on port 80
- Displays status on SSD1306 OLED (128×64)
- Receives `?result=Correct` or `?result=Wrong` via HTTP GET
- Plays corresponding audio via I2S speaker

### Pin Config (same as main firmware)

| Function | GPIO |
|----------|------|
| I2S Data | 33 |
| I2S BCLK | 27 |
| I2S LRCLK | 26 |
| OLED | I2C default (SDA/SCL) |
