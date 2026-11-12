#include "web_server.h"
#include "config.h"
#include "relay.h"
#include <WebServer.h>
#include <【entity-ArduinoJson¦canonical_name=Arduino】.h>

static WebServer server(WEB_PORT);

String getStatusJson() {
  JsonDocument doc;
  doc["time"] = "WIB";
  JsonArray ch = doc["channels"].to<JsonArray>();
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    JsonObject o = ch.add<JsonObject>();
    o["id"] = i;
    o["name"] = CHANNEL_NAMES[i];
    o["state"] = relayGet(i) ? 1 : 0;
    Schedule s = scheduleGet(i);
    o["sched_enabled"] = s.enabled ? 1 : 0;
    o["onHour"] = s.onHour;
    o["onMinute"] = s.onMinute;
    o["offHour"] = s.offHour;
    o["offMinute"] = s.offMinute;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

void handleRoot() {
  server.send(200, "text/plain", "Novia Home V2 - API OK. Gunakan /api/status");
}

void handleStatus() {
  server.send(200, "application/json", getStatusJson());
}

void handleSet() {
  if (!server.hasArg("ch") || !server.hasArg("state")) {
    server.send(400, "application/json", "{\"error\":\"ch & state wajib\"}");
    return;
  }
  int ch = server.arg("ch").toInt();
  int st = server.arg("state").toInt();
  if (ch < 0 || ch >= CHANNEL_COUNT) {
    server.send(400, "application/json", "{\"error\":\"channel tidak valid\"}");
    return;
  }
  relaySet(ch, st == 1);
  server.send(200, "application/json", getStatusJson());
}

void handleToggle() {
  if (!server.hasArg("ch")) {
    server.send(400, "application/json", "{\"error\":\"ch wajib\"}");
    return;
  }
  int ch = server.arg("ch").toInt();
  relayToggle(ch);
  server.send(200, "application/json", getStatusJson());
}

void handleAllOff() {
  relayAllOff();
  server.send(200, "application/json", getStatusJson());
}

void handleSchedule() {
  // Contoh: /api/schedule?ch=0&enabled=1&onH=18&onM=0&offH=6&offM=0
  if (!server.hasArg("ch")) {
    server.send(400, "application/json", "{\"error\":\"ch wajib\"}");
    return;
  }
  int ch = server.arg("ch").toInt();
  Schedule s = scheduleGet(ch);
  if (server.hasArg("enabled")) s.enabled = server.arg("enabled").toInt() == 1;
  if (server.hasArg("onH")) s.onHour = server.arg("onH").toInt();
  if (server.hasArg("onM")) s.onMinute = server.arg("onM").toInt();
  if (server.hasArg("offH")) s.offHour = server.arg("offH").toInt();
  if (server.hasArg("offM")) s.offMinute = server.arg("offM").toInt();

  scheduleSet(ch, s);
  server.send(200, "application/json", getStatusJson());
}

void webServerInit() {
  server.on("/", handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/set", HTTP_GET, handleSet);
  server.on("/api/toggle", HTTP_GET, handleToggle);
  server.on("/api/alloff", HTTP_GET, handleAllOff);
  server.on("/api/schedule", HTTP_GET, handleSchedule);
  server.begin();
  Serial.printf("[WEB] Server jalan di port %d\n", WEB_PORT);
}

void webServerHandle() {
  server.handleClient();
}