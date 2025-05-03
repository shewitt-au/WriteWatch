// WriteWatchDLL.cpp
//

#include "pch.h"
#include "WriteWatchDLL.h"

DWORD g_TlsSlot;

void InitDLL()
{
    g_TlsSlot = TlsAlloc();
}

void DeinitDLL()
{
    TlsFree(g_TlsSlot);
}

void InitThread()
{
    PVOID pTrampoline = VirtualAlloc(
        NULL,                   // LPVOID lpAddress
        65536,                  // SIZE_T dwSize
        MEM_COMMIT,             // DWORD  flAllocationType
        PAGE_EXECUTE_READWRITE  // DWORD flProtect
    );

    TlsSetValue(g_TlsSlot, pTrampoline);
}

void DeinitThread()
{
    PVOID pTrampoline = TlsGetValue(g_TlsSlot);
    if (pTrampoline)
    {
        VirtualFree(
            pTrampoline,    // LPVOID lpAddress,
            0,              // SIZE_T dwSize,
            MEM_RELEASE     // DWORD  dwFreeType
        );
    }
}

EXPORTED_FN void __stdcall Test()
{
    MessageBoxA(NULL, "In DLL!", "WriteWatchDLL", MB_OK);
}
