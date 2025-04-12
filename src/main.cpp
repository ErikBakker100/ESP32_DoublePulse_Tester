// Dual Pulse Generator for ESP
// Erik Bakker 2025
// Partly used from Teensy 4.0 Signal Generator, Electronics Workshop, Robin O'Reilly

#include <Arduino.h>
#include <ArduinoJson.h>
#include "esp_timer.h"

// Timing Variables
//                _____________                   ____________
// Pulseinterval | PulseWidth1 | interPulseDelay | PulseWith2 | Pulseinterval
// ______________               _________________              _______________

unsigned long pulseInterval;
unsigned long interPulseDelay;
unsigned long pulseWidth1;
unsigned long pulseWidth2;
uint8_t P=1; // Start with pulseinterval

// Lower limits for the parameters
const unsigned long minPulseInterval = 10; // Minimum 10ms
const unsigned long minInterPulseDelay = 1; // Minimum 20us
const unsigned long minPulseWidth = 1;  // Minimum 20us

bool enable = 0; // Wait for first JSON before enabling
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

// put function declarations here:
static void oneshot_timer_callback(void* arg);
esp_timer_handle_t oneshot_timer;

volatile bool timerActive = false;

void setup() {
  setCpuFrequencyMhz(CPU_FREQ);
  Serial.begin(115200);
  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(1, LOW); // Initial state low (inactive)
  delay(300); // Give serial port time to open
  Serial.println("**************Dual Pulse Generator**************");
  Serial.println("> Usage: Send JSON string, for e.g {\"pulseInterval\": 100, \"interPulseDelay\": 200, \"pulseWidth1\": 10, \"pulseWidth2\": 10}.");
  Serial.print("> Values are in microseconds (note: ESP32 adds 20usec per interval). Using output port: ");
  Serial.println(OUTPUT_PIN);
  Serial.println(">                _____________                   ____________");
  Serial.println("> pulseInterval | pulseWidth1 | interPulseDelay | pulseWith2 | pulseInterval");
  Serial.println("> ______________               _________________              _______________");

  //create timer parameters..
  const esp_timer_create_args_t oneshot_timer_args = {
    .callback = &oneshot_timer_callback, //link the call back
    .arg = nullptr, //not passing in anything
    .name = "one-shot" //nice name
  };
  //create timer, not running yet..
  ESP_ERROR_CHECK(esp_timer_create(&oneshot_timer_args, &oneshot_timer));
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
      unsigned long tempInterPulseDelay = doc["interPulseDelay"];
      unsigned long tempPulseWidth1 = doc["pulseWidth1"];
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
        pulseInterval = tempPulseInterval;
        interPulseDelay = tempInterPulseDelay;
        pulseWidth1 = tempPulseWidth1;
        pulseWidth2 = tempPulseWidth2;
        
        // Send back the values to the PC for verification
        Serial.print(F(">OK Parsed values - pulseInterval: "));
        Serial.print(pulseInterval);
        Serial.print(F("usec, interPulseDelay: "));
        Serial.print(interPulseDelay);
        Serial.print(F("usec, pulseWidth1: "));
        Serial.println(pulseWidth1);
        Serial.print(F("usec, pulseWidth2: "));
        Serial.println(pulseWidth2);
        enable = 1;
      }
    }
  }
  if (enable == 1) {
    if (!timerActive) {
      //check state of timer and stop it if needed..
      if (esp_timer_is_active(oneshot_timer)) {
        ESP_ERROR_CHECK(esp_timer_stop(oneshot_timer));
      }
      switch(P){
        case 1:
          ESP_ERROR_CHECK(esp_timer_start_once(oneshot_timer, pulseInterval));
          P++;
          break;
        case 2:
          ESP_ERROR_CHECK(esp_timer_start_once(oneshot_timer, pulseWidth1));
          P++;
        break;
        case 3:
          ESP_ERROR_CHECK(esp_timer_start_once(oneshot_timer, interPulseDelay));
          P++;
          break;
        case 4:
          ESP_ERROR_CHECK(esp_timer_start_once(oneshot_timer, pulseWidth2));
          P=1;
          break;
        default:
          P=1;
      }
      timerActive = true;
    }
  }
}

static void oneshot_timer_callback(void* arg)
{
  digitalWrite(OUTPUT_PIN, !digitalRead(OUTPUT_PIN));
  timerActive = false;//say we're done..
}