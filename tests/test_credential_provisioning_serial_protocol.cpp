#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <map>
#include <string>
#include "CredentialProvisioningSerialProtocol.h"

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

static std::vector<uint8_t> makeFrame(const char* apName, const char* apPassword,
                                      const char* adminPassword) {
    std::vector<uint8_t> payload;
    const char* values[3] = {apName, apPassword, adminPassword};
    for (unsigned field = 0; field < 3; ++field) {
        const size_t length = strlen(values[field]);
        payload.push_back(static_cast<uint8_t>(length));
        payload.insert(payload.end(), values[field], values[field] + length);
    }
    std::vector<uint8_t> frame;
    frame.push_back(static_cast<uint8_t>(HWC::CredentialProvisioningSerialParser::MAGIC_0));
    frame.push_back(static_cast<uint8_t>(HWC::CredentialProvisioningSerialParser::MAGIC_1));
    frame.push_back(static_cast<uint8_t>(HWC::CredentialProvisioningSerialParser::VERSION));
    frame.push_back(static_cast<uint8_t>(HWC::CredentialProvisioningSerialParser::COMMAND_SET_CREDENTIALS));
    frame.push_back(static_cast<uint8_t>((payload.size() >> 8U) & 0xFFU));
    frame.push_back(static_cast<uint8_t>(payload.size() & 0xFFU));
    frame.insert(frame.end(), payload.begin(), payload.end());
    const uint16_t crc = testCrc16(&frame[2], frame.size() - 2U);
    frame.push_back(static_cast<uint8_t>((crc >> 8U) & 0xFFU));
    frame.push_back(static_cast<uint8_t>(crc & 0xFFU));
    return frame;
}

static void authorize(HWC::LocalServiceAuthorization& gate, uint32_t start) {
    gate.samplePhysicalInput(start, true);
    gate.samplePhysicalInput(static_cast<uint32_t>(start + HWC::LocalServiceAuthorization::HOLD_MS), true);
}

class FakeBackend : public HWC::CredentialBackend {
public:
    std::map<std::string, std::string> data;
    HWC::CredentialReadResult get(const char* key, char* out, size_t capacity) {
        std::map<std::string, std::string>::const_iterator it = data.find(key);
        if (it == data.end()) return HWC::CREDENTIAL_READ_MISSING;
        if (it->second.size() + 1U > capacity) return HWC::CREDENTIAL_READ_ERROR;
        memcpy(out, it->second.c_str(), it->second.size() + 1U);
        return HWC::CREDENTIAL_READ_FOUND;
    }
    bool put(const char* key, const char* value) { data[key] = value; return true; }
    bool erase(const char* key) { data.erase(key); return true; }
};

static bool wiped(const HWC::Credentials& c) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&c);
    for (size_t i = 0; i < sizeof(c); ++i) if (p[i] != 0U) return false;
    return true;
}

static HWC::CredentialSerialServiceReply sendFrame(HWC::CredentialProvisioningSerialService& service,
                                                     const std::vector<uint8_t>& frame,
                                                     uint32_t start, uint32_t step) {
    HWC::CredentialSerialServiceReply reply = {HWC::CREDENTIAL_SERIAL_WAITING, HWC::CREDENTIAL_PROVISION_SESSION_CLOSED};
    for (size_t i = 0; i < frame.size(); ++i) {
        reply = service.feed(static_cast<uint32_t>(start + static_cast<uint32_t>(i * step)), frame[i]);
    }
    return reply;
}

