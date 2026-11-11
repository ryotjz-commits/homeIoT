const names = ["Lampu Luar", "Lampu Tengah", "Lampu Kamar", "Kipas"];
const icons = ["☀", "◉", "◉", "〰"];
let states = [false, false, false, false];
let busy = false;
let mode = "local";
let cloudBase = "";
let cloudToken = "";

const $ = id => document.getElementById(id);

function render() {
  $("devices").innerHTML = names.map((name, i) => `
    <div class="device">
      <div class="device-top">
        <span class="device-icon">${icons[i]}</span>
        <button class="switch ${states[i] ? "on" : ""}" onclick="toggle(${i})" aria-label="Toggle ${name}"></button>
      </div>
      <div class="device-name">${name}</div>
      <div class="device-state">${states[i] ? "Perangkat aktif" : "Perangkat mati"}</div>
    </div>`).join("");
  $("count").textContent = `${states.filter(Boolean).length}/4 Perangkat Aktif`;
}

function setMode(next) {
  mode = next;
  $("mode").textContent = next === "cloud" ? "CLOUD" : "LOCAL";
  $("status").textContent = next === "cloud" ? "CLOUD MODE" : "LOCAL MODE";
}

function cloudHeaders(json = false) {
  const headers = { Authorization: `Bearer ${cloudToken}` };
  if (json) headers["Content-Type"] = "application/json";
  return headers;
}

async function cloudFetch(path, options = {}) {
  const response = await fetch(`${cloudBase}${path}`, {
    ...options,
    headers: { ...cloudHeaders(!!options.body), ...(options.headers || {}) }
  });
  if (!response.ok) throw new Error(`Cloud API ${response.status}`);
  return response;
}

async function connectCloud() {
  const url = $("cloudUrl").value.trim().replace(/\/$/, "");
  const token = $("cloudToken").value.trim();
  if (!/^https:\/\//i.test(url) || token.length < 12) {
    $("status").textContent = "URL / TOKEN TIDAK VALID";
    return;
  }
  cloudBase = url;
  cloudToken = token;
  try {
    const response = await cloudFetch("/api/state");
    const data = await response.json();
    sessionStorage.setItem("noviaCloudUrl", cloudBase);
    sessionStorage.setItem("noviaCloudToken", cloudToken);
    setMode("cloud");
    applyState(data);
    $("status").textContent = "CLOUD CONNECTED";
    await update();
  } catch (error) {
    cloudToken = "";
    $("status").textContent = "CLOUD CONNECTION FAILED";
    console.error(error);
  }
}

function useLocal() {
  setMode("local");
  update();
}

function applyState(data) {
  if (Array.isArray(data.relays) && data.relays.length === 4) states = data.relays.map(Boolean);
  render();
  $("wifi").textContent = data.wifi ? "Connected" : "Disconnected";
  $("wifi").className = data.wifi ? "online" : "offline";
  $("blynk").textContent = data.blynk ? "Connected" : "Disconnected";
  $("blynk").className = data.blynk ? "online" : "offline";
  $("ip").textContent = data.ip || "-";
  if ($("cloud")) { $("cloud").textContent = data.cloud ? "Connected" : (mode === "cloud" ? "Connected" : "Not checked"); $("cloud").className = data.cloud || mode === "cloud" ? "online" : "offline"; }
  if (mode === "cloud") {
    const stale = !data.receivedAt || (Date.now() - data.receivedAt > 20000);
    $("status").textContent = stale ? "DEVICE OFFLINE / STALE" : (data.wifi ? "DEVICE ONLINE" : "DEVICE WIFI OFFLINE");
  } else $("status").textContent = data.wifi ? "SYSTEM ONLINE" : "WIFI OFFLINE";
}

async function command(ch, value) {
  if (busy) return;
  busy = true;
  try {
    if (mode === "cloud") {
      const path = ch === "all" ? "/api/all" : "/api/relay";
      const body = ch === "all" ? { value } : { ch, value };
      await cloudFetch(path, { method: "POST", body: JSON.stringify(body) });
      $("status").textContent = "COMMAND QUEUED";
    } else {
      const path = ch === "all" ? `/api/all?value=${value}` : `/api/relay?ch=${ch}&value=${value}`;
      const response = await fetch(path, { cache: "no-store" });
      if (!response.ok) throw new Error("Local command failed");
      await update();
    }
  } catch (error) {
    $("status").textContent = "COMMAND ERROR";
    console.error(error);
  } finally {
    busy = false;
  }
}

function toggle(i) { command(i, states[i] ? 0 : 1); }
function master(on) { command("all", on ? 1 : 0); }

async function update() {
  try {
    let data;
    if (mode === "cloud") {
      const response = await cloudFetch("/api/state");
      data = await response.json();
    } else {
      const response = await fetch("/api/state", { cache: "no-store" });
      if (!response.ok) throw new Error("Local offline");
      data = await response.json();
    }
    applyState(data);
  } catch (error) {
    $("status").textContent = mode === "cloud" ? "CLOUD / DEVICE OFFLINE" : "CONNECTION LOST";
    console.warn(error);
  }
}

render();
const savedUrl = sessionStorage.getItem("noviaCloudUrl");
const savedToken = sessionStorage.getItem("noviaCloudToken");
if (savedUrl) $("cloudUrl").value = savedUrl;
if (savedToken) $("cloudToken").value = savedToken;
update();
setInterval(update, 2500);
if ("serviceWorker" in navigator) navigator.serviceWorker.register("/sw.js").catch(console.warn);
