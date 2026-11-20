#ifndef HWC_CREDENTIAL_PROVISIONING_SERIAL_PROTOCOL_H
#define HWC_CREDENTIAL_PROVISIONING_SERIAL_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>
#include "CredentialProvisioningController.h"

namespace HWC {

enum CredentialSerialParseResult {
    CREDENTIAL_SERIAL_PARSE_NONE,
    CREDENTIAL_SERIAL_PARSE_READY,
    CREDENTIAL_SERIAL_PARSE_REJECTED
};

enum CredentialSerialServiceEvent {
    CREDENTIAL_SERIAL_NO_SESSION,
    CREDENTIAL_SERIAL_WAITING,
    CREDENTIAL_SERIAL_FRAME_REJECTED,
    CREDENTIAL_SERIAL_PROVISION_RESULT
};

struct CredentialSerialServiceReply {
    CredentialSerialServiceEvent event;
    CredentialProvisioningResult result;
};

// Binary, bounded USB/UART protocol. This class parses data only; it cannot
// create or extend physical authorization. The board adapter must open the
// controller session only after sampling the real service input.
class CredentialProvisioningSerialParser {
public:
    static const uint8_t MAGIC_0 = 0x48U; // 'H'
    static const uint8_t MAGIC_1 = 0x57U; // 'W'
    static const uint8_t VERSION = 1U;
    static const uint8_t COMMAND_SET_CREDENTIALS = 0x01U;
    static const uint8_t RESPONSE_RESULT = 0x81U;
    static const uint8_t STATUS_SESSION_READY = 0x01U;
    static const uint32_t INTERBYTE_TIMEOUT_MS = 2000U;
    static const size_t MAX_PAYLOAD_SIZE = 162U;
    static const size_t MAX_FRAME_SIZE = 170U;
    static const size_t RESULT_FRAME_SIZE = 9U;

    CredentialProvisioningSerialParser() : used_(0U), expected_(0U), lastByteAt_(0U) {
        wipeBuffer();
    }

    ~CredentialProvisioningSerialParser() { reset(); }

    CredentialProvisioningSerialParser(const CredentialProvisioningSerialParser&) = delete;
    CredentialProvisioningSerialParser& operator=(const CredentialProvisioningSerialParser&) = delete;

    void reset() {
        wipeBuffer();
        used_ = 0U;
        expected_ = 0U;
        lastByteAt_ = 0U;
    }

    CredentialSerialParseResult feed(uint32_t nowMs, uint8_t byte, Credentials& output) {
        if (used_ != 0U && elapsed(nowMs, lastByteAt_) >= INTERBYTE_TIMEOUT_MS) reset();

        if (used_ == 0U) {
            if (byte == MAGIC_0) {
                frame_[0] = byte;
                used_ = 1U;
                lastByteAt_ = nowMs;
            }
            return CREDENTIAL_SERIAL_PARSE_NONE;
        }

        if (used_ == 1U && byte != MAGIC_1) {
            if (byte == MAGIC_0) {
                frame_[0] = byte;
                lastByteAt_ = nowMs;
            } else {
                reset();
            }
            return CREDENTIAL_SERIAL_PARSE_NONE;
        }

        if (used_ >= MAX_FRAME_SIZE) {
            reset();
            CredentialStore::clear(output);
            return CREDENTIAL_SERIAL_PARSE_REJECTED;
        }
        frame_[used_++] = byte;
        lastByteAt_ = nowMs;

        if (used_ == HEADER_SIZE) {
            const uint16_t payloadLength = readU16(&frame_[4]);
            if (frame_[2] != VERSION || frame_[3] != COMMAND_SET_CREDENTIALS ||
                payloadLength == 0U || payloadLength > MAX_PAYLOAD_SIZE) {
                reset();
                CredentialStore::clear(output);
                return CREDENTIAL_SERIAL_PARSE_REJECTED;
            }
            expected_ = HEADER_SIZE + static_cast<size_t>(payloadLength) + CRC_SIZE;
        }

        if (expected_ == 0U || used_ < expected_) return CREDENTIAL_SERIAL_PARSE_NONE;
        if (used_ != expected_) {
            reset();
            CredentialStore::clear(output);
            return CREDENTIAL_SERIAL_PARSE_REJECTED;
        }

        const size_t payloadLength = expected_ - HEADER_SIZE - CRC_SIZE;
        const uint16_t receivedCrc = readU16(&frame_[HEADER_SIZE + payloadLength]);
        const uint16_t calculatedCrc = crc16(&frame_[2], 4U + payloadLength);
        bool valid = receivedCrc == calculatedCrc;
        Credentials parsed;
        CredentialStore::clear(parsed);
        if (valid) valid = decodePayload(&frame_[HEADER_SIZE], payloadLength, parsed);

        reset();
        if (!valid) {
            CredentialStore::clear(output);
            CredentialStore::clear(parsed);
            return CREDENTIAL_SERIAL_PARSE_REJECTED;
        }

        output = parsed;
        CredentialStore::clear(parsed);
        return CREDENTIAL_SERIAL_PARSE_READY;
    }

