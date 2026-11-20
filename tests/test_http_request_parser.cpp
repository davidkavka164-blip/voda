#include "HttpRequestParser.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

using namespace HWC::Http;

static ParseResult parse(const char *wire, Request &r) {
    return parseRequest(reinterpret_cast<const uint8_t *>(wire), strlen(wire), r);
}

int main() {
    Request r;
    assert(parse("GET /?mode=auto&name=Upper+Tank HTTP/1.1\r\nHost: 192.168.0.10\r\nOrigin: http://192.168.0.10\r\n\r\n", r) == PARSE_OK);
    assert(strcmp(r.method, "GET") == 0 && strcmp(r.path, "/") == 0);
    assert(strcmp(parameter(r, "mode"), "auto") == 0);
    assert(strcmp(parameter(r, "name"), "Upper Tank") == 0);
    assert(strcmp(r.headers[0].name, "Host") == 0);

    assert(parse("POST /save HTTP/1.1\r\nHost: box\r\nOrigin: http://box\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 19\r\n\r\nmode=on&value=42%25", r) == PARSE_OK);
    assert(strcmp(parameter(r, "mode"), "on") == 0);
    assert(strcmp(parameter(r, "value"), "42%") == 0);

    assert(parse("POST /x HTTP/1.0\r\nContent-Type: application/x-www-form-urlencoded; charset=UTF-8\r\nContent-Length: 3\r\n\r\na=b", r) == PARSE_OK);
    assert(parse("GET /a%20b HTTP/1.1\r\n\r\n", r) == PARSE_OK); // Path is not decoded.
    assert(strcmp(r.path, "/a%20b") == 0);

    assert(parse("PUT / HTTP/1.1\r\n\r\n", r) == PARSE_UNSUPPORTED_METHOD);
    assert(parse("GET http://box/ HTTP/1.1\r\n\r\n", r) == PARSE_BAD_REQUEST);
    assert(parse("GET /bad\tpath HTTP/1.1\r\n\r\n", r) == PARSE_BAD_REQUEST);
    assert(parse("GET / HTTP/2.0\r\n\r\n", r) == PARSE_BAD_REQUEST);
    assert(parse("GET / HTTP/1.1\nHost: box\n\n", r) == PARSE_INCOMPLETE);
    assert(parse("GET / HTTP/1.1\r\n folded: no\r\n\r\n", r) == PARSE_BAD_REQUEST);
    assert(parse("GET / HTTP/1.1\r\nHost: a\r\nHost: b\r\n\r\n", r) == PARSE_OK);
    const char *host = 0;
    assert(!findHeader(r, "Host", &host)); // Duplicate security-sensitive header is ambiguous.
    assert(headerOccurrences(r, "host") == 2);
    assert(parse("POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n", r) == PARSE_UNSUPPORTED_TRANSFER_ENCODING);
    assert(parse("POST / HTTP/1.1\r\nContent-Length: 2\r\nContent-Type: text/plain\r\n\r\nhi", r) == PARSE_UNSUPPORTED_CONTENT_TYPE);
    assert(parse("POST / HTTP/1.1\r\nContent-Length: 3\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\na=%", r) == PARSE_BAD_REQUEST);
    assert(parse("GET /?x=%GG HTTP/1.1\r\n\r\n", r) == PARSE_BAD_REQUEST);
    assert(parse("GET /?x=%00 HTTP/1.1\r\n\r\n", r) == PARSE_BAD_REQUEST);
    assert(parse("GET /?=bad HTTP/1.1\r\n\r\n", r) == PARSE_BAD_REQUEST);
    assert(parse("GET /?x=1& HTTP/1.1\r\n\r\n", r) == PARSE_BAD_REQUEST);
    assert(parse("GET /?x=1 HTTP/1.1\r\nContent-Length: 1\r\n\r\na", r) == PARSE_BAD_REQUEST);
    assert(parse("POST / HTTP/1.1\r\nContent-Length: 4\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\na=1", r) == PARSE_INCOMPLETE);
    assert(parse("POST / HTTP/1.1\r\nContent-Length: 1025\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n", r) == PARSE_TOO_LARGE);
    assert(parse("GET /?a=first&a=second HTTP/1.1\r\n\r\n", r) == PARSE_OK);
    assert(strcmp(parameter(r, "a", 0), "first") == 0);
    assert(strcmp(parameter(r, "a", 1), "second") == 0);
    assert(parse("GET /?x=hello%20world HTTP/1.1\r\n\r\n", r) == PARSE_OK);
    assert(strcmp(parameter(r, "x"), "hello world") == 0);
    char longValue[301];
    for (size_t i = 0; i < 300; ++i) longValue[i] = 'a';
    longValue[300] = '\0';
    char longRequest[768];
    snprintf(longRequest, sizeof(longRequest), "POST / HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 307\r\n\r\npoints=%s", longValue);
    assert(parse(longRequest, r) == PARSE_OK);
    assert(strlen(parameter(r, "points")) == 300);

    // Browsers can send longer Accept and User-Agent values than REV96 accepted.
    char browserValue[221];
    memset(browserValue, 'a', sizeof(browserValue) - 1);
    browserValue[sizeof(browserValue) - 1] = '\0';
    char browserRequest[1900];
    snprintf(browserRequest, sizeof(browserRequest),
             "GET /status HTTP/1.1\r\nHost: 192.168.0.231\r\n"
             "Accept: %s\r\nUser-Agent: %s\r\nAccept-Language: %s\r\n"
             "Cookie: %s\r\nReferer: %s\r\nAuthorization: Basic dGVzdA==\r\n\r\n",
             browserValue, browserValue, browserValue, browserValue, browserValue);
    assert(strlen(browserRequest) > 1024);
    assert(parse(browserRequest, r) == PARSE_OK);
    const char *authorization = 0;
    assert(findHeader(r, "Authorization", &authorization));
    assert(strcmp(authorization, "Basic dGVzdA==") == 0);

    char oversized[MAX_HEADER_VALUE + 2];
    memset(oversized, 'b', sizeof(oversized) - 1);
    oversized[sizeof(oversized) - 1] = '\0';
    snprintf(browserRequest, sizeof(browserRequest),
             "GET /status HTTP/1.1\r\nAccept: %s\r\n\r\n", oversized);
    assert(parse(browserRequest, r) == PARSE_TOO_LARGE);

    puts("PASS: HTTP request parser tests");
    return 0;
}
