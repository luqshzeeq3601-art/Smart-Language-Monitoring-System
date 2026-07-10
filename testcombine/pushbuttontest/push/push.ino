// =========================================================
// ESP32 Simple Push Button Test (GPIO 15)
// =========================================================
// This code reads the state of a push button connected to GPIO 15
// and displays its state on the Serial Monitor.
//
// Hardware Setup:
// 1. Push Button:
//    - Connect one leg of the tactile push button to ESP32 GPIO 15.
//    - Connect the other leg of the push button to ESP32 GND.
//    (This setup uses the ESP32's internal pull-up resistor for simplicity).
//
// Uploading and Serial Monitor:
// - Board: NodeMCU-32S (or ESP32 Dev Module)
// - Port: Select the correct COM/USB port.
// - If "Wrong boot mode detected" error occurs during upload:
//   - Follow the manual boot mode sequence (hold BOOT, brief EN, release BOOT).
// - Serial Monitor: Set baud rate to 115200.
// =========================================================

// Define the GPIO pin connected to the push button
const int buttonPin = 15; // Changed to GPIO 15 as requested

// Variable to store the current state of the button
int buttonState = 0;

void setup() {
  // Initialize serial communication at 115200 bits per second
  Serial.begin(115200);

  // Configure the button pin as an input with an internal pull-up resistor.
  // This means the pin will be HIGH when the button is not pressed,
  // and LOW when the button is pressed (connecting it to GND).
  pinMode(buttonPin, INPUT_PULLUP);

  Serial.println("==========================================");
  Serial.println("   ESP32 Simple Push Button Test");
  Serial.println("==========================================");
  Serial.println("Connect button to GPIO " + String(buttonPin) + " and GND.");
  Serial.println("------------------------------------------");
  Serial.println("Watch the Serial Monitor for button state changes.");
  Serial.println("------------------------------------------");
}

void loop() {
  // Read the current state of the push button
  // Returns LOW if the button is pressed (pulled to GND)
  // Returns HIGH if the button is not pressed (pulled up by internal resistor)
  buttonState = digitalRead(buttonPin);

  // Check if the button is pressed
  if (buttonState == LOW) { // Button is pressed
    Serial.println("Button is Pressed!");
    // Add a small delay for debouncing to prevent multiple "pressed" readings
    delay(50);
    // Optional: If you want the "Button is Pressed!" message to appear only ONCE
    // per press (not continuously while held), uncomment the following while loop:
    /*
    while (digitalRead(buttonPin) == LOW) {
      delay(10);
    }
    */
  } else { // Button is not pressed
    Serial.println("Button is NOT Pressed.");
  }

  // A small delay in the loop to prevent the Serial Monitor from being flooded
  // and to give the ESP32 a moment between readings.
  delay(100); // Adjust this delay as needed (e.g., 50ms, 200ms)
}