#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <time.h>
#include <Thread.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <vector>
#include "wordclock.h"
#include <FastLED.h>


// Fast LED
#define DATA_PIN 7
#define NUM_LEDS 114
#define MAX_BRIGHTNESS 255
#define MIN_BRIGHTNESS 10
CRGB ledstrip[NUM_LEDS];
//

// Threads
Thread t_lightIntensity = Thread();
Thread t_updateTime = Thread();
Thread t_updater = Thread();
//

//
int old_minute = 61;
//

// Photo resistor
int lightIntensity = 100;

// WebUpdater
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
const char* host = "wordclock";

std::vector<int> getHourLEDs(int h) {
  switch(h) {
    case 1: return eins;
    break;
    case 2: return zwei;
    break;
    case 3: return drei;
    break;
    case 4: return vier;
    break;
    case 5: return fuenf;
    break;
    case 6: return sechs;
    break;
    case 7: return sieben;
    break;
    case 8: return acht;
    break;
    case 9: return neun;
    break;
    case 10: return zehn;
    break;
    case 11: return elf;
    break;
    case 12: return zwoelf;
    break;
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("Starting...");

  setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
  tzset();
  configTime(0, 0, "at.pool.ntp.org");
  
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(ledstrip, NUM_LEDS);
  FastLED.setBrightness(255);

  for(int i = 0; i < NUM_LEDS; i++) {
    ledstrip[i] = CRGB::White;
  }
  
  startUpLed1();
  startUpLed2();
  
  WiFiManager wifiManager;
  wifiManager.autoConnect("WordClock_By_Lechner");
  Serial.println("Got network connection");
  
  MDNS.begin(host);
  httpUpdater.setup(&httpServer);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);

  t_lightIntensity.onRun(getLightIntensity);
  t_lightIntensity.setInterval(200);
  t_updateTime.onRun(updateTime);
  t_updateTime.setInterval(2000);
  t_updater.onRun(updater);
  t_updater.setInterval(20);
}

void loop() {
  if (t_lightIntensity.shouldRun()) {
    t_lightIntensity.run();
  }

  if (t_updateTime.shouldRun()) {
    t_updateTime.run();
  }

  if (t_updater.shouldRun()) {
    t_updater.run();
  }

  delay(20);
}

void updater() {
  httpServer.handleClient();
  MDNS.update();
}

void getLightIntensity() {
  int i_light = analogRead(A0);

  if (i_light <= 10) {
    lightIntensity = 40;
  }
  else if (i_light > 10 && i_light <= 40) {
    lightIntensity = 100;
  }
  else if (i_light > 40 && i_light <= 70) {
    lightIntensity = 140;
  }
  else if (i_light > 70 && i_light <= 80) {
    lightIntensity = 160;
  }
  else if (i_light > 80 && i_light <= 100) {
    lightIntensity = 190;
  }
  else if (i_light > 100 && i_light <= 150) {
    lightIntensity = 210;
  }
  else {
    lightIntensity = 240;
  }
  
  FastLED.setBrightness(constrain(lightIntensity, MIN_BRIGHTNESS, MAX_BRIGHTNESS));
  FastLED.show();
  Serial.print("Lichtintensität: ");
  Serial.println(i_light);
}

void updateTime() {
  time_t now;
  struct tm * timeinfo;
  time(&now);
  timeinfo = localtime(&now);
  int m = timeinfo->tm_min;
  int h = timeinfo->tm_hour;
  
  Serial.print("HOUR: ");
  Serial.println(h);
  Serial.print("MINUTE: ");
  Serial.println(m);

  // check if a minute is over
  if(m != old_minute) {
    processTime(h, m);
    old_minute = m;
  }
}

