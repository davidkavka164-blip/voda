#ifndef HWC_ETHERNET_STATUS_H
#define HWC_ETHERNET_STATUS_H

#include <Arduino.h>
#include <Ethernet.h>
#include <SPI.h>
#include <freertos/queue.h>
#include <mbedtls/base64.h>
#include <strings.h>
#include "HttpRequestParser.h"
#include "WebAuthRateLimiter.h"

namespace HWC {

struct EthernetLinkView {
    uint8_t octets[4];
    bool ready;
    bool linkDown;
};

// Arduino-ESP32 1.0.6 requires Server::begin(uint16_t), while Ethernet 2.0.2
// provides a parameterless begin() and takes the port in the constructor.
class EthernetStatusServer : public EthernetServer {
public:
    EthernetStatusServer() : EthernetServer(80) {}
    void begin(uint16_t) { EthernetServer::begin(); }
};

// Each sketch creates one instance. The worker is the sole owner of W5500.
template <typename Snapshot>
class EthernetStatus {
public:
    using Renderer = void (*)(const Snapshot&, uint32_t, char*, size_t);

    EthernetStatus(uint8_t cs, uint8_t reset, uint8_t sck, uint8_t miso, uint8_t mosi,
                   Renderer render)
        : cs_(cs), reset_(reset), sck_(sck), miso_(miso), mosi_(mosi), render_(render) {}

    void begin(const String& adminPassword) {
        ethernetWaterQueue = xQueueCreate(1, sizeof(Snapshot));
        ethernetLinkQueue = xQueueCreate(1, sizeof(EthernetLinkView));
        if (!ethernetWaterQueue || !ethernetLinkQueue) {
            stopBeforeStart();
            return;
        }

        if (adminPassword.length() >= 12 && adminPassword.length() <= 64) {
            char userAndPassword[71];
            const int length = snprintf(userAndPassword, sizeof(userAndPassword), "admin:%s",
                                        adminPassword.c_str());
            size_t encodedLength = 0;
            if (mbedtls_base64_encode(reinterpret_cast<unsigned char*>(ethernetBasicToken),
                                      sizeof(ethernetBasicToken), &encodedLength,
                                      reinterpret_cast<const unsigned char*>(userAndPassword),
                                      static_cast<size_t>(length)) != 0) {
                ethernetBasicToken[0] = '\0';
            }
            volatile char* clear = userAndPassword;
            for (size_t i = 0; i < sizeof(userAndPassword); ++i) clear[i] = 0;
        }
        // A separate task owns all W5500 access, including any DHCP, TCP and SPI waits.
        if (xTaskCreatePinnedToCore(taskEntry, "hwc-eth", 6144, this, 1, nullptr, 0) != pdPASS) {
            Serial.println("Ethernet task failed to start.");
            stopBeforeStart();
        }
    }

    void publish(const Snapshot& snapshot) {
        if (ethernetWaterQueue) xQueueOverwrite(ethernetWaterQueue, &snapshot);
    }

    EthernetLinkView linkView() const {
        EthernetLinkView view = {};
        if (ethernetLinkQueue) xQueuePeek(ethernetLinkQueue, &view, 0);
        return view;
    }

private:
    static void taskEntry(void* instance) {
        static_cast<EthernetStatus*>(instance)->runEthernetStatusTask();
    }

    void stopBeforeStart() {
        if (ethernetWaterQueue) vQueueDelete(ethernetWaterQueue);
        if (ethernetLinkQueue) vQueueDelete(ethernetLinkQueue);
        ethernetWaterQueue = nullptr;
        ethernetLinkQueue = nullptr;
        memset(ethernetBasicToken, 0, sizeof(ethernetBasicToken));
    }

    void sendEthernetStatus(EthernetClient& client, const char* status, const char* body,
                            const char* extraHeader = "") {
        char response[768];
        const int size = snprintf(response, sizeof(response),
            "HTTP/1.1 %s\r\nContent-Type: text/plain; charset=utf-8\r\n"
            "Cache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\n"
            "Connection: close\r\nContent-Length: %u\r\n%s\r\n%s",
            status, static_cast<unsigned>(strlen(body)), extraHeader, body);
        if (size > 0 && static_cast<size_t>(size) < sizeof(response)) {
            client.write(reinterpret_cast<const uint8_t*>(response), static_cast<size_t>(size));
        }
    }

    bool ethernetCredentialsMatch(const HWC::Http::Request& request) {
        const char* authorization = nullptr;
        if (!HWC::Http::findHeader(request, "Authorization", &authorization) ||
            strncasecmp(authorization, "Basic ", 6) != 0) return false;
        const char* received = authorization + 6;
        const size_t length = strlen(ethernetBasicToken);
        if (strlen(received) != length) return false;
        uint8_t difference = 0;
        for (size_t i = 0; i < length; ++i) {
            difference |= static_cast<uint8_t>(received[i] ^ ethernetBasicToken[i]);
        }
        return difference == 0;
    }

