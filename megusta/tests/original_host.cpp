// Start the pinned game suspended so Windows maps its fixed image before heap
// allocation. Replace only the remote entry point with a blocking wait stub,
// allowing loader initialization but never original game startup. Load our test
// DLL and call its runner, then terminate this isolated process.
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

struct Process {
    PROCESS_INFORMATION p{};
    ~Process(){if(p.hProcess){TerminateProcess(p.hProcess,0);CloseHandle(p.hThread);CloseHandle(p.hProcess);}}
};
static void* remote_string(HANDLE process,const std::string& s) {
    void* p=VirtualAllocEx(process,nullptr,s.size()+1,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    SIZE_T count=0;
    if(!p || !WriteProcessMemory(process,p,s.c_str(),s.size()+1,&count) || count!=s.size()+1)throw std::runtime_error("Cannot write oracle argument");
    return p;
}
static DWORD invoke(HANDLE process,LPTHREAD_START_ROUTINE function,void* argument) {
    HANDLE thread=CreateRemoteThread(process,nullptr,0,function,argument,0,nullptr);
    if(!thread)throw std::runtime_error("Cannot start oracle thread");
    DWORD wait=WaitForSingleObject(thread,30000),code=0;
    if(wait!=WAIT_OBJECT_0){CloseHandle(thread);throw std::runtime_error("Oracle thread did not finish");}
    if(!GetExitCodeThread(thread,&code)){CloseHandle(thread);throw std::runtime_error("Cannot read oracle thread status");}
    CloseHandle(thread);return code;
}
static void block_game_startup(Process& process) {
    IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS32 pe{};SIZE_T count=0;
    if(!ReadProcessMemory(process.p.hProcess,reinterpret_cast<void*>(0x400000),&dos,sizeof(dos),&count) || dos.e_magic!=IMAGE_DOS_SIGNATURE)throw std::runtime_error("Cannot read original DOS header");
    if(!ReadProcessMemory(process.p.hProcess,reinterpret_cast<void*>(0x400000+dos.e_lfanew),&pe,sizeof(pe),&count) || pe.Signature!=IMAGE_NT_SIGNATURE)throw std::runtime_error("Cannot read original PE header");
    void* entry=reinterpret_cast<void*>(0x400000+pe.OptionalHeader.AddressOfEntryPoint);
    // push INFINITE; mov eax,Sleep; call eax; jmp back. Sleep is a platform
    // service; no original CRT or game entry-point instruction is executed.
    unsigned char stub[]={0x6a,0xff,0xb8,0,0,0,0,0xff,0xd0,0xeb,0xf5};
    uintptr_t sleep=reinterpret_cast<uintptr_t>(GetProcAddress(GetModuleHandleA("kernel32.dll"),"Sleep"));
    std::memcpy(stub+3,&sleep,4);
    DWORD old=0;
    if(!VirtualProtectEx(process.p.hProcess,entry,sizeof(stub),PAGE_EXECUTE_READWRITE,&old) || !WriteProcessMemory(process.p.hProcess,entry,stub,sizeof(stub),&count) || count!=sizeof(stub))throw std::runtime_error("Cannot block original entry point");
    DWORD ignored;VirtualProtectEx(process.p.hProcess,entry,sizeof(stub),old,&ignored);
    FlushInstructionCache(process.p.hProcess,entry,sizeof(stub));
    if(ResumeThread(process.p.hThread)==DWORD(-1))throw std::runtime_error("Cannot initialize original process loader");
}
int main(int argc,char** argv) {
    try {
        if(argc!=2)throw std::runtime_error("usage: original_plib_host MINIGOLF.EXE");
        char executable[MAX_PATH];GetModuleFileNameA(nullptr,executable,sizeof(executable));
        std::string directory(executable);directory.resize(directory.find_last_of("\\/")+1);
        const std::string dll=directory+"megusta_original_plib_oracle.dll",output=directory+"original-leaf-result.json";
        DeleteFileA(output.c_str());
        Process process;STARTUPINFOA startup{};startup.cb=sizeof(startup);
        startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;
        std::string command="\""+std::string(argv[1])+"\"";
        if(!CreateProcessA(argv[1],command.data(),nullptr,nullptr,TRUE,CREATE_SUSPENDED,nullptr,nullptr,&startup,&process.p))throw std::runtime_error("Cannot create suspended original process");
        block_game_startup(process);
        auto loader=reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleA("kernel32.dll"),"LoadLibraryA"));
        DWORD remote_module=invoke(process.p.hProcess,loader,remote_string(process.p.hProcess,dll));
        if(!remote_module)throw std::runtime_error("Cannot load oracle DLL in original process");
        HMODULE local=LoadLibraryExA(dll.c_str(),nullptr,DONT_RESOLVE_DLL_REFERENCES);
        if(!local)throw std::runtime_error("Cannot inspect oracle export");
        auto symbol=GetProcAddress(local,"MegustaRunOriginalChecks");
        if(!symbol){FreeLibrary(local);throw std::runtime_error("Missing oracle export");}
        uintptr_t offset=reinterpret_cast<uintptr_t>(symbol)-reinterpret_cast<uintptr_t>(local);FreeLibrary(local);
        DWORD code=invoke(process.p.hProcess,reinterpret_cast<LPTHREAD_START_ROUTINE>(remote_module+offset),remote_string(process.p.hProcess,output));
        std::ifstream result(output);std::string text((std::istreambuf_iterator<char>(result)),{});
        if(text.empty()){std::fprintf(stderr,"Oracle returned %08lx without a result\n",static_cast<unsigned long>(code));return 1;}
        std::printf("%s",text.c_str());return code?1:0;
    }catch(const std::exception& e){std::fprintf(stderr,"%s; Windows error=%lu\n",e.what(),GetLastError());return 1;}
}
