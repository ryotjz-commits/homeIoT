
/*
  ==========================================
       NOVIA HOME V3 - HYBRID SMART HOME
  ==========================================

  ESP32 + Blynk Cloud + WebServer
  WiFi Manager + Preferences + Authentication

  Relay:
  GPIO 26 = Lampu Luar   = V0
  GPIO 27 = Lampu Tengah = V1
  GPIO 14 = Lampu Kamar  = V2
  GPIO 13 = Kipas        = V3

  Relay logic: ACTIVE LOW
*/

#include "config.h"

#define BLYNK_TEMPLATE_ID BLYNK_TEMPLATE_ID_VALUE
#define BLYNK_TEMPLATE_NAME BLYNK_TEMPLATE_NAME_VALUE
#define BLYNK_AUTH_TOKEN BLYNK_AUTH_TOKEN_VALUE

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <BlynkSimpleEsp32.h>
#include <LittleFS.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <string.h>

// ==========================================
// CONFIGURATION
// ==========================================

const char* AP_NAME = "Novia-Home-Setup";
const char* AP_PASSWORD = Noke0719;

const char* WEB_USER = "admin";
const char* WEB_PASSWORD = Noke0710;

const uint8_t relayPins[4] = {
  26, 27, 14, 13
};

const char* deviceNames[4] = {
  "Lampu Luar",
  "Lampu Tengah",
  "Lampu Kamar",
  "Kipas"
};

// ==========================================
// OBJECTS
// ==========================================

WebServer server(80);
Preferences preferences;
DNSServer dnsServer;
BlynkTimer timer;

// ==========================================
// VARIABLES
// ==========================================

String wifiSSID;
String wifiPassword;

bool relayState[4] = {
  false, false, false, false
};

bool configMode = false;

unsigned long lastBlynkAttempt = 0;

const unsigned long reconnectInterval = 5000;

// Cloud sync timers. Keep cloud traffic independent from Blynk/local control.
unsigned long lastCloudPoll = 0;
unsigned long lastCloudState = 0;
const unsigned long cloudPollInterval = 1500;
const unsigned long cloudStateInterval = 7000;
const unsigned long cloudTimeoutMs = 1200;
bool cloudAvailable = false;

// ==========================================
// HTML DASHBOARD
// ==========================================



// ==========================================
// WIFI CONFIGURATION PORTAL
// ==========================================



// ==========================================
// AUTHENTICATION
// ==========================================

bool authenticate() {

  if (!server.authenticate(
    WEB_USER,
    WEB_PASSWORD
  )) {

    server.requestAuthentication();

    return false;
  }

  return true;
}

// ==========================================
// RELAY CONTROL
// ==========================================

void setRelay(
  uint8_t ch,
  bool state,
  bool sync = true
) {

  if(ch >= 4) return;

  relayState[ch] = state;

  digitalWrite(
    relayPins[ch],
    state ? LOW : HIGH
  );

  if(sync && Blynk.connected()) {

    Blynk.virtualWrite(
      ch,
      state ? 1 : 0
    );

  }

}

void allRelays(bool state) {

  for(uint8_t i=0;i<4;i++) {

    setRelay(i,state);

  }

}

// ==========================================
// BLYNK SYNCHRONIZATION
// ==========================================

BLYNK_CONNECTED() {

  Serial.println("Blynk Connected");

  Blynk.syncVirtual(
    V0,V1,V2,V3
  );

}

BLYNK_WRITE(V0) {
  setRelay(0,param.asInt()==1,false);
}

BLYNK_WRITE(V1) {
  setRelay(1,param.asInt()==1,false);
}

BLYNK_WRITE(V2) {
  setRelay(2,param.asInt()==1,false);
}

BLYNK_WRITE(V3) {
  setRelay(3,param.asInt()==1,false);
}

// ==========================================
// CLOUDFLARE HYBRID API
// ==========================================

String cloudUrl(const char* path) {
  String base = String(CLOUD_API_BASE);
  while (base.endsWith("/")) base.remove(base.length() - 1);
  return base + path;
}