    void serveEthernetStatus(EthernetClient& client, HWC::WebAuthRateLimiter& limiter) {
        // These buffers have a single owner, the Ethernet task.
        static uint8_t bytes[HWC::Http::MAX_REQUEST_LINE + 2 + HWC::Http::MAX_HEADER_BYTES];
        static HWC::Http::Request request;
        size_t used = 0;
        const uint32_t started = millis();
        bool complete = false;
        while (used < sizeof(bytes) && millis() - started < 1000U) {
            const int byteRead = client.read();
            if (byteRead < 0) {
                if (!client.connected()) return;
                vTaskDelay(1);
                continue;
            }
            bytes[used++] = static_cast<uint8_t>(byteRead);
            if (used >= 4 && memcmp(bytes + used - 4, "\r\n\r\n", 4) == 0) {
                complete = true;
                break;
            }
        }

        if (!complete || HWC::Http::parseRequest(bytes, used, request) != HWC::Http::PARSE_OK) {
            sendEthernetStatus(client, "400 Bad Request", "Neplatný požadavek.\n");
            return;
        }
        if (strcmp(request.method, "GET") != 0) {
            sendEthernetStatus(client, "405 Method Not Allowed", "Dostupné je pouze čtení.\n", "Allow: GET\r\n");
            return;
        }
        if (strcmp(request.target, "/") != 0 && strcmp(request.target, "/status") != 0) {
            sendEthernetStatus(client, "404 Not Found", "Stránka nenalezena.\n");
            return;
        }
        if (!ethernetBasicToken[0]) {
            sendEthernetStatus(client, "503 Service Unavailable", "Přihlášení správce není dostupné.\n");
            return;
        }

        const uint32_t clientKey = static_cast<uint32_t>(client.remoteIP());
        const uint32_t now = millis();
        if (limiter.isBlocked(clientKey, now)) {
            sendEthernetStatus(client, "429 Too Many Requests", "Zkus to za 60 sekund.\n", "Retry-After: 60\r\n");
            return;
        }
        if (!ethernetCredentialsMatch(request)) {
            // A normal browser opens the page first to obtain the Basic challenge.
            for (size_t i = 0; i < request.headerCount; ++i) {
                if (HWC::Http::equalsIgnoreCase(request.headers[i].name, "Authorization")) {
                    limiter.recordFailure(clientKey, now);
                    break;
                }
            }
            sendEthernetStatus(client, "401 Unauthorized", "Přihlášení správce.\n",
                               "WWW-Authenticate: Basic realm=\"Home Water Controller\"\r\n");
            return;
        }
        limiter.recordSuccess(clientKey);

        Snapshot water = {};
        xQueuePeek(ethernetWaterQueue, &water, 0);
        char body[192];
        render_(water, millis(), body, sizeof(body));
        sendEthernetStatus(client, "200 OK", body);
    }

    void runEthernetStatusTask() {
        const uint64_t efuse = ESP.getEfuseMac();
        uint8_t mac[6] = {0x02, static_cast<uint8_t>(efuse >> 32), static_cast<uint8_t>(efuse >> 24),
            static_cast<uint8_t>(efuse >> 16), static_cast<uint8_t>(efuse >> 8), static_cast<uint8_t>(efuse)};

        pinMode(reset_, OUTPUT);
        digitalWrite(reset_, LOW);
        vTaskDelay(pdMS_TO_TICKS(50));
        digitalWrite(reset_, HIGH);
        vTaskDelay(pdMS_TO_TICKS(200));
        SPI.begin(sck_, miso_,
                  mosi_, cs_);
        Ethernet.init(cs_);

        HWC::WebAuthRateLimiter limiter;
        bool listening = false;
        bool wasLinkDown = false;
        uint32_t attemptedAt = millis() - 10000U;
        for (;;) {
            const uint32_t now = millis();
            const bool linkDown = Ethernet.linkStatus() == LinkOFF;
            if (linkDown && listening) {
                listening = false;
            }
            if (!linkDown && !listening &&
                (wasLinkDown || static_cast<uint32_t>(now - attemptedAt) >= 10000U)) {
                attemptedAt = now;
                if (Ethernet.begin(mac, 2000, 250) != 0) {
                    Ethernet.setRetransmissionTimeout(150);
                    Ethernet.setRetransmissionCount(3);
                    listening = true;
                    Serial.printf("Ethernet status: http://%s/status\n",
                                  Ethernet.localIP().toString().c_str());
                }
            }
            wasLinkDown = linkDown;

            if (listening) {
                Ethernet.maintain();
                listening = Ethernet.localIP() != IPAddress(0, 0, 0, 0);
            }
            const IPAddress ip = listening ? Ethernet.localIP() : IPAddress(0, 0, 0, 0);
            const EthernetLinkView view = {{ip[0], ip[1], ip[2], ip[3]}, listening, linkDown};
            xQueueOverwrite(ethernetLinkQueue, &view);
            if (listening) {
                EthernetClient client = ethernetStatusServer.accept();
                if (client) {
                    client.setConnectionTimeout(100);
                    serveEthernetStatus(client, limiter);
                    client.stop();
                }
            }
            vTaskDelay(1);
        }
    }

    const uint8_t cs_, reset_, sck_, miso_, mosi_;
    const Renderer render_;
    QueueHandle_t ethernetWaterQueue = nullptr;
    QueueHandle_t ethernetLinkQueue = nullptr;
    char ethernetBasicToken[97] = {};
    EthernetStatusServer ethernetStatusServer;
};

} // namespace HWC
#endif
