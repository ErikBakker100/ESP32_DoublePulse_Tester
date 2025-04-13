// Dual Pulse Generator for ESP
// Erik Bakker 2025
// Partly used from Teensy 4.0 Signal Generator, Electronics Workshop, Robin O'Reilly

#include <Arduino.h>
#include <ArduinoJson.h>
#include "esp_rom_sys.h"

// Timing Variables
//                _____________                   ____________
// Pulseinterval | PulseWidth1 | interPulseDelay | PulseWith2 | Pulseinterval
// ______________               _________________              _______________

uint32_t Intervals[4] {500, 70, 30, 50}; // Default values: PI = 500usec, PW1 = 70usec, IPD = 30usec, PW2 = 50usec.

// Lower limits for the parameters
const uint32_t minPulseInterval = 10; // Minimum 10ms
const uint32_t minInterPulseDelay = 1; // Minimum 1us
const uint32_t minPulseWidth = 1;  // Minimum 1us

// D5 D6 pins are defined on boards like WeMOS, You can redefine it to numeric values
#if defined(ESP32)
#pragma message "Using ESP32 pins!"
#define OUTPUT_PIN GPIO_NUM_18
#define CPU_FREQ 240
#elif defined(ESP8266)
#pragma message "Using ESP8266 pins!"
#define OUTPUT_PIN D5
#define CPU_FREQ 80
#elif defined(STM32)
#pragma message "Using STM32 pins!"
#define OUTPUT_PIN GPIO_NUM_34
#define CPU_FREQ 80
#else
#pragma message "Check configuration, not using any pin!!"
#define OUTPUT_PIN
#define CPU_FREQ 100
#endif

// Prepare to run OUTPUT_PIN control in a separate thread. This to allow for more stable output when using short pulses
TaskHandle_t OutputTask;

// put function declarations here:
void DoublePulseControl(void* parameters);

void setup() {
  setCpuFrequencyMhz(CPU_FREQ);
  Serial.begin(115200);
  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(OUTPUT_PIN, LOW); // Initial state low (inactive)
  delay(300); // Give serial port time to open
  Serial.println("**************Dual Pulse Generator**************");
  Serial.println("> Usage: Send JSON string, for e.g {\"pulseInterval\": 100, \"pulseWidth1\": 10, \"interPulseDelay\": 200, \"pulseWidth2\": 10}.");
  Serial.print("> Values are in microseconds (note: ESP32 adds 20usec per interval). Using output port: ");
  Serial.println(OUTPUT_PIN);
  Serial.println("> Default        _____________                   ____________");
  Serial.println("> pulseInterval | pulseWidth1 | interPulseDelay | pulseWith2 | pulseInterval");
  Serial.println("> ___ 500_______      70       _____ 30 ________      50      _____ 500 ____");

  // Run code to control the output onto core 0
  xTaskCreatePinnedToCore(
    DoublePulseControl,     /* Function to implement the task */
    "OuputTask",            /* Name of the task */
    10000,                  /* Stack size in words */
    NULL,                   /* Task input parameter */
    0,                      /* Priority of the task, 0 = lowest */
    &OutputTask,            /* Task handle. */
    0);                     /* Core where the task should run */
}

void loop() {
  if (Serial.available() > 0) {
    String jsonString = Serial.readStringUntil('\n');
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonString);
    
    if (error) {
      Serial.print(F(">ERR DeserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    } else {
      unsigned long tempPulseInterval = doc["pulseInterval"];
      unsigned long tempPulseWidth1 = doc["pulseWidth1"];
      unsigned long tempInterPulseDelay = doc["interPulseDelay"];
      unsigned long tempPulseWidth2 = doc["pulseWidth2"];

      // Check and enforce lower bounds
      bool outOfBounds = false;
      if (tempPulseInterval < minPulseInterval) {
        Serial.print(F(">ERR Out of Bounds: pulseInterval "));
        Serial.print(tempPulseInterval);
        Serial.print(F("us is less than the minimum "));
        Serial.print(minPulseInterval);
        Serial.println(F("us"));
        outOfBounds = true;
      } 
      if (tempInterPulseDelay < minInterPulseDelay) {
        Serial.print(F(">ERR Out of Bounds: interPulseDelay "));
        Serial.print(tempInterPulseDelay);
        Serial.print(F("us is less than the minimum "));
        Serial.print(minInterPulseDelay);
        Serial.println(F("us"));
        outOfBounds = true;
      }
      if (tempPulseWidth1 < minPulseWidth) {
        Serial.print(F(">ERR Out of Bounds: pulseWidth1 "));
        Serial.print(tempPulseWidth1);
        Serial.print(F("us is less than the minimum "));
        Serial.print(minPulseWidth);
        Serial.println(F("us"));
        outOfBounds = true;
      }
      if (tempPulseWidth2 < minPulseWidth) {
        Serial.print(F(">ERR Out of Bounds: pulseWidth2 "));
        Serial.print(tempPulseWidth2);
        Serial.print(F("us is less than the minimum "));
        Serial.print(minPulseWidth);
        Serial.println(F("us"));
        outOfBounds = true;
      }

      // Update values only if all are within bounds
      if (!outOfBounds) {
        Intervals[0] = tempPulseInterval;
        Intervals[1] = tempPulseWidth1;
        Intervals[2] = tempInterPulseDelay;
        Intervals[3] = tempPulseWidth2;
        
        // Send back the values to the PC for verification
        Serial.print(F(">OK Parsed values - pulseInterval: "));
        Serial.print(Intervals[0]);
        Serial.print(F("usec, pulseWidth1: "));
        Serial.println(Intervals[1]);
        Serial.print(F("usec, interPulseDelay: "));
        Serial.print(Intervals[2]);
        Serial.print(F("usec, pulseWidth2: "));
        Serial.println(Intervals[3]);
      }
    }
  }
}

// Runs in core 0, seperate from the rest of the program
void DoublePulseControl(void* parameters) {
  for(;;) { 
    digitalWrite(OUTPUT_PIN, LOW);
    esp_rom_delay_us(Intervals[0]); // Pulseinterval
    digitalWrite(OUTPUT_PIN, HIGH); 
    esp_rom_delay_us(Intervals[1]); // PulseWidth1
    digitalWrite(OUTPUT_PIN, LOW);
    esp_rom_delay_us(Intervals[2]); // interPulseDelay
    digitalWrite(OUTPUT_PIN, HIGH);
    esp_rom_delay_us(Intervals[3]); // PulseWith2
  }
}
