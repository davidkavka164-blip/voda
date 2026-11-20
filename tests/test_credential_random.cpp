#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "CredentialRandom.h"

static uint32_t sequence[] = {0x01234567U, 0x89ABCDEFU, 0x00000000U, 0xFFFFFFFFU};
static size_t sequenceIndex = 0U;
static uint32_t fixedWordSource() {
    const uint32_t value = sequence[sequenceIndex % (sizeof(sequence) / sizeof(sequence[0]))];
    ++sequenceIndex;
    return value;
}

int main() {
    char output[33];
    memset(output, 'X', sizeof(output));
    sequenceIndex = 0U;
    assert(HWC::generateHexCredential(output, sizeof(output), fixedWordSource, 4U));
    assert(strcmp(output, "0123456789ABCDEF00000000FFFFFFFF") == 0);
    assert(strlen(output) == 32U);

    char small[32];
    memset(small, 'X', sizeof(small));
    sequenceIndex = 0U;
    assert(!HWC::generateHexCredential(small, sizeof(small), fixedWordSource, 4U));
    for (size_t i = 0U; i < sizeof(small); ++i) assert(small[i] == '\0');

    char empty[1] = {'X'};
    assert(!HWC::generateHexCredential(empty, sizeof(empty), fixedWordSource, 1U));
    assert(empty[0] == '\0');
    assert(!HWC::generateHexCredential(output, sizeof(output), 0, 4U));
    for (size_t i = 0U; i < sizeof(output); ++i) assert(output[i] == '\0');
    assert(!HWC::generateHexCredential(output, sizeof(output), fixedWordSource, 0U));
    for (size_t i = 0U; i < sizeof(output); ++i) assert(output[i] == '\0');

    char twoWords[17];
    sequenceIndex = 0U;
    assert(HWC::generateHexCredential(twoWords, sizeof(twoWords), fixedWordSource, 2U));
    assert(strcmp(twoWords, "0123456789ABCDEF") == 0);

    return 0;
}
