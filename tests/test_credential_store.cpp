#include "CredentialProvisioningController.h"
#include <assert.h>
#include <map>
#include <string>
#include <stdio.h>

class FakeBackend : public HWC::CredentialBackend {
public:
    bool failPut; bool failGet; bool mismatch; bool commitWriteThenFail;
    std::string failPutKey; std::map<std::string, std::string> data;
    FakeBackend() : failPut(false), failGet(false), mismatch(false), commitWriteThenFail(false) {}
    HWC::CredentialReadResult get(const char* key, char* out, size_t cap) {
        if (failGet) return HWC::CREDENTIAL_READ_ERROR;
        std::map<std::string,std::string>::const_iterator it = data.find(key);
        if (it == data.end()) return HWC::CREDENTIAL_READ_MISSING;
        if (it->second.size() >= cap) return HWC::CREDENTIAL_READ_ERROR;
        std::string v = it->second;
        if (mismatch && std::string(key) == "c0_ap") v += "X";
        if (v.size() >= cap) return HWC::CREDENTIAL_READ_ERROR;
        memcpy(out, v.c_str(), v.size() + 1U); return HWC::CREDENTIAL_READ_FOUND;
    }
    bool put(const char* key, const char* value) {
        if (failPut || (!failPutKey.empty() && failPutKey == key)) {
            if (commitWriteThenFail && std::string(key) == "cred_active") data[key] = value;
            return false;
        }
        data[key] = value; return true;
    }
    bool erase(const char* key) { data.erase(key); return true; }
};

static bool physicalProvision(HWC::CredentialStore& store, HWC::Credentials& input, uint32_t start) {
    HWC::LocalServiceAuthorization gate;
    HWC::CredentialProvisioningController controller(store, gate);
    gate.samplePhysicalInput(start, true);
    gate.samplePhysicalInput(static_cast<uint32_t>(start + HWC::LocalServiceAuthorization::HOLD_MS), true);
    if (!controller.openSession(static_cast<uint32_t>(start + HWC::LocalServiceAuthorization::HOLD_MS))) {
        HWC::CredentialStore::clear(input);
        return false;
    }
    return controller.provision(static_cast<uint32_t>(start + HWC::LocalServiceAuthorization::HOLD_MS + 1U), input) == HWC::CREDENTIAL_PROVISIONED;
}

int main() {
    FakeBackend b; HWC::CredentialStore s(b); HWC::Credentials c, out;
    HWC::CredentialStore::clear(c);
    strcpy(c.apName, "HWC-UPPER-1234"); strcpy(c.apPassword, "AP-pass-123456");
    strcpy(c.adminPassword, "Admin-pass-123456");
    HWC::Credentials overlongApPassword = c;
    memset(overlongApPassword.apPassword, 'P', 64U);
    overlongApPassword.apPassword[64] = '\0';
    assert(!HWC::CredentialStore::validate(overlongApPassword));
    HWC::CredentialStore::clear(overlongApPassword);
    HWC::Credentials controlCharacter = c;
    controlCharacter.apPassword[2] = '\n';
    assert(!HWC::CredentialStore::validate(controlCharacter));
    HWC::CredentialStore::clear(controlCharacter);
    HWC::Credentials nonAscii = c;
    nonAscii.adminPassword[1] = static_cast<char>(0xC3);
    assert(!HWC::CredentialStore::validate(nonAscii));
    HWC::CredentialStore::clear(nonAscii);
    assert(s.load(out) == HWC::CREDENTIAL_EMPTY);
    HWC::Credentials failedFirst = c;
    b.failPut = true; assert(!physicalProvision(s, failedFirst, 100U)); assert(s.load(out) == HWC::CREDENTIAL_EMPTY);
    b.failPut = false; b.mismatch = true;
    HWC::Credentials mismatchFirst = c; assert(!physicalProvision(s, mismatchFirst, 10000U));
    b.mismatch = false;
    HWC::Credentials initial = c; assert(physicalProvision(s, initial, 20000U));
    assert(s.load(out) == HWC::CREDENTIAL_VALID);
    assert(strcmp(out.apPassword, c.apPassword) == 0);
    HWC::Credentials next = c; strcpy(next.apPassword, "AP-pass-654321");
    b.failPut = true; assert(!physicalProvision(s, next, 30000U));
    b.failPut = false; assert(s.load(out) == HWC::CREDENTIAL_VALID);
    assert(strcmp(out.apPassword, c.apPassword) == 0);
    // Simulate NVS reporting an error after the selector actually reached flash.
    // The read-back, not the put() return code, determines the committed state.
    HWC::Credentials committed = c; strcpy(committed.apPassword, "AP-pass-999999");
    HWC::Credentials committedExpected = committed;
    b.failPutKey = "cred_active"; b.commitWriteThenFail = true;
    assert(physicalProvision(s, committed, 40000U));
    b.failPutKey.clear(); b.commitWriteThenFail = false;
    assert(s.load(out) == HWC::CREDENTIAL_VALID);
    assert(strcmp(out.apPassword, committedExpected.apPassword) == 0);
    b.data.erase("c1_admin"); assert(s.load(out) == HWC::CREDENTIAL_CORRUPT);
    b.failGet = true; assert(s.load(out) == HWC::CREDENTIAL_IO_ERROR);
    puts("PASS: CredentialStore staging/read-back/commit policy tests");
}
