#include "CredentialProvisioningBoardAdapter.h"
#include <assert.h>
#include <deque>
#include <map>
#include <string>
#include <vector>
#include <stdio.h>
#include <string.h>

class FakeBackend : public HWC::CredentialBackend {
public:
    std::map<std::string, std::string> data;
    HWC::CredentialReadResult get(const char* key, char* out, size_t capacity) {
        std::map<std::string, std::string>::const_iterator it = data.find(key);
        if (it == data.end()) { if (capacity) out[0] = '\0'; return HWC::CREDENTIAL_READ_MISSING; }
        if (it->second.size() >= capacity) { if (capacity) out[0] = '\0'; return HWC::CREDENTIAL_READ_ERROR; }
        memcpy(out, it->second.c_str(), it->second.size() + 1U);
        return HWC::CREDENTIAL_READ_FOUND;
    }
    bool put(const char* key, const char* value) { data[key] = value; return true; }
    bool erase(const char* key) { data.erase(key); return true; }
};

class FakeStream {
public:
    std::deque<uint8_t> rx;
    std::vector<uint8_t> tx;
    bool failWrite;
    FakeStream() : failWrite(false) {}
    int available() const { return static_cast<int>(rx.size()); }
    int read() {
        if (rx.empty()) return -1;
        const uint8_t value = rx.front();
        rx.pop_front();
        return value;
    }
    size_t write(const uint8_t* data, size_t length) {
        if (failWrite) return 0U;
        tx.insert(tx.end(), data, data + length);
        return length;
    }
    void flush() {}
    void append(const uint8_t* data, size_t length) {
        for (size_t i = 0; i < length; ++i) rx.push_back(data[i]);
    }
};

class FakeInput {
public:
    bool active;
    FakeInput() : active(false) {}
    bool readActive() const { return active; }
};

static const uint8_t kRequest[] = {
    0x48,0x57,0x01,0x01,0x00,0x33,
    0x0D,'H','W','C','-','T','E','S','T','-','1','2','3','4',
    0x12,'A','P','-','p','a','s','s','p','h','r','a','s','e','-','2','0','2','6',
    0x11,'A','d','m','i','n','-','s','e','c','r','e','t','-','2','0','2','6',
    0xAE,0x41
};

static uint16_t testCrc16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFFU;
    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8U;
        for (uint8_t bit = 0; bit < 8U; ++bit) {
            crc = (crc & 0x8000U) ? static_cast<uint16_t>((crc << 1U) ^ 0x1021U)
                                 : static_cast<uint16_t>(crc << 1U);
        }
    }
    return crc;
}

static uint8_t responseStatus(const FakeStream& stream, size_t offset) {
    assert(stream.tx.size() >= offset + HWC::CredentialProvisioningSerialParser::RESULT_FRAME_SIZE);
    return stream.tx[offset + 6U];
}

static void assertValidResponse(const FakeStream& stream, size_t offset) {
    assert(stream.tx.size() >= offset + 9U);
    assert(stream.tx[offset] == 0x48U && stream.tx[offset + 1U] == 0x57U);
    assert(stream.tx[offset + 2U] == 1U && stream.tx[offset + 3U] == 0x81U);
    assert(stream.tx[offset + 4U] == 0U && stream.tx[offset + 5U] == 1U);
    const uint16_t expected = testCrc16(&stream.tx[offset + 2U], 5U);
    const uint16_t received = static_cast<uint16_t>(
        (static_cast<uint16_t>(stream.tx[offset + 7U]) << 8U) | stream.tx[offset + 8U]);
    assert(expected == received);
}

