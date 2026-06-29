#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEScan.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "secrets.h"

// ── Temperature sensor ────────────────────────────
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ── BLE ───────────────────────────────────────────
#define SERVICE_UUID        "180C"
#define CHARACTERISTIC_UUID "2A56"
BLEClient* pClient = nullptr;
BLERemoteCharacteristic* pChar = nullptr;

// ── State ─────────────────────────────────────────
bool motionDetected = false;
bool plugOn = true;
float temperature = 0.0;
unsigned long lastTemp = 0;
unsigned long lastMotion = 0;
const unsigned long TEMP_INTERVAL = 5000;    // measure every 5 seconds
const unsigned long MOTION_TIMEOUT = 10000;  // 10 sec after motion -> plug on

// ── Hue control ───────────────────────────────────
void setPlug(bool on) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = "http://" + String(HUE_BRIDGE) + "/api/" +
               String(HUE_USERNAME) + "/lights/" +
               String(HUE_LIGHT_ID) + "/state";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  String body = on ? "{\"on\":true}" : "{\"on\":false}";
  http.PUT(body);
  http.end();

  plugOn = on;
  Serial.println(on ? "Plug ON" : "Plug OFF");
}

// ── BLE callback ──────────────────────────────────
void motionCallback(BLERemoteCharacteristic* pChar,
                    uint8_t* pData, size_t length, bool isNotify) {
  motionDetected = (pData[0] == 1);
  if (motionDetected) {
    lastMotion = millis();
    Serial.println("Motion received from Nano!");
  }
}

// ── BLE connect ───────────────────────────────────
bool connectToNano() {
  Serial.println("Searching for NanoBewegung...");
  BLEScan* pScan = BLEDevice::getScan();
  pScan->setActiveScan(true);
  BLEScanResults* results = pScan->start(5);

  for (int i = 0; i < results->getCount(); i++) {
    BLEAdvertisedDevice device = results->getDevice(i);
    if (device.getName() == "NanoBewegung") {
      Serial.println("Nano found! Connecting...");
      pClient = BLEDevice::createClient();
      pClient->connect(&device);

      BLERemoteService* pService = pClient->getService(SERVICE_UUID);
      if (!pService) return false;

      pChar = pService->getCharacteristic(CHARACTERISTIC_UUID);
      if (!pChar) return false;

      pChar->registerForNotify(motionCallback);
      Serial.println("BLE connected!");
      return true;
    }
  }
  Serial.println("Nano not found, retrying...");
  return false;
}

// ── Web dashboard ─────────────────────────────────
WiFiServer server(80);

void sendDashboard(WiFiClient client) {
  String html = "";
  html += "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='5'>";
  html += "<title>Fridge Monitor</title>";
  html += "<style>";
  html += "body{font-family:sans-serif;max-width:500px;margin:40px auto;padding:20px;}";
  html += ".card{background:#f5f5f5;border-radius:12px;padding:20px;margin:16px 0;}";
  html += ".value{font-size:48px;font-weight:bold;margin:8px 0;}";
  html += ".label{color:#666;font-size:14px;}";
  html += ".on{color:#22c55e;}.off{color:#ef4444;}";
  html += "</style></head><body>";
  html += "<h1>Fridge Monitor</h1>";

  // Temperature card
  html += "<div class='card'>";
  html += "<div class='label'>Temperature</div>";
  html += "<div class='value'>" + String(temperature, 1) + " &deg;C</div>";
  if (temperature > 8.0) html += "<div style='color:#ef4444'>Too warm!</div>";
  else if (temperature < 1.0) html += "<div style='color:#3b82f6'>Freezing zone!</div>";
  else html += "<div style='color:#22c55e'>OK</div>";
  html += "</div>";

  // Motion card
  html += "<div class='card'>";
  html += "<div class='label'>Motion</div>";
  html += "<div class='value'>" + String(motionDetected ? "Yes" : "No") + "</div>";
  html += "</div>";

  // Plug card
  html += "<div class='card'>";
  html += "<div class='label'>Smart plug</div>";
  html += "<div class='value " + String(plugOn ? "on" : "off") + "'>";
  html += plugOn ? "ON" : "OFF";
  html += "</div></div>";

  html += "<div class='label'>Refreshes every 5 seconds</div>";
  html += "</body></html>";

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.print("Content-Length: ");
  client.println(html.length());
  client.println();
  client.print(html);
  client.flush();
  delay(10);
  client.stop();
}

// ── Setup ─────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);

  // WiFi
  Serial.print("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("Dashboard available at: http://");
  Serial.println(WiFi.localIP());
  server.begin();
  server.setNoDelay(true);

  // Temperature sensor
  sensors.begin();
  sensors.setResolution(12);

  // BLE
  BLEDevice::init("ESP32Hub");
  while (!connectToNano()) {
    delay(2000);
  }
}

// ── Loop ──────────────────────────────────────────
void loop() {
  // Check BLE connection
  if (pClient && !pClient->isConnected()) {
    Serial.println("BLE disconnected, reconnecting...");
    while (!connectToNano()) delay(2000);
  }

  unsigned long now = millis();

  // Measure temperature
  if (now - lastTemp >= TEMP_INTERVAL) {
    lastTemp = now;
    sensors.requestTemperatures();
    temperature = sensors.getTempCByIndex(0);
    Serial.print("Temperature: ");
    Serial.print(temperature, 1);
    Serial.println(" °C");

    // Temperature too high -> turn plug on (fridge needs power)
    if (temperature > 8.0 && !plugOn) {
      setPlug(true);
    }
  }

  // Motion logic
  if (motionDetected) {
    // Motion detected -> turn plug off
    if (plugOn && temperature <= 8.0) {
      setPlug(false);
    }
    // No motion for 10 seconds -> turn plug back on
    if (now - lastMotion > MOTION_TIMEOUT) {
      motionDetected = false;
      if (!plugOn && temperature <= 8.0) {
        setPlug(true);
      }
    }
  }

  // Web dashboard
  WiFiClient client = server.available();
  if (client) {
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 2000) {
        client.stop();
        return;
      }
    }
    while (client.available()) {
      client.read();
    }
    sendDashboard(client);
  }
}
