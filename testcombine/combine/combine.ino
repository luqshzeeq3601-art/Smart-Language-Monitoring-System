#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h> // For outgoing HTTP requests
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <driver/i2s.h>
#include <pgmspace.h>

// Include your audio data header files
// Make sure these files are in the same sketch folder
#include "good_job.h"    // Contains good_job_raw, good_job_raw_len
#include "wrong_answer.h" // Contains wrong_answer_raw, wrong_answer_raw_len

// ====== WiFi Configuration ======
const char* ssid = "ZeeqRyz";         // REPLACE WITH YOUR WIFI NAME
const char* password = "qwerty123456"; // REPLACE WITH YOUR WIFI PASSWORD

// ====== OLED Setup ======
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ====== I2S Speaker Pin Config ======
#define I2S_SPK_DATA    33
#define I2S_SPK_BCLK    27
#define I2S_SPK_LRCLK   26

// ====== Audio Config ======
#define SAMPLE_RATE     16000
#define BUFFER_SIZE     (SAMPLE_RATE * 2) // Not used for playback, but good to keep
int16_t audioBuffer[BUFFER_SIZE]; // Not used for playback, but good to keep

// ====== Web Server (Incoming Commands) ======
WebServer server(80);

// ====== Server Details (Outgoing Status) ======
const char* statusServerUrl = "http://172.20.10.2/update_device_status.php"; // <--- VERIFY THIS URL
const char* deviceID = "ESP32_LangMon_002";     // Unique ID for this device. CHANGE if multiple.
const char* apiKey = "ESP32_SECRET_KEY"; // !!! IMPORTANT: MUST MATCH $API_KEY IN YOUR PHP SCRIPT !!!

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

// ====== Audio Playback Function ======
void playAudio(const unsigned char* rawData, size_t len) {
  size_t bytes_written;
  size_t samples = len / 2; // Each sample is 2 bytes (int16_t)

  // Ensure I2S is ready before writing
  i2s_zero_dma_buffer(I2S_NUM_0); // Clear any old data
  
  for (size_t i = 0; i < samples; i++) {
    // Read 16-bit sample from PROGMEM
    int16_t sample = (int16_t)(pgm_read_byte_near(rawData + 2 * i) |
                               (pgm_read_byte_near(rawData + 2 * i + 1) << 8));
    
    // Write the sample. For 16-bit, I2S_BITS_PER_SAMPLE_16BIT expects 32-bit (short is padded to long)
    // or you can cast to 32-bit if it expects that. Simpler to write the 16-bit sample directly.
    i2s_write(I2S_NUM_0, (const char*)&sample, sizeof(sample), &bytes_written, portMAX_DELAY);
  }
}

// ====== I2S Setup ======
void setupI2SSpeaker() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT, // Assuming mono audio from good_job.h/wrong_answer.h
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
  i2s_zero_dma_buffer(I2S_NUM_0); // Ensure buffer is clear on startup
}

// ====== WiFi Connection Function (Consolidated) ======
void connectToWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return; // Already connected
  }
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  showOnOLED("Connecting to", ssid); // Update OLED

  WiFi.begin(ssid, password);
  int attemptCount = 0;
  while (WiFi.status() != WL_CONNECTED && attemptCount < 30) { // Try for ~15 seconds
    delay(500);
    Serial.print(".");
    showOnOLED("Connecting.", String(attemptCount)); // Show progress on OLED
    attemptCount++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    IPAddress ip = WiFi.localIP();
    Serial.print("IP address: ");
    Serial.println(ip);
    showOnOLED("Connected!", ip.toString()); // Show IP on OLED
  } else {
    Serial.println("\n❌ Failed to connect to WiFi. Retrying...");
    showOnOLED("WiFi Failed!", "Retrying...");
  }
}

// ====== Send Status Function (Outgoing HTTP POST) ======
void sendStatus(String currentStatus) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    Serial.print("[HTTP] Sending status to: ");
    Serial.println(statusServerUrl);

    if (http.begin(statusServerUrl)) {
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      String postData = "api_key=" + String(apiKey) + // Matches PHP expected param name
                        "&device_id=" + String(deviceID) +
                        "&status=" + currentStatus;

      Serial.print("[HTTP] POST data: " + postData + "\n");
      int httpCode = http.POST(postData);

      if (httpCode > 0) {
        Serial.printf("[HTTP] POST... code: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
          String payload = http.getString();
          Serial.println("Received payload:\n<<");
          Serial.println(payload);
          Serial.println(">>");
          showOnOLED("Status Sent:", currentStatus);
        } else {
          Serial.printf("[HTTP] Server responded with error: %d. Payload:\n%s\n", httpCode, http.getString().c_str());
          showOnOLED("Status Error", String(httpCode));
        }
      } else {
        Serial.printf("[HTTP] POST failed, error: %s (Code: %d)\n", http.errorToString(httpCode).c_str(), httpCode);
        showOnOLED("POST Failed!", String(http.errorToString(httpCode)));
      }
      http.end();
    } else {
      Serial.printf("[HTTP] Unable to begin connection to server: %s\n", statusServerUrl);
      showOnOLED("HTTP Init Err", "Check URL");
    }
  } else {
    Serial.println("WiFi Disconnected. Cannot send status.");
    showOnOLED("No WiFi", "Status Not Sent");
  }
}

// ====== HTTP Handler (Incoming Commands from Web Server) ======
void handleRoot() {
  if (server.hasArg("result")) {
    String result = server.arg("result");
    Serial.println("📥 Received result from Web Server: " + result);
    showOnOLED("Result:", result);

    if (result.equalsIgnoreCase("Correct")) {
      playAudio(good_job_raw, good_job_raw_len);
      showOnOLED("Good Job!", "Correct!");
    } else if (result.equalsIgnoreCase("Wrong")) {
      playAudio(wrong_answer_raw, wrong_answer_raw_len);
      showOnOLED("Try Again!", "Wrong Answer!");
    } else {
      showOnOLED("Unknown ", result); // Display received unknown command
    }

    server.send(200, "text/plain", "Result received and processed: " + result);
  } else {
    server.send(200, "text/plain", "No 'result' arg provided.");
    showOnOLED("No Cmd", "Waiting...");
  }
}

// ====== Setup ======
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\nESP32 Combined Device Monitor");

  // OLED Init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED init failed!");
    while (true) { delay(100); } // Halt if OLED fails
  }
  showOnOLED("Starting Up...", "Please Wait");

  // WiFi Init
  connectToWiFi(); // Call the consolidated WiFi connection function

  // Audio I2S Init
  setupI2SSpeaker();

  // Web Server Init (for incoming commands)
  server.on("/", handleRoot);
  server.begin();
  Serial.println("🚀 HTTP Server (for incoming commands) Started");

  // Initial status send (after connected)
  if(WiFi.status() == WL_CONNECTED) {
    sendStatus("online");
    lastStatusUpdateTime = millis(); // Initialize update timer
  }
}

// ====== Loop ======
void loop() {
  server.handleClient(); // Handle incoming HTTP requests
  
  // Check for outgoing status update
  unsigned long currentTime = millis();
  if (WiFi.status() == WL_CONNECTED) {
    if (currentTime - lastStatusUpdateTime >= statusUpdateInterval) {
      sendStatus("online"); // Send "online" status periodically
      lastStatusUpdateTime = currentTime;
    }
  } else {
    // If WiFi is disconnected, attempt to reconnect
    Serial.println("Wi-Fi disconnected. Attempting to reconnect...");
    connectToWiFi(); 
    delay(1000); // Small delay to avoid hammering WiFi.begin()
  }
}