bool cloudRequest(const String& method, const char* path, const String& body, String* response = nullptr) {
  if (!WiFi.isConnected() || String(CLOUD_API_BASE).length() < 10 || String(DEVICE_TOKEN).length() < 8) return false;

  WiFiClientSecure client;
  // Certificate validation is mandatory. Put the issuing root CA PEM in config.h.
  client.setCACert(CLOUD_ROOT_CA);
  HTTPClient http;
  http.setTimeout(cloudTimeoutMs);
  if (!http.begin(client, cloudUrl(path))) return false;
  http.addHeader("Authorization", String("Bearer ") + DEVICE_TOKEN);
  if (method == "POST") http.addHeader("Content-Type", "application/json");

  int code = (method == "POST") ? http.POST(body) : http.GET();
  bool ok = code >= 200 && code < 300;
  if (response && code > 0) *response = http.getString();
  if (!ok) {
    Serial.printf("Cloud API %s %s failed: %d\n", method.c_str(), path, code);
  }
  http.end();
  return ok;
}

void publishCloudState() {
  StaticJsonDocument<256> doc;
  JsonArray relays = doc.createNestedArray("relays");
  for (uint8_t i = 0; i < 4; i++) relays.add(relayState[i]);
  doc["wifi"] = WiFi.isConnected();
  doc["blynk"] = Blynk.connected();
  doc["ip"] = WiFi.isConnected() ? WiFi.localIP().toString() : String("");
  doc["device"] = "NoviaHome-ESP32";
  doc["uptime"] = millis() / 1000;
  String body;
  serializeJson(doc, body);
  cloudAvailable = cloudRequest("POST", "/api/device/state", body);
}

void pollCloudCommands() {
  String response;
  if (!cloudRequest("GET", "/api/device/poll", "", &response)) {
    cloudAvailable = false;
    return;
  }
  cloudAvailable = true;
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, response);
  if (error || doc["command"].isNull()) return;

  JsonObject command = doc["command"].as<JsonObject>();
  const char* type = command["type"] | "";
  if (strcmp(type, "relay") == 0) {
    int ch = command["ch"] | -1;
    int value = command["value"] | -1;
    if (ch >= 0 && ch < 4 && (value == 0 || value == 1)) {
      setRelay((uint8_t)ch, value == 1, true);
    }
  } else if (strcmp(type, "all") == 0) {
    int value = command["value"] | -1;
    if (value == 0 || value == 1) allRelays(value == 1);
  }
}

void serviceCloud() {
  if (configMode || !WiFi.isConnected()) return;
  unsigned long now = millis();
  if (now - lastCloudPoll >= cloudPollInterval) {
    lastCloudPoll = now;
    pollCloudCommands();
  }
  if (now - lastCloudState >= cloudStateInterval) {
    lastCloudState = now;
    publishCloudState();
  }
}

// ==========================================
// JSON ESCAPE
// ==========================================

String jsonEscape(String value) {

  value.replace("\\","\\\\");
  value.replace("\"","\\\"");
  value.replace("\n","\\n");
  value.replace("\r","\\r");
  value.replace("\t","\\t");

  return value;

}

// ==========================================
// WIFI SCANNER
// ==========================================

void handleScan() {

  if(!authenticate()) return;

  int n = WiFi.scanNetworks();

  String json = "[";

  bool first = true;

  for(int i=0;i<n;i++) {

    String name = WiFi.SSID(i);

    if(name.length()==0) continue;

    if(!first) json += ",";

    first = false;

    json += "{\"ssid\":\"";
    json += jsonEscape(name);

    json += "\",\"rssi\":";
    json += String(WiFi.RSSI(i));

    json += "}";

  }

  json += "]";

  WiFi.scanDelete();

  server.sendHeader(
    "Cache-Control",
    "no-store"
  );

  server.send(
    200,
    "application/json",
    json
  );

}


