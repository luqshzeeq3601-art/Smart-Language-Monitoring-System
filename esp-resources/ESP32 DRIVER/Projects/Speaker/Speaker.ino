#define MIC_PIN 34       // Analog input
#define SPEAKER_PIN 25   // Output for tone
#define THRESHOLD 600    // Adjust based on testing

void setup() {
  Serial.begin(115200);
  pinMode(SPEAKER_PIN, OUTPUT);
}

void loop() {s
  int micValue = analogRead(MIC_PIN);
  Serial.println(micValue);

  if (micValue > THRESHOLD) {
    playTone();
  }

  delay(50); // Prevent rapid re-triggering
}

void playTone() {
  ledcAttachPin(SPEAKER_PIN, 0);         // Channel 0
  ledcSetup(0, 1000, 8);                 // 1kHz tone, 8-bit resolution
  ledcWrite(0, 127);                     // 50% duty cycle
  delay(200);
  ledcWrite(0, 0);                       // Turn off tone
}
