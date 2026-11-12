#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "config.h"
#include "secrets.h"
#include "relay.h"
#include "blynk_control.h"
#include "web_server.h"

unsigned long lastScheduleCheck = 0;
unsigned long lastWifiCheck = 0;
int lastCheckedMinute = -1;

void setupTime() {
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  Serial.println("[TIME] Sinkron NTP WIB...");
  struct tm timeinfo;
  int retry = 0;
  while (!getLocalTime(&timeinfo) && retry < 10) {
    delay(1000);
    retry++;
    Serial.print(".");
  }
  Serial.println();
  if (getLocalTime(&timeinfo)) {
    Serial.printf("[TIME] Jam sekarang: %02d:%02d:%02d\n",
      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  } else {
    Serial.println("[TIME] Gagal sinkron NTP, scheduler pakai jam ESP32");
  }
}

void connectWifi() {
  Serial.printf("[WIFI] Connect ke %s ...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WIFI] Connected! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WIFI] Gagal connect, coba lagi nanti di loop");
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n===== NOVIA HOME V2 - 4 CHANNEL =====");
  Serial.println("Depan, Tengah, Belakang, Kipas");

  relayInit();
  connectWifi();
  setupTime();
  blynkInit();
  webServerInit();

  Serial.println("[SYSTEM] Siap!");
  Serial.println("Blynk: V0 Depan, V1 Tengah, V2 Belakang, V3 Kipas");
  Serial.println("Web API: /api/status, /api/set, /api/schedule");
}

void loop() {
  // 1. Web Server lokal selalu jalan
  webServerHandle();

  // 2. Blynk kalau WiFi connect
  if (WiFi.status() == WL_CONNECTED) {
    blynkRun();
  }

  // 3. Reconnect WiFi tiap 10 detik kalau putus
  if (WiFi.status() != WL_CONNECTED && millis() - lastWifiCheck > 10000) {
    lastWifiCheck = millis();
    Serial.println("[WIFI] Reconnect...");
    WiFi.reconnect();
  }

  // 4. Scheduler cek tiap 1 menit / tiap ganti menit
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    int currentMinute = timeinfo.tm_hour * 60 + timeinfo.tm_min;
    if (currentMinute != lastCheckedMinute) {
      lastCheckedMinute = currentMinute;
      scheduleCheck(timeinfo.tm_hour, timeinfo.tm_min);

      // Sinkronkan status ke Blynk biar tombol di HP tidak ketinggalan
      // Hanya kalau ada perubahan scheduler, blynkUpdate dipanggil di sini
      for (int i = 0; i < CHANNEL_COUNT; i++) {
        blynkUpdateChannel(i);
      }
    }
  }

  delay(10);
}