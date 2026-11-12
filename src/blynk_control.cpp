#include "blynk_control.h"
#include "config.h"
#include "relay.h"
#include "secrets.h"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

void blynkInit() {
  Blynk.config(BLYNK_AUTH_TOKEN);
}

void blynkRun() {
  Blynk.run();
}

void blynkSyncAll() {
  Blynk.syncVirtual(VPIN_DEPAN, VPIN_TENGAH, VPIN_BELAKANG, VPIN_KIPAS);
}

void blynkUpdateChannel(int channel) {
  bool st = relayGet(channel);
  switch (channel) {
    case CH_DEPAN: Blynk.virtualWrite(VPIN_DEPAN, st ? 1 : 0); break;
    case CH_TENGAH: Blynk.virtualWrite(VPIN_TENGAH, st ? 1 : 0); break;
    case CH_BELAKANG: Blynk.virtualWrite(VPIN_BELAKANG, st ? 1 : 0); break;
    case CH_KIPAS: Blynk.virtualWrite(VPIN_KIPAS, st ? 1 : 0); break;
  }
}

// ===== BLYNK HANDLER - Dari Aplikasi Blynk =====

BLYNK_CONNECTED() {
  Serial.println("[BLYNK] Connected, sync...");
  blynkSyncAll();
}

BLYNK_WRITE(VPIN_DEPAN) {
  int v = param.asInt();
  relaySet(CH_DEPAN, v == 1);
}

BLYNK_WRITE(VPIN_TENGAH) {
  int v = param.asInt();
  relaySet(CH_TENGAH, v == 1);
}

BLYNK_WRITE(VPIN_BELAKANG) {
  int v = param.asInt();
  relaySet(CH_BELAKANG, v == 1);
}

BLYNK_WRITE(VPIN_KIPAS) {
  int v = param.asInt();
  relaySet(CH_KIPAS, v == 1);
}