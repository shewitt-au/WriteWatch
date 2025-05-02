// WriteWatch.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include <detours.h>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    DetourTransactionBegin();
    DetourTransactionCommit();

    LPVOID p = VirtualAlloc(
                    NULL,           // LPVOID lpAddress
                    65536,          // SIZE_T dwSize
                    MEM_COMMIT,     // DWORD  flAllocationType
                    PAGE_READONLY   // DWORD flProtect
                    );

    *(DWORDLONG*)p = 5;

    return 0;
}
