#include <Arduino.h>
#include <FastLED.h>
#include "OTA.h"

#define DATA_PIN 8
#define NUM_RING_LEDS 24
#define NUM_STRIP_LEDS 23
#define NUM_LEDS NUM_RING_LEDS
#define RTC_PIN 12

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define ms_TO_S_FACTOR 1000  /* Conversion factor for milliseconds to seconds */
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */
#define AWAKE_TIME 15           /* Time to stay awake before going to sleep */ 

/*
  The ESP32 has 8KB of SRAM on the RTC part, called RTC fast memory. The data
  saved here is not erased during deep sleep.
  To save data on RTC memory, you just have to add RTC_DATA_ATTR before a variable
  definition.
*/
RTC_DATA_ATTR int bootCount = 0;

uint32_t previousMillis = 0;


CRGB leds[NUM_LEDS];
CRGB color;

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : WebSerial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : WebSerial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : WebSerial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : WebSerial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : WebSerial.println("Wakeup caused by ULP program"); break;
    default : WebSerial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void goToSleep() {
  Serial.println(F("Going to sleep now"));
  WebSerial.println(F("Going to sleep now"));
  delay(1000);
  WebSerial.flush(); 
  
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void showBootIndicator() {
  if (bootCount == 0) {
    Serial.println("Initial boot");
    digitalWrite(LED_RED, LOW);
    bootCount = bootCount + 1;
  }
  else {
    Serial.println("Wakeup boot");
    digitalWrite(LED_GREEN, LOW);
  }
  print_wakeup_reason();
}

void lightTheOnboardLED() {
  digitalWrite(LED_RED, LOW); //red
  digitalWrite(LED_GREEN, LOW); //green
  digitalWrite(LED_BLUE, LOW); //blue
}

void setup() {
  Serial.begin(115200);

  //lightTheOnboardLED();
  setupOTAUpdateAndSerialMonitor();
  
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 800);
  FastLED.setBrightness(20);

  color = CRGB::Red;

  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();

  // setupDeepSleep();

  // setupDeepSleep2();
  showBootIndicator();
}



void loop() {
  ElegantOTA.loop();  // Required for OTA update
  
  static uint8_t hue_i = 0;
  fill_rainbow_circular(leds, NUM_LEDS, hue_i);

  EVERY_N_MILLIS(20) {
    hue_i++;
  }
  
  FastLED.show();

  //Serial.println("Loop");

  printWebSerialIP();

  // Go to sleep if awake time has elapsed
  if (millis() - previousMillis >= AWAKE_TIME * ms_TO_S_FACTOR) {
    previousMillis = millis();
    goToSleep();
  }
}
