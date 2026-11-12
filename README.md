
# 🏠 Novia Home V2

Control 3 Lampu + 1 Kipas Angin pakai ESP32, bisa dari HP lewat **Blynk** dan **Web Dashboard Lokal**.

Edit dari HP Android pakai Acode → Push ke 【entity-GitHub¦canonical_name=GitHub】 → Compile otomatis pakai 【entity-GitHub¦canonical_name=GitHub】 Actions → Download `.bin` → Flash ke ESP32 pakai USB OTG.

> Dibuat untuk ESP32 DevKit V1, tanpa perlu laptop / PC.

---

## ✨ Fitur

- [x] 4 Channel Relay: Lampu Depan, Lampu Tengah, Lampu Belakang, Kipas Angin
- [x] Control dari Blynk (V0 - V3)
- [x] Control dari Web Dashboard lokal (HTML / CSS / JS)
- [x] Scheduler ON / OFF per channel, jam WIB (NTP)
- [x] Scheduler bisa lewat tengah malam, contoh: 18:00 - 06:00
- [x] Status terakhir tersimpan (Preferences), kalau listrik mati tidak reset acak
- [x] Jadwal tersimpan permanen di ESP32
- [x] Auto reconnect WiFi
- [x] Build otomatis pakai 【entity-GitHub¦canonical_name=GitHub】 Actions + PlatformIO

---

## 🛠️ Hardware yang Dibutuhkan

| Komponen | Jumlah | Keterangan |
| :--- | :---: | :--- |
| ESP32 DevKit V1 | 1 | Board utama |
| Modul Relay 4 Channel 5V | 1 | Aktif LOW, minimal 10A |
| Adaptor 5V 2A | 1 | Power ESP32 + Relay |
| Kabel Jumper | Secukupnya | |
| Box Panel / Terminal | 1 | Untuk instalasi 220V |
| Kabel USB OTG + Kabel Data | 1 | Untuk flashing dari HP |

> ⚠️ **Peringatan Listrik 220V:**
> Putuskan kabel **FASA / LIVE**, jangan kabel Netral.
> Kalau belum pernah instalasi listrik, tes dulu pakai lampu DC / USB, baru pindah ke 220V setelah semua channel jalan.

---

## 🔌 Wiring

### Relay ke ESP32

| Relay | Pin ESP32 | Channel | Blynk |
| :---: | :---: | :--- | :---: |
| IN1 | GPIO 26 | 0 - Lampu Depan | V0 |
| IN2 | GPIO 25 | 1 - Lampu Tengah | V1 |
| IN3 | GPIO 33 | 2 - Lampu Belakang | V2 |
| IN4 | GPIO 32 | 3 - Kipas Angin | V3 |



