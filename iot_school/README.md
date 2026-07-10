# Smart Language Monitoring System (IoT School)

> An IoT-based multilingual speech recognition system for Malaysian schools that monitors and encourages students to practice speaking in multiple languages.

---

## 📋 Overview

This system allows students to interact with an **ESP32-based device** that listens to their speech, detects the language spoken, and provides immediate audio feedback. All interactions are logged to a **PHP/MySQL web dashboard** where teachers and admins can monitor language usage, manage settings, and view analytics.

### Supported Languages

| Language | Detection Key | Audio Feedback |
|----------|--------------|----------------|
| 🇬🇧 English | `ENGLISH` | ✅ Correct / ❌ Wrong |
| 🇲🇾 Bahasa Melayu | `MALAY` | ✅ Correct / ❌ Wrong |
| 🇨🇳 Mandarin | `CHINESE` | ✅ Correct / ❌ Wrong |
| 🇮🇳 Tamil | `TAMIL` | ✅ Correct / ❌ Wrong |

---

## 🔧 Hardware

| Component | Specs | Purpose |
|-----------|-------|---------|
| ESP32 | WiFi-enabled MCU | Main controller + web server |
| SSD1306 OLED | 128×64 pixels, I2C | Display status & results |
| I2S Speaker | MAX98357A DAC + speaker | Play correct/wrong audio |
| Push Button | Digital input | Trigger speech recognition |

### Pin Configuration

| Function | GPIO Pin |
|----------|----------|
| I2S Data (Speaker) | GPIO 33 |
| I2S BCLK | GPIO 27 |
| I2S LRCLK | GPIO 26 |
| OLED SDA | Default I2C |
| OLED SCL | Default I2C |

---

## 📁 File Structure

```
iot_school/
├── composer.json                 # PHP dependency (PHPMailer)
├── composer.lock
├── vendor/                       # Composer packages
└── main/
    ├── index.php                 # Login page (Teacher/Admin)
    ├── register.php              # Teacher registration
    ├── db_connection.php         # MySQL connection config
    ├── header.php                # Shared HTML header
    ├── sidebar.php               # Navigation sidebar
    │
    ├── teacher_dashboard.php     # Teacher main dashboard
    ├── teacher_views/            # Teacher sub-views
    ├── update_profile.php        # Profile settings
    │
    ├── admin_dashboard.php       # Admin language management
    ├── admin_manage_users.php    # Admin user management
    ├── admin_monitor_devices.php # ESP32 device monitoring
    ├── admin_overview.php        # Admin analytics overview
    │
    ├── esp_communication_handler.php  # ESP32 ↔ Server API
    ├── trigger_webpage.php            # Speech trigger endpoint
    ├── process_language_request.php   # Language request handler
    ├── process_language_actions.php   # Admin action processor
    │
    ├── language_monitor.py       # Python speech recognition engine
    ├── check_mics.py             # Microphone availability checker
    │
    ├── forgot_password.php       # Password recovery
    ├── reset_password.php        # Password reset
    ├── logout_admin.php          # Admin logout
    ├── logout_teacher.php        # Teacher logout
    │
    ├── mark_notifications_read.php   # Notification handler
    ├── delete_notification.php       # Notification cleanup
    ├── export_student_logs.php       # CSV export
    ├── export_weekly_usage.php       # Weekly report export
    │
    ├── language_monitor (2).sql  # Database schema + seed data
    ├── login-style.css           # Login page styles
    ├── my-login.css              # Additional login styles
    ├── assets/                   # Static assets
    ├── img/                      # Images
    ├── js/                       # JavaScript files
    └── script/                   # Helper scripts
```

---

## 🚀 Setup

### Prerequisites

- **XAMPP** (Apache + MySQL)
- **Python 3.8+** with packages:
  ```
  pip install SpeechRecognition gtts playsound lingua-language-detector mysql-connector-python openai-whisper noisereduce numpy
  ```
- **Arduino IDE** with ESP32 board support

### Database Setup

