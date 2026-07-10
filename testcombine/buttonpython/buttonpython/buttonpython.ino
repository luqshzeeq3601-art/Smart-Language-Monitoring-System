#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h> // For outgoing HTTP requests
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <driver/i2s.h>
#include <pgmspace.h>

// Include your audio data header files.
// These need to be present in your sketch folder.
// IMPORTANT: If you generated good_job.h and wrong_answer.h with actual audio data,
// you MUST DELETE the dummy definitions below this comment block.
#include "good_job.h"     // Contains good_job_raw, good_job_raw_len
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
#define BUFFER_SIZE     (SAMPLE_RATE * 2) // Not directly used by playAudio as written
int16_t audioBuffer[BUFFER_SIZE]; 

// ====== Web Server (Incoming Commands) ======
// This is the ESP32's own internal web server, listening for results from your Python script on port 80.
WebServer server(80);

// ====== Server Details (Outgoing Requests from ESP32 to PC) ======
// IMPORTANT: These URLs assume your PC's Apache DocumentRoot is
// C:\Users\luqma\OneDrive\Desktop\IDP\iot_school\main\
// If your PHP files are in a subfolder of this DocumentRoot, you need to add that subfolder name here.
const char* statusServerUrl = "http://172.20.10.2/update_device_status.php";     // URL for sending status to PC
const char* triggerSpeechServerUrl = "http://172.20.10.2/decoy_trigger.php"; // Point to the new decoy script

const char* deviceID = "ESP32_LangMon_002";      // Unique ID for this device.
const char* apiKey = "ESP32_SECRET_KEY"; // !!! IMPORTANT: MUST MATCH API_KEY IN YOUR PHP SCRIPTS !!!

// ====== Update Interval (Outgoing Status) ======
unsigned long lastStatusUpdateTime = 0;
const unsigned long statusUpdateInterval = 60000; // Send status every 60 seconds

// ====== Push Button Configuration ======
#define BUTTON_PIN 4 // Choose an available GPIO pin (e.g., 15, 4, 2, 0). Connect to GND.
int lastButtonState = HIGH; // The previous state of the button (HIGH if using INPUT_PULLUP)
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 1000; // Milliseconds (Adjust if experiencing button bounce)

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
// Plays raw 16-bit audio data through the I2S speaker.
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

// ====== WiFi Connection Function (Consolidated) ======
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
// Sends the ESP32's current status (e.g., "online") to a PHP script on the PC.
void sendStatus(String currentStatus) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    Serial.print("[HTTP] Sending status to: ");
    Serial.println(statusServerUrl);

    if (http.begin(statusServerUrl)) {
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      String postData = "api_key=" + String(apiKey) + 
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

// ====== Trigger Speech Function (Outgoing HTTP GET from ESP32 to PC's PHP script) ======
// This function is called by the push button on the ESP32 to trigger the Python script via PHP.
void triggerSpeechOnServer() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    Serial.print("[HTTP] Triggering speech recognition on server: ");
    Serial.println(triggerSpeechServerUrl);

    String urlWithParams = String(triggerSpeechServerUrl) + "?api_key=" + String(apiKey) + "&device_id=" + String(deviceID);
if (http.begin(urlWithParams)) {

      // Send a GET request with device ID and API key for authentication/identification
      //String urlWithParams = String(triggerSpeechServerUrl) + "?api_key=" + String(apiKey) + "&device_id=" + String(deviceID);
      Serial.println("FULL GET URL: " + urlWithParams);

      Serial.print("[HTTP] GET request URL: " + urlWithParams + "\n");
      int httpCode = http.GET(); // Send GET request

      if (httpCode > 0) {
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          Serial.println("Server response to trigger:\n<<");
          Serial.println(payload);
          Serial.println(">>");
          showOnOLED("Triggered!", "Server OK");
        } else {
          Serial.printf("[HTTP] Server responded with error to trigger: %d\n", httpCode);
          showOnOLED("Trigger Err", String(httpCode));
        }
      } else {
        Serial.printf("[HTTP] Trigger GET failed, error: %s (Code: %d)\n", http.errorToString(httpCode).c_str(), httpCode);
        showOnOLED("Trigger Fail!", "No Response");
      }
      http.end();
    } else {
      Serial.printf("[HTTP] Unable to begin connection to trigger server: %s\n", triggerSpeechServerUrl);
      showOnOLED("Trigger Init Err", "Check URL");
    }
  } else {
    Serial.println("WiFi Disconnected. Cannot trigger speech.");
    showOnOLED("No WiFi", "No Trigger");
  }
}


