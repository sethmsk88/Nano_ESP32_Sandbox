#include <Arduino.h>
#include <FastLED.h>
#include "OTA.h"

#define DATA_PIN 8
#define NUM_RING_LEDS 24
#define NUM_STRIP_LEDS 23
#define NUM_LEDS NUM_RING_LEDS
#define WAKEUP_PIN A3 // GPIO 8 = D5
#define SLEEP_BTN_PIN D6
#define LED_PIN 10  // GPIO 10 = D7

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define ms_TO_S_FACTOR 1000  /* Conversion factor for milliseconds to seconds */
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */
#define AWAKE_TIME 5           /* Time to stay awake before going to sleep */ 

#define BUTTON_PIN_BITMASK 0x200000000 // 2^33 in hex

/*
  The ESP32 has 8KB of SRAM on the RTC part, called RTC fast memory. The data
  saved here is not erased during deep sleep.
  To save data on RTC memory, you just have to add RTC_DATA_ATTR before a variable
  definition.
*/
RTC_DATA_ATTR int bootCount = 0;

uint32_t timerStartTime = 0;

byte lastButtonState; // the previous state of button
byte currentButtonState; // the current state of button
byte ledState = LOW;



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
  Serial.println(F("Going to sleep now. Press the button to wake me up."));
  WebSerial.println(F("Going to sleep now. Press the button to wake me up."));
  delay(1000);
  WebSerial.flush(); 
  
  // esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void showBootIndicator() {
  bootCount++;
  Serial.print("Boot #");
  Serial.println(bootCount);

  uint8_t ledToTurnOn = bootCount <= 1 ? LED_RED : LED_GREEN;
  digitalWrite(ledToTurnOn, LOW);

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

  showBootIndicator();

  /*
  First we configure the wake up source
  We set our ESP32 to wake up for an external trigger.
  There are two types for ESP32, ext0 and ext1 .
  ext0 uses RTC_IO to wakeup thus requires RTC peripherals
  to be on while ext1 uses RTC Controller so doesnt need
  peripherals to be powered on.
  Note that using internal pullups/pulldowns also requires
  RTC peripherals to be turned on.
  */
  //esp_sleep_enable_ext0_wakeup(GPIO_NUM_12, HIGH); //1 = High, 0 = Low

  /*
    Setup button
  */
  pinMode(WAKEUP_PIN, INPUT_PULLUP);  // Set button pin to input pullup mode
  pinMode(SLEEP_BTN_PIN, INPUT_PULLUP);

  // I don't know why HIGH works instead of LOW. LOW does not work!!!!!!!
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, HIGH);  // Configure external wake-up. GPIO_NUM_4 is A3 on Nano ESP32

  Serial.println(digitalRead(WAKEUP_PIN));
  delay(1000);  // Adding a 1 second delay to debounce the button press


  //If you were to use ext1, you would use it like
  //esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);


  // goToSleep();
}



void loop() {
  ElegantOTA.loop();  // Required for OTA update
  
  static uint8_t hue_i = 0;
  fill_rainbow_circular(leds, NUM_LEDS, hue_i);

  EVERY_N_MILLIS(20) {
    hue_i++;
  }
  
  FastLED.show();

  // printWebSerialIP();
  
  /*
  EVERY_N_SECONDS(1) {
    if (digitalRead(WAKEUP_PIN) == LOW) {
      Serial.println("Button pressed");
    }
    else {
      Serial.println("Button NOT pressed");
    }
  }
  */

  // Go to sleep if awake time has elapsed
  /*if (millis() - timerStartTime >= AWAKE_TIME * ms_TO_S_FACTOR) {
    timerStartTime = millis();
    goToSleep();
  }*/

  // read the state of the button
  currentButtonState = digitalRead(SLEEP_BTN_PIN);

  if (lastButtonState == LOW && currentButtonState == HIGH) {
    WebSerial.println("The state changed from LOW to HIGH");
  }

  // Save last button state
  lastButtonState = currentButtonState;

  /*
  if (digitalRead(SLEEP_BTN_PIN) == LOW) {
    goToSleep();
  }
  */
}
