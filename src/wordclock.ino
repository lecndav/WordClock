#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Thread.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <PubSubClient.h>
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

// NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "at.pool.ntp.org");
String formattedTime;
String formattedDate;
String dayStamp;
String timeStamp;
int old_minute = 61;
//

// Photo resistor
int lightIntensity = 100;

// WebUpdater
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
const char* host = "wordclock";

// MQTT
WiFiClient wifiClient;
const char* MQTT_BROKER = "192.168.0.24";
PubSubClient mqttClient(wifiClient);

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
  
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(ledstrip, NUM_LEDS);
  FastLED.setBrightness(255);
  startUpLed1();
  startUpLed2();
  
  WiFiManager wifiManager;
  wifiManager.autoConnect("WordClock_By_Lechner");
  Serial.println("Got network connection");
  
  MDNS.begin(host);
  httpUpdater.setup(&httpServer);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);

  mqttClient.setServer(MQTT_BROKER, 1883);
  
  // Initialize a NTPClient to get time
  timeClient.begin();
  timeClient.setTimeOffset(3600);

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

//  if (!mqttClient.connected()) {
//    mqttClient.connect("ESP8266Client");
//  }
//  mqttClient.loop();

  delay(20);
}

void updater() {
  httpServer.handleClient();
  MDNS.update();
}

void getLightIntensity() {
  int i_light = analogRead(A0);
  lightIntensity = map(i_light, 0, 1023, 0, 255);
//  FastLED.setBrightness(constrain(lightIntensity, MIN_BRIGHTNESS, MAX_BRIGHTNESS));
  FastLED.setBrightness(220);
  FastLED.show();
  Serial.print("Lichtintensität: ");
  Serial.println(lightIntensity);
//  char buf[4];
//  String(i_light).toCharArray(buf, 4);
//  mqttClient.publish("/word_clock/light", buf);
}

void updateTime() {
  while(!timeClient.update()) {
    Serial.println("waiting for time...");
    timeClient.forceUpdate();
  }

  formattedDate = timeClient.getFormattedDate();
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  String hour = timeStamp.substring(0, 2);
  String minute = timeStamp.substring(3, 5);
  String year = dayStamp.substring(0, 4);
  String month = dayStamp.substring(5, 7);
  String day = dayStamp.substring(8, 10);
  
  int y = atoi(year.c_str());
  int mo = atoi(month.c_str());
  int d = atoi(day.c_str());
  int m = atoi(minute.c_str());
  int h = atoi(hour.c_str());

  if (summerTime(y, mo, d, h, 1)) {
    h += 1;
  }

  
  Serial.print("HOUR: ");
  Serial.println(h);
  Serial.print("MINUTE: ");
  Serial.println(minute);
  Serial.print("YEAR: ");
  Serial.println(year);
  Serial.print("MONTH: ");
  Serial.println(month);
  Serial.print("DAY: ");
  Serial.println(day);

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

  Serial.print("hour: ");
  Serial.println(h);
  Serial.print("min: ");
  Serial.println(m - (m % 5));
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

boolean summerTime(int year, byte month, byte day, byte hour, byte tzHours)
// input parameters: "normal time" for year, month, day, hour and tzHours (0=UTC, 1=MEZ)
// return value: returns true during Daylight Saving Time, false otherwise
{ 
  if (month<3 || month>10) return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
  if (month>3 && month<10) return true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
  if (month==3 && (hour + 24 * day)>=(1 + tzHours + 24*(31 - (5 * year /4 + 4) % 7)) || month==10 && (hour + 24 * day)<(1 + tzHours + 24*(31 - (5 * year /4 + 1) % 7))) 
    return true; 
  else 
    return false;
}

void showError() {
  for(int i = 0; i < NUM_LEDS; i++) {
    ledstrip[i] = CRGB::Black;
  }
  
  FastLED.setBrightness(constrain(lightIntensity, MIN_BRIGHTNESS, MAX_BRIGHTNESS));
  for(std::vector<int>::iterator led = mins.begin(); led != mins.end(); ++led) {
    ledstrip[*led] = CRGB::Green;
  }

  FastLED.show();
  delay(15000);
}
