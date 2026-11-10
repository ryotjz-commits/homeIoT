const names = [
 "Lampu Luar",
 "Lampu Tengah",
 "Lampu Kamar",
 "Kipas"
];

const icons = ["☀","◉","◉","〰"];

let states = [false,false,false,false];
let busy = false;

function render() {

 document.getElementById("devices").innerHTML =
 names.map((name,i) => `

 <div class="device">

  <div class="device-top">

   <span class="device-icon">
    ${icons[i]}
   </span>

   <button
    class="switch ${states[i]?'on':''}"
    onclick="toggle(${i})"
    aria-label="Toggle ${name}">
   </button>

  </div>

  <div class="device-name">${name}</div>

  <div class="device-state">
   ${states[i]?'Perangkat aktif':'Perangkat mati'}
  </div>

 </div>

 `).join("");

}

async function command(url) {

 if(busy) return;

 busy = true;

 try {

  const r = await fetch(url);

  if(!r.ok) throw Error("Request failed");

  await update();

 } catch(e) {

  console.log(e);
  document.getElementById("status").textContent =
   "COMMAND ERROR";

 } finally {

  busy = false;

 }

}

async function toggle(i) {

 await command(
  `/api/relay?ch=${i}&value=${states[i]?0:1}`
 );

}

async function master(on) {

 await command(
  `/api/all?value=${on?1:0}`
 );

}

async function update() {

 try {

  const r = await fetch("/api/state");

  if(!r.ok) throw Error("Offline");

  const d = await r.json();

  states = d.relays;

  render();

  document.getElementById("wifi").textContent =
   d.wifi ? "Connected" : "Disconnected";

  document.getElementById("wifi").className =
   d.wifi ? "online" : "offline";

  document.getElementById("blynk").textContent =
   d.blynk ? "Connected" : "Disconnected";

  document.getElementById("blynk").className =
   d.blynk ? "online" : "offline";

  document.getElementById("ip").textContent =
   d.ip;

  document.getElementById("status").textContent =
   d.wifi ? "SYSTEM ONLINE" : "WIFI OFFLINE";

 } catch(e) {

  document.getElementById("status").textContent =
   "CONNECTION LOST";

 }

}

render();
update();

setInterval(update,2000);
\nif ("serviceWorker" in navigator) navigator.serviceWorker.register("/sw.js").catch(()=>{});\n