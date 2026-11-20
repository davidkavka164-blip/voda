#ifndef HWC_HTTP_ORIGIN_POLICY_H
#define HWC_HTTP_ORIGIN_POLICY_H

#include <stddef.h>
#include <ctype.h>

namespace HWC {

// Strict HTTP same-origin authority check shared by all HTTP transports.
// HTTPS is intentionally not accepted: this firmware currently serves plain HTTP.
inline bool isSameHttpOriginAuthority(const char* origin, const char* host) {
    if (!origin || !host || !*origin || !*host) return false;
    static const char prefix[] = "http://";
    for (size_t i = 0; i < sizeof(prefix) - 1U; ++i) {
        if (!origin[i] || tolower(static_cast<unsigned char>(origin[i])) != prefix[i]) return false;
    }
    const char* authority = origin + sizeof(prefix) - 1U;
    if (!*authority) return false;

    size_t authorityLength = 0U;
    while (authority[authorityLength]) {
        const char c = authority[authorityLength];
        if (c == '/' || c == '\\' || c == '?' || c == '#' || c == '@' ||
            c == '\r' || c == '\n' || c == ' ' || c == '\t') return false;
        if (++authorityLength > 127U) return false;
    }

    size_t hostLength = 0U;
    while (host[hostLength]) {
        const char c = host[hostLength];
        if (c == '/' || c == '\\' || c == '?' || c == '#' || c == '@' ||
            c == '\r' || c == '\n' || c == ' ' || c == '\t') return false;
        if (++hostLength > 127U) return false;
    }
    if (authorityLength != hostLength) return false;
    for (size_t i = 0; i < authorityLength; ++i) {
        if (tolower(static_cast<unsigned char>(authority[i])) !=
            tolower(static_cast<unsigned char>(host[i]))) return false;
    }
    return true;
}

} // namespace HWC
#endif
