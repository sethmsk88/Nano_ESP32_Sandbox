#include <Arduino.h>
#include <FastLED.h>
#include "OTA.h"
#include <driver/rtc_io.h>

// #define LED_STRIP  // Normal sized LEDs test strip
#define SMALL_LED_STRIP // Small LEDs test strip
// #define LED_RING

#ifdef LED_STRIP
  #define LED_TYPE WS2812
  #define NUM_LEDS 23
  #define COLOR_ORDER GRB
#endif

#ifdef SMALL_LED_STRIP
  #define LED_TYPE WS2812
  #define NUM_LEDS 150
  #define COLOR_ORDER GRB
#endif

#ifdef LED_RING
  #define LED_TYPE WS2812
  #define NUM_LEDS 24
  #define COLOR_ORDER GRB
#endif

/*
  PIN Map for ESP32 Nano
  D0	GPIO44
  D1	GPIO43
  D2	GPIO5
  D3	GPIO6
  D4	GPIO7
  D5	GPIO8
  D6	GPIO9
  D7	GPIO10
  D8	GPIO17
  D9	GPIO18
  D10	GPIO21
  D11	GPIO38
  D12	GPIO47
  D13	GPIO48
  A0	GPIO1
  A1	GPIO2
  A2	GPIO3
  A3	GPIO4
  A4	GPIO11
  A5	GPIO12
  A6	GPIO13
  A7	GPIO14
*/

#define DATA_PIN 9  // GPIO9 = D6
#define ANALOG_PIN A3 // GPIO8 = D5
//#define DEEP_SLEEP_PIN D10 // D10 = GPIO21
#define DEEP_SLEEP_PIN D2 // D2 = GPIO5
#define WAKEUP_PIN A3 // GPIO 8 = D5 -- This pin can be used to wakeup from deep sleep

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define ms_TO_S_FACTOR 1000  /* Conversion factor for milliseconds to seconds */
#define TIME_UNTIL_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */
#define TIME_TO_SLEEP  10        /* Time ESP32 will sleep (in seconds) */
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
byte currentButtonState; // Fthe current state of button
byte ledState = LOW;

int sensorValue = 0;

CRGB leds[NUM_LEDS];
CRGB color;

