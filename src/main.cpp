#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <time.h>
#include <math.h>
#include "frames.h"
#include "secrets.h"

// Can edit these
const float LAT = 57.7089;   // Gothenburg. Change for your city.
const float LON = 11.9746;
const int TEST_CODE = -1;  // -1 = real weather. Try 0 (sun), 3 (cloud), 61 (rain), 71 (snow)
const int TEST_DAY  = -1;  // -1 = real. 1 = force day, 0 = force night
// --------------------

const unsigned long WEATHER_EVERY_MS = 15UL * 60UL * 1000UL;
const int SCR_W = 160, SCR_H = 128;
const int GROUND_Y = 112;
const int FX = (SCR_W - FRAME_W) / 2;
const int FY = 20;

enum Scene { SCENE_SUN, SCENE_CLOUD, SCENE_NIGHT, SCENE_RAIN, SCENE_SNOW };

TFT_eSPI tft;
TFT_eSprite canvas = TFT_eSprite(&tft);

int weatherCode = 3;
float temperature = 0;
bool isDay = true;
bool haveWeather = false;

int current = ANIM_CLOUDY;
int frame = 0;
unsigned long nextFrame = 0, nextRender = 0, nextWeather = 0;
unsigned long tick = 0;

const int N_DROPS = 22, N_FLAKES = 28, N_STARS = 14;
int dropX[N_DROPS], dropY[N_DROPS];
int flakeX[N_FLAKES], flakeY[N_FLAKES];
int starX[N_STARS], starY[N_STARS];
int cloudX[3] = {10, 70, 120};

uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// ---------- weather ----------
bool fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) { WiFi.reconnect(); return false; }
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  String url = String("https://api.open-meteo.com/v1/forecast?latitude=") + String(LAT, 4) +
               "&longitude=" + String(LON, 4) +
               "&current=temperature_2m,weather_code,is_day&timezone=auto";
  if (!http.begin(client, url)) return false;
  int status = http.GET();
  if (status != 200) { Serial.printf("Weather HTTP %d\n", status); http.end(); return false; }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getString());
  http.end();
  if (err) { Serial.println("JSON error"); return false; }
  weatherCode = doc["current"]["weather_code"];
  temperature = doc["current"]["temperature_2m"];
  isDay = doc["current"]["is_day"];
  Serial.printf("Weather code %d, %.1f C, day=%d\n", weatherCode, temperature, isDay);
  return true;
}

bool isSnow(int c) { return (c >= 71 && c <= 77) || c == 85 || c == 86; }
bool isRain(int c) { return (c >= 51 && c <= 67) || (c >= 80 && c <= 82) || c >= 95; }

int pickAnimation(int code, bool day) {
  if (isSnow(code)) return ANIM_SNOWY;
  if (isRain(code)) return ANIM_RAINY;
  if (code <= 1) return day ? ANIM_SUNNY : ANIM_CLOUDY;
  return ANIM_CLOUDY;
}

int pickScene(int code, bool day) {
  if (isSnow(code)) return SCENE_SNOW;
  if (isRain(code)) return SCENE_RAIN;
  if (!day) return SCENE_NIGHT;
  if (code <= 1) return SCENE_SUN;
  return SCENE_CLOUD;
}

// ---------- drawing ----------
void drawCloud(int x, int y, uint16_t c) {
  canvas.fillCircle(x, y, 7, c);
  canvas.fillCircle(x + 9, y - 4, 9, c);
  canvas.fillCircle(x + 19, y, 7, c);
  canvas.fillRect(x, y, 20, 8, c);
}

void updateParticles(int scene) {
  tick++;
  if (tick % 3 == 0) {
    for (int i = 0; i < 3; i++) {
      cloudX[i]++;
      if (cloudX[i] > SCR_W) cloudX[i] = -30;
    }
  }
  if (scene == SCENE_RAIN) {
    for (int i = 0; i < N_DROPS; i++) {
      dropY[i] += 7;
      if (dropY[i] > GROUND_Y) { dropY[i] = -6; dropX[i] = random(SCR_W); }
    }
  }
  if (scene == SCENE_SNOW) {
    for (int i = 0; i < N_FLAKES; i++) {
      if (i % 2 == 0 || tick % 2 == 0) flakeY[i]++;
      flakeX[i] += random(-1, 2);
      if (flakeY[i] > GROUND_Y) { flakeY[i] = -3; flakeX[i] = random(SCR_W); }
    }
  }
}