int main() {
    const std::vector<uint8_t> valid = makeFrame("HWC-TEST-1234", "AP-passphrase-2026", "Admin-secret-2026");
    // Golden frame shared with the Python host client test in tests/python/.
    // This pins the cross-language field order, big-endian length and CRC range.
    const uint8_t golden[] = {
        0x48U,0x57U,0x01U,0x01U,0x00U,0x33U,0x0DU,0x48U,0x57U,0x43U,0x2DU,0x54U,
        0x45U,0x53U,0x54U,0x2DU,0x31U,0x32U,0x33U,0x34U,0x12U,0x41U,0x50U,0x2DU,
        0x70U,0x61U,0x73U,0x73U,0x70U,0x68U,0x72U,0x61U,0x73U,0x65U,0x2DU,0x32U,
        0x30U,0x32U,0x36U,0x11U,0x41U,0x64U,0x6DU,0x69U,0x6EU,0x2DU,0x73U,0x65U,
        0x63U,0x72U,0x65U,0x74U,0x2DU,0x32U,0x30U,0x32U,0x36U,0xAEU,0x41U
    };
    assert(valid.size() == sizeof(golden));
    assert(memcmp(valid.data(), golden, sizeof(golden)) == 0);
    assert(valid.size() <= HWC::CredentialProvisioningSerialParser::MAX_FRAME_SIZE);

    // The serial service cannot open a session or provision without a physical authorization.
    {
        FakeBackend backend; HWC::CredentialStore store(backend); HWC::LocalServiceAuthorization gate;
        HWC::CredentialProvisioningController controller(store, gate);
        HWC::CredentialProvisioningSerialService service(controller);
        for (size_t i = 0; i < valid.size(); ++i) {
            HWC::CredentialSerialServiceReply reply = service.feed(static_cast<uint32_t>(100U + i), valid[i]);
            assert(reply.event == HWC::CREDENTIAL_SERIAL_NO_SESSION);
        }
        HWC::Credentials loaded; HWC::CredentialStore::clear(loaded);
        assert(store.load(loaded) == HWC::CREDENTIAL_EMPTY);
        assert(!controller.isSessionOpen(500U));
    }

    // A valid frame may arrive fragmented; a successful operation consumes the one-shot session.
    {
        FakeBackend backend; HWC::CredentialStore store(backend); HWC::LocalServiceAuthorization gate;
        HWC::CredentialProvisioningController controller(store, gate);
        HWC::CredentialProvisioningSerialService service(controller);
        authorize(gate, 1000U);
        assert(controller.openSession(4000U));
        const HWC::CredentialSerialServiceReply reply = sendFrame(service, valid, 4001U, 1U);
        assert(reply.event == HWC::CREDENTIAL_SERIAL_PROVISION_RESULT);
        assert(reply.result == HWC::CREDENTIAL_PROVISIONED);
        assert(!controller.isSessionOpen(5000U));
        HWC::Credentials loaded; HWC::CredentialStore::clear(loaded);
        assert(store.load(loaded) == HWC::CREDENTIAL_VALID);
        assert(strcmp(loaded.apName, "HWC-TEST-1234") == 0);
        assert(strcmp(loaded.apPassword, "AP-passphrase-2026") == 0);
        HWC::CredentialStore::clear(loaded);
    }

    // Noise before the frame is ignored; the valid frame is still decoded.
    {
        HWC::CredentialProvisioningSerialParser parser;
        HWC::Credentials out; HWC::CredentialStore::clear(out);
        const uint8_t noise[] = {0x00U, 0x0AU, 0x48U, 0x00U, 0xFFU};
        for (size_t i = 0; i < sizeof(noise); ++i)
            assert(parser.feed(static_cast<uint32_t>(i), noise[i], out) == HWC::CREDENTIAL_SERIAL_PARSE_NONE);
        HWC::CredentialSerialParseResult result = HWC::CREDENTIAL_SERIAL_PARSE_NONE;
        for (size_t i = 0; i < valid.size(); ++i)
            result = parser.feed(static_cast<uint32_t>(100U + i), valid[i], out);
        assert(result == HWC::CREDENTIAL_SERIAL_PARSE_READY);
        assert(strcmp(out.apName, "HWC-TEST-1234") == 0);
        HWC::CredentialStore::clear(out);
        assert(wiped(out));
    }

    // A bad CRC is rejected and closes the active service session immediately.
    {
        FakeBackend backend; HWC::CredentialStore store(backend); HWC::LocalServiceAuthorization gate;
        HWC::CredentialProvisioningController controller(store, gate);
        HWC::CredentialProvisioningSerialService service(controller);
        authorize(gate, 10000U);
        assert(controller.openSession(13000U));
        std::vector<uint8_t> bad = valid;
        bad[bad.size() - 1U] ^= 0x55U;
        const HWC::CredentialSerialServiceReply reply = sendFrame(service, bad, 13001U, 1U);
        assert(reply.event == HWC::CREDENTIAL_SERIAL_FRAME_REJECTED);
        assert(!controller.isSessionOpen(14000U));
        HWC::Credentials loaded; HWC::CredentialStore::clear(loaded);
        assert(store.load(loaded) == HWC::CREDENTIAL_EMPTY);
    }

    // Unsupported version, command and oversized declared payload are rejected at the header.
    {
        HWC::CredentialProvisioningSerialParser parser; HWC::Credentials out; HWC::CredentialStore::clear(out);
        const uint8_t badVersion[] = {0x48U, 0x57U, 2U, 1U, 0U, 1U};
        HWC::CredentialSerialParseResult result = HWC::CREDENTIAL_SERIAL_PARSE_NONE;
        for (size_t i = 0; i < sizeof(badVersion); ++i) result = parser.feed(static_cast<uint32_t>(i), badVersion[i], out);
        assert(result == HWC::CREDENTIAL_SERIAL_PARSE_REJECTED);
        parser.reset();
        const uint8_t badCommand[] = {0x48U, 0x57U, 1U, 2U, 0U, 1U};
        for (size_t i = 0; i < sizeof(badCommand); ++i) result = parser.feed(static_cast<uint32_t>(20U + i), badCommand[i], out);
        assert(result == HWC::CREDENTIAL_SERIAL_PARSE_REJECTED);
        parser.reset();
        const uint8_t tooLarge[] = {0x48U, 0x57U, 1U, 1U, 0U, 164U};
        for (size_t i = 0; i < sizeof(tooLarge); ++i) result = parser.feed(static_cast<uint32_t>(40U + i), tooLarge[i], out);
        assert(result == HWC::CREDENTIAL_SERIAL_PARSE_REJECTED);
        assert(wiped(out));
    }

    // Invalid text (control byte) is rejected even with a correct CRC.
    {
        std::vector<uint8_t> bad = valid;
        const size_t firstFieldStart = 6U + 1U;
        bad[firstFieldStart] = 0x1BU;
        const uint16_t crc = testCrc16(&bad[2], bad.size() - 4U);
        bad[bad.size() - 2U] = static_cast<uint8_t>((crc >> 8U) & 0xFFU);
        bad[bad.size() - 1U] = static_cast<uint8_t>(crc & 0xFFU);
        HWC::CredentialProvisioningSerialParser parser; HWC::Credentials out; HWC::CredentialStore::clear(out);
        HWC::CredentialSerialParseResult result = HWC::CREDENTIAL_SERIAL_PARSE_NONE;
        for (size_t i = 0; i < bad.size(); ++i) result = parser.feed(static_cast<uint32_t>(100U + i), bad[i], out);
        assert(result == HWC::CREDENTIAL_SERIAL_PARSE_REJECTED);
        assert(wiped(out));
    }

    // A partial frame expires at the exact 2s inter-byte boundary; later bytes cannot complete it.
    {
        HWC::CredentialProvisioningSerialParser parser; HWC::Credentials out; HWC::CredentialStore::clear(out);
        const uint32_t start = 0xFFFFFFF0U;
        for (size_t i = 0; i < 5U; ++i)
            assert(parser.feed(static_cast<uint32_t>(start + static_cast<uint32_t>(i)), valid[i], out) == HWC::CREDENTIAL_SERIAL_PARSE_NONE);
        const uint32_t lastPartial = static_cast<uint32_t>(start + 4U);
        const uint32_t expiredAt = static_cast<uint32_t>(lastPartial + HWC::CredentialProvisioningSerialParser::INTERBYTE_TIMEOUT_MS);
        assert(parser.feed(expiredAt, valid[5], out) == HWC::CREDENTIAL_SERIAL_PARSE_NONE);
        HWC::CredentialSerialParseResult result = HWC::CREDENTIAL_SERIAL_PARSE_NONE;
        const uint32_t restartAt = static_cast<uint32_t>(expiredAt + 100U);
        for (size_t i = 0; i < valid.size(); ++i)
            result = parser.feed(static_cast<uint32_t>(restartAt + static_cast<uint32_t>(i)), valid[i], out);
        assert(result == HWC::CREDENTIAL_SERIAL_PARSE_READY);
        HWC::CredentialStore::clear(out);
    }

    // Maximum legal field lengths fit the fixed parser buffer without overflow.
    {
        const std::string maxName(32U, 'N');
        const std::string maxAp(63U, 'P');
        const std::string maxAdmin(64U, 'A');
        const std::vector<uint8_t> maxFrame = makeFrame(maxName.c_str(), maxAp.c_str(), maxAdmin.c_str());
        assert(maxFrame.size() == HWC::CredentialProvisioningSerialParser::MAX_FRAME_SIZE);
        HWC::CredentialProvisioningSerialParser parser; HWC::Credentials out; HWC::CredentialStore::clear(out);
        HWC::CredentialSerialParseResult result = HWC::CREDENTIAL_SERIAL_PARSE_NONE;
        for (size_t i = 0; i < maxFrame.size(); ++i)
            result = parser.feed(static_cast<uint32_t>(5000U + i), maxFrame[i], out);
        assert(result == HWC::CREDENTIAL_SERIAL_PARSE_READY);
        assert(strlen(out.apName) == 32U && strlen(out.apPassword) == 63U && strlen(out.adminPassword) == 64U);
        HWC::CredentialStore::clear(out);

        const std::string tooLongAp(64U, 'P');
        const std::vector<uint8_t> overlongFrame = makeFrame(maxName.c_str(), tooLongAp.c_str(), maxAdmin.c_str());
        assert(overlongFrame.size() > HWC::CredentialProvisioningSerialParser::MAX_FRAME_SIZE);
        parser.reset();
        result = HWC::CREDENTIAL_SERIAL_PARSE_NONE;
        for (size_t i = 0; i < 6U; ++i)
            result = parser.feed(static_cast<uint32_t>(6000U + i), overlongFrame[i], out);
        assert(result == HWC::CREDENTIAL_SERIAL_PARSE_REJECTED);
    }

    // Extra payload bytes after the three fields are rejected, even with a valid CRC.
    {
        std::vector<uint8_t> extra = valid;
        const size_t crcStart = extra.size() - 2U;
        extra.insert(extra.begin() + static_cast<std::ptrdiff_t>(crcStart), 0x41U);
        const uint16_t newPayloadLength = static_cast<uint16_t>((extra[4] << 8U) | extra[5]);
        const uint16_t updatedLength = static_cast<uint16_t>(newPayloadLength + 1U);
        extra[4] = static_cast<uint8_t>((updatedLength >> 8U) & 0xFFU);
        extra[5] = static_cast<uint8_t>(updatedLength & 0xFFU);
        const uint16_t crc = testCrc16(&extra[2], extra.size() - 4U);
        extra[extra.size() - 2U] = static_cast<uint8_t>((crc >> 8U) & 0xFFU);
        extra[extra.size() - 1U] = static_cast<uint8_t>(crc & 0xFFU);
        HWC::CredentialProvisioningSerialParser parser; HWC::Credentials out; HWC::CredentialStore::clear(out);
        HWC::CredentialSerialParseResult result = HWC::CREDENTIAL_SERIAL_PARSE_NONE;
        for (size_t i = 0; i < extra.size(); ++i) result = parser.feed(static_cast<uint32_t>(7000U + i), extra[i], out);
        assert(result == HWC::CREDENTIAL_SERIAL_PARSE_REJECTED);
        assert(wiped(out));
    }

    // Reply encoding is fixed-size and carries only a status byte, never credential bytes.
    {
        HWC::CredentialSerialServiceReply waiting = {HWC::CREDENTIAL_SERIAL_WAITING, HWC::CREDENTIAL_PROVISION_SESSION_CLOSED};
        uint8_t wire[HWC::CredentialProvisioningSerialParser::RESULT_FRAME_SIZE] = {0};
        size_t written = 99U;
        assert(!HWC::CredentialProvisioningSerialService::encodeReply(waiting, wire, sizeof(wire), written));
        assert(written == 0U);
        HWC::CredentialSerialServiceReply reply = {HWC::CREDENTIAL_SERIAL_PROVISION_RESULT, HWC::CREDENTIAL_PROVISIONED};
        written = 0U;
        assert(HWC::CredentialProvisioningSerialService::encodeReply(reply, wire, sizeof(wire), written));
        assert(written == sizeof(wire));
        assert(wire[0] == 0x48U && wire[1] == 0x57U && wire[3] == 0x81U && wire[5] == 1U);
        assert(!HWC::CredentialProvisioningSerialService::encodeReply(reply, wire, 8U, written));
        assert(written == 0U);
        assert(!HWC::CredentialProvisioningSerialService::encodeReply(reply, 0, sizeof(wire), written));
        HWC::CredentialSerialServiceReply rejected = {HWC::CREDENTIAL_SERIAL_FRAME_REJECTED, HWC::CREDENTIAL_PROVISION_SESSION_CLOSED};
        assert(HWC::CredentialProvisioningSerialService::encodeReply(rejected, wire, sizeof(wire), written));
        assert(wire[6] == 0x12U);
    }

    puts("PASS: bounded physically gated credential serial protocol");
    return 0;
}
