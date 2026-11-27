#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <cstdio>
#include <cwchar>
#include <string>
static DWORD find(){HANDLE s=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);PROCESSENTRY32W p{sizeof p};DWORD id=0;if(Process32FirstW(s,&p))do{if(_wcsicmp(p.szExeFile,L"JustCause3.exe")==0){id=p.th32ProcessID;break;}}while(Process32NextW(s,&p));CloseHandle(s);return id;}
int wmain(int argc,wchar_t**argv){wchar_t file[MAX_PATH];GetModuleFileNameW(nullptr,file,MAX_PATH);std::wstring dir=file;dir=dir.substr(0,dir.find_last_of(L"\\/")+1);FILE*output=nullptr;_wfreopen_s(&output,(dir+L"launcher.log").c_str(),L"w",stdout);setvbuf(stdout,nullptr,_IONBF,0);puts("JC3FPV launcher starting");std::wstring dll=dir+L"JC3FPV.dll";
 FILE*diagnostic=nullptr;if(_wfopen_s(&diagnostic,(dir+L"JC3FPV.log").c_str(),L"rb")==0){char b[2048];size_t n;while((n=fread(b,1,sizeof b,diagnostic))>0)fwrite(b,1,n,stdout);fclose(diagnostic);}
 DWORD id=find();if(!id){if(argc>1&&wcscmp(argv[1],L"--attach")==0){puts("JC3 is not running.");return 1;}ShellExecuteW(nullptr,L"open",L"steam://rungameid/225540",nullptr,nullptr,SW_SHOWNORMAL);puts("Waiting up to 60 seconds for Just Cause 3...");for(int i=0;i<120&&!id;i++){Sleep(500);id=find();}}
 if(!id){puts("JC3 did not launch. Start the game via Steam and run JC3FPV again.");return 1;}
 HANDLE s=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,id);MODULEENTRY32W m{sizeof m};if(Module32FirstW(s,&m))do{if(_wcsicmp(m.szModule,L"JC3FPV.dll")==0){puts("JC3FPV already loaded. F7 enable/disable, F8 reset, F9 reload config.");CloseHandle(s);return 0;}if(wcsstr(m.szModule,L"jc3vr")||wcsstr(m.szModule,L"JC3VR")||_wcsicmp(m.szModule,L"JC3.dll")==0){puts("Another camera mod is loaded. Restart JC3 without it; JC3FPV did not change anything.");CloseHandle(s);return 1;}}while(Module32NextW(s,&m));CloseHandle(s);
 HANDLE p=OpenProcess(PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ,FALSE,id);if(!p){printf("OpenProcess failed %lu\n",GetLastError());return 1;}
 size_t size=(dll.size()+1)*sizeof(wchar_t);void*mem=VirtualAllocEx(p,nullptr,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!mem||!WriteProcessMemory(p,mem,dll.c_str(),size,nullptr)){puts("Cannot stage standalone DLL.");CloseHandle(p);return 1;}
 // System DLL base addresses are resolved in the target rather than assumed identical.
 uintptr_t local=(uintptr_t)GetModuleHandleW(L"kernel32.dll"),remote=0;s=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,id);m.dwSize=sizeof m;if(Module32FirstW(s,&m))do{if(_wcsicmp(m.szModule,L"kernel32.dll")==0)remote=(uintptr_t)m.modBaseAddr;}while(Module32NextW(s,&m));CloseHandle(s);
 auto load=(LPTHREAD_START_ROUTINE)(remote+(uintptr_t)GetProcAddress((HMODULE)local,"LoadLibraryW")-local);HANDLE t=remote?CreateRemoteThread(p,nullptr,0,load,mem,0,nullptr):nullptr;if(!t){printf("Load failed %lu\n",GetLastError());CloseHandle(p);return 1;}DWORD wait=WaitForSingleObject(t,15000),code=0;GetExitCodeThread(t,&code);if(wait==WAIT_OBJECT_0)VirtualFreeEx(p,mem,0,MEM_RELEASE);CloseHandle(t);CloseHandle(p);printf("%s PID %lu. See JC3FPV.log. F7 enable, F8 reset, F9 reload.\n",code&&wait==WAIT_OBJECT_0?"Loaded into":"Load unconfirmed for",id);return code&&wait==WAIT_OBJECT_0?0:1;}