void processTime(int h, int m) {
  int h1 = h + 1;
  h = h % 12;
  h1 = h1 % 12;
  if(h == 0) {
    h = 12;
  }
  if(h1 == 0) {
    h1 = 12;
  }
  
  std::vector<int> leds;
  leds.insert(leds.end(), esist.begin(), esist.end());

  std::vector<int> hourLeds = getHourLEDs(h);
  std::vector<int> hourLeds1 = getHourLEDs(h1);

  if (h == 1 && m < 5) {
    hourLeds = ein;
  }

  switch(m - (m % 5)) {
    case 0:
      leds.insert(leds.end(), hourLeds.begin(), hourLeds.end());
      leds.insert(leds.end(), uhr.begin(), uhr.end());
      break;
    case 5:
      leds.insert(leds.end(), fuenfa.begin(), fuenfa.end());
      leds.insert(leds.end(), nach.begin(), nach.end());
      leds.insert(leds.end(), hourLeds.begin(), hourLeds.end());
      break;
    case 10: 
      leds.insert(leds.end(), zehna.begin(), zehna.end());
      leds.insert(leds.end(), nach.begin(), nach.end());
      leds.insert(leds.end(), hourLeds.begin(), hourLeds.end());
      break;
    case 15:
      leds.insert(leds.end(), viertel.begin(), viertel.end());
      leds.insert(leds.end(), hourLeds1.begin(), hourLeds1.end());
      break;
    case 20:
      leds.insert(leds.end(), zwanzig.begin(), zwanzig.end());
      leds.insert(leds.end(), nach.begin(), nach.end());
      leds.insert(leds.end(), hourLeds.begin(), hourLeds.end());
      break;
    case 25:
      leds.insert(leds.end(), fuenfa.begin(), fuenfa.end());
      leds.insert(leds.end(), vor.begin(), vor.end());
      leds.insert(leds.end(), halb.begin(), halb.end());
      leds.insert(leds.end(), hourLeds1.begin(), hourLeds1.end());
      break;
    case 30:
      leds.insert(leds.end(), halb.begin(), halb.end());
      leds.insert(leds.end(), hourLeds1.begin(), hourLeds1.end());
      break;
    case 35:
      leds.insert(leds.end(), fuenfa.begin(), fuenfa.end());
      leds.insert(leds.end(), nach.begin(), nach.end());
      leds.insert(leds.end(), halb.begin(), halb.end());
      leds.insert(leds.end(), hourLeds1.begin(), hourLeds1.end());
      break;
    case 40:
      leds.insert(leds.end(), zwanzig.begin(), zwanzig.end());
      leds.insert(leds.end(), vor.begin(), vor.end());
      leds.insert(leds.end(), hourLeds1.begin(), hourLeds1.end());
      break;
    case 45:
      leds.insert(leds.end(), dreiviertel.begin(), dreiviertel.end());
      leds.insert(leds.end(), hourLeds1.begin(), hourLeds1.end());
      break;
    case 50:
      leds.insert(leds.end(), zehna.begin(), zehna.end());
      leds.insert(leds.end(), vor.begin(), vor.end());
      leds.insert(leds.end(), hourLeds1.begin(), hourLeds1.end());
      break;
    case 55:
      leds.insert(leds.end(), fuenfa.begin(), fuenfa.end());
      leds.insert(leds.end(), vor.begin(), vor.end());
      leds.insert(leds.end(), hourLeds1.begin(), hourLeds1.end());
      break;
  }

  int mins_to_5 = m % 5;
  if(mins_to_5 > 0 ) {
    leds.insert(leds.end(), mins.begin(), mins.begin() + mins_to_5);
  }

  controllLEDs(leds);
}

void controllLEDs(std::vector<int> leds) {
  // reset all LEDs
  for(int i = 0; i < NUM_LEDS; i++) {
    ledstrip[i] = CRGB::Black;
  }
  
  FastLED.setBrightness(constrain(lightIntensity, MIN_BRIGHTNESS, MAX_BRIGHTNESS));
  for(std::vector<int>::iterator led = leds.begin(); led != leds.end(); ++led) {
    ledstrip[*led] = CRGB::White;
  }

  FastLED.show();
}

void startUpLed1() {
  for(int i = 0; i < NUM_LEDS; i++) {
    ledstrip[i] = CRGB::Black;
  }
  FastLED.show();
  FastLED.setBrightness(255);
  for(int i = 0; i < NUM_LEDS; i++) {
    if(i>0) ledstrip[i-1] = CRGB::Black;
    ledstrip[i] = CRGB::White;
    FastLED.show();
    delay(50);
  }
  for(int i = 0; i < 4; i++) {
    for(int i = 0; i < NUM_LEDS; i++) {
      ledstrip[i] = CRGB::White;
    }
    FastLED.setBrightness(255);
    FastLED.show();
    for(int i = 80; i > 20; i--) {
      FastLED.setBrightness(i);
      FastLED.show();
      delay(15);
    }
    for(int i = 0; i < NUM_LEDS; i++) {
      ledstrip[i] = CRGB::Black;
    }
    FastLED.show();
    delay(200);  
  }
  delay(1000);
}

void startUpLed2() {
  FastLED.setBrightness(255);
  for(int i = 0; i < NUM_LEDS; i++) {
    ledstrip[i] = CRGB::Black;
  }

  for(std::vector<int>::iterator led = mins.begin(); led != mins.end(); ++led) {
    ledstrip[*led] = CRGB::White;
  }
  FastLED.show();
}