void drawScene(int scene, unsigned long now) {
  switch (scene) {
    case SCENE_SUN: {
      canvas.fillSprite(rgb(120, 200, 255));
      int sx = 136, sy = 42;
      float spin = (now / 4000.0f) * 6.2832f;
      for (int i = 0; i < 8; i++) {
        float a = spin + i * 0.7854f;
        canvas.drawLine(sx + cosf(a) * 15, sy + sinf(a) * 15,
                        sx + cosf(a) * 21, sy + sinf(a) * 21, rgb(255, 230, 90));
      }
      canvas.fillCircle(sx, sy, 11, rgb(255, 215, 50));
      drawCloud(cloudX[0], 44, TFT_WHITE);
      canvas.fillRect(0, GROUND_Y, SCR_W, SCR_H - GROUND_Y, rgb(70, 170, 70));
      break;
    }
    case SCENE_CLOUD: {
      canvas.fillSprite(rgb(150, 170, 190));
      uint16_t c = rgb(235, 240, 245);
      drawCloud(cloudX[0], 32, c);
      drawCloud(cloudX[1], 55, c);
      drawCloud(cloudX[2], 40, c);
      canvas.fillRect(0, GROUND_Y, SCR_W, SCR_H - GROUND_Y, rgb(90, 140, 90));
      break;
    }
    case SCENE_NIGHT: {
      uint16_t sky = rgb(15, 20, 50);
      canvas.fillSprite(sky);
      for (int i = 0; i < N_STARS; i++) {
        bool bright = ((now / 500) + i) % 3 != 0;
        canvas.drawPixel(starX[i], starY[i], bright ? TFT_WHITE : rgb(90, 90, 120));
      }
      canvas.fillCircle(136, 42, 10, rgb(250, 245, 200));
      canvas.fillCircle(140, 39, 9, sky);
      canvas.fillRect(0, GROUND_Y, SCR_W, SCR_H - GROUND_Y, rgb(25, 50, 45));
      break;
    }
    case SCENE_RAIN: {
      canvas.fillSprite(rgb(70, 80, 100));
      uint16_t c = rgb(50, 55, 70);
      drawCloud(cloudX[0], 32, c);
      drawCloud(cloudX[1], 28, c);
      drawCloud(cloudX[2], 36, c);
      for (int i = 0; i < N_DROPS; i++)
        canvas.drawLine(dropX[i], dropY[i], dropX[i] - 2, dropY[i] + 6, rgb(150, 190, 255));
      canvas.fillRect(0, GROUND_Y, SCR_W, SCR_H - GROUND_Y, rgb(50, 70, 80));
      break;
    }
    case SCENE_SNOW: {
      canvas.fillSprite(rgb(170, 190, 215));
      for (int i = 0; i < N_FLAKES; i++)
        canvas.fillRect(flakeX[i], flakeY[i], 2, 2, TFT_WHITE);
      canvas.fillRect(0, GROUND_Y, SCR_W, SCR_H - GROUND_Y, rgb(240, 245, 255));
      break;
    }
  }
}

void drawHeader() {
  char buf[12];
  struct tm t;
  if (getLocalTime(&t, 0)) strftime(buf, sizeof(buf), "%H:%M", &t);
  else strcpy(buf, "--:--");
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(TFT_BLACK);
  canvas.drawString(buf, 3, 1, 2);
  canvas.setTextColor(TFT_WHITE);
  canvas.drawString(buf, 2, 0, 2);

  if (haveWeather) snprintf(buf, sizeof(buf), "%.0fC", temperature);
  else strcpy(buf, "--");
  canvas.setTextDatum(TR_DATUM);
  canvas.setTextColor(TFT_BLACK);
  canvas.drawString(buf, SCR_W - 1, 1, 2);
  canvas.setTextColor(TFT_WHITE);
  canvas.drawString(buf, SCR_W - 2, 0, 2);
}

void drawFairy(const uint16_t* data, int x0, int y0) {
  for (int y = 0; y < FRAME_H; y++) {
    for (int x = 0; x < FRAME_W; x++) {
      uint16_t c = pgm_read_word(&data[y * FRAME_W + x]);
      if (c != 0) canvas.drawPixel(x0 + x, y0 + y, c);   // 0 = empty, skip it
    }
  }
}

// ---------- main ----------
void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Connecting WiFi...", 5, 55, 2);

  canvas.setColorDepth(16);
  if (!canvas.createSprite(SCR_W, SCR_H)) Serial.println("Canvas allocation failed");
  canvas.setSwapBytes(true);

  for (int i = 0; i < N_DROPS; i++)  { dropX[i] = random(SCR_W);  dropY[i] = random(-6, GROUND_Y); }
  for (int i = 0; i < N_FLAKES; i++) { flakeX[i] = random(SCR_W); flakeY[i] = random(-3, GROUND_Y); }
  for (int i = 0; i < N_STARS; i++)  { starX[i] = random(SCR_W);  starY[i] = random(18, 100); }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) delay(250);
  Serial.println(WiFi.status() == WL_CONNECTED ? "WiFi connected" : "WiFi failed");

  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org");
  haveWeather = fetchWeather();
  nextWeather = millis() + WEATHER_EVERY_MS;
}

void loop() {
  unsigned long now = millis();

  if (now >= nextWeather) {
    haveWeather = fetchWeather() || haveWeather;
    nextWeather = now + WEATHER_EVERY_MS;
  }

  int code = (TEST_CODE >= 0) ? TEST_CODE : weatherCode;
  bool day = (TEST_DAY >= 0) ? (TEST_DAY == 1) : isDay;
  int anim = pickAnimation(code, day);
  int scene = pickScene(code, day);
  if (anim != current) { current = anim; frame = 0; }

  if (now >= nextFrame) {
    frame = (frame + 1) % ANIMS[current].count;
    nextFrame = now + ANIMS[current].delayMs;
  }

  if (now >= nextRender) {
    nextRender = now + 50;   // about 20 redraws per second
    updateParticles(scene);
    drawScene(scene, now);
    const Animation& a = ANIMS[current];
    drawFairy(a.frames[frame % a.count], FX, FY);
    drawHeader();
    canvas.pushSprite(0, 0);
  }
}