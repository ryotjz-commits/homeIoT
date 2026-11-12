// ===== CONFIG =====
let ESP_IP = localStorage.getItem('novia_esp_ip') || '';
let channels = []; // diisi dari /api/status
let isLoading = false;

// Info tiap channel: harus sama urutannya dengan firmware
const CHANNEL_META = [
  { id: 0, icon: '💡', subtitle: 'Lampu Depan • Blynk V0 • Relay 1' },
  { id: 1, icon: '💡', subtitle: 'Lampu Tengah • Blynk V1 • Relay 2' },
  { id: 2, icon: '💡', subtitle: 'Lampu Belakang • Blynk V2 • Relay 3' },
  { id: 3, icon: '🌀', subtitle: 'Kipas Angin • Blynk V3 • Relay 4' },
];

// ===== HELPER =====
function apiUrl(path) {
  if (!ESP_IP) return null;
  let ip = ESP_IP.replace('http://', '').replace('https://', '').replace(/\/$/, '');
  return `http://${ip}${path}`;
}

function showToast(msg) {
  const t = document.getElementById('toast');
  t.textContent = msg;
  t.classList.add('show');
  setTimeout(() => t.classList.remove('show'), 2500);
}

function pad(n) { return String(n).padStart(2, '0'); }

function timeToInput(hour, minute) {
  return `${pad(hour)}:${pad(minute)}`;
}

function inputToTime(val) {
  const [h, m] = val.split(':').map(Number);
  return { h: h || 0, m: m || 0 };
}

// ===== IP HANDLING =====
function saveIP() {
  const val = document.getElementById('espIP').value.trim();
  if (!val) { showToast('Isi IP ESP32 dulu'); return; }
  ESP_IP = val;
  localStorage.setItem('novia_esp_ip', val);
  showToast('IP disimpan: ' + val);
  refreshStatus();
}

function changeIP() {
  document.getElementById('espIP').focus();
  showToast('Ubah IP lalu tekan Simpan');
}

// ===== RENDER CARDS =====
function renderCards() {
  const container = document.getElementById('cardsContainer');
  if (!channels.length) {
    container.innerHTML = '<p class="loading">Belum ada data. Cek IP ESP32 & WiFi.</p>';
    return;
  }

  container.innerHTML = channels.map(ch => {
    const meta = CHANNEL_META[ch.id] || CHANNEL_META[0];
    const isOn = ch.state === 1;
    const schedOn = ch.sched_enabled === 1;

    return `
    <section class="card device-card ${isOn ? 'on' : ''}">
      <div class="card-top">
        <div class="icon">${meta.icon}</div>
        <div>
          <h2>${ch.name}</h2>
          <p>${meta.subtitle}</p>
        </div>
        <span class="badge ${isOn ? 'on' : ''}">${isOn ? '● ON' : '○ OFF'}</span>
      </div>

      <div class="toggle-row">
        <span>Status: <b>${isOn ? 'Menyala' : 'Mati'}</b></span>
        <label class="switch">
          <input type="checkbox" ${isOn ? 'checked' : ''} onchange="toggleChannel(${ch.id}, this.checked)" />
          <span class="slider"></span>
        </label>
      </div>

      <div class="divider"></div>

      <div class="sched-head">
        <h3>⏰ Jadwal Otomatis</h3>
        <label>
          <input type="checkbox" ${schedOn ? 'checked' : ''} onchange="toggleSchedEnabled(${ch.id}, this.checked)" />
          Aktif
        </label>
      </div>

      <div class="time-grid">
        <div class="time-box">
          <label>Jam ON</label>
          <input type="time" id="on-${ch.id}" value="${timeToInput(ch.onHour, ch.onMinute)}" />
        </div>
        <div class="time-box">
          <label>Jam OFF</label>
          <input type="time" id="off-${ch.id}" value="${timeToInput(ch.offHour, ch.offMinute)}" />
        </div>
      </div>

      <button class="btn-save" onclick="saveSchedule(${ch.id})">Simpan Jadwal ${ch.name}</button>
    </section>
    `;
  }).join('');
}

