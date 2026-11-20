/*
 * HOME WATER CONTROLLER – isolated W5500 + service AP diagnostic
 *
 * BENCH TEST ONLY. This sketch is NOT production firmware and contains no
 * pump/valve control, credentials, OTA, configuration writes or safety logic.
 * It verifies whether the installed ESP32 core + Ethernet library can serve
 * one read-only HTTP page simultaneously over Wi-Fi AP and W5500 Ethernet.
 *
 * Change the temporary AP password before use. Connect W5500 to a trusted LAN.
 */
#include <SPI.h>
#include <Ethernet.h>
#include <WiFi.h>
#include <WebServer.h>
#include <string.h>

static const uint8_t PIN_W5500_SCK  = 18;
static const uint8_t PIN_W5500_MISO = 19;
static const uint8_t PIN_W5500_MOSI = 23;
static const uint8_t PIN_W5500_CS   = 27;
static const uint8_t PIN_W5500_RST  = 26;

static const char* AP_SSID = "HWC-W5500-TEST";
static const char* AP_PASSWORD = "ChangeThis-Test-123";

WebServer apServer(80);
EthernetServer ethernetServer(80);
bool ethernetReady = false;
uint32_t lastEthernetRetryMs = 0;
EthernetClient ethernetActiveClient;
bool ethernetClientActive = false;
uint32_t ethernetClientStartedMs = 0U;
char ethernetRequestLine[96] = {0};
char ethernetHeaderLine[128] = {0};
size_t ethernetRequestUsed = 0U;
size_t ethernetHeaderUsed = 0U;
uint8_t ethernetHeaderCount = 0U;
bool ethernetRequestLineComplete = false;

String diagnosticPage(const char* interfaceName) {
    String page;
    page.reserve(700);
    page += F("<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>");
    page += F("<title>HWC W5500 test</title></head><body><h2>HOME WATER CONTROLLER</h2>");
    page += F("<p>Read-only network diagnostic</p><p>Interface: ");
    page += interfaceName;
    page += F("</p><p>Wi-Fi AP IP: ");
    page += WiFi.softAPIP().toString();
    page += F("</p><p>W5500 IP: ");
    page += Ethernet.localIP().toString();
    page += F("</p><p>Both pages should remain reachable independently.</p></body></html>");
    return page;
}

void sendEthernetResponse(EthernetClient& client, const String& body) {
    client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\nContent-Length: "));
    client.print(body.length());
    client.print(F("\r\nCache-Control: no-store\r\n\r\n"));
    client.print(body);
}

void resetEthernetClient() {
    ethernetActiveClient.stop();
    ethernetClientActive = false;
    ethernetClientStartedMs = 0U;
    ethernetRequestUsed = 0U;
    ethernetHeaderUsed = 0U;
    ethernetHeaderCount = 0U;
    ethernetRequestLineComplete = false;
    ethernetRequestLine[0] = '\0';
    ethernetHeaderLine[0] = '\0';
}

void rejectEthernetClient() {
    if (ethernetClientActive && ethernetActiveClient.connected()) {
        ethernetActiveClient.print(F("HTTP/1.1 400 Bad Request\r\nConnection: close\r\nContent-Length: 0\r\n\r\n"));
    }
    resetEthernetClient();
}

