#pragma once
#include "collision.h"
namespace fpv {
// Independently verified against Steam PE timestamp 0x5787641c.
// CCastRayInfo is 0x78 bytes, normal +0x18, fraction +0x24, world
// endpoints +0x50/+0x5c, collision layer +0x68, hasHit +0x70.
struct NativeCollision {
 uintptr_t base=0;bool available=false,faulted=false;unsigned long long queries=0,hits=0;
 bool init(uintptr_t b){base=b;const unsigned char ctor[]={0x48,0x89,0x4c,0x24,0x08,0x57,0x48,0x83,0xec,0x30};const unsigned char cast[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x48,0x89,0x78,0x10,0x55};available=!memcmp((void*)(b+0x26d090),ctor,sizeof ctor)&&!memcmp((void*)(b+0x29cc60),cast,sizeof cast);return available;}
 bool ready()const {if(!available||faulted)return false;__try{uintptr_t system=*(uintptr_t*)(base+0x2edc120);return system&&*(uintptr_t*)(system+0x30);}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
 bool ray(V a,V b,RayHit&out){
  if(!ready())return false;
  // No heap allocation and no foreign ownership. The native constructor sets
  // the result vtable/default filter; synchronous execute fills this buffer.
  alignas(16) unsigned char info[0x80]{};
  __try {
   ((void(__fastcall*)(void*,const char*))(base+0x26d090))(info,"JC3FPV collision");
   float*start=(float*)(info+0x50);float*end=(float*)(info+0x5c);start[0]=(float)a.x;start[1]=(float)a.y;start[2]=(float)a.z;end[0]=(float)b.x;end[1]=(float)b.y;end[2]=(float)b.z;
   ((void(__fastcall*)(void*))(base+0x29cc60))(info);++queries;
   if(!info[0x70])return false;out.fraction=*(float*)(info+0x24);auto*n=(float*)(info+0x18);out.normal={n[0],n[1],n[2]};++hits;return true;
  } __except(EXCEPTION_EXECUTE_HANDLER){faulted=true;return false;}
 }
};
}
