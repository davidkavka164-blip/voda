#ifndef HWC_CREDENTIAL_PROVISIONING_BOARD_ADAPTER_H
#define HWC_CREDENTIAL_PROVISIONING_BOARD_ADAPTER_H

#include <stddef.h>
#include <stdint.h>
#include "CredentialProvisioningSerialProtocol.h"

namespace HWC {

enum CredentialBoardAdapterEvent {
    CREDENTIAL_BOARD_ADAPTER_IDLE,
    CREDENTIAL_BOARD_ADAPTER_INPUT_DRAINED,
    CREDENTIAL_BOARD_ADAPTER_SESSION_OPENED,
    CREDENTIAL_BOARD_ADAPTER_SESSION_ACTIVE,
    CREDENTIAL_BOARD_ADAPTER_RESULT_SENT,
    CREDENTIAL_BOARD_ADAPTER_TRANSPORT_ERROR
};

// Board adapter for a Stream-like UART and a board-specific physical input.
// StreamT must provide available(), read(), write(data, length), flush().
// InputT must provide readActive(). No GPIO number or polarity is assumed here.
//
// Security properties:
// - the physical input is the only source of authorization;
// - bytes received outside an authorized session are actively discarded;
// - an armed session is opened only after the RX queue is empty;
// - READY is emitted only after the physical authorization was consumed;
// - the host must wait for READY before sending credentials;
// - after one result, malformed frame, expiry or TX failure, the parser is reset.
template <typename StreamT, typename InputT>
class CredentialProvisioningBoardAdapter {
public:
    static const size_t RX_DRAIN_BUDGET = 256U;
    static const size_t RX_SERVICE_BUDGET = 64U;

    CredentialProvisioningBoardAdapter(StreamT& stream, InputT& input,
                                       LocalServiceAuthorization& authorization,
                                       CredentialProvisioningController& controller,
                                       CredentialProvisioningSerialService& service)
        : stream_(stream), input_(input), authorization_(authorization),
          controller_(controller), service_(service) {}

    CredentialBoardAdapterEvent poll(uint32_t nowMs) {
        authorization_.samplePhysicalInput(nowMs, input_.readActive());

        if (!controller_.isSessionOpen(nowMs)) {
            service_.reset();
            bool backlogPending = false;
            const bool inputDiscarded = discardPendingInput(backlogPending);
            // Require a completely quiet poll after draining unsolicited RX
            // data. The host starts its request only after READY is received.
            if (inputDiscarded || backlogPending) return CREDENTIAL_BOARD_ADAPTER_INPUT_DRAINED;

            if (!authorization_.isArmed(nowMs)) return CREDENTIAL_BOARD_ADAPTER_IDLE;
            if (!controller_.openSession(nowMs)) return CREDENTIAL_BOARD_ADAPTER_IDLE;

            uint8_t ready[CredentialProvisioningSerialParser::RESULT_FRAME_SIZE];
            size_t readyLength = 0U;
            const bool encoded = CredentialProvisioningSerialParser::encodeResult(
                CredentialProvisioningSerialParser::STATUS_SESSION_READY,
                ready, sizeof(ready), readyLength);
            const bool sent = encoded && writeFrame(ready, readyLength);
            wipe(ready, sizeof(ready));
            if (!sent) {
                controller_.closeSession();
                service_.reset();
                return CREDENTIAL_BOARD_ADAPTER_TRANSPORT_ERROR;
            }
            return CREDENTIAL_BOARD_ADAPTER_SESSION_OPENED;
        }

        size_t processed = 0U;
        while (processed < RX_SERVICE_BUDGET && stream_.available() > 0) {
            const int value = stream_.read();
            if (value < 0) break;
            ++processed;
            const CredentialSerialServiceReply reply = service_.feed(
                nowMs, static_cast<uint8_t>(value));
            if (reply.event == CREDENTIAL_SERIAL_FRAME_REJECTED ||
                reply.event == CREDENTIAL_SERIAL_PROVISION_RESULT) {
                uint8_t response[CredentialProvisioningSerialParser::RESULT_FRAME_SIZE];
                size_t responseLength = 0U;
                const bool encoded = CredentialProvisioningSerialService::encodeReply(
                    reply, response, sizeof(response), responseLength);
                const bool sent = encoded && writeFrame(response, responseLength);
                wipe(response, sizeof(response));
                service_.reset();
                if (!sent) {
                    controller_.closeSession();
                    return CREDENTIAL_BOARD_ADAPTER_TRANSPORT_ERROR;
                }
                // Any bytes after the one-shot command are not allowed to
                // survive into a future physical authorization window.
                bool ignoredBacklog = false;
                discardPendingInput(ignoredBacklog);
                return CREDENTIAL_BOARD_ADAPTER_RESULT_SENT;
            }
        }

        if (!controller_.isSessionOpen(nowMs)) {
            service_.reset();
            bool ignoredBacklog = false;
            discardPendingInput(ignoredBacklog);
            return CREDENTIAL_BOARD_ADAPTER_IDLE;
        }
        return CREDENTIAL_BOARD_ADAPTER_SESSION_ACTIVE;
    }

    void reset() {
        service_.reset();
        controller_.closeSession();
    }

    bool sessionOpen(uint32_t nowMs) { return controller_.isSessionOpen(nowMs); }

private:
    StreamT& stream_;
    InputT& input_;
    LocalServiceAuthorization& authorization_;
    CredentialProvisioningController& controller_;
    CredentialProvisioningSerialService& service_;

    bool discardPendingInput(bool& backlogPending) {
        size_t discarded = 0U;
        while (discarded < RX_DRAIN_BUDGET && stream_.available() > 0) {
            if (stream_.read() < 0) break;
            ++discarded;
        }
        // If bytes remain, caller must return without opening a session. The
        // next poll continues draining; this bounds work in the main loop.
        backlogPending = stream_.available() > 0;
        return discarded != 0U;
    }

    bool writeFrame(const uint8_t* data, size_t length) {
        if (data == 0 || length == 0U) return false;
        const size_t written = stream_.write(data, length);
        if (written != length) return false;
        stream_.flush();
        return true;
    }

    static void wipe(uint8_t* data, size_t length) {
        volatile uint8_t* p = data;
        while (length--) *p++ = 0U;
    }
};

} // namespace HWC

#endif