void processEthernetClient() {
    if (!ethernetClientActive) {
        EthernetClient incoming = ethernetServer.available();
        if (!incoming) return;
        ethernetActiveClient = incoming;
        ethernetClientActive = true;
        ethernetClientStartedMs = millis();
        ethernetRequestUsed = 0U;
        ethernetHeaderUsed = 0U;
        ethernetHeaderCount = 0U;
        ethernetRequestLineComplete = false;
        ethernetRequestLine[0] = '\0';
        ethernetHeaderLine[0] = '\0';
    }

    if (!ethernetActiveClient.connected()) {
        resetEthernetClient();
        return;
    }
    if (static_cast<uint32_t>(millis() - ethernetClientStartedMs) >= 500U) {
        rejectEthernetClient();
        return;
    }

    // Non-blocking bounded work: at most 64 received bytes per loop pass.
    // This lets the Wi-Fi AP server continue to run while Ethernet headers arrive.
    uint8_t processed = 0U;
    while (ethernetActiveClient.available() && processed < 64U) {
        const int value = ethernetActiveClient.read();
        if (value < 0) break;
        ++processed;
        const char ch = static_cast<char>(value);
        if (ch == '\r') continue;
        if (ch == '\n') {
            if (!ethernetRequestLineComplete) {
                ethernetRequestLine[ethernetRequestUsed] = '\0';
                ethernetRequestLineComplete = true;
            } else {
                ethernetHeaderLine[ethernetHeaderUsed] = '\0';
                if (ethernetHeaderLine[0] == '\0') {
                    const bool getRoot = strcmp(ethernetRequestLine, "GET / HTTP/1.0") == 0 ||
                                         strcmp(ethernetRequestLine, "GET / HTTP/1.1") == 0;
                    if (getRoot) {
                        sendEthernetResponse(ethernetActiveClient, diagnosticPage("W5500 Ethernet"));
                        resetEthernetClient();
                    } else {
                        rejectEthernetClient();
                    }
                    return;
                }
                ++ethernetHeaderCount;
                if (ethernetHeaderCount >= 16U) {
                    rejectEthernetClient();
                    return;
                }
                ethernetHeaderUsed = 0U;
            }
            continue;
        }

        if (!ethernetRequestLineComplete) {
            if (ethernetRequestUsed + 1U >= sizeof(ethernetRequestLine)) {
                rejectEthernetClient();
                return;
            }
            ethernetRequestLine[ethernetRequestUsed++] = ch;
        } else {
            if (ethernetHeaderUsed + 1U >= sizeof(ethernetHeaderLine)) {
                rejectEthernetClient();
                return;
            }
            ethernetHeaderLine[ethernetHeaderUsed++] = ch;
        }
    }
}

bool startEthernet() {
    const uint64_t efuse = ESP.getEfuseMac();
    uint8_t mac[6] = {0x02,
        static_cast<uint8_t>((efuse >> 32) & 0xFFU),
        static_cast<uint8_t>((efuse >> 24) & 0xFFU),
        static_cast<uint8_t>((efuse >> 16) & 0xFFU),
        static_cast<uint8_t>((efuse >> 8) & 0xFFU),
        static_cast<uint8_t>(efuse & 0xFFU)};

    pinMode(PIN_W5500_RST, OUTPUT);
    digitalWrite(PIN_W5500_RST, LOW);
    delay(50);
    digitalWrite(PIN_W5500_RST, HIGH);
    delay(200);
    SPI.begin(PIN_W5500_SCK, PIN_W5500_MISO, PIN_W5500_MOSI, PIN_W5500_CS);
    Ethernet.init(PIN_W5500_CS);
    Ethernet.begin(mac, 5000U, 250U);
    if (Ethernet.localIP()[0] == 0U) {
        Serial.println(F("W5500 DHCP failed; AP remains available."));
        return false;
    }
    ethernetServer.begin();
    Serial.print(F("W5500 HTTP test IP: "));
    Serial.println(Ethernet.localIP());
    return true;
}

void setup() {
    Serial.begin(115200);
    WiFi.persistent(false);
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
        Serial.println(F("ERROR: test AP could not start."));
    }
    apServer.on("/", HTTP_GET, []() {
        apServer.send(200, "text/html; charset=utf-8", diagnosticPage("Wi-Fi service AP"));
    });
    apServer.onNotFound([]() { apServer.send(404, "text/plain", "Not found"); });
    apServer.begin();
    Serial.print(F("Service AP: "));
    Serial.println(WiFi.softAPIP());

    ethernetReady = startEthernet();
    lastEthernetRetryMs = millis();
}

void loop() {
    apServer.handleClient();
    if (ethernetReady) {
        Ethernet.maintain();
        if (Ethernet.linkStatus() == LinkOFF || Ethernet.localIP()[0] == 0U) {
            ethernetReady = false;
            lastEthernetRetryMs = millis();
            Serial.println(F("W5500 link/DHCP lost; AP continues."));
        } else {
            processEthernetClient();
        }
    } else if (static_cast<uint32_t>(millis() - lastEthernetRetryMs) >= 60000U &&
               Ethernet.linkStatus() != LinkOFF) {
        lastEthernetRetryMs = millis();
        ethernetReady = startEthernet();
    }
    yield();
}
