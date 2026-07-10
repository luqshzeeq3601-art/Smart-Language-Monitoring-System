#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <driver/i2s.h>

// ----------- WiFi ----------
const char* ssid     = "ESP32-Network";
const char* password = "Esp32-Password";
WiFiServer server(80);
String header;

// ----------- OLED ----------
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// ----------- Pins ----------
#define BUTTON_PIN      4
#define I2S_MIC_DATA    32
#define I2S_MIC_BCLK    14
#define I2S_MIC_WS      15
#define I2S_SPK_DATA    33
#define I2S_SPK_BCLK    27
#define I2S_SPK_LRCLK   26

// ----------- Audio ----------
#define SAMPLE_RATE     16000
#define BUFFER_SIZE     (SAMPLE_RATE * 2) // 1 second 16-bit mono
int16_t audioBuffer[BUFFER_SIZE];
bool isRecording = false;

// ----------- Timeouts ----------
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

// ----------- I2S ----------
void setupI2SMic() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_MIC_BCLK,
    .ws_io_num = I2S_MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_DATA
  };
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

void setupI2SSpeaker() {
  i2s_config_t i2s_config_out = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };
  i2s_pin_config_t pin_config_out = {
    .bck_io_num = I2S_SPK_BCLK,
    .ws_io_num = I2S_SPK_LRCLK,
    .data_out_num = I2S_SPK_DATA,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  i2s_driver_install(I2S_NUM_1, &i2s_config_out, 0, NULL);
  i2s_set_pin(I2S_NUM_1, &pin_config_out);
}

// ----------- Setup ----------
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // OLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // I2S
  setupI2SMic();
  setupI2SSpeaker();

  // WiFi AP
  WiFi.softAP(ssid, password);
  server.begin();

  displayStatus("Ready. IP:");
  displayStatus(WiFi.softAPIP().toString());
}

// ----------- Helper ----------
void displayStatus(String msg) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(msg);
  display.display();
}

// ----------- Main ----------
void loop() {
  // Push button logic
  if (digitalRead(BUTTON_PIN) == LOW) {
    displayStatus("Recording...");
    isRecording = true;
    size_t bytesRead;
    i2s_read(I2S_NUM_0, (void*)audioBuffer, sizeof(audioBuffer), &bytesRead, portMAX_DELAY);
    isRecording = false;
    displayStatus("Done Recording");
    delay(1000);
    
    displayStatus("Playing...");
    size_t bytesWritten;
    i2s_write(I2S_NUM_1, (const char*)audioBuffer, sizeof(audioBuffer), &bytesWritten, portMAX_DELAY);
    displayStatus("Done Playing");
    delay(1000);
  }

  // Web Server Logic
  WiFiClient client = server.available();
  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");
    String header = "";
    String currentLine = "";

    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {
        char c = client.read();
        header += c;
        if (c == '\n' && currentLine.length() == 0) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println("Connection: close\n");

          client.println("<!DOCTYPE html><html><head><title>ESP32 Voice</title></head><body>");
          client.println("<h2>ESP32 Voice Recorder</h2>");
          client.println("<p>Press hardware button to record and playback.</p>");
          client.println("</body></html>");
          break;
        }
        if (c == '\n') currentLine = "";
        else if (c != '\r') currentLine += c;
      }
    }
    client.stop();
    Serial.println("Client disconnected.");
  }
}
