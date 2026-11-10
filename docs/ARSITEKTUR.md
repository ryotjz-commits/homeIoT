# Novia Home V3 - Hybrid (fondasi proyek)

## Yang dipertahankan dari V2
- 4 relay active-low pada GPIO 26, 27, 14, 13
- Blynk V0-V3 dan sinkronisasi saat tersambung
- WiFi STA + AP setup, DNS captive portal
- Penyimpanan SSID/password WiFi melalui Preferences
- Basic authentication dashboard
- Endpoint lokal `/api/state`, `/api/relay`, `/api/all`, `/scan`

## Perubahan
- HTML, CSS, JavaScript dipisah di `dashboard/`
- Dashboard disajikan dari LittleFS
- Manifest + service worker untuk fondasi PWA
- Worker Cloudflare menyediakan antrean perintah remote berbasis KV

## Penting: status hybrid
Folder Cloudflare berisi API Worker untuk antrean perintah. Firmware V3 saat ini masih menjalankan seluruh fungsi lokal dan Blynk dari V2; integrasi polling Worker dari firmware dan pemilihan API cloud pada dashboard adalah tahap berikutnya sebelum akses cloud benar-benar aktif. Jangan menganggap deploy Worker saja sudah membuat kendali jarak jauh berfungsi.

## Build dan upload
1. Buka `firmware/NoviaHome_V3.ino` di Arduino IDE.
2. Salin `config.h.example` menjadi `config.h`, lalu isi kredensial baru.
3. Pasang library: Blynk, dan gunakan board package ESP32 yang sesuai.
4. Upload firmware ke ESP32.
5. Upload isi folder `dashboard/` ke partisi LittleFS (struktur root: `index.html`, `style.css`, `app.js`, `portal.html`, `manifest.json`, `sw.js`, `icons/`).
6. Pastikan partisi flash board menyediakan ruang LittleFS yang cukup.

## Keamanan
- Kredensial asli V2 tidak disalin ke paket.
- Segera rotasi Blynk Auth Token, password AP dan password dashboard karena nilai lama tertanam di source yang dibagikan.
- Ganti Basic Auth sebelum akses jaringan yang tidak dipercaya. Untuk cloud, gunakan HTTPS dan token kuat; jangan menaruh token dashboard di JavaScript publik untuk deployment produksi. Tambahkan autentikasi pengguna yang sesungguhnya sebelum membuka Worker ke internet.
