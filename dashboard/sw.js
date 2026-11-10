const CACHE = "novia-home-v3";
const ASSETS = ["/", "/style.css", "/app.js", "/manifest.json"];
self.addEventListener("install", e => e.waitUntil(caches.open(CACHE).then(c => c.addAll(ASSETS))));
self.addEventListener("activate", e => e.waitUntil(caches.keys().then(keys => Promise.all(keys.filter(k => k !== CACHE).map(k => caches.delete(k))))));
self.addEventListener("fetch", e => {
  const u = new URL(e.request.url);
  if (u.pathname.startsWith("/api/") || u.pathname === "/scan" || e.request.method !== "GET") return;
  e.respondWith(caches.match(e.request).then(cached => cached || fetch(e.request)));
});