    // Result frames contain one status byte only; credentials are never echoed.
    static bool encodeResult(uint8_t status, uint8_t* output, size_t capacity, size_t& written) {
        written = 0U;
        if (output == 0 || capacity < RESULT_FRAME_SIZE) return false;
        output[0] = MAGIC_0;
        output[1] = MAGIC_1;
        output[2] = VERSION;
        output[3] = RESPONSE_RESULT;
        output[4] = 0U;
        output[5] = 1U;
        output[6] = status;
        const uint16_t crc = crc16(&output[2], 5U);
        output[7] = static_cast<uint8_t>((crc >> 8U) & 0xFFU);
        output[8] = static_cast<uint8_t>(crc & 0xFFU);
        written = RESULT_FRAME_SIZE;
        return true;
    }

private:
    static const size_t HEADER_SIZE = 6U;
    static const size_t CRC_SIZE = 2U;

    uint8_t frame_[MAX_FRAME_SIZE];
    size_t used_;
    size_t expected_;
    uint32_t lastByteAt_;

    static uint32_t elapsed(uint32_t now, uint32_t then) {
        return static_cast<uint32_t>(now - then);
    }

    static uint16_t readU16(const uint8_t* p) {
        return static_cast<uint16_t>((static_cast<uint16_t>(p[0]) << 8U) | p[1]);
    }

    static uint16_t crc16(const uint8_t* data, size_t length) {
        uint16_t crc = 0xFFFFU;
        for (size_t i = 0; i < length; ++i) {
            crc ^= static_cast<uint16_t>(data[i]) << 8U;
            for (uint8_t bit = 0U; bit < 8U; ++bit) {
                crc = (crc & 0x8000U) ? static_cast<uint16_t>((crc << 1U) ^ 0x1021U)
                                     : static_cast<uint16_t>(crc << 1U);
            }
        }
        return crc;
    }

    static bool copyField(const uint8_t* payload, size_t payloadLength, size_t& offset,
                          char* destination, size_t capacity) {
        if (offset >= payloadLength || destination == 0 || capacity < 2U) return false;
        const size_t length = payload[offset++];
        if (length == 0U || length >= capacity || length > payloadLength - offset) return false;
        for (size_t i = 0; i < length; ++i) {
            const uint8_t ch = payload[offset + i];
            // Restrict service-provisioned text to printable ASCII. This avoids
            // embedded NULs, control sequences and ambiguous terminal encoding.
            if (ch < 0x20U || ch > 0x7EU) return false;
            destination[i] = static_cast<char>(ch);
        }
        destination[length] = '\0';
        offset += length;
        return true;
    }

    static bool decodePayload(const uint8_t* payload, size_t payloadLength, Credentials& out) {
        size_t offset = 0U;
        if (!copyField(payload, payloadLength, offset, out.apName, sizeof(out.apName)) ||
            !copyField(payload, payloadLength, offset, out.apPassword, sizeof(out.apPassword)) ||
            !copyField(payload, payloadLength, offset, out.adminPassword, sizeof(out.adminPassword)) ||
            offset != payloadLength || !CredentialStore::validate(out)) {
            CredentialStore::clear(out);
            return false;
        }
        return true;
    }

    void wipeBuffer() {
        volatile uint8_t* p = frame_;
        for (size_t i = 0; i < MAX_FRAME_SIZE; ++i) p[i] = 0U;
    }
};

// Adapter-facing service wrapper. It accepts frames only while a session that
// was previously opened by the physical-input path is still valid. A malformed
// frame closes that session; a complete frame consumes the controller's one
// provisioning attempt. This wrapper has no API to sample or create authority.
class CredentialProvisioningSerialService {
public:
    explicit CredentialProvisioningSerialService(CredentialProvisioningController& controller)
        : controller_(controller), parser_() {}

    CredentialSerialServiceReply feed(uint32_t nowMs, uint8_t byte) {
        CredentialSerialServiceReply reply = {CREDENTIAL_SERIAL_NO_SESSION, CREDENTIAL_PROVISION_SESSION_CLOSED};
        if (!controller_.isSessionOpen(nowMs)) {
            parser_.reset();
            return reply;
        }

        Credentials submitted;
        CredentialStore::clear(submitted);
        const CredentialSerialParseResult parsed = parser_.feed(nowMs, byte, submitted);
        if (parsed == CREDENTIAL_SERIAL_PARSE_NONE) {
            reply.event = CREDENTIAL_SERIAL_WAITING;
            return reply;
        }
        if (parsed == CREDENTIAL_SERIAL_PARSE_REJECTED) {
            controller_.closeSession();
            reply.event = CREDENTIAL_SERIAL_FRAME_REJECTED;
            return reply;
        }

        reply.result = controller_.provision(nowMs, submitted);
        reply.event = CREDENTIAL_SERIAL_PROVISION_RESULT;
        return reply;
    }

    void reset() { parser_.reset(); }

    static bool encodeReply(const CredentialSerialServiceReply& reply,
                            uint8_t* output, size_t capacity, size_t& written) {
        written = 0U;
        // Silence is intentional when no session exists or a partial frame is
        // still arriving. This avoids a response for every received byte.
        if (reply.event == CREDENTIAL_SERIAL_NO_SESSION || reply.event == CREDENTIAL_SERIAL_WAITING) return false;
        const uint8_t status = reply.event == CREDENTIAL_SERIAL_FRAME_REJECTED
            ? 0x12U
            : static_cast<uint8_t>(0x20U + static_cast<uint8_t>(reply.result));
        return CredentialProvisioningSerialParser::encodeResult(status, output, capacity, written);
    }

private:
    CredentialProvisioningController& controller_;
    CredentialProvisioningSerialParser parser_;
};

} // namespace HWC

#endif
