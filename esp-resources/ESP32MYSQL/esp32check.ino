#include <WiFi.h>        // For ESP32 Wi-Fi
#include <HTTPClient.h>  // For ESP32 HTTP client

// --------------- WiFi Settings ---------------
const char* ssid = "ZeeqRyz";         // CHANGE THIS to your Wi-Fi network name
const char* password = "qwerty123456"; // CHANGE THIS to your Wi-Fi password

// --------------- Server Details ---------------
// Replace with your server's domain or IP address and path to the script
// Example: "http://yourdomain.com/api/update_device_status.php"
// If your server uses HTTPS (recommended), you'll need to handle root CA certificates.
// For simplicity, this example uses HTTP.
const char* serverUrl = "http://172.20.10.3/update_device_status.php"; // <--- VERIFY THIS IP AND PATH

// --------------- Device & API Key ---------------
const char* deviceID = "ESP32_LangMon_002";   // Unique ID for this device. CHANGE if you have multiple devices.
// !!! IMPORTANT: THIS API KEY MUST MATCH THE $API_KEY IN YOUR PHP SCRIPT (update_device_status.php) !!!
const char* apiKey = "ESP32_SECRET_KEY"; // Ensure this EXACTLY MATCHES the API_KEY in your PHP script

// --------------- Update Interval ---------------
unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 60000; // Send status every 60 seconds (60000 milliseconds)

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\nESP32 NodeMCU - Device Status Updater");

  connectToWiFi();
}

void loop() {
  unsigned long currentTime = millis();
  if (WiFi.status() == WL_CONNECTED && (currentTime - lastUpdateTime >= updateInterval)) {
    sendStatus("online"); // Example: send "online" status
    lastUpdateTime = currentTime;
  } else if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi disconnected. Attempting to reconnect...");
    connectToWiFi(); // Attempt to reconnect if Wi-Fi is lost
    delay(5000);     // Wait before retrying
  }
  // You could add logic here to detect an "error" state and send that status
  // e.g., if a sensor reading is out of bounds.
}

void connectToWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  int attemptCount = 0;
  while (WiFi.status() != WL_CONNECTED && attemptCount < 30) { // Try for ~15 seconds
    delay(500);
    Serial.print(".");
    attemptCount++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi. Please check credentials and network.");
  }
}

void sendStatus(String currentStatus) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http; // Create an HTTPClient object

    Serial.print("[HTTP] Attempting to connect to server: ");
    Serial.println(serverUrl);

    // Start HTTP connection
    if (http.begin(serverUrl)) { // Check if begin was successful
      Serial.println("[HTTP] Connection begin successful. Setting header and sending POST data.");
      http.addHeader("Content-Type", "application/x-www-form-urlencoded"); // Set content type for POST

      // Prepare POST data - Changed "apikey" to "api_key" to match PHP script
      String postData = "api_key=" + String(apiKey) + // <<< CRUCIAL: Changed "apikey" to "api_key"
                        "&device_id=" + String(deviceID) +
                        "&status=" + currentStatus;

      Serial.print("[HTTP] POST data: " + postData + "\n");
      int httpCode = http.POST(postData); // Send POST request

      if (httpCode > 0) {
        Serial.printf("[HTTP] POST... code: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) { // HTTP_CODE_CREATED is 201
          String payload = http.getString();
          Serial.println("Received payload:\n<<");
          Serial.println(payload);
          Serial.println(">>");
        } else {
          // Print server response even if it's an HTTP error code (e.g., 403, 405, 500)
          Serial.printf("[HTTP] Server responded with an HTTP error code: %d. Payload:\n%s\n", httpCode, http.getString().c_str());
        }
      } else {
        Serial.printf("[HTTP] POST... failed, error: %s (Code: %d)\n", http.errorToString(httpCode).c_str(), httpCode);
      }
      http.end(); // Free resources
    } else {
      Serial.printf("[HTTP] Unable to begin connection to server: %s\n", serverUrl); // Added serverUrl to error message
    }
  } else {
    Serial.println("WiFi Disconnected. Cannot send status.");
  }
}