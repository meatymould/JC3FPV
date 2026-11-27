#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <Xinput.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "flight.h"
#include "native_collision.h"
#include "settings_ui.h"
using namespace fpv;
static Flight flight;static bool enabled=false;static uintptr_t base;static std::string folder,ini;static FILE* logFile;
static NativeCollision worldCollision;static CollisionSettings collisionSettings;static unsigned long long contacts=0;static double collisionTest=-1;static V collisionTestOrigin;
static LARGE_INTEGER lastTime,frequency;static V home;static double homeYaw=0,testTime=-1;static unsigned long long frames=0;static bool lastKeys[256]{};
static void log(const char*s){FILE*f=nullptr;if(fopen_s(&f,(folder+"JC3FPV.log").c_str(),"a")==0){fprintf(f,"%s\n",s);fclose(f);}}
static bool valid(void*p,size_t n=1){MEMORY_BASIC_INFORMATION m{};return p&&VirtualQuery(p,&m,sizeof m)&&m.State==MEM_COMMIT&&!(m.Protect&(PAGE_GUARD|PAGE_NOACCESS))&&(uintptr_t)p+n<=(uintptr_t)m.BaseAddress+m.RegionSize;}
static bool edge(int k){SHORT state=GetAsyncKeyState(k);bool s=(state&0x8000)!=0;bool e=(state&1)||(s&&!lastKeys[k]);lastKeys[k]=s;return e;}
static double cfg(const char*k,double d,double lo,double hi){char b[100],fallback[100];sprintf_s(fallback,"%.8g",d);GetPrivateProfileStringA("Flight",k,fallback,b,sizeof b,ini.c_str());double v=atof(b);return std::isfinite(v)?std::clamp(v,lo,hi):d;}
static void config(){auto&c=flight.c;
 c.gravity=cfg("Gravity",9.80665,0,30);c.speedLimit=cfg("SpeedLimit",0,0,100);c.dragScale=cfg("DragScale",1,0,5);
 collisionSettings.enabled=cfg("Collision",1,0,1)!=0;collisionSettings.radius=cfg("CollisionRadius",.18,.05,1);collisionSettings.skin=cfg("CollisionSkin",.015,.005,.1);collisionSettings.restitution=cfg("CollisionBounce",.08,0,.5);collisionSettings.friction=cfg("CollisionFriction",.18,0,1);
 c.mass=cfg("MassKg",.65,.1,5);c.maxThrust=cfg("MaxThrustN",26,1,200);c.dragArea=cfg("DragAreaM2",.018,.001,.5);c.verticalDragArea=cfg("VerticalDragAreaM2",.035,.001,.5);c.linearDrag=cfg("LinearDrag",.12,0,3);
 c.maxRate=cfg("MaxRate",670,100,1600);c.yawMaxRate=cfg("YawMaxRate",500,100,1600);c.centerRate=cfg("CenterRate",100,10,c.maxRate);c.expo=cfg("RateExpo",.3,0,1);
 c.rateTime=cfg("RateResponseSeconds",.035,.005,.5);c.maxAccel=cfg("MaxAngularAccel",18000,1000,100000);c.motorTime=cfg("MotorResponseSeconds",.025,.005,.3);c.thrustExponent=cfg("ThrustExponent",2,1,3);
 c.tilt=cfg("CameraTilt",25,0,80);c.fov=cfg("HorizontalFOV",110,50,150);c.deadzone=cfg("Deadzone",.05,0,.3);c.throttleExpo=cfg("ThrottleExpo",0,0,1);c.controller=(int)cfg("Controller",0,0,3);c.throttleMode=(int)cfg("ThrottleMode",0,0,1);settings::publish(c);log("Config loaded");
}
static double axis(SHORT v){double s=std::clamp(v/32767.,-1.,1.);double a=std::abs(s);return a<flight.c.deadzone?0:std::copysign((a-flight.c.deadzone)/(1-flight.c.deadzone),s);}
static Input input(){XINPUT_STATE s{};Input i{};if(XInputGetState(flight.c.controller,&s)==ERROR_SUCCESS){i.roll=axis(s.Gamepad.sThumbRX);i.pitch=axis(s.Gamepad.sThumbRY);i.yaw=axis(s.Gamepad.sThumbLX);i.throttle=flight.c.throttleMode?s.Gamepad.bRightTrigger/255.:(s.Gamepad.sThumbLY/32767.+1)*.5;}
 else {auto down=[](int k){return (GetAsyncKeyState(k)&0x8000)?1.:0.;};i.roll=down(VK_RIGHT)-down(VK_LEFT);i.pitch=down(VK_UP)-down(VK_DOWN);i.yaw=down('D')-down('A');i.throttle=down('W')*.65+down(VK_SPACE)*.35;}
 return i;}
