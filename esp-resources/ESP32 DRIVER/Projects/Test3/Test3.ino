#include <Arduino.h>
#include <driver/ledc.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <driver/i2s.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
#define BUZZER_PIN     18
#define BUTTON_PIN     15

#define I2C_SDA        21
#define I2C_SCL        22
#define BUZZER_CHANNEL 0

#define I2S_WS         25 // L/R clock
#define I2S_SD         34 // Serial data in (from mic)
#define I2S_SCK        26 // Bit clock

TwoWire I2CBus = TwoWire(0);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2CBus, OLED_RESET);

const char* expectedPhrases[] = {
  "i want fried rice",
  "i want nasi lemak",
  "i want roti canai"
};

void setupOLED() {
  I2CBus.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, &I2CBus)) {
    Serial.println("❌ OLED init failed");
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("ESP32 Canteen AI Ready!");
  display.display();
}

void setupBuzzer() {
  pinMode(BUZZER_PIN, OUTPUT);
 
}

void setupI2SMic() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 512,
    .use_apll = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

void playFeedbackTone(bool correct) {
  int freq = correct ? 1000 : 300;
  ledcWriteTone(BUZZER_CHANNEL, freq);
  delay(300);
  ledcWriteTone(BUZZER_CHANNEL, 0);
}

bool checkPhrase(String input) {
  input.toLowerCase();
  for (int i = 0; i < sizeof(expectedPhrases) / sizeof(expectedPhrases[0]); i++) {
    if (input.indexOf(expectedPhrases[i]) >= 0) {
      return true;
    }
  }
  return false;
}

String recordAndSimulateSpeech() {
  const int bufferSize = 1024;
  int16_t buffer[bufferSize];
  size_t bytesRead;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("🎤 Listening 2s...");
  display.display();

  unsigned long start = millis();
  while (millis() - start < 2000) {
    i2s_read(I2S_NUM_0, (void*)buffer, sizeof(buffer), &bytesRead, portMAX_DELAY);
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("📝 Say simulated...");
  display.display();

  // For now, we simulate by typing
  Serial.println("📥 Type the phrase you said (simulate recognition):");
  while (!Serial.available());
  String input = Serial.readStringUntil('\n');
  input.trim();
  return input;
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  setupOLED();
  setupBuzzer();
  setupI2SMic();
}

void loop() {
  display.setCursor(0, 20);
  display.println("Press button to start...");
  display.display();

  if (digitalRead(BUTTON_PIN) == LOW) {
    String result = recordAndSimulateSpeech();

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("You said:");
    display.println(result);
    display.display();

    bool correct = checkPhrase(result);

    display.setCursor(0, 40);
    if (correct) {
      display.println("✅ Correct phrase");
      playFeedbackTone(true);
    } else {
      display.println("❌ Try again");
      playFeedbackTone(false);
    }
    display.display();

    delay(2500);
    display.clearDisplay();
  }
}