static void test_pre_authorization_bytes_are_discarded_and_ready_is_required() {
    FakeBackend backend;
    HWC::CredentialStore store(backend);
    HWC::LocalServiceAuthorization authorization;
    HWC::CredentialProvisioningController controller(store, authorization);
    HWC::CredentialProvisioningSerialService service(controller);
    FakeStream stream;
    FakeInput input;
    HWC::CredentialProvisioningBoardAdapter<FakeStream, FakeInput> adapter(
        stream, input, authorization, controller, service);

    stream.append(kRequest, sizeof(kRequest)); // stale/early host data must not be accepted
    assert(adapter.poll(0U) == HWC::CREDENTIAL_BOARD_ADAPTER_INPUT_DRAINED);
    assert(stream.rx.empty());
    assert(stream.tx.empty());
    HWC::Credentials check;
    HWC::CredentialStore::clear(check);
    assert(store.load(check) == HWC::CREDENTIAL_EMPTY);

    input.active = true;
    assert(adapter.poll(100U) == HWC::CREDENTIAL_BOARD_ADAPTER_IDLE);
    assert(adapter.poll(3099U) == HWC::CREDENTIAL_BOARD_ADAPTER_IDLE);
    assert(adapter.poll(3100U) == HWC::CREDENTIAL_BOARD_ADAPTER_SESSION_OPENED);
    assert(adapter.sessionOpen(3100U));
    assert(stream.tx.size() == HWC::CredentialProvisioningSerialParser::RESULT_FRAME_SIZE);
    assert(responseStatus(stream, 0U) == HWC::CredentialProvisioningSerialParser::STATUS_SESSION_READY);
    assertValidResponse(stream, 0U);
    assert(store.load(check) == HWC::CREDENTIAL_EMPTY);
    HWC::CredentialStore::clear(check);

    // A request is accepted only after the READY indication has been sent.
    stream.append(kRequest, sizeof(kRequest));
    assert(adapter.poll(3101U) == HWC::CREDENTIAL_BOARD_ADAPTER_RESULT_SENT);
    assert(!adapter.sessionOpen(3101U));
    assert(stream.tx.size() == 2U * HWC::CredentialProvisioningSerialParser::RESULT_FRAME_SIZE);
    assert(responseStatus(stream, HWC::CredentialProvisioningSerialParser::RESULT_FRAME_SIZE) == 0x20U);
    assertValidResponse(stream, HWC::CredentialProvisioningSerialParser::RESULT_FRAME_SIZE);
    assert(store.load(check) == HWC::CREDENTIAL_VALID);
    assert(strcmp(check.apName, "HWC-TEST-1234") == 0);
    HWC::CredentialStore::clear(check);

    // A second payload cannot reuse the consumed one-shot authorization.
    stream.append(kRequest, sizeof(kRequest));
    assert(adapter.poll(3102U) == HWC::CREDENTIAL_BOARD_ADAPTER_INPUT_DRAINED);
    assert(stream.rx.empty());
    assert(stream.tx.size() == 2U * HWC::CredentialProvisioningSerialParser::RESULT_FRAME_SIZE);
}

static void test_fragmented_request_is_processed_within_one_session() {
    FakeBackend backend;
    HWC::CredentialStore store(backend);
    HWC::LocalServiceAuthorization authorization;
    HWC::CredentialProvisioningController controller(store, authorization);
    HWC::CredentialProvisioningSerialService service(controller);
    FakeStream stream;
    FakeInput input;
    HWC::CredentialProvisioningBoardAdapter<FakeStream, FakeInput> adapter(
        stream, input, authorization, controller, service);

    input.active = true;
    adapter.poll(0U);
    assert(adapter.poll(3000U) == HWC::CREDENTIAL_BOARD_ADAPTER_SESSION_OPENED);
    stream.append(kRequest, 11U);
    assert(adapter.poll(3001U) == HWC::CREDENTIAL_BOARD_ADAPTER_SESSION_ACTIVE);
    assert(stream.tx.size() == 9U);
    stream.append(kRequest + 11U, sizeof(kRequest) - 11U);
    assert(adapter.poll(3010U) == HWC::CREDENTIAL_BOARD_ADAPTER_RESULT_SENT);
    assert(stream.tx.size() == 18U);
    assert(responseStatus(stream, 9U) == 0x20U);
}

