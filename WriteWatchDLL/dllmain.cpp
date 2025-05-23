// dllmain.cpp
#include "pch.h"
#include "WriteWatchDLL.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        InitDLL();
        InitThread();
        break;
    case DLL_THREAD_ATTACH:
        InitThread();
        break;
    case DLL_THREAD_DETACH:
        UninitThread();
        break;
    case DLL_PROCESS_DETACH:
        UninitDLL();
        break;
    }
    return TRUE;
}
