#include "relay.h"
#include <Preferences.h>

static const int relayPins[CHANNEL_COUNT] = {
  RELAY_DEPAN,
  RELAY_TENGAH,
  RELAY_BELAKANG,
  RELAY_KIPAS
};

static bool relayState[CHANNEL_COUNT] = {false, false, false, false};
static Schedule schedules[CHANNEL_COUNT];
static Preferences prefs;

void relayInit() {
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], RELAY_OFF);
  }
  // Load status terakhir + jadwal
  relayLoadState();
  scheduleLoad();

  // Terapkan status terakhir
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    digitalWrite(relayPins[i], relayState[i] ? RELAY_ON : RELAY_OFF);
    Serial.printf("[RELAY] %s -> %s\n", CHANNEL_NAMES[i], relayState[i] ? "ON" : "OFF");
  }
}

void relaySet(int channel, bool on) {
  if (channel < 0 || channel >= CHANNEL_COUNT) return;
  relayState[channel] = on;
  digitalWrite(relayPins[channel], on ? RELAY_ON : RELAY_OFF);
  relaySaveState();
  Serial.printf("[RELAY] %s -> %s\n", CHANNEL_NAMES[channel], on ? "ON" : "OFF");
}

void relayToggle(int channel) {
  if (channel < 0 || channel >= CHANNEL_COUNT) return;
  relaySet(channel, !relayState[channel]);
}

bool relayGet(int channel) {
  if (channel < 0 || channel >= CHANNEL_COUNT) return false;
  return relayState[channel];
}

void relayAllOff() {
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    relaySet(i, false);
  }
}

void relaySaveState() {
  prefs.begin("novia-home", false);
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    String key = "state" + String(i);
    prefs.putBool(key.c_str(), relayState[i]);
  }
  prefs.end();
}

void relayLoadState() {
  prefs.begin("novia-home", true);
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    String key = "state" + String(i);
    relayState[i] = prefs.getBool(key.c_str(), false);
  }
  prefs.end();
}

// ===== SCHEDULER =====

void scheduleInit() {
  // Default jadwal biar tidak kosong
  // Depan: 17:30 - 05:30
  schedules[CH_DEPAN] = {true, 17, 30, 5, 30};
  // Tengah: 18:00 - 22:00
  schedules[CH_TENGAH] = {true, 18, 0, 22, 0};
  // Belakang: 18:00 - 06:00
  schedules[CH_BELAKANG] = {true, 18, 0, 6, 0};
  // Kipas: mati default, user atur sendiri
  schedules[CH_KIPAS] = {false, 19, 0, 22, 0};
}

void scheduleSave() {
  prefs.begin("novia-sched", false);
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    String p = "s" + String(i);
    prefs.putBool((p + "_en").c_str(), schedules[i].enabled);
    prefs.putInt((p + "_onH").c_str(), schedules[i].onHour);
    prefs.putInt((p + "_onM").c_str(), schedules[i].onMinute);
    prefs.putInt((p + "_offH").c_str(), schedules[i].offHour);
    prefs.putInt((p + "_offM").c_str(), schedules[i].offMinute);
  }
  prefs.end();
}

void scheduleLoad() {
  scheduleInit(); // isi default dulu
  prefs.begin("novia-sched", true);
  // Cek apakah sudah pernah disimpan
  if (!prefs.isKey("s0_en")) {
    prefs.end();
    scheduleSave();
    return;
  }
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    String p = "s" + String(i);
    schedules[i].enabled = prefs.getBool((p + "_en").c_str(), schedules[i].enabled);
    schedules[i].onHour = prefs.getInt((p + "_onH").c_str(), schedules[i].onHour);
    schedules[i].onMinute = prefs.getInt((p + "_onM").c_str(), schedules[i].onMinute);
    schedules[i].offHour = prefs.getInt((p + "_offH").c_str(), schedules[i].offHour);
    schedules[i].offMinute = prefs.getInt((p + "_offM").c_str(), schedules[i].offMinute);
  }
  prefs.end();
}

Schedule scheduleGet(int channel) {
  if (channel < 0 || channel >= CHANNEL_COUNT) return Schedule();
  return schedules[channel];
}

void scheduleSet(int channel, Schedule s) {
  if (channel < 0 || channel >= CHANNEL_COUNT) return;
  schedules[channel] = s;
  scheduleSave();
}

// Cek tiap menit dari loop utama
void scheduleCheck(int hour, int minute) {
  int nowMin = hour * 60 + minute;
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    if (!schedules[i].enabled) continue;

    int onMin = schedules[i].onHour * 60 + schedules[i].onMinute;
    int offMin = schedules[i].offHour * 60 + schedules[i].offMinute;

    bool shouldOn;
    if (onMin < offMin) {
      // Jadwal normal, misal 18:00 - 22:00
      shouldOn = (nowMin >= onMin && nowMin < offMin);
    } else if (onMin > offMin) {
      // Jadwal lewat tengah malam, misal 18:00 - 06:00
      shouldOn = (nowMin >= onMin || nowMin < offMin);
    } else {
      shouldOn = false;
    }

    if (relayGet(i) != shouldOn) {
      Serial.printf("[SCHED] %s auto -> %s (jam %02d:%02d)\n",
        CHANNEL_NAMES[i], shouldOn ? "ON" : "OFF", hour, minute);
      relaySet(i, shouldOn);
      // Update Blynk biar sinkron
      // Dipanggil dari blynk_control via callback, dihindari loop di sini
    }
  }
}