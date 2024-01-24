#include <Arduino.h>
#include <FastLED.h>
#include "OTA.h"

#define DATA_PIN 8
#define NUM_RING_LEDS 24
#define NUM_STRIP_LEDS 23
#define NUM_LEDS NUM_RING_LEDS

CRGB leds[NUM_LEDS];
CRGB color;

void setup() {
  Serial.begin(115200);

  setupOTAUpdateAndSerialMonitor();

  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 800);
  FastLED.setBrightness(20);

  color = CRGB::Red;

  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
  
}

void lightTheOnboardLED() {
  // digitalWrite(LED_RED, LOW); //red
  // digitalWrite(LED_GREEN, LOW); //green
  // digitalWrite(LED_BLUE, LOW); //blue
}

void loop() {
  ElegantOTA.loop();  // Required for OTA update
  
  static uint8_t hue_i = 0;
  fill_rainbow_circular(leds, NUM_LEDS, hue_i);

  EVERY_N_MILLIS(20) {
    hue_i++;
  }
  
  FastLED.show();

  printWebSerialIP();
}
