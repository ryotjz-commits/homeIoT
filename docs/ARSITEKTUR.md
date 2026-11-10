# Novia Home V3 — Arsitektur Hybrid

## Diagram alur

```text
Dashboard LOCAL (ESP32 LittleFS) ── HTTP ──┐
                                           ├── ESP32 ── 4 relay active-low
Dashboard CLOUD (Cloudflare Pages) ─ Worker ┘    ├── Blynk V0–V3
                                                 └── WiFi / Preferences
```

Dashboard LOCAL berbicara langsung ke ESP32. Dashboard CLOUD mengirim perintah ke antrean Cloudflare KV. ESP32 mengambil antrean setiap ±1,5 detik dan mengirim status setiap ±7 detik. Sinkronisasi cloud memerlukan koneksi internet di ESP32.

## Fungsi yang dipertahankan
- GPIO 26 = Lampu Luar, V0.
- GPIO 27 = Lampu Tengah, V1.
- GPIO 14 = Lampu Kamar, V2.
- GPIO 13 = Kipas, V3.
- Relay active-low; kondisi boot awal OFF.
- Blynk V0–V3, WiFi Manager AP, DNS captive portal, Preferences, WebServer, Basic Auth, WiFi scan, dan endpoint lokal.

## A. Persiapan firmware
1. Buka `firmware/` di komputer/Arduino IDE. File utama: `NoviaHome_V3.ino`.
2. Salin `config.h.example` menjadi `config.h` (jangan commit atau bagikan file ini).
3. Isi Blynk Template ID, Auth Token BARU, password AP, password dashboard.
4. Isi `CLOUD_API_BASE`, contoh `https://novia-home-api.nama-akun.workers.dev`. Jangan tambahkan `/` di akhir.
5. Isi `DEVICE_TOKEN` dengan secret yang sama dengan Cloudflare Worker. Gunakan token acak panjang (minimal 32 byte).
6. Isi `CLOUD_ROOT_CA` dengan PEM root CA yang memvalidasi sertifikat TLS domain Worker. Ambil chain sertifikat domain Worker dari browser/layanan pemeriksa sertifikat, lalu tempel root CA (bukan leaf certificate) di antara penanda PEM. Verifikasi TLS aktif; firmware tidak menggunakan `setInsecure()`.
7. Pasang library ArduinoJson 6.x dan Blynk (Blynk by Volodymyr Shymanskyy). Gunakan board package ESP32 yang sesuai.
8. Pilih partisi flash yang menyediakan LittleFS, compile, dan upload firmware.

> Jika root CA salah/kosong, koneksi cloud akan gagal dengan error TLS. Kontrol lokal dan Blynk tetap dapat berfungsi. Perbarui root CA sebelum sertifikat root kedaluwarsa/berubah.

## B. Upload dashboard ke LittleFS (akses lokal)
Upload ISI folder `dashboard/` ke root filesystem LittleFS. Struktur pada perangkat harus seperti ini:

```text
/index.html
/portal.html
/style.css
/app.js
/manifest.json
/sw.js
/icons/icon-192.png
/icons/icon-512.png
```

Jangan upload folder `dashboard` sebagai subfolder tambahan. Buka IP ESP32 dari browser; browser akan meminta Basic Auth. Kredensial default username `admin`, password sesuai `WEB_PASSWORD` di `config.h`.

## C. Siapkan Cloudflare Worker + KV
1. Masuk ke folder `cloudflare/`.
2. Buat KV namespace: `npx wrangler kv namespace create NOVIA_KV`.
3. Salin `id` hasil perintah ke `wrangler.toml`, menggantikan placeholder.
4. Buat dua secret yang berbeda dan kuat:
   - `npx wrangler secret put DEVICE_TOKEN`
   - `npx wrangler secret put DASHBOARD_TOKEN`
5. Isi secret `DEVICE_TOKEN` sama persis dengan `DEVICE_TOKEN` pada `config.h` firmware.
6. Deploy Worker: `npx wrangler deploy`.
7. Salin URL Worker yang muncul ke `CLOUD_API_BASE` pada firmware.

API Worker:
- `GET /api/device/poll` — device token; mengambil satu perintah antrean.
- `POST /api/device/state` — device token; memperbarui status.
- `GET /api/state` — dashboard token; membaca status terakhir.
- `POST /api/relay` — dashboard token; antrekan satu relay.
- `POST /api/all` — dashboard token; antrekan semua relay.

## D. Hosting dashboard cloud
1. Deploy isi folder `dashboard/` ke Cloudflare Pages, misalnya: `npx wrangler pages deploy ../dashboard --project-name novia-home-dashboard`.
2. Buka URL Pages.
3. Isi Worker URL dan `DASHBOARD_TOKEN` pada panel CLOUD CONNECTION, lalu tekan CONNECT CLOUD. Token hanya disimpan di `sessionStorage` pada sesi tab browser; tidak ditanam di file JavaScript.
4. ESP32 harus online dan berhasil terhubung ke Worker agar status berubah dan perintah cloud dijalankan.

**Catatan keamanan:** bearer token dashboard yang dimasukkan ke browser tetap dapat dilihat oleh pemilik sesi/perangkat browser dan melalui developer tools. Ini cocok untuk tahap uji terbatas, bukan deployment multi-pengguna/produksi. Untuk produksi, letakkan API di belakang autentikasi pengguna yang benar dan validasi identitas pengguna di Worker (misalnya verifikasi JWT Cloudflare Access), gunakan HTTPS, token terpisah, dan jangan membagikan token. CORS `*` bukan mekanisme autentikasi.

## E. Urutan pengujian
1. Uji boot ESP32 dan pastikan semua relay OFF.
2. Uji dashboard lokal: satu relay ON/OFF, ALL ON/OFF, WiFi status, Blynk status.
3. Uji Blynk V0–V3; pastikan dashboard lokal mengikuti perubahan.
4. Uji WiFi setup dengan SSID/password yang salah atau kosong.
5. Deploy Worker dan pastikan perangkat mengirim state.
6. Buka dashboard Pages, masukkan URL/token, tekan CONNECT CLOUD.
7. Uji satu relay dan ALL ON/OFF dari cloud. Tunggu hingga ±1–3 detik untuk polling dan KV propagation.
8. Putuskan internet ESP32: mode lokal seharusnya tetap berfungsi selama WiFi LAN tersedia; cloud tidak dapat mengendalikan perangkat sampai koneksi internet pulih.

## Batasan yang perlu diketahui
- Cloudflare KV bersifat eventual consistency, bukan kanal real-time deterministik. Perintah dapat tertunda beberapa detik.
- Antrean saat ini menyimpan satu perintah terbaru; beberapa pengguna yang menekan kontrol bersamaan dapat saling menimpa.
- Status cloud disimpan dengan TTL 120 detik; dashboard menandai perangkat stale/offline jika heartbeat lebih dari 20 detik.
- Basic Auth lokal adalah proteksi sederhana. Jangan port-forward WebServer ESP32 langsung ke internet.
- `LittleFS.begin(false)` dipakai agar kegagalan mount tidak otomatis memformat filesystem.