1. Start XAMPP → Start Apache & MySQL
2. Open phpMyAdmin (`http://localhost/phpmyadmin`)
3. Create database: `language_monitor`
4. Import `main/language_monitor (2).sql`

### Web Server

1. Copy `iot_school/main/` to `C:\xampp\htdocs\main\`
2. Run `composer install` in the `iot_school/` directory
3. Access at `http://localhost/main/`

### ESP32 Firmware

1. Open `testcombine/combine/combine.ino` in Arduino IDE
2. Update WiFi credentials:
   ```cpp
   const char* ssid = "YOUR_WIFI";
   const char* password = "YOUR_PASSWORD";
   ```
3. Update server URL:
   ```cpp
   const char* statusServerUrl = "http://YOUR_PC_IP/main/esp_communication_handler.php";
   ```
4. Flash to ESP32

### Configuration

| Setting | File | Value |
|---------|------|-------|
| DB Host | `db_connection.php` | `127.0.0.1` |
| DB User | `db_connection.php` | `root` |
| DB Name | `db_connection.php` | `language_monitor` |
| Timezone | `db_connection.php` | `+08:00` (Malaysia) |
| ESP32 IP | `language_monitor.py` | `172.20.10.4` |
| API Key | `esp_communication_handler.php` | `ESP32_SECRET_KEY` |
| Device ID | `esp_communication_handler.php` | `ESP32_LangMon_002` |
| Whisper Model | `language_monitor.py` | `base` |

---

## 🧩 Architecture

```
┌──────────────┐   WiFi HTTP    ┌──────────────────────┐
│   ESP32      │ ──────────────►│  PHP Web Server      │
│   + OLED     │   status/      │  (XAMPP Apache)      │
│   + Speaker  │   trigger      │                      │
│   + Button   │ ◄──────────────│  esp_communication   │
└──────────────┘   result       │  _handler.php        │
                                │         │            │
                                │         ▼            │
                                │  ┌──────────────┐    │
                                │  │ Python       │    │
                                │  │ language_    │    │
                                │  │ monitor.py   │    │
                                │  │ (Whisper +   │    │
                                │  │  Lingua)     │    │
                                │  └──────────────┘    │
                                │         │            │
                                │         ▼            │
                                │  ┌──────────────┐    │
                                │  │   MySQL      │    │
                                │  │   Database   │    │
                                │  └──────────────┘    │
                                └──────────────────────┘
                                          │
                                          ▼
                                ┌──────────────────────┐
                                │  Web Dashboards      │
                                │  - Teacher Dashboard  │
                                │  - Admin Overview     │
                                │  - Device Monitor     │
                                └──────────────────────┘
```

### Data Flow

1. **Student presses button** on ESP32
2. ESP32 sends HTTP request to `esp_communication_handler.php`
3. PHP triggers `language_monitor.py` (Python subprocess)
4. Python records audio → Whisper transcribes → Lingua detects language
5. Result compared to teacher's configured target language
6. Correct/Wrong result sent back to ESP32 → plays audio feedback
7. Interaction logged to MySQL database
8. Teachers/Admins view analytics on web dashboard

---

## 👤 User Roles

### Teacher
- Set target language for their class
- View student interaction logs
- Export weekly usage reports
- Manage profile & notifications

### Admin
- Manage all languages in the system
- Approve/reject language requests from teachers
- Monitor ESP32 device status (online/offline)
- Manage user accounts
- View system-wide analytics & overview

---

## 📊 Database

**Database name:** `language_monitor`

Key tables: `users`, `student_interaction_logs`, `language`, `language_usage`, `language_request`, `teacher_daily_languages`, `teacher_settings`, `teacher_notifications`, `password_resets`, `password_reset_logs`, `device_status`

---

## ⚠️ Security Notes

- Passwords are currently stored in **plaintext** (marked for migration to `password_hash()`)
- API key authentication between ESP32 and server (`ESP32_SECRET_KEY`)
- Session-based authentication for web users
- CORS headers enabled for ESP32 communication
