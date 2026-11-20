#include "HmacSha256.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void fromHex(const char* hex,uint8_t out[32]){for(unsigned i=0;i<32;i++){unsigned v=0;sscanf(hex+i*2,"%2x",&v);out[i]=uint8_t(v);}}
int main(){using namespace HWC;uint8_t key[20];memset(key,0x0b,sizeof(key));const char msg[]="Hi There";uint8_t tag[32],expected[32];
 fromHex("b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7",expected);hmacSha256(key,sizeof(key),reinterpret_cast<const uint8_t*>(msg),strlen(msg),tag);assert(constantTimeTagEquals(tag,expected));
 const uint8_t key2[]="Jefe";const char msg2[]="what do ya want for nothing?";fromHex("5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843",expected);hmacSha256(key2,sizeof(key2)-1,reinterpret_cast<const uint8_t*>(msg2),strlen(msg2),tag);assert(constantTimeTagEquals(tag,expected));
 tag[0]^=1;assert(!constantTimeTagEquals(tag,expected));
 uint8_t longKey[131];memset(longKey,0xaa,sizeof(longKey));const char data[]="Test Using Larger Than Block-Size Key - Hash Key First";fromHex("60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54",expected);hmacSha256(longKey,sizeof(longKey),reinterpret_cast<const uint8_t*>(data),strlen(data),tag);assert(constantTimeTagEquals(tag,expected));
 puts("PASS: HmacSha256 native tests");return 0;}