// ===== API CALLS =====
async function refreshStatus() {
  if (!ESP_IP) {
    document.getElementById('cardsContainer').innerHTML =
      '<p class="loading">Masukkan IP ESP32 dulu di atas, contoh 192.168.1.105</p>';
    return;
  }
  if (isLoading) return;
  isLoading = true;

  try {
    const res = await fetch(apiUrl('/api/status'), { signal: AbortSignal.timeout(5000) });
    if (!res.ok) throw new Error('HTTP ' + res.status);
    const data = await res.json();

    // Data dari firmware: { channels: [...] }
    channels = data.channels || [];
    renderCards();

    document.getElementById('connStatus').classList.add('online');
    document.getElementById('connStatus').style.color = '#2ed573';
    document.getElementById('lastUpdate').textContent = 'Update: ' + new Date().toLocaleTimeString('id-ID');

    // Kalau firmware kirim jam, tampilkan. Kalau tidak, pakai jam HP
    if (data.time) {
      document.getElementById('espTime').textContent = 'Jam: ' + data.time;
    }
  } catch (e) {
    console.error(e);
    document.getElementById('connStatus').classList.remove('online');
    document.getElementById('connStatus').style.color = '#ff4757';
    showToast('Gagal connect ke ESP32. Cek IP & WiFi');
  } finally {
    isLoading = false;
  }
}

async function toggleChannel(ch, checked) {
  try {
    const res = await fetch(apiUrl(`/api/set?ch=${ch}&state=${checked ? 1 : 0}`));
    const data = await res.json();
    channels = data.channels || channels;
    renderCards();
    showToast((CHANNEL_META[ch].subtitle.split('•')[0] || '') + (checked ? ' ON' : ' OFF'));
  } catch (e) {
    showToast('Gagal ubah status');
    refreshStatus();
  }
}

async function allOff() {
  if (!confirm('Matikan semua lampu + kipas?')) return;
  try {
    const res = await fetch(apiUrl('/api/alloff'));
    const data = await res.json();
    channels = data.channels || channels;
    renderCards();
    showToast('Semua dimatikan');
  } catch (e) {
    showToast('Gagal matikan semua');
  }
}

async function toggleSchedEnabled(ch, enabled) {
  // Simpan langsung biar tidak perlu tekan tombol Simpan 2x
  await saveSchedule(ch, enabled);
}

async function saveSchedule(ch, forcedEnabled = null) {
  const onVal = document.getElementById(`on-${ch}`).value;
  const offVal = document.getElementById(`off-${ch}`).value;
  if (!onVal || !offVal) { showToast('Isi Jam ON & OFF dulu'); return; }

  const on = inputToTime(onVal);
  const off = inputToTime(offVal);

  // Cari status enabled saat ini
  let enabled = forcedEnabled;
  if (enabled === null) {
    const chData = channels.find(c => c.id === ch);
    enabled = chData ? chData.sched_enabled === 1 : true;
    // Kalau user klik tombol Simpan, anggap dia mau aktifkan
    // Tapi kalau checkbox di-uncheck sebelumnya, forcedEnabled sudah handle
  }

  const url = `/api/schedule?ch=${ch}&enabled=${enabled ? 1 : 0}&onH=${on.h}&onM=${on.m}&offH=${off.h}&offM=${off.m}`;

  try {
    const res = await fetch(apiUrl(url));
    const data = await res.json();
    channels = data.channels || channels;
    renderCards();
    showToast(`Jadwal ${channels[ch]?.name || ''} disimpan: ${onVal} - ${offVal}`);
  } catch (e) {
    showToast('Gagal simpan jadwal');
  }
}

// ===== INIT =====
document.addEventListener('DOMContentLoaded', () => {
  if (ESP_IP) {
    document.getElementById('espIP').value = ESP_IP;
  }
  refreshStatus();
  // Auto refresh tiap 10 detik
  setInterval(refreshStatus, 10000);
});