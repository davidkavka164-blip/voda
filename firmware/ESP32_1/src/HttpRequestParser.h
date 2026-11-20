#ifndef HWC_HTTP_REQUEST_PARSER_H
#define HWC_HTTP_REQUEST_PARSER_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace HWC {
namespace Http {

// Bounded parser for one HTTP/1.x request. It deliberately does not perform
// socket I/O, authentication, routing, or multipart/OTA handling.
static const size_t MAX_REQUEST_LINE = 191;
static const size_t MAX_PATH = 127;
static const size_t MAX_QUERY = 255;
static const size_t MAX_HEADER_BYTES = 1024;
static const size_t MAX_HEADERS = 16;
static const size_t MAX_HEADER_NAME = 31;
static const size_t MAX_HEADER_VALUE = 127;
static const size_t MAX_BODY = 1024;
static const size_t MAX_PARAMS = 24;
static const size_t MAX_PARAM_NAME = 31;
static const size_t MAX_PARAM_VALUE = 511;
static const size_t MAX_PARAM_VALUE_DATA = 1024;

struct Header {
    char name[MAX_HEADER_NAME + 1];
    char value[MAX_HEADER_VALUE + 1];
};

struct Parameter {
    char name[MAX_PARAM_NAME + 1];
    uint16_t valueOffset;
    uint16_t valueLength;
};

struct Request {
    char method[8];
    char target[MAX_REQUEST_LINE + 1];
    char path[MAX_PATH + 1];
    char query[MAX_QUERY + 1];
    char version[9];
    Header headers[MAX_HEADERS];
    size_t headerCount;
    Parameter params[MAX_PARAMS];
    size_t paramCount;
    char parameterValues[MAX_PARAM_VALUE_DATA + 1];
    size_t parameterValuesLength;
    char body[MAX_BODY + 1];
    size_t bodyLength;