// Serve dashboard assets from LittleFS. Files are stored in /dashboard.
String contentType(const String& path) {
  if (path.endsWith(".html")) return "text/html";
  if (path.endsWith(".css")) return "text/css";
  if (path.endsWith(".js")) return "application/javascript";
  if (path.endsWith(".json")) return "application/json";
  if (path.endsWith(".png")) return "image/png";
  if (path.endsWith(".svg")) return "image/svg+xml";
  return "text/plain";
}

void serveDashboard() {
  String path = configMode ? "/portal.html" : "/index.html";
  if (!LittleFS.exists(path)) {
    server.send(500, "text/plain", "Dashboard files missing. Upload the dashboard folder to LittleFS.");
    return;
  }
  File file = LittleFS.open(path, "r");
  server.streamFile(file, contentType(path));
  file.close();
}

void handleStaticFile() {
  if (!authenticate()) return;
  String path = server.uri();
  if (path == "/") path = "/index.html";
  if (path.indexOf("..") >= 0) {
    server.send(400, "text/plain", "Bad path");
    return;
  }
  if (configMode) path = "/portal.html";
  if (!LittleFS.exists(path)) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  File file = LittleFS.open(path, "r");
  server.streamFile(file, contentType(path));
  file.close();
}

// ==========================================
// WEB SERVER ROUTES
// ==========================================

void setupRoutes() {

  server.on("/",HTTP_GET,[]() {

    if(!authenticate()) return;

    serveDashboard();

  });

  server.on("/scan",HTTP_GET,handleScan);
  server.on("/style.css", HTTP_GET, handleStaticFile);
  server.on("/app.js", HTTP_GET, handleStaticFile);
  server.on("/manifest.json", HTTP_GET, handleStaticFile);
  server.on("/sw.js", HTTP_GET, handleStaticFile);
  server.on("/icons/icon-192.png", HTTP_GET, handleStaticFile);
  server.on("/icons/icon-512.png", HTTP_GET, handleStaticFile);

  server.on("/save",HTTP_POST,[]() {

    if(!authenticate()) return;

    String newSSID = server.arg("ssid");
    String newPass = server.arg("password");

    if(
      newSSID.length()==0 ||
      newSSID.length()>32 ||
      newPass.length()>63
    ) {

      server.send(
        400,
        "text/plain",
        "Invalid WiFi Settings"
      );

      return;

    }

    preferences.begin("novia",false);

    preferences.putString(
      "ssid",
      newSSID
    );

    preferences.putString(
      "pass",
      newPass
    );

    preferences.end();

    server.send(
      200,
      "text/html",
      "<h2>Konfigurasi tersimpan. ESP32 restart...</h2>"
    );

    delay(1200);

    ESP.restart();

  });

  server.on("/api/state",HTTP_GET,[]() {

    if(configMode) {
      server.send(503,"text/plain","Setup Mode");
      return;
    }

    if(!authenticate()) return;

    String json = "{\"relays\":[";

    for(int i=0;i<4;i++) {

      json += relayState[i] ? "true" : "false";

      if(i<3) json += ",";

    }

    json += "],\"wifi\":";
    json += WiFi.isConnected() ? "true" : "false";

    json += ",\"blynk\":";
    json += Blynk.connected() ? "true" : "false";

    json += ",\"ip\":\"";
    json += WiFi.localIP().toString();
    json += "\",\"cloud\":";
    json += cloudAvailable ? "true" : "false";
    json += "}";

    server.sendHeader(
      "Cache-Control",
      "no-store"
    );

    server.send(
      200,
      "application/json",
      json
    );

  });

  server.on("/api/relay",HTTP_GET,[]() {

    if(!authenticate()) return;

    if(
      !server.hasArg("ch") ||
      !server.hasArg("value")
    ) {

      server.send(400,"text/plain","Missing Parameters");
      return;

    }

    int ch = server.arg("ch").toInt();
    int value = server.arg("value").toInt();

    if(
      ch<0 || ch>3 ||
      (value!=0 && value!=1)
    ) {

      server.send(400,"text/plain","Invalid Parameters");
      return;

    }

    setRelay(ch,value==1);

    server.send(
      200,
      "text/plain",
      "OK"
    );

  });

  server.on("/api/all",HTTP_GET,[]() {

    if(!authenticate()) return;

    if(!server.hasArg("value")) {

      server.send(400,"text/plain","Missing Value");
      return;

    }

    int value = server.arg("value").toInt();

    if(value!=0 && value!=1) {

      server.send(400,"text/plain","Invalid Value");
      return;

    }

    allRelays(value==1);

    server.send(
      200,
      "text/plain",
      "OK"
    );

  });

  server.onNotFound([]() {

    if(configMode) {

      server.sendHeader(
        "Location",
        "http://192.168.4.1/",
        true
      );

      server.send(302,"text/plain","");

    } else {

      server.send(
        404,
        "text/plain",
        "Not Found"
      );

    }

  });

  server.begin();

  Serial.println("WebServer Started");

}

