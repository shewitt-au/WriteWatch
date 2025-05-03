// WriteWatch.cpp
//

#include "framework.h"
#include <detours.h>
#include "WriteWatchDLL/WriteWatchDLL.h"

LPVOID g_pTrampoline = NULL;

LONG WINAPI
VectoredHandler(struct _EXCEPTION_POINTERS* ep)
{
    EXCEPTION_RECORD &er = *(ep->ExceptionRecord);
    if (er.ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;
    if (er.ExceptionInformation[0] != 1)
        return EXCEPTION_CONTINUE_SEARCH;

    // Access violation on a write

    LPVOID pPool = (PVOID)((char*)g_pTrampoline+sizeof(g_pTrampoline)-1);
    LPVOID pNext = DetourCopyInstruction(
                    g_pTrampoline,       // _In_opt_ PVOID pDst
                    &pPool,              // _Inout_opt_ PVOID * ppDstPool
                    er.ExceptionAddress, // _In_ PVOID pSrc
                    NULL,                // _Out_opt_ PVOID * ppTarget
                    NULL                 // _Out_opt_ LONG * plExtra
                    );

    ep->ContextRecord->Rip = (DWORD64)pNext;

    return EXCEPTION_CONTINUE_EXECUTION;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    PVOID pt = AllocWatched(10);

    g_pTrampoline = VirtualAlloc(
                        NULL,                   // LPVOID lpAddress
                        65536,                  // SIZE_T dwSize
                        MEM_COMMIT,             // DWORD  flAllocationType
                        PAGE_EXECUTE_READWRITE  // DWORD flProtect
                        );

    LPVOID p = VirtualAlloc(
                    NULL,           // LPVOID lpAddress
                    65536,          // SIZE_T dwSize
                    MEM_COMMIT,     // DWORD  flAllocationType
                    PAGE_READONLY   // DWORD flProtect
                    );

    PVOID vxh = AddVectoredExceptionHandler(0, VectoredHandler);

    DWORDLONG v = *(DWORDLONG*)p;

    *(DWORDLONG*)p = 1;
    *(DWORDLONG*)p = 2;
    *(DWORDLONG*)p = 3;

    if (vxh)
        RemoveVectoredExceptionHandler(vxh);

    return 0;
}
