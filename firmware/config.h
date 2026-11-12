#pragma once

// ===== RELAY PIN - ESP32 DevKit V1 =====
#define RELAY_DEPAN    26
#define RELAY_TENGAH   25
#define RELAY_BELAKANG 33
#define RELAY_KIPAS    32

// Relay aktif LOW
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// Jumlah channel
#define CHANNEL_COUNT 4

// Channel index
#define CH_DEPAN    0
#define CH_TENGAH   1
#define CH_BELAKANG 2
#define CH_KIPAS    3

// Nama channel untuk dashboard / serial
static const char* CHANNEL_NAMES[CHANNEL_COUNT] = {
  "Lampu Depan",
  "Lampu Tengah",
  "Lampu Belakang",
  "Kipas Angin"
};

// Blynk Virtual Pin
#define VPIN_DEPAN    V0
#define VPIN_TENGAH   V1
#define VPIN_BELAKANG V2
#define VPIN_KIPAS    V3

// NTP WIB
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 25200 // GMT+7
#define DAYLIGHT_OFFSET_SEC 0

// Web Server Port
#define WEB_PORT 80