static void test_session_expiry_closes_and_drains_uart() {
    FakeBackend backend;
    HWC::CredentialStore store(backend);
    HWC::LocalServiceAuthorization authorization;
    HWC::CredentialProvisioningController controller(store, authorization);
    HWC::CredentialProvisioningSerialService service(controller);
    FakeStream stream;
    FakeInput input;
    HWC::CredentialProvisioningBoardAdapter<FakeStream, FakeInput> adapter(
        stream, input, authorization, controller, service);

    input.active = true;
    adapter.poll(0U);
    assert(adapter.poll(3000U) == HWC::CREDENTIAL_BOARD_ADAPTER_SESSION_OPENED);
    assert(adapter.sessionOpen(3000U));
    stream.append(kRequest, 5U);
    assert(adapter.poll(3001U) == HWC::CREDENTIAL_BOARD_ADAPTER_SESSION_ACTIVE);
    stream.rx.push_back(0x55U);
    assert(adapter.poll(63000U) == HWC::CREDENTIAL_BOARD_ADAPTER_INPUT_DRAINED);
    assert(!adapter.sessionOpen(63000U));
    assert(stream.rx.empty());
    assert(stream.tx.size() == 9U); // READY only; no credential result
    HWC::Credentials check;
    HWC::CredentialStore::clear(check);
    assert(store.load(check) == HWC::CREDENTIAL_EMPTY);
    HWC::CredentialStore::clear(check);
}

static void test_partial_uart_write_closes_session() {
    FakeBackend backend;
    HWC::CredentialStore store(backend);
    HWC::LocalServiceAuthorization authorization;
    HWC::CredentialProvisioningController controller(store, authorization);
    HWC::CredentialProvisioningSerialService service(controller);
    FakeStream stream;
    FakeInput input;
    HWC::CredentialProvisioningBoardAdapter<FakeStream, FakeInput> adapter(
        stream, input, authorization, controller, service);

    input.active = true;
    adapter.poll(0U);
    stream.failWrite = true;
    assert(adapter.poll(3000U) == HWC::CREDENTIAL_BOARD_ADAPTER_TRANSPORT_ERROR);
    assert(!adapter.sessionOpen(3000U));
    assert(stream.tx.empty());
}

static void test_large_untrusted_rx_backlog_is_drained_in_bounded_polls() {
    FakeBackend backend;
    HWC::CredentialStore store(backend);
    HWC::LocalServiceAuthorization authorization;
    HWC::CredentialProvisioningController controller(store, authorization);
    HWC::CredentialProvisioningSerialService service(controller);
    FakeStream stream;
    FakeInput input;
    HWC::CredentialProvisioningBoardAdapter<FakeStream, FakeInput> adapter(
        stream, input, authorization, controller, service);

    input.active = true;
    adapter.poll(0U);
    for (size_t i = 0; i < 600U; ++i) stream.rx.push_back(static_cast<uint8_t>(0x55U));
    assert(adapter.poll(3000U) == HWC::CREDENTIAL_BOARD_ADAPTER_INPUT_DRAINED);
    assert(stream.rx.size() == 344U);
    assert(!adapter.sessionOpen(3000U));
    assert(adapter.poll(3010U) == HWC::CREDENTIAL_BOARD_ADAPTER_INPUT_DRAINED);
    assert(stream.rx.size() == 88U);
    assert(adapter.poll(3020U) == HWC::CREDENTIAL_BOARD_ADAPTER_INPUT_DRAINED);
    assert(stream.rx.empty());
    // One quiet poll is required after all unsolicited bytes have been cleared.
    assert(adapter.poll(3030U) == HWC::CREDENTIAL_BOARD_ADAPTER_SESSION_OPENED);
    assert(adapter.sessionOpen(3030U));
    assert(responseStatus(stream, 0U) == HWC::CredentialProvisioningSerialParser::STATUS_SESSION_READY);
    assertValidResponse(stream, 0U);
}

int main() {
    test_pre_authorization_bytes_are_discarded_and_ready_is_required();
    test_fragmented_request_is_processed_within_one_session();
    test_session_expiry_closes_and_drains_uart();
    test_partial_uart_write_closes_session();
    test_large_untrusted_rx_backlog_is_drained_in_bounded_polls();
    puts("PASS: credential provisioning board adapter tests");
    return 0;
}
