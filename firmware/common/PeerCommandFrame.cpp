#include "PeerCommandFrame.h"
#include "HmacSha256.h"
#include <string.h>
namespace HWC { namespace {
void put16(uint8_t* p,uint16_t v){p[0]=uint8_t(v);p[1]=uint8_t(v>>8);}
void put32(uint8_t* p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=uint8_t(v>>(8U*i));}
uint16_t get16(const uint8_t* p){return uint16_t(p[0])|uint16_t(uint16_t(p[1])<<8);}
uint32_t get32(const uint8_t* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
bool valid(const PeerCommandFrame& f){return (f.frameType==PEER_FRAME_COMMAND||f.frameType==PEER_FRAME_RESPONSE)&&f.commandCode!=0U&&f.senderDeviceId!=0U&&f.targetDeviceId!=0U&&f.senderBootId!=0U&&f.targetBootId!=0U&&f.sequence!=0U&&f.requestId!=0U;}
void serialize(const PeerCommandFrame& f,uint8_t* p){put32(p,PEER_COMMAND_MAGIC);put16(p+4,PEER_COMMAND_VERSION);p[6]=f.frameType;p[7]=f.commandCode;p[8]=f.senderDeviceId;p[9]=f.targetDeviceId;p[10]=0;p[11]=0;put32(p+12,f.senderBootId);put32(p+16,f.targetBootId);put32(p+20,f.sequence);put32(p+24,f.requestId);put32(p+28,static_cast<uint32_t>(f.argument0));put32(p+32,static_cast<uint32_t>(f.argument1));}
}
bool encodePeerCommandFrame(const PeerCommandFrame& f,const uint8_t* key,size_t keyLength,uint8_t* out,size_t capacity,size_t& written){written=0;if(!valid(f)||!key||keyLength<PEER_COMMAND_MIN_KEY_SIZE||!out||capacity<PEER_COMMAND_FRAME_SIZE)return false;serialize(f,out);hmacSha256(key,keyLength,out,PEER_COMMAND_SIGNED_SIZE,out+PEER_COMMAND_SIGNED_SIZE);written=PEER_COMMAND_FRAME_SIZE;return true;}
bool decodePeerCommandFrame(const uint8_t* data,size_t length,const uint8_t* key,size_t keyLength,PeerCommandFrame& out){if(!data||length!=PEER_COMMAND_FRAME_SIZE||!key||keyLength<PEER_COMMAND_MIN_KEY_SIZE)return false;if(get32(data)!=PEER_COMMAND_MAGIC||get16(data+4)!=PEER_COMMAND_VERSION||data[10]!=0U||data[11]!=0U)return false;uint8_t expected[HMAC_SHA256_SIZE];hmacSha256(key,keyLength,data,PEER_COMMAND_SIGNED_SIZE,expected);if(!constantTimeTagEquals(expected,data+PEER_COMMAND_SIGNED_SIZE)){memset(expected,0,sizeof(expected));return false;}memset(expected,0,sizeof(expected));PeerCommandFrame f;f.frameType=data[6];f.commandCode=data[7];f.senderDeviceId=data[8];f.targetDeviceId=data[9];f.senderBootId=get32(data+12);f.targetBootId=get32(data+16);f.sequence=get32(data+20);f.requestId=get32(data+24);f.argument0=static_cast<int32_t>(get32(data+28));f.argument1=static_cast<int32_t>(get32(data+32));if(!valid(f))return false;out=f;return true;}
}
