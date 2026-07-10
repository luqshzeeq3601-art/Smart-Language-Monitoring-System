# Smart Language Monitoring System — Screenshots

> Visual documentation and screenshots of the Smart Language Monitoring System interface and hardware.

---

## 📁 Categories

### 🔐 Login & Registration
`LOGIN ADMIN&TEACHER ,REGISTER TEACHER/`
- Login page (Teacher & Admin)
- Teacher registration form

### 👨‍💼 Admin Panel
`ADMIN/`
- Admin sidebar navigation
- Manage users page
- Manage languages (add/edit/delete)
- Approve language requests
- Monitor devices (ESP32 online/offline)
- Overview dashboard (charts & analytics)
- Notification panel
- Password reset request logs

### 👩‍🏫 Teacher Panel
`TEACHER/`
- Teacher dashboard
- Forgot password flow

### 🎓 Student Interaction (ESP32 + Python)
`STUDENT ORDER FOOD/`
- Program start screen
- Listening for audio input
- Correct answers (English, Malay, Chinese, Tamil)
- Wrong answers (English, Malay, Chinese, Tamil)
- No input / no audio detected

### 🗄️ Database & Web Server
`DATABASE & WEBSERVER/`
- XAMPP control panel
- MySQL table structures:
  - `users`, `language`, `language_request`, `language_usage`
  - `student_interaction_logs`, `teacher_daily_languages`
  - `teacher_settings`, `teacher_notifications`
  - `password_resets`, `password_reset_logs`
  - `device_status`
- Table relationship diagram (SQL)

### 🔌 ESP32 Hardware
`ESP32/`
- Hardware photos (14 images)
- Wiring and connections
- ESP32 + OLED + speaker setup
- Arduino IDE code screenshot
- Test video (`test.mp4`)
