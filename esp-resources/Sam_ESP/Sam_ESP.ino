#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <driver/i2s.h>
#include "good_job.h"
#include "wrong_answer.h"
#include <pgmspace.h>

// ====== WiFi Configuration ======
const char* ssid = "SamWiFi";         // Replace with your WiFi name
const char* password = "Iamgay2302";   // Replace with your WiFi password

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
#define BUFFER_SIZE     (SAMPLE_RATE * 2)
int16_t audioBuffer[BUFFER_SIZE];

// ====== Web Server ======
WebServer server(80);

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
  size_t samples = len / 2;

  for (size_t i = 0; i < samples; i++) {
    int16_t sample = (int16_t)(pgm_read_byte_near(rawData + 2 * i) |
                                (pgm_read_byte_near(rawData + 2 * i + 1) << 8));
    i2s_write(I2S_NUM_0, (const char*)&sample, sizeof(sample), &bytes_written, portMAX_DELAY);
  }
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
}

// ====== HTTP Handler ======
void handleRoot() {
  if (server.hasArg("result")) {
    String result = server.arg("result");
    Serial.println("📥 Received result from Python: " + result);
    showOnOLED("Result:", result);

    if (result.equalsIgnoreCase("Correct")) {
      playAudio(good_job_raw, good_job_raw_len);
    } else if (result.equalsIgnoreCase("Wrong")) {
      playAudio(wrong_answer_raw, wrong_answer_raw_len);
    } else {
      showOnOLED("Unknown Input");
    }

    server.send(200, "text/plain", "Result received: " + result);
  } else {
    server.send(200, "text/plain", "No result provided.");
  }
}

// ====== Setup ======
void setup() {
  Serial.begin(115200);
  delay(1000);

  // OLED Init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED init failed!");
    while (true);
  }
  showOnOLED("Connecting to", ssid);

  // WiFi Init
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi Connected");
  IPAddress ip = WiFi.localIP();
  Serial.print("🌐 ESP32 IP Address: ");
  Serial.println(ip);
  showOnOLED("Connected!", ip.toString());

  // Audio I2S Init
  setupI2SSpeaker();

  // Web Server Init
  server.on("/", handleRoot);
  server.begin();
  Serial.println("🚀 HTTP Server Started");
}

// ====== Loop ======
void loop() {
  server.handleClient();
}
