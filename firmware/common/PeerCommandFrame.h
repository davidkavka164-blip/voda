#ifndef HWC_PEER_COMMAND_FRAME_H
#define HWC_PEER_COMMAND_FRAME_H
#include <stddef.h>
#include <stdint.h>
namespace HWC {
static const uint32_t PEER_COMMAND_MAGIC = 0x48574343UL; // HWCC
static const uint16_t PEER_COMMAND_VERSION = 1U;
static const uint8_t PEER_FRAME_COMMAND = 1U;
static const uint8_t PEER_FRAME_RESPONSE = 2U;
static const size_t PEER_COMMAND_SIGNED_SIZE = 36U;
static const size_t PEER_COMMAND_FRAME_SIZE = 68U;
static const size_t PEER_COMMAND_MIN_KEY_SIZE = 32U;
struct PeerCommandFrame {
 uint8_t frameType; uint8_t commandCode; uint8_t senderDeviceId; uint8_t targetDeviceId;
 uint32_t senderBootId; uint32_t targetBootId; uint32_t sequence; uint32_t requestId;
 int32_t argument0; int32_t argument1;
};
bool encodePeerCommandFrame(const PeerCommandFrame& frame, const uint8_t* key, size_t keyLength, uint8_t* out, size_t capacity, size_t& written);
bool decodePeerCommandFrame(const uint8_t* data, size_t length, const uint8_t* key, size_t keyLength, PeerCommandFrame& out);
}
#endif