// ====== HTTP Handler (Incoming Commands from Python Script to ESP32) ======
// This function is called when your Python script sends an HTTP GET request to this ESP32.
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
    } else { // Handle general errors or unknown commands from Python
      // Check if it's one of the error codes Python might send
      if (result.equalsIgnoreCase("NoSpeech") || result.equalsIgnoreCase("MicError") ||
          result.equalsIgnoreCase("Unknown") || result.equalsIgnoreCase("API_Error") ||
          result.equalsIgnoreCase("SystemError") || result.equalsIgnoreCase("Timeout")) { 
          showOnOLED("Speech Error", result);
          Serial.println("[ESP32] Speech processing error reported by Python: " + result); 
      } else {
          showOnOLED("Unknown Cmd", result); 
          Serial.println("[ESP32] Unknown command received from server: " + result); 
      }
    }

    server.send(200, "text/plain", "Result received and processed: " + result);
  } else {
    server.send(200, "text/plain", "No 'result' arg provided.");
    showOnOLED("No Cmd", "Waiting...");
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

  // Button Pin Init (For physical button)
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Use INPUT_PULLUP if connecting button to GND
  Serial.println("[SETUP] Button pin " + String(BUTTON_PIN) + " initialized as INPUT_PULLUP.");

  // Start ESP32's internal web server to listen for incoming commands from Python
  server.on("/", handleRoot); 
  server.begin();
  Serial.println("🚀 HTTP Server (for incoming commands) Started on port 80.");

  // Send initial "online" status to the PC server if WiFi is connected
  if(WiFi.status() == WL_CONNECTED) {
    sendStatus("online");
    lastStatusUpdateTime = millis(); 
  }
}

void loop() {
  server.handleClient(); 
  
  // Check for outgoing status update
  unsigned long currentTime = millis();
  if (WiFi.status() == WL_CONNECTED) { 
    if (currentTime - lastStatusUpdateTime >= statusUpdateInterval) {
      Serial.println("[STATUS] Sending periodic online status."); 
      sendStatus("online"); 
      lastStatusUpdateTime = currentTime;
    }

    // --- Push Button State Check ---
    int reading = digitalRead(BUTTON_PIN);
    //Serial.print("Raw reading: "); // <-- IMPORTANT: Make sure this is uncommented!
    //Serial.println(reading);      // <-- IMPORTANT: Make sure this is uncommented!
    
    if (reading != lastButtonState) {
      lastDebounceTime = millis(); 
      //Serial.println("[BUTTON] State changed detected. Resetting debounce timer."); 
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (reading != lastButtonState) { 
        lastButtonState = reading; 

        if (lastButtonState == LOW) { 
          Serial.println("---------- Button Pressed! ----------"); 
          Serial.println("[BUTTON] Attempting to trigger speech recognition on server...");
          showOnOLED("Button Pressed!", "Speaking...");
          triggerSpeechOnServer(); 
          Serial.println("[BUTTON] Trigger speech function called. "); 
        } else { 
          Serial.println("---------- Button Released! ----------"); 
          showOnOLED("Button Released!", ""); 
        }
      }
    }
    // --- End Push Button State Check ---

  } else {
    Serial.println("Wi-Fi disconnected. Attempting to reconnect...");
    showOnOLED("WiFi Lost!", "Reconnecting...");
    connectToWiFi(); 
    delay(1000); 
  }
}