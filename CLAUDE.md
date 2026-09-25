# Project: Embedded Systems & IoT Engineering

## Stack & Hardware
- **Target Microcontroller**: ESP32 / ESP8266 / STM32 / Raspberry Pi
- **Frameworks**: ESP-IDF / Arduino Framework / PlatformIO / MicroPython
- **Protocols**: MQTT, WebSockets, HTTP REST, BLE, I2C, SPI, UART

## Development Guidelines
1. **Memory Safety**: Prevent dynamic heap allocations in hot loop routines; prefer stack and static allocation.
2. **Non-Blocking I/O**: Use hardware timers, interrupts, and FreeRTOS tasks instead of blocking `delay()` calls.
3. **Resilience**: Implement watchdog timers (WDT) and auto-reconnect logic for Wi-Fi/MQTT connections.
4. **Telemetry**: Keep payload serialization lightweight (compact JSON or Protocol Buffers / MsgPack).

## Verification & Build
```bash
# PlatformIO build & upload
pio run
pio run --target upload
pio device monitor
```