/* Function to initialize all available GPIOs as inputs with either pullup or pulldown resistors */
void initGPIO()
{
  /*pinMode(0, INPUT_PULLUP); // External green LED with pull-up
  pinMode(1, INPUT_PULLDOWN);
  pinMode(2, INPUT_PULLDOWN);
  pinMode(3, INPUT_PULLDOWN);
  pinMode(4, INPUT_PULLDOWN);
  pinMode(5, INPUT_PULLDOWN);
  pinMode(6, INPUT_PULLDOWN);
  pinMode(7, INPUT_PULLDOWN);
  pinMode(8, INPUT_PULLDOWN);
  pinMode(9, INPUT_PULLDOWN);
  pinMode(10, INPUT_PULLDOWN);
  pinMode(11, INPUT_PULLDOWN);
  */
  
  // pinMode(ANALOG_PIN, INPUT_PULLUP);
/*  pinMode(13, INPUT_PULLDOWN);
  pinMode(14, INPUT_PULLDOWN); // For unknown reasons, GPIO14 floats at a half-level if configured as input with pullup, so configure with pulldown.
  // GPIO15 & GPIO16 are connected to 32kHz xtal on Nano board, we'll skip configuring them for now
  pinMode(17, INPUT_PULLDOWN); 
  pinMode(18, INPUT_PULLDOWN); 
  pinMode(19, INPUT_PULLDOWN); 
  pinMode(20, INPUT_PULLUP); // USB_DP
  pinMode(21, INPUT_PULLDOWN); // USB_DN
  // GPIO22 through GPIO25 are not present on ESP32-S3R8 SoC, so we'll skip configuring them.
  // GPIO26 through GPIO37 used as octal SPI interface to internal SPI PSRAM on ESP32-S3R8 SoC and external SPI FLASH on Nano ESP32 board, so we'll skip configuring them for now.
  pinMode(38, INPUT_PULLDOWN);
  pinMode(39, INPUT_PULLDOWN);
  pinMode(40, INPUT_PULLDOWN);
  pinMode(41, INPUT_PULLDOWN);
  pinMode(42, INPUT_PULLDOWN);
  pinMode(43, INPUT_PULLDOWN);
  pinMode(44, INPUT_PULLDOWN);
  pinMode(45, INPUT_PULLUP); // External blue LED with pull-up
  pinMode(46, INPUT_PULLUP); // External red LED with pull-up
  pinMode(47, INPUT_PULLDOWN);
  pinMode(48, INPUT_PULLDOWN); // External NFET gate for yellow LED drive with pull-down.
  */
}

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
  WebSerial.printf("Going to sleep for %i seconds. Press the button to wake me up.\n", TIME_TO_SLEEP);
  fill_solid(leds, NUM_LEDS, CRGB::Black);  // turn off LEDs
  FastLED.show();
  delay(1000);
  
  
  WebSerial.flush(); 
  
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void showBootIndicator() {
  bootCount++;
  WebSerial.printf("Boot #%i\n", bootCount);

  uint8_t ledToTurnOn = bootCount <= 1 ? LED_RED : LED_GREEN;
  digitalWrite(ledToTurnOn, LOW);
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
  
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 800);
  FastLED.setBrightness(20);

  color = CRGB::Green;

  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  showBootIndicator();

  /*************************************************************
   * First we configure the wake up source
   * We set our ESP32 to wake up for an external trigger.
   * There are two types for ESP32, ext0 and ext1 .
   * ext0 uses RTC_IO to wakeup thus requires RTC peripherals
   * to be on while ext1 uses RTC Controller so doesnt need
   * peripherals to be powered on.
   * Note that using internal pullups/pulldowns also requires
   * RTC peripherals to be turned on.
   *************************************************************/
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_12, HIGH); //1 = High, 0 = Low
  pinMode(DEEP_SLEEP_PIN, INPUT_PULLUP); // GPIO21 - Wakeup pin - GPIO21 = D10

  // I don't know why HIGH works instead of LOW. LOW does not work!!!!!!!
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, HIGH);  // Configure external wake-up. GPIO_NUM_4 is A3 on Nano ESP32
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, LOW);/
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_21, LOW); // GPIO21 = D10
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  rtc_gpio_pulldown_dis(GPIO_NUM_5);
  rtc_gpio_pullup_en(GPIO_NUM_5);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_5, LOW); // GPIO5 = D2

  // touchAttachInterrupt(GPIO_NUM_47, callback, 40);
  //esp_sleep_enable_touchpad_wakeup();

  delay(1000);  // Adding a 1 second delay to debounce the button press


  //If you were to use ext1, you would use it like
  //esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);


  // goToSleep();
}

void snake() {
  static int pos = 0;

  EVERY_N_MILLIS(8)
  {
    pos = (pos + 1) % NUM_LEDS;

    fadeToBlackBy(leds, NUM_LEDS, 20);
    leds[pos] = color;
  }

  FastLED.show();
}

void loop() {
  ElegantOTA.loop();  // Required for OTA update

  EVERY_N_SECONDS(5) {
    print_wakeup_reason();
  }

  snake();
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
  sensorValue = digitalRead(DEEP_SLEEP_PIN);

  // currentButtonState = analogRead(WAKEUP_PIN);

  if (sensorValue == LOW) {
    goToSleep();
  }

  EVERY_N_MILLIS(1000) {
    WebSerial.printf("%i, ", sensorValue);
    // int num = (int)NUM_LEDS;
    // WebSerial.println(num);
    // WebSerial.printf("A3: %i\n", sensorValue);
  }

  /*
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    WebSerial.println("The button is pressed");
  }
  else if (lastButtonState == LOW && currentButtonState == HIGH) {
    WebSerial.println("The button is released");
  }
  */

  // Save last button state
  // lastButtonState = currentButtonState;

  /*
  if (digitalRead(SLEEP_BTN_PIN) == LOW) {
    goToSleep();
  }
  */

 delay(1);  // THIS DELAY IS REQUIRED TO FIX ESP32 TIMING ISSUE
}
