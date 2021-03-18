#include <Adafruit_NeoPixel.h>

#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <time.h>
#include <Thread.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <vector>
#include "wordclock.h"

// LED
#define DATA_PIN D7
#define NUM_LEDS 114
#define MAX_BRIGHTNESS 255
#define MIN_BRIGHTNESS 10
#define DELAYVAL 500 // Time (in milliseconds) to pause between pixels

Adafruit_NeoPixel ledstrip(NUM_LEDS, DATA_PIN, NEO_RGBW + NEO_KHZ800);
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
int lightVal = 0;
int lightCounter = 0;

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

  configTime(0, 0, "at.pool.ntp.org");
  setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
  tzset();
  ledstrip.begin();
  ledstrip.setBrightness(100);

  ledsOff();

  startUpLed1();
  ledsOff();
  ledWaitForConnection();

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

void turnOnLed(int i) {
  ledstrip.setPixelColor(i, ledstrip.Color(0, 0, 0, 255));
}

void turnOffLed(int i) {
  ledstrip.setPixelColor(i, ledstrip.Color(0, 0, 0, 0));
}

void ledsOff() {
  for(int i = 0; i < NUM_LEDS; i++) {
    turnOffLed(i);
  }
}

void updater() {
  httpServer.handleClient();
  MDNS.update();
}

void getLightIntensity() {
  int i_light = analogRead(A0);
  lightVal += i_light;

  if (lightCounter == 14) {
    lightVal = lightVal / 15;
    lightCounter = 0;

    if (lightVal <= 8) {
      lightIntensity = 30;
    }
    else if (lightVal > 13 && lightVal <= 33) {
      lightIntensity = 80;
    }
    else if (lightVal > 47 && lightVal <= 63) {
      lightIntensity = 120;
    }
    else if (lightVal > 77 && lightVal <= 73) {
      lightIntensity = 150;
    }
    else if (lightVal > 87) {
      lightIntensity = 150;
    }
    else {
      lightIntensity = lightIntensity;
    }

    ledstrip.setBrightness(constrain(lightIntensity, MIN_BRIGHTNESS, MAX_BRIGHTNESS));
    ledstrip.show();
    Serial.print("Lichtintensität: ");
    Serial.println(lightVal);
  }

  lightCounter++;
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
    turnOffLed(i);
  }

  ledstrip.setBrightness(constrain(lightIntensity, MIN_BRIGHTNESS, MAX_BRIGHTNESS));
  for(std::vector<int>::iterator led = leds.begin(); led != leds.end(); ++led) {
    turnOnLed(*led);
  }

  ledstrip.show();
}

void startUpLed1() {
  ledstrip.show();
  ledstrip.setBrightness(255);
  for(int i = 0; i < NUM_LEDS; i++) {
    if(i>0) turnOffLed(i-1);
    turnOnLed(i);
    ledstrip.show();
    delay(50);
  }
  turnOffLed(NUM_LEDS-1);
  delay(1000);
}

void ledWaitForConnection() {
  for(std::vector<int>::iterator led = mins.begin(); led != mins.end(); ++led) {
    turnOnLed(*led);
  }
  ledstrip.show();
}
