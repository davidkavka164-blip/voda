#include "HttpOriginPolicy.h"
#include <assert.h>

int main() {
    using HWC::isSameHttpOriginAuthority;
    assert(isSameHttpOriginAuthority("http://192.168.0.10", "192.168.0.10"));
    assert(isSameHttpOriginAuthority("HTTP://DEVICE.LOCAL:80", "device.local:80"));
    assert(!isSameHttpOriginAuthority("https://192.168.0.10", "192.168.0.10"));
    assert(!isSameHttpOriginAuthority("http://192.168.0.10/evil", "192.168.0.10"));
    assert(!isSameHttpOriginAuthority("http://user@192.168.0.10", "192.168.0.10"));
    assert(!isSameHttpOriginAuthority("http://192.168.0.10?x=1", "192.168.0.10"));
    assert(!isSameHttpOriginAuthority("http://192.168.0.10", "192.168.0.10:80"));
    assert(!isSameHttpOriginAuthority("http://192.168.0.10", ""));
    assert(!isSameHttpOriginAuthority(nullptr, "192.168.0.10"));
    assert(!isSameHttpOriginAuthority("http://", "192.168.0.10"));
    char longHost[130]; for (int i=0;i<129;++i) longHost[i]='a'; longHost[129]='\0';
    assert(!isSameHttpOriginAuthority("http://aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", longHost));
    return 0;
}
