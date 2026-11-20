#ifndef HWC_CREDENTIAL_RANDOM_H
#define HWC_CREDENTIAL_RANDOM_H

#include <stddef.h>
#include <stdint.h>

namespace HWC {

typedef uint32_t (*CredentialRandomWordSource)(void);

// Formats whole 32-bit source words as uppercase hexadecimal. Four words
// produce 32 hex characters (128 bits) without modulo bias. The caller is
// responsible for enabling a documented entropy source before invoking this
// function; the helper itself deliberately cannot enable Wi-Fi or hardware RF.
inline bool generateHexCredential(char* output, size_t outputCapacity,
                                  CredentialRandomWordSource source,
                                  size_t wordCount) {
    if (output != 0 && outputCapacity != 0U) {
        volatile char* clear = output;
        for (size_t i = 0U; i < outputCapacity; ++i) clear[i] = '\0';
    }
    if (output == 0 || source == 0 || outputCapacity == 0U || wordCount == 0U ||
        wordCount > (outputCapacity - 1U) / 8U) {
        return false;
    }

    static const char hexDigits[] = "0123456789ABCDEF";
    for (size_t wordIndex = 0U; wordIndex < wordCount; ++wordIndex) {
        const uint32_t word = source();
        const size_t base = wordIndex * 8U;
        for (size_t nibble = 0U; nibble < 8U; ++nibble) {
            const unsigned shift = static_cast<unsigned>((7U - nibble) * 4U);
            output[base + nibble] = hexDigits[(word >> shift) & 0x0FU];
        }
    }
    output[wordCount * 8U] = '\0';
    return true;
}

} // namespace HWC
#endif
