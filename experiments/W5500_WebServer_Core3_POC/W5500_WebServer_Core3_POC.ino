/*
  HOME WATER CONTROLLER – W5500 + WebServer compatibility probe
  PURPOSE: isolated bench test only; NOT production firmware.
  Requires ESP32 Arduino core >= 3.0 and EthernetESP32 library.
  W5500 pins follow current HWC mapping: CS=27, RESET=26, SCK=18,
  MISO=19, MOSI=23. INT is not connected.
*/
#include <WiFi.h>
#include <SPI.h>
#include <EthernetESP32.h>
#include <WebServer.h>

static const int W5500_CS = 27;
static const int W5500_RESET = 26;
static const int W5500_SCK = 18;
static const int W5500_MISO = 19;
static const int W5500_MOSI = 23;

static const char* WIFI_SSID = "CHANGE_ME";
static const char* WIFI_PASSWORD = "CHANGE_ME";

W5500Driver w5500(W5500_CS, -1, W5500_RESET);
WebServer server(80);

void handleRoot() {
  String body = "HWC W5500/WebServer POC\n";
  body += "Remote IP: ";
  body += server.client().remoteIP().toString();
  body += "\nWi-Fi: ";
  body += (WiFi.status() == WL_CONNECTED) ? "connected" : "not connected";
  body += "\nEthernet IP: ";
  body += Ethernet.localIP().toString();
  server.send(200, "text/plain; charset=utf-8", body);
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  SPI.begin(W5500_SCK, W5500_MISO, W5500_MOSI);
  Ethernet.init(w5500);
  Ethernet.begin();

  server.on("/", HTTP_GET, handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();
  delay(2);
}
