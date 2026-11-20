#ifndef HWC_NVS_TRANSFER_JOURNAL_STORE_H
#define HWC_NVS_TRANSFER_JOURNAL_STORE_H
#include <Preferences.h>
#include "TransferRestartGuard.h"
namespace HWC {
class NvsTransferJournalStore : public ITransferJournalStore {
public:
    explicit NvsTransferJournalStore(const char* nameSpace = "hwc_xfer_jrnl");
    bool begin();
    void end();
    bool isOpen() const;
    TransferJournalLoadResult load(TransferJournalRecord& record);
    bool save(const TransferJournalRecord& record);
private:
    Preferences preferences_;
    const char* nameSpace_;
    bool opened_;
    TransferJournalLoadResult readSlot(uint8_t slot, TransferJournalRecord& record);
    static bool newerGeneration(uint32_t a, uint32_t b);
};
}
#endif
