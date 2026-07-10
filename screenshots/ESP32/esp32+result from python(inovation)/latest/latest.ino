#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h> // For outgoing HTTP requests
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <driver/i2s.h>
#include <pgmspace.h>

#include "good_job.h"     // Contains good_job_raw, good_job_raw_len
#include "wrong_answer.h" // Contains wrong_answer_raw, wrong_answer_raw_len

// ====== WiFi Configuration ======
const char* ssid = "ZeeqRyz";       // REPLACE WITH YOUR WIFI NAME
const char* password = "ayamgepuk";  // REPLACE WITH YOUR WIFI PASSWORD

// ====== OLED Setup ======
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ====== I2S Speaker Pin Config ======
#define I2S_SPK_DATA    33
#define I2S_SPK_BCLK    27
#define I2S_SPK_LRCLK   26

// ====== Audio Config ======
#define SAMPLE_RATE     16000

// ====== Web Server (Incoming Commands) ======
WebServer server(80);

// ====== Server Details (Outgoing Requests from ESP32 to PC) ======
const char* phpHandlerUrl = "http://172.20.10.2/esp_communication_handler.php"; // IP of your PC running XAMPP
const char* deviceID = "ESP32_LangMon_002";      // Unique ID for this device.
const char* apiKey = "ESP32_SECRET_KEY"; // !!! IMPORTANT: MUST MATCH API_KEY IN YOUR PHP SCRIPT !!!

// ====== Update Interval (Outgoing Status) ======
unsigned long lastStatusUpdateTime = 0;
const unsigned long statusUpdateInterval = 60000; // Send status every 60 seconds

// ====== OLED Display Function ======
void showOnOLED(String line1, String line2 = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println(line1);
  if (line2 != "") {
    display.setCursor(0, 30);
    display.println(line2);
  }
  display.display();
}

// ====== Function to display detailed results on the OLED ======
void showDetailedResultOnOLED(String result, String text, String detected, String expected) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.setTextWrap(true); // Enable text wrapping for long lines

  if (result.equalsIgnoreCase("Correct")) {
    display.println("Result: Correct!");
    display.println("Language: " + detected);
    display.println("");
    display.println("Heard:");
    display.print(text);
  } else if (result.equalsIgnoreCase("Wrong")) {
    display.println("Result: WRONG LANG");
    display.println("Heard: " + detected);
    display.println("Need: " + expected);
    display.println("");
    display.print("Text:" + text);
  } else {
    // Handle other errors like NoSpeech, MicError etc.
    showOnOLED("Error", result);
  }
  display.display();
}


// ====== Audio Playback Function ======
void playAudio(const unsigned char* rawData, size_t len) {
  size_t bytes_written;
  size_t samples = len / 2;

  i2s_zero_dma_buffer(I2S_NUM_0);

  for (size_t i = 0; i < samples; i++) {
    int16_t sample = (int16_t)(pgm_read_byte_near(rawData + 2 * i) |
                               (pgm_read_byte_near(rawData + 2 * i + 1) << 8));
    i2s_write(I2S_NUM_0, (const char*)&sample, sizeof(sample), &bytes_written, portMAX_DELAY);
  }
  Serial.println("[AUDIO] Playback finished.");
}

// ====== I2S Setup ======
void setupI2SSpeaker() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SPK_BCLK,
    .ws_io_num = I2S_SPK_LRCLK,
    .data_out_num = I2S_SPK_DATA,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_zero_dma_buffer(I2S_NUM_0);
  Serial.println("[I2S] Speaker setup complete.");
}

// ====== WiFi Connection Function ======
void connectToWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  showOnOLED("Connecting to", ssid);

  WiFi.begin(ssid, password);
  int attemptCount = 0;
  while (WiFi.status() != WL_CONNECTED && attemptCount < 30) {
    delay(500);
    Serial.print(".");
    showOnOLED("Connecting.", String(attemptCount));
    attemptCount++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    IPAddress ip = WiFi.localIP();
    Serial.print("IP address: ");
    Serial.println(ip);
    showOnOLED("Connected!", ip.toString());
  } else {
    Serial.println("\n❌ Failed to connect to WiFi. Retrying...");
    showOnOLED("WiFi Failed!", "Retrying...");
  }
}

// ====== Send Status Function (Outgoing HTTP POST to PC Server) ======
void sendStatus(String currentStatus) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(phpHandlerUrl) + "?action=update_status";
    
    if (http.begin(url)) {
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      String postData = "api_key=" + String(apiKey) +
                        "&device_id=" + String(deviceID) +
                        "&status=" + currentStatus;

      int httpCode = http.POST(postData);
      if (httpCode > 0) {
        Serial.printf("[HTTP] POST... code: %d\n", httpCode);
      } else {
        Serial.printf("[HTTP] POST failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    } else {
      Serial.printf("[HTTP] Unable to begin connection to server: %s\n", url.c_str());
    }
  } else {
    Serial.println("WiFi Disconnected. Cannot send status.");
  }
}

// ====== HTTP Handler (Incoming Commands from Python Script to ESP32) ======
void handleRoot() {
  if (server.hasArg("result")) {
    // Read all the arguments from the URL
    String result = server.arg("result");
    String text = server.hasArg("text") ? server.arg("text") : "N/A";
    String detected = server.hasArg("detected") ? server.arg("detected") : "N/A";
    String expected = server.hasArg("expected") ? server.arg("expected") : "N/A";

    // Print to Serial Monitor for debugging
    Serial.println("📥 Received detailed result from Python Script.");
    Serial.println("Result: " + result);
    Serial.println("Text: " + text);
    Serial.println("Detected: " + detected);
    Serial.println("Expected: " + expected);

    // Call the new display function to show details on the OLED
    showDetailedResultOnOLED(result, text, detected, expected);

    // Play the corresponding audio feedback
    if (result.equalsIgnoreCase("Correct")) {
      playAudio(good_job_raw, good_job_raw_len);
    } else if (result.equalsIgnoreCase("Wrong")) {
      playAudio(wrong_answer_raw, wrong_answer_raw_len);
    }
    
    // Send a confirmation response back to the Python script
    server.send(200, "text/plain", "Detailed result received: " + result);

  } else {
     server.send(200, "text/plain", "No 'result' arg provided.");
     showOnOLED("Waiting for", "Trigger...");
  }
}


// ====== Setup Function ======
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\nESP32 Language Monitor - Starting Up");

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED init failed!");
    while (true) { delay(100); }
  }
  showOnOLED("Starting Up...", "Please Wait");

  // Connect to WiFi network
  connectToWiFi();

  // Setup I2S speaker
  setupI2SSpeaker();

  // Start ESP32's internal web server to listen for incoming commands from Python
  server.on("/", handleRoot);
  server.begin();
  Serial.println("🚀 HTTP Server (for incoming commands) Started on port 80.");

  // Send initial "online" status to the PC server if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    sendStatus("online");
    lastStatusUpdateTime = millis();
  }
}

// ====== Loop Function ======
void loop() {
  server.handleClient();

  unsigned long currentTime = millis();
  if (WiFi.status() == WL_CONNECTED) {
    if (currentTime - lastStatusUpdateTime >= statusUpdateInterval) {
      sendStatus("online");
      lastStatusUpdateTime = currentTime;
    }
  } else {
    Serial.println("Wi-Fi disconnected. Attempting to reconnect...");
    showOnOLED("WiFi Lost!", "Reconnecting...");
    connectToWiFi();
    delay(1000);
  }
}