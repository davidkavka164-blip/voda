#ifndef HWC_NVS_TRANSFER_HISTORY_STORE_H
#define HWC_NVS_TRANSFER_HISTORY_STORE_H
#include "TransferHistory.h"
#include <Preferences.h>
namespace HWC {
static const uint32_t TRANSFER_HISTORY_STORE_MAGIC = 0x48585431UL;
static const uint16_t TRANSFER_HISTORY_STORE_VERSION = 1;
struct TransferHistoryRecord { uint32_t magic; uint16_t version; TransferHistorySnapshot snapshot; uint32_t checksum; };
uint32_t transferHistoryRecordChecksum(const TransferHistoryRecord& record);
bool isTransferHistoryRecordValid(const TransferHistoryRecord& record);
class NvsTransferHistoryStore {
public:
    NvsTransferHistoryStore(); bool begin(); void end(); bool hasRecord();
    bool load(TransferHistorySnapshot& out); bool save(const TransferHistorySnapshot& snapshot);
private: Preferences preferences_; bool opened_;
};
}
#endif