    Request() { clear(); }
    void clear() {
        memset(this, 0, sizeof(*this));
    }
};

enum ParseResult {
    PARSE_OK = 0,
    PARSE_BAD_REQUEST,
    PARSE_TOO_LARGE,
    PARSE_INCOMPLETE,
    PARSE_UNSUPPORTED_METHOD,
    PARSE_UNSUPPORTED_TRANSFER_ENCODING,
    PARSE_UNSUPPORTED_CONTENT_TYPE
};

inline bool isTokenChar(char c) {
    const unsigned char u = static_cast<unsigned char>(c);
    if ((u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') ||
        (u >= '0' && u <= '9')) return true;
    return c == '!' || c == '#' || c == '$' || c == '%' || c == '&' ||
           c == '\'' || c == '*' || c == '+' || c == '-' || c == '.' ||
           c == '^' || c == '_' || c == '`' || c == '|' || c == '~';
}

inline char lowerAscii(char c) {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
}

inline bool equalsIgnoreCase(const char *a, const char *b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (lowerAscii(*a++) != lowerAscii(*b++)) return false;
    }
    return *a == '\0' && *b == '\0';
}

inline bool copySpan(char *dst, size_t capacity, const char *src, size_t length) {
    if (!dst || !src || capacity == 0 || length >= capacity) return false;
    memcpy(dst, src, length);
    dst[length] = '\0';
    return true;
}

inline bool decodeComponent(const char *src, size_t length, char *dst, size_t capacity) {
    size_t out = 0;
    for (size_t i = 0; i < length; ++i) {
        unsigned char value = static_cast<unsigned char>(src[i]);
        if (value == '+') {
            value = ' ';
        } else if (value == '%') {
            if (i + 2 >= length) return false;
            const char a = src[i + 1];
            const char b = src[i + 2];
            const int hi = (a >= '0' && a <= '9') ? a - '0' :
                           (lowerAscii(a) >= 'a' && lowerAscii(a) <= 'f') ? lowerAscii(a) - 'a' + 10 : -1;
            const int lo = (b >= '0' && b <= '9') ? b - '0' :
                           (lowerAscii(b) >= 'a' && lowerAscii(b) <= 'f') ? lowerAscii(b) - 'a' + 10 : -1;
            if (hi < 0 || lo < 0) return false;
            value = static_cast<unsigned char>((hi << 4) | lo);
            i += 2;
        }
        // NUL and control bytes are not valid form parameter content here.
        if (value == 0 || value < 0x20 || value == 0x7f || out + 1 >= capacity) return false;
        dst[out++] = static_cast<char>(value);
    }
    dst[out] = '\0';
    return true;
}

inline bool addParameters(const char *encoded, size_t length, Request &request) {
    if (length == 0 || encoded[length - 1] == '&') return false;
    size_t pos = 0;
    while (pos < length) {
        size_t end = pos;
        while (end < length && encoded[end] != '&') ++end;
        if (end == pos) return false;
        size_t equal = pos;
        while (equal < end && encoded[equal] != '=') ++equal;
        if (equal == pos || request.paramCount >= MAX_PARAMS) return false;
        Parameter &p = request.params[request.paramCount];
        if (!decodeComponent(encoded + pos, equal - pos, p.name, sizeof(p.name))) return false;
        if (request.parameterValuesLength >= MAX_PARAM_VALUE_DATA) return false;
        p.valueOffset = static_cast<uint16_t>(request.parameterValuesLength);
        char *value = request.parameterValues + request.parameterValuesLength;
        const size_t remaining = MAX_PARAM_VALUE_DATA + 1 - request.parameterValuesLength;
        if (equal < end) {
            if (!decodeComponent(encoded + equal + 1, end - equal - 1, value, remaining)) return false;
        } else {
            value[0] = '\0';
        }
        p.valueLength = static_cast<uint16_t>(strlen(value));
        request.parameterValuesLength += static_cast<size_t>(p.valueLength) + 1;
        ++request.paramCount;
        pos = end + 1;
    }
    return true;
}

inline size_t headerOccurrences(const Request &request, const char *name) {
    size_t count = 0;
    if (!name) return 0;
    for (size_t i = 0; i < request.headerCount; ++i) {
        if (equalsIgnoreCase(request.headers[i].name, name)) ++count;
    }
    return count;
}

inline bool findHeader(const Request &request, const char *name, const char **value = 0) {
    bool found = false;
    for (size_t i = 0; i < request.headerCount; ++i) {
        if (equalsIgnoreCase(request.headers[i].name, name)) {
            if (found) return false; // Ambiguous duplicate security-sensitive header.
            found = true;
            if (value) *value = request.headers[i].value;
        }
    }
    return found;
}

inline bool parseUnsignedDecimal(const char *text, size_t length, size_t &value) {
    if (length == 0) return false;
    size_t result = 0;
    for (size_t i = 0; i < length; ++i) {
        if (text[i] < '0' || text[i] > '9') return false;
        const size_t digit = static_cast<size_t>(text[i] - '0');
        if (result > (static_cast<size_t>(-1) - digit) / 10) return false;
        result = result * 10 + digit;
    }
    value = result;
    return true;
}

inline ParseResult parseRequest(const uint8_t *bytes, size_t length, Request &out) {
    out.clear();
    if (!bytes || length == 0) return PARSE_INCOMPLETE;
    const char *data = reinterpret_cast<const char *>(bytes);
    size_t pos = 0;

    // Request line: exactly METHOD SP origin-form-target SP HTTP/1.x CRLF.
    size_t lineEnd = pos;
    while (lineEnd + 1 < length && !(data[lineEnd] == '\r' && data[lineEnd + 1] == '\n')) ++lineEnd;
    if (lineEnd + 1 >= length) return PARSE_INCOMPLETE;
    const size_t lineLength = lineEnd - pos;
    if (lineLength == 0) return PARSE_BAD_REQUEST;
    if (lineLength > MAX_REQUEST_LINE) return PARSE_TOO_LARGE;
    size_t firstSpace = pos;
    while (firstSpace < lineEnd && data[firstSpace] != ' ') ++firstSpace;
    size_t secondSpace = firstSpace + 1;
    while (secondSpace < lineEnd && data[secondSpace] != ' ') ++secondSpace;
    if (firstSpace == pos || firstSpace >= lineEnd || secondSpace <= firstSpace + 1 || secondSpace >= lineEnd ||
        secondSpace + 1 >= lineEnd) return PARSE_BAD_REQUEST;
    for (size_t i = pos; i < firstSpace; ++i) if (!isTokenChar(data[i])) return PARSE_BAD_REQUEST;
    if (!copySpan(out.method, sizeof(out.method), data + pos, firstSpace - pos) ||
        !copySpan(out.target, sizeof(out.target), data + firstSpace + 1, secondSpace - firstSpace - 1) ||
        !copySpan(out.version, sizeof(out.version), data + secondSpace + 1, lineEnd - secondSpace - 1)) return PARSE_TOO_LARGE;
    if (!equalsIgnoreCase(out.version, "HTTP/1.0") && !equalsIgnoreCase(out.version, "HTTP/1.1")) return PARSE_BAD_REQUEST;
    if (strcmp(out.method, "GET") != 0 && strcmp(out.method, "POST") != 0) return PARSE_UNSUPPORTED_METHOD;
    if (out.target[0] != '/' || strchr(out.target, '#') != 0) return PARSE_BAD_REQUEST;
    for (size_t i = 0; out.target[i]; ++i) {
        const unsigned char c = static_cast<unsigned char>(out.target[i]);
        if (c <= 0x20 || c == 0x7f) return PARSE_BAD_REQUEST;
    }

    const char *question = strchr(out.target, '?');
    const size_t targetLength = strlen(out.target);
    const size_t pathLength = question ? static_cast<size_t>(question - out.target) : targetLength;
    const size_t queryLength = question ? targetLength - pathLength - 1 : 0;
    if (pathLength == 0 || pathLength > MAX_PATH || queryLength > MAX_QUERY ||
        !copySpan(out.path, sizeof(out.path), out.target, pathLength)) return PARSE_TOO_LARGE;
    if (question && !copySpan(out.query, sizeof(out.query), question + 1, queryLength)) return PARSE_TOO_LARGE;
    if (question && queryLength && !addParameters(out.query, queryLength, out)) return PARSE_BAD_REQUEST;

    pos = lineEnd + 2;
    size_t headerBytes = 0;
    while (true) {
        size_t end = pos;
        while (end + 1 < length && !(data[end] == '\r' && data[end + 1] == '\n')) ++end;
        if (end + 1 >= length) return PARSE_INCOMPLETE;
        const size_t lineSize = end - pos;
        headerBytes += lineSize + 2;
        if (headerBytes > MAX_HEADER_BYTES) return PARSE_TOO_LARGE;
        if (lineSize == 0) { pos = end + 2; break; }
        if (data[pos] == ' ' || data[pos] == '\t' || out.headerCount >= MAX_HEADERS) return PARSE_BAD_REQUEST;
        size_t colon = pos;
        while (colon < end && data[colon] != ':') ++colon;
        if (colon == pos || colon == end) return PARSE_BAD_REQUEST;
        for (size_t i = pos; i < colon; ++i) if (!isTokenChar(data[i])) return PARSE_BAD_REQUEST;
        size_t valueStart = colon + 1;
        while (valueStart < end && (data[valueStart] == ' ' || data[valueStart] == '\t')) ++valueStart;
        size_t valueEnd = end;
        while (valueEnd > valueStart && (data[valueEnd - 1] == ' ' || data[valueEnd - 1] == '\t')) --valueEnd;
        for (size_t i = valueStart; i < valueEnd; ++i) {
            const unsigned char c = static_cast<unsigned char>(data[i]);
            if ((c < 0x20 && c != '\t') || c == 0x7f) return PARSE_BAD_REQUEST;
        }
        Header &h = out.headers[out.headerCount];
        if (!copySpan(h.name, sizeof(h.name), data + pos, colon - pos) ||
            !copySpan(h.value, sizeof(h.value), data + valueStart, valueEnd - valueStart)) return PARSE_TOO_LARGE;
        ++out.headerCount;
        pos = end + 2;
    }

    const char *transferEncoding = 0;
    if (findHeader(out, "Transfer-Encoding", &transferEncoding)) return PARSE_UNSUPPORTED_TRANSFER_ENCODING;
    // A duplicate Transfer-Encoding is rejected as ambiguous as well.
    for (size_t i = 0; i < out.headerCount; ++i) {
        if (equalsIgnoreCase(out.headers[i].name, "Transfer-Encoding")) return PARSE_UNSUPPORTED_TRANSFER_ENCODING;
    }

    size_t contentLength = 0;
    bool hasLength = false;
    for (size_t i = 0; i < out.headerCount; ++i) {
        if (equalsIgnoreCase(out.headers[i].name, "Content-Length")) {
            if (hasLength || !parseUnsignedDecimal(out.headers[i].value, strlen(out.headers[i].value), contentLength)) return PARSE_BAD_REQUEST;
            hasLength = true;
        }
    }
    if (!hasLength) contentLength = 0;
    if (contentLength && strcmp(out.method, "POST") != 0) return PARSE_BAD_REQUEST;
    if (contentLength > MAX_BODY) return PARSE_TOO_LARGE;
    if (length - pos < contentLength) return PARSE_INCOMPLETE;
    if (length - pos != contentLength) return PARSE_BAD_REQUEST; // One request per connection; no pipelining.
    if (contentLength) {
        memcpy(out.body, data + pos, contentLength);
        out.body[contentLength] = '\0';
        out.bodyLength = contentLength;
    }

    if (contentLength && strcmp(out.method, "POST") == 0) {
        const char *contentType = 0;
        if (!findHeader(out, "Content-Type", &contentType)) return PARSE_UNSUPPORTED_CONTENT_TYPE;
        // Accept only ordinary HTML form posts in this parser. Multipart OTA and
        // raw binary uploads are deliberately routed to a separate streaming path.
        const char *semi = strchr(contentType, ';');
        const size_t typeLength = semi ? static_cast<size_t>(semi - contentType) : strlen(contentType);
        char type[48];
        if (!copySpan(type, sizeof(type), contentType, typeLength) ||
            !equalsIgnoreCase(type, "application/x-www-form-urlencoded")) return PARSE_UNSUPPORTED_CONTENT_TYPE;
        if (!addParameters(out.body, out.bodyLength, out)) return PARSE_BAD_REQUEST;
    }
    return PARSE_OK;
}

inline const char *parameter(const Request &request, const char *name, size_t occurrence = 0) {
    if (!name) return 0;
    for (size_t i = 0; i < request.paramCount; ++i) {
        if (strcmp(request.params[i].name, name) == 0) {
            if (occurrence == 0) return request.parameterValues + request.params[i].valueOffset;
            --occurrence;
        }
    }
    return 0;
}

} // namespace Http
} // namespace HWC

#endif
