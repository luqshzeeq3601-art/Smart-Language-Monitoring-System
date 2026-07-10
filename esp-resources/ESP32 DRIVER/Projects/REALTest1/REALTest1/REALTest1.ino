#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "driver/i2s.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// OLED I2C: SDA=21, SCL=22
#define BUTTON_PIN 4

// Speaker I2S (MAX98357A)
#define I2S_SPEAKER_BCLK 26
#define I2S_SPEAKER_WS   25
#define I2S_SPEAKER_DOUT 23

// Mic I2S (INMP441)
#define I2S_MIC_BCLK  27
#define I2S_MIC_WS    33
#define I2S_MIC_DIN   32

#define RECORD_TIME_MS 3000  // 3 seconds
#define SAMPLE_RATE    16000

const int buffer_size = SAMPLE_RATE * RECORD_TIME_MS / 1000;
int16_t *recording_buffer = new int16_t[buffer_size];

// OLED display helper
void showMessage(const char* msg) {
  display.clearDisplay();
  display.setCursor(0, 20);
  display.setTextSize(1);
  display.println(msg);
  display.display();
}

// I2S MIC (Receiver)
void setupMic() {
  i2s_config_t mic_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t mic_pins = {
    .bck_io_num = I2S_MIC_BCLK,
    .ws_io_num = I2S_MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_DIN
  };

  i2s_driver_install(I2S_NUM_0, &mic_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &mic_pins);
}

// I2S SPEAKER (Transmitter)
void setupSpeaker() {
  i2s_config_t speaker_config = {
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

  i2s_pin_config_t speaker_pins = {
    .bck_io_num = I2S_SPEAKER_BCLK,
    .ws_io_num = I2S_SPEAKER_WS,
    .data_out_num = I2S_SPEAKER_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_1, &speaker_config, 0, NULL);
  i2s_set_pin(I2S_NUM_1, &speaker_pins);
  i2s_zero_dma_buffer(I2S_NUM_1);
}

// Modified: Record from microphone and print values to Serial Monitor
void recordAudio() {
  size_t bytes_read;
  showMessage("🎙️ Recording...");
  
  for (int i = 0; i < buffer_size; i++) {
    i2s_read(I2S_NUM_0, &recording_buffer[i], sizeof(int16_t), &bytes_read, portMAX_DELAY);

    // Print every 100th sample to reduce Serial Monitor spam

      Serial.println(recording_buffer[i]);
    
  }

  showMessage("✅ Recording done");
}

// Play to speaker
void playAudio() {
  size_t bytes_written;
  showMessage("🔊 Playing...");
  
  for (int i = 0; i < buffer_size; i++) {
    int16_t sample = recording_buffer[i];
    i2s_write(I2S_NUM_1, &sample, sizeof(int16_t), &bytes_written, portMAX_DELAY);
  }

  showMessage("✅ Playback done");
}

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("Ready...");
  display.display();

  setupMic();
  setupSpeaker();
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(100); // debounce
    recordAudio();
    delay(500);
    playAudio();
    delay(1000);
  }
}