// ==========================================
// START CONFIGURATION ACCESS POINT
// ==========================================

void startConfigPortal() {

  configMode = true;

  WiFi.mode(WIFI_AP_STA);

  WiFi.softAP(
    AP_NAME,
    AP_PASSWORD
  );

  dnsServer.start(
    53,
    "*",
    WiFi.softAPIP()
  );

  Serial.println("CONFIGURATION MODE");

  Serial.print("AP IP: ");

  Serial.println(
    WiFi.softAPIP()
  );

}

// ==========================================
// WIFI INITIALIZATION
// ==========================================

void connectWiFi() {

  preferences.begin("novia",true);

  wifiSSID = preferences.getString(
    "ssid",
    ""
  );

  wifiPassword = preferences.getString(
    "pass",
    ""
  );

  preferences.end();

  if(wifiSSID.length()==0) {

    startConfigPortal();

    return;

  }

  WiFi.mode(WIFI_STA);

  WiFi.begin(
    wifiSSID.c_str(),
    wifiPassword.c_str()
  );

  Serial.print("Connecting WiFi");

  unsigned long start = millis();

  while(
    WiFi.status()!=WL_CONNECTED &&
    millis()-start<15000
  ) {

    delay(300);

    Serial.print(".");

  }

  if(WiFi.status()==WL_CONNECTED) {

    Serial.println("\nWiFi Connected");

    Serial.print("IP: ");

    Serial.println(
      WiFi.localIP()
    );

  } else {

    Serial.println("\nWiFi Failed");

    startConfigPortal();

  }

}

// ==========================================
// BLYNK RECONNECT
// ==========================================

void reconnectBlynk() {

  if(
    configMode ||
    !WiFi.isConnected() ||
    Blynk.connected()
  ) return;

  if(
    millis()-lastBlynkAttempt >=
    reconnectInterval
  ) {

    lastBlynkAttempt = millis();

    Serial.println("Reconnecting Blynk...");

    Blynk.connect(1000);

  }

}

// ==========================================
// SETUP
// ==========================================

void setup() {

  Serial.begin(115200);

  Serial.println();
  Serial.println("======================");
  Serial.println(" NOVIA HOME V3 HYBRID");
  Serial.println("======================");

  // Initialize relay OFF

  for(int i=0;i<4;i++) {

    digitalWrite(
      relayPins[i],
      HIGH
    );

    pinMode(
      relayPins[i],
      OUTPUT
    );

  }

  if (!LittleFS.begin(false)) {
    Serial.println("LittleFS mount failed");
  }

  connectWiFi();

  // Blynk initialization

  Blynk.config(
    BLYNK_AUTH_TOKEN
  );

  if(WiFi.isConnected()) {

    Blynk.connect(1000);

  }

  setupRoutes();

  Serial.println("SYSTEM READY");

}

// ==========================================
// MAIN LOOP
// ==========================================

void loop() {

  if(configMode) {

    dnsServer.processNextRequest();

  }

  server.handleClient();

  if(!configMode) {

    if(WiFi.isConnected()) {

      if(Blynk.connected()) {

        Blynk.run();

      } else {

        reconnectBlynk();

      }

    }

  }

  serviceCloud();

  timer.run();

}