static void setEnabled(bool on,const float*m){if(on&&collisionSettings.enabled&&!worldCollision.ready()){log("Cannot enable: native collision world is unavailable");return;}enabled=on;if(on){home={m[12],m[13]+2,m[14]};homeYaw=std::atan2(m[8],m[10]);flight.reset(home,homeYaw);QueryPerformanceCounter(&lastTime);log("FPV ENABLED: Acro. F7 disable, F8 reset, F9 reload. Mode2 leftY throttle; center is 50%.");}else log("FPV DISABLED; normal camera restored");}
static void tick(float*m){++frames;DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);bool foreground=pid==GetCurrentProcessId();
 if(settings::take(flight.c)){log("Settings preview applied");}
 bool f6=edge(VK_F6);if(settings::suppressF6){if(!(GetAsyncKeyState(VK_F6)&0x8000))settings::suppressF6=false;f6=false;}if(foreground&&!settings::open&&f6)settings::toggle();
 if(foreground&&!settings::open&&edge(VK_F7))setEnabled(!enabled,m);
 if(foreground&&!settings::open&&edge(VK_F9))config();
 if(enabled&&foreground&&!settings::open&&edge(VK_F8)){flight.reset(home,homeYaw);log("Reset");}
 // One-shot local test bridge: only checked every 30 camera calls, no input injection.
 if(frames%30==0){std::string cmd=folder+"command.txt";FILE*f=nullptr;if(fopen_s(&f,cmd.c_str(),"r")==0){char s[32]{};fgets(s,sizeof s,f);fclose(f);DeleteFileA(cmd.c_str());if(strncmp(s,"settings",8)==0)settings::toggle();if(strncmp(s,"probe",5)==0){V origin{m[12],m[13],m[14]};V dirs[6]={{0,-1,0},{0,1,0},{1,0,0},{-1,0,0},{0,0,1},{0,0,-1}};for(int j=0;j<6;j++){RayHit hit;bool ok=worldCollision.ray(origin,origin+dirs[j]*100,hit);char report[256];sprintf_s(report,"PROBE dir=%d hit=%d fraction=%.6f normal=%.3f %.3f %.3f fault=%d",j,ok,hit.fraction,hit.normal.x,hit.normal.y,hit.normal.z,worldCollision.faulted);log(report);}}
if(strncmp(s,"collisiontest",13)==0){setEnabled(true,m);collisionTest=0;collisionTestOrigin=flight.p;flight.v={0,-60,0};contacts=0;log("COLLISION TEST: 60m/s drop");}
if(strncmp(s,"enable",6)==0)setEnabled(true,m);if(strncmp(s,"disable",7)==0){setEnabled(false,m);testTime=-1;}if(strncmp(s,"reset",5)==0)flight.reset(home,homeYaw);if(strncmp(s,"test",4)==0){setEnabled(true,m);testTime=0;log("AUTOTEST start: climb, pitch forward, roll, restore");}}}
 LARGE_INTEGER now;QueryPerformanceCounter(&now);double dt=lastTime.QuadPart?double(now.QuadPart-lastTime.QuadPart)/frequency.QuadPart:0;lastTime=now;
 if(enabled){if(settings::open){flight.matrix(m);return;}Input controls=input();if(testTime>=0){testTime+=std::min(dt,.1);controls={0,testTime>4&&testTime<5?.1:0,0,.6};if(testTime>9&&testTime<10)controls.roll=.55;if(testTime>12){log("AUTOTEST complete; restored normal camera");setEnabled(false,m);testTime=-1;return;}}if(collisionTest>=0){collisionTest+=std::min(dt,.1);controls={};if(collisionTest>4){char report[256];sprintf_s(report,"COLLISION TEST END contacts=%llu deltaY=%.3f velocity=%.3f fault=%d",contacts,flight.p.y-collisionTestOrigin.y,flight.v.length(),worldCollision.faulted);log(report);setEnabled(false,m);collisionTest=-1;return;}}
if((foreground||testTime>=0||collisionTest>=0)&&dt<.25){if(collisionSettings.enabled&&!worldCollision.ready()){setEnabled(false,m);log("Collision unavailable: flight disabled");return;}if(advanceCollision(flight,controls,dt,collisionSettings,[](V a,V b,RayHit&h){return worldCollision.ray(a,b,h);})){++contacts;if(contacts==1||contacts%120==0){char report[256];sprintf_s(report,"CONTACT count=%llu pos=%.3f %.3f %.3f speed=%.3f",contacts,flight.p.x,flight.p.y,flight.p.z,flight.v.length());log(report);}}if(worldCollision.faulted){setEnabled(false,m);log("Native collision fault: flight disabled");return;}}flight.matrix(m);if(frames%120==0){char line[256];sprintf_s(line,"frame=%llu enabled=1 pos=%.2f %.2f %.2f speed=%.2f physics=%llu",frames,flight.p.x,flight.p.y,flight.p.z,flight.v.length(),flight.steps);log(line);}}
 else if(frames%600==0){char line[256];sprintf_s(line,"camera alive frame=%llu pos=%.2f %.2f %.2f",frames,m[12],m[13],m[14]);log(line);}
}
using TransformFn=void(__fastcall*)(void*,const float*);using FovFn=void(__fastcall*)(void*,float);
static TransformFn originalTransform;static FovFn originalFov;
static void __fastcall cameraHook(void*manager,const float*source){float m[16];memcpy(m,source,64);tick(m);originalTransform(manager,m);if(enabled){ // JC3 FOV scalar is vertical radians; convert configured horizontal using camera aspect.
 uintptr_t c=*(uintptr_t*)((uintptr_t)manager+0x5c0);if(valid((void*)(c+0x588),4)){float aspect=16.f/9.f; // replace with window client aspect when available
 RECT r{};if(GetClientRect(FindWindowW(nullptr,L"Just Cause 3"),&r)&&r.bottom>0)aspect=float(r.right)/r.bottom;
 float vf=float(2*std::atan(std::tan(flight.c.fov*rad/2)/aspect));originalFov(manager,vf);*(float*)(c+0x588)=vf;}}
}
static void __fastcall fovHook(void*m,float f){if(enabled)f=float(2*std::atan(std::tan(flight.c.fov*rad/2)/(16./9.)));originalFov(m,f);}
static void jump(unsigned char*p,void*to){p[0]=0xff;p[1]=0x25;memset(p+2,0,4);memcpy(p+6,&to,8);}
static void* hook(uintptr_t rva,void*fn,size_t len){auto*p=(unsigned char*)(base+rva);auto*t=(unsigned char*)VirtualAlloc(nullptr,len+14,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);if(!t)return nullptr;memcpy(t,p,len);jump(t+len,p+len);DWORD old;if(!VirtualProtect(p,len,PAGE_EXECUTE_READWRITE,&old))return nullptr;jump(p,fn);memset(p+14,0x90,len-14);VirtualProtect(p,len,old,&old);FlushInstructionCache(GetCurrentProcess(),p,len);return t;}
// Neutralise the game's imported XInput only; our DLL still polls the unmodified system API.
static DWORD WINAPI gameInput(DWORD idx,XINPUT_STATE*s){DWORD r=XInputGetState(idx,s);if((enabled||settings::open)&&r==ERROR_SUCCESS)memset(&s->Gamepad,0,sizeof s->Gamepad);return r;}
static void blockGamepad(){auto*dos=(IMAGE_DOS_HEADER*)base;auto*nt=(IMAGE_NT_HEADERS64*)(base+dos->e_lfanew);auto dir=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];if(!dir.VirtualAddress)return;auto*d=(IMAGE_IMPORT_DESCRIPTOR*)(base+dir.VirtualAddress);for(;d->Name;d++){if(!d->OriginalFirstThunk)continue;auto*n=(IMAGE_THUNK_DATA64*)(base+d->OriginalFirstThunk);auto*t=(IMAGE_THUNK_DATA64*)(base+d->FirstThunk);for(;n->u1.AddressOfData;n++,t++){if(IMAGE_SNAP_BY_ORDINAL64(n->u1.Ordinal))continue;auto*b=(IMAGE_IMPORT_BY_NAME*)(base+n->u1.AddressOfData);if(strcmp((char*)b->Name,"XInputGetState")==0){DWORD old;VirtualProtect(&t->u1.Function,8,PAGE_READWRITE,&old);t->u1.Function=(ULONGLONG)&gameInput;VirtualProtect(&t->u1.Function,8,old,&old);log("Game XInput import isolated");}}}}
static DWORD WINAPI init(void*module){char path[MAX_PATH];GetModuleFileNameA((HMODULE)module,path,MAX_PATH);folder=path;folder=folder.substr(0,folder.find_last_of("\\/")+1);ini=folder+"JC3FPV.ini";fopen_s(&logFile,(folder+"JC3FPV.log").c_str(),"w");if(logFile){fclose(logFile);logFile=nullptr;}log("JC3FPV standalone prototype starting");base=(uintptr_t)GetModuleHandleW(nullptr);log(worldCollision.init(base)?"Native collision signatures verified":"Native collision unsupported");QueryPerformanceFrequency(&frequency);config();
 auto*nt=(IMAGE_NT_HEADERS64*)(base+((IMAGE_DOS_HEADER*)base)->e_lfanew);
 const unsigned char expected[]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0x99,0xc0,0x05,0x00,0x00};
 const unsigned char ef[]={0x48,0x8b,0x81,0xc0,0x05,0,0,0x80,0x88,0x5e,0x05,0,0,0x10};
 if(nt->FileHeader.TimeDateStamp!=0x5787641c||memcmp((void*)(base+0x9d390),expected,sizeof expected)||memcmp((void*)(base+0x9d400),ef,sizeof ef)){log("Unsupported executable or conflicting camera hook. No patches installed.");return 0;}
 // Suspend other threads only for the short patch transaction; complete instructions are copied.
 std::vector<HANDLE> suspended;HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);THREADENTRY32 te{sizeof te};if(Thread32First(snap,&te))do{if(te.th32OwnerProcessID==GetCurrentProcessId()&&te.th32ThreadID!=GetCurrentThreadId()){HANDLE h=OpenThread(THREAD_SUSPEND_RESUME,FALSE,te.th32ThreadID);if(h){if(SuspendThread(h)!=(DWORD)-1)suspended.push_back(h);else CloseHandle(h);}}}while(Thread32Next(snap,&te));CloseHandle(snap);
 originalTransform=(TransformFn)hook(0x9d390,(void*)&cameraHook,17);originalFov=(FovFn)hook(0x9d400,(void*)&fovHook,14);blockGamepad();
 for(HANDLE h:suspended){ResumeThread(h);CloseHandle(h);}settings::start((HINSTANCE)module,ini,flight.c);log(originalTransform&&originalFov?"Camera hooks installed; F6 settings, F7 to enable in gameplay":"HOOK FAILURE");return 0;}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){DisableThreadLibraryCalls(h);HANDLE t=CreateThread(nullptr,0,init,h,0,nullptr);if(t)CloseHandle(t);}return TRUE;}

