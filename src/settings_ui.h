#pragma once
#include <commctrl.h>
#include <atomic>
#include <mutex>
#include <string>
#include "flight.h"
namespace settings {
using fpv::Config;
struct Field {const wchar_t*label;const char*key;double Config::*member;double lo,hi,step,scale;const wchar_t*unit;};
inline Field fields[]={
 {L"Weight","MassKg",&Config::mass,100,5000,10,1000,L"g"},
 {L"Maximum thrust","MaxThrustN",&Config::maxThrust,1,200,1,1,L"N"},
 {L"Speed limit (0 = unlimited)","SpeedLimit",&Config::speedLimit,0,360,1,3.6,L"km/h"},
 {L"Gravity","Gravity",&Config::gravity,0,30,.1,1,L"m/s2"},
 {L"Pitch / roll speed","MaxRate",&Config::maxRate,100,1600,10,1,L"deg/s"},
 {L"Yaw speed","YawMaxRate",&Config::yawMaxRate,100,1600,10,1,L"deg/s"},
 {L"Stick centre sensitivity","CenterRate",&Config::centerRate,10,500,5,1,L"deg/s"},
 {L"Stick expo","RateExpo",&Config::expo,0,1,.01,1,L""},
 {L"Throttle expo","ThrottleExpo",&Config::throttleExpo,0,1,.01,1,L""},
 {L"Air drag multiplier","DragScale",&Config::dragScale,0,5,.05,1,L"x"},
 {L"Motor response","MotorResponseSeconds",&Config::motorTime,5,200,1,1000,L"ms"},
 {L"Rotation response","RateResponseSeconds",&Config::rateTime,5,300,1,1000,L"ms"},
 {L"Camera tilt","CameraTilt",&Config::tilt,0,80,1,1,L"deg"},
 {L"Horizontal field of view","HorizontalFOV",&Config::fov,50,150,1,1,L"deg"}
};
constexpr int count=sizeof(fields)/sizeof(fields[0]);
inline std::mutex mutex;inline Config published,pending,editing;inline bool changed=false;
inline std::atomic<bool> open{false},suppressF6{false};inline std::atomic<DWORD> threadId{0};inline HWND window=nullptr,sliders[count]{},values[count]{},status=nullptr;inline HFONT font=nullptr;inline HINSTANCE instance=nullptr;inline std::string configPath;
inline void publish(const Config&c){std::lock_guard<std::mutex>l(mutex);published=c;}
inline bool take(Config&c){std::lock_guard<std::mutex>l(mutex);if(!changed)return false;c=pending;published=c;changed=false;return true;}
inline void queue(){editing.centerRate=std::min(editing.centerRate,std::min(editing.maxRate,editing.yawMaxRate));std::lock_guard<std::mutex>l(mutex);pending=editing;changed=true;}
inline void refresh(){for(int i=0;i<count;i++){auto&f=fields[i];double value=editing.*(f.member)*f.scale;SendMessageW(sliders[i],TBM_SETPOS,TRUE,(LPARAM)std::lround((value-f.lo)/f.step));wchar_t text[64];swprintf_s(text,L"%.2f %s",value,f.unit);SetWindowTextW(values[i],text);}}
inline bool save(){bool ok=true;for(auto&f:fields){char s[64];sprintf_s(s,"%.9g",editing.*(f.member));ok=WritePrivateProfileStringA("Flight",f.key,s,configPath.c_str())&&ok;}return ok;}
inline HWND child(const wchar_t*type,const wchar_t*text,DWORD style,int x,int y,int w,int h,int id){HWND c=CreateWindowExW(0,type,text,WS_CHILD|WS_VISIBLE|style,x,y,w,h,window,(HMENU)(INT_PTR)id,instance,nullptr);SendMessageW(c,WM_SETFONT,(WPARAM)font,TRUE);return c;}
inline void hide(){suppressF6=true;open=false;ShowWindow(window,SW_HIDE);HWND game=FindWindowW(nullptr,L"Just Cause 3");if(game)SetForegroundWindow(game);}
inline LRESULT CALLBACK proc(HWND w,UINT message,WPARAM wp,LPARAM lp){
 switch(message){
 case WM_CLOSE:hide();return 0;
 case WM_HSCROLL:{HWND source=(HWND)lp;for(int i=0;i<count;i++)if(source==sliders[i]){auto&f=fields[i];editing.*(f.member)=(f.lo+SendMessageW(source,TBM_GETPOS,0,0)*f.step)/f.scale;queue();refresh();SetWindowTextW(status,L"Preview applied. Save to keep these settings next time.");break;}return 0;}
 case WM_COMMAND:switch(LOWORD(wp)){
  case 1:queue();SetWindowTextW(status,save()?L"Saved. These settings will load next time.":L"Could not save settings. Preview remains active.");return 0;
  case 2:editing=Config{};queue();refresh();SetWindowTextW(status,L"Default physics restored. Save to keep them.");return 0;
  case 3:hide();return 0;
 }break;
 }return DefWindowProcW(w,message,wp,lp);
}
inline DWORD WINAPI run(void*){
 threadId=GetCurrentThreadId();MSG msg{};PeekMessageW(&msg,nullptr,0,0,PM_NOREMOVE);
 INITCOMMONCONTROLSEX cc{sizeof cc,ICC_BAR_CLASSES};InitCommonControlsEx(&cc);
 WNDCLASSW wc{};wc.hInstance=instance;wc.lpfnWndProc=proc;wc.lpszClassName=L"JC3FPVSettings";wc.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512));wc.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);RegisterClassW(&wc);
 font=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
 RECT bounds{0,0,720,806};AdjustWindowRectEx(&bounds,WS_CAPTION|WS_SYSMENU,FALSE,WS_EX_APPWINDOW|WS_EX_TOPMOST);
 window=CreateWindowExW(WS_EX_APPWINDOW|WS_EX_TOPMOST,wc.lpszClassName,L"JC3FPV - Drone settings (F6)",WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,bounds.right-bounds.left,bounds.bottom-bounds.top,nullptr,nullptr,instance,nullptr);
 if(!window){threadId=0;return 1;}
 child(L"STATIC",L"DRONE SETTINGS",0,22,16,650,25,0);
 child(L"STATIC",L"Flight pauses while this window is open. Sliders preview changes immediately.",0,22,46,680,23,0);
 for(int i=0;i<count;i++){int y=82+i*45;child(L"STATIC",fields[i].label,0,22,y+5,245,24,0);sliders[i]=child(TRACKBAR_CLASSW,L"",TBS_HORZ|TBS_NOTICKS|WS_TABSTOP,270,y,310,32,100+i);SendMessageW(sliders[i],TBM_SETRANGE,TRUE,MAKELPARAM(0,(int)std::lround((fields[i].hi-fields[i].lo)/fields[i].step)));SendMessageW(sliders[i],TBM_SETPAGESIZE,0,10);values[i]=child(L"STATIC",L"",SS_RIGHT,585,y+5,110,24,0);}
 child(L"BUTTON",L"Save settings",BS_PUSHBUTTON|WS_TABSTOP,22,727,160,34,1);child(L"BUTTON",L"Restore defaults",BS_PUSHBUTTON|WS_TABSTOP,198,727,170,34,2);child(L"BUTTON",L"Return to flight",BS_PUSHBUTTON|WS_TABSTOP,495,727,200,34,3);
 status=child(L"STATIC",L"F6 or Esc closes this window. Zero gravity and unlimited speed are supported.",0,22,775,680,25,0);
 while(GetMessageW(&msg,nullptr,0,0)>0){
  if(msg.message==WM_APP+6){if(open){hide();continue;}{std::lock_guard<std::mutex>l(mutex);editing=changed?pending:published;}refresh();open=true;ShowWindow(window,SW_SHOW);SetForegroundWindow(window);SetFocus(sliders[0]);continue;}
  if(open&&msg.message==WM_KEYDOWN&&(msg.wParam==VK_F6||msg.wParam==VK_ESCAPE)){hide();continue;}
  if(!IsDialogMessageW(window,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}
 }return 0;
}
inline void toggle(){DWORD id=threadId;if(id)PostThreadMessageW(id,WM_APP+6,0,0);}
inline void start(HINSTANCE module,const std::string&path,const Config&c){instance=module;configPath=path;publish(c);HANDLE t=CreateThread(nullptr,0,run,nullptr,0,nullptr);if(t)CloseHandle(t);}
}


