#include "HmacSha256.h"
#include <string.h>

namespace HWC {
namespace {
static const uint32_t K[64] = {
0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U};
struct Sha256 { uint32_t h[8]; uint64_t bits; uint8_t block[64]; size_t used; };
uint32_t rotr(uint32_t x, unsigned n) { return (x >> n) | (x << (32U-n)); }
void transform(Sha256& s, const uint8_t b[64]) {
 uint32_t w[64]; for (unsigned i=0;i<16;i++) w[i]=(uint32_t(b[i*4])<<24)|(uint32_t(b[i*4+1])<<16)|(uint32_t(b[i*4+2])<<8)|b[i*4+3];
 for(unsigned i=16;i<64;i++){uint32_t a=rotr(w[i-15],7)^rotr(w[i-15],18)^(w[i-15]>>3);uint32_t z=rotr(w[i-2],17)^rotr(w[i-2],19)^(w[i-2]>>10);w[i]=w[i-16]+a+w[i-7]+z;}
 uint32_t a=s.h[0],c=s.h[2],b0=s.h[1],d=s.h[3],e=s.h[4],f=s.h[5],g=s.h[6],h=s.h[7];
 for(unsigned i=0;i<64;i++){uint32_t s1=rotr(e,6)^rotr(e,11)^rotr(e,25),ch=(e&f)^(~e&g),t1=h+s1+ch+K[i]+w[i];uint32_t s0=rotr(a,2)^rotr(a,13)^rotr(a,22),maj=(a&b0)^(a&c)^(b0&c),t2=s0+maj;h=g;g=f;f=e;e=d+t1;d=c;c=b0;b0=a;a=t1+t2;}
 s.h[0]+=a;s.h[1]+=b0;s.h[2]+=c;s.h[3]+=d;s.h[4]+=e;s.h[5]+=f;s.h[6]+=g;s.h[7]+=h;
}
void init(Sha256& s){const uint32_t iv[8]={0x6a09e667U,0xbb67ae85U,0x3c6ef372U,0xa54ff53aU,0x510e527fU,0x9b05688cU,0x1f83d9abU,0x5be0cd19U};memcpy(s.h,iv,sizeof(iv));s.bits=0;s.used=0;}
void update(Sha256& s,const uint8_t* p,size_t n){while(n){size_t take=64-s.used;if(take>n)take=n;memcpy(s.block+s.used,p,take);s.used+=take;p+=take;n-=take;s.bits+=uint64_t(take)*8U;if(s.used==64){transform(s,s.block);s.used=0;}}}
void finish(Sha256& s,uint8_t out[32]){uint64_t bits=s.bits;s.block[s.used++]=0x80;if(s.used>56){while(s.used<64)s.block[s.used++]=0;transform(s,s.block);s.used=0;}while(s.used<56)s.block[s.used++]=0;for(int i=7;i>=0;i--)s.block[s.used++]=uint8_t(bits>>(i*8));transform(s,s.block);for(unsigned i=0;i<8;i++){out[i*4]=uint8_t(s.h[i]>>24);out[i*4+1]=uint8_t(s.h[i]>>16);out[i*4+2]=uint8_t(s.h[i]>>8);out[i*4+3]=uint8_t(s.h[i]);}memset(&s,0,sizeof(s));}
}
void hmacSha256(const uint8_t* key,size_t keyLength,const uint8_t* message,size_t messageLength,uint8_t output[HMAC_SHA256_SIZE]){
 uint8_t k0[64]={0},innerPad[64],outerPad[64],inner[32];
 if(keyLength>64U){Sha256 hs;init(hs);update(hs,key,keyLength);finish(hs,k0);}else if(keyLength && key)memcpy(k0,key,keyLength);
 for(size_t i=0;i<64;i++){innerPad[i]=uint8_t(k0[i]^0x36U);outerPad[i]=uint8_t(k0[i]^0x5cU);}
 Sha256 hs;init(hs);update(hs,innerPad,64);if(messageLength && message)update(hs,message,messageLength);finish(hs,inner);
 init(hs);update(hs,outerPad,64);update(hs,inner,32);finish(hs,output);
 memset(k0,0,sizeof(k0));memset(innerPad,0,sizeof(innerPad));memset(outerPad,0,sizeof(outerPad));memset(inner,0,sizeof(inner));
}
bool constantTimeTagEquals(const uint8_t a[HMAC_SHA256_SIZE],const uint8_t b[HMAC_SHA256_SIZE]){uint8_t diff=0;for(size_t i=0;i<HMAC_SHA256_SIZE;i++)diff=uint8_t(diff|(a[i]^b[i]));return diff==0;}
}
