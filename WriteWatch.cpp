// WriteWatch.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include <detours.h>

VOID(NTAPI* Real_KiUserExceptionDispatcher)(IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextFrame) = NULL;

char buffer[1024];

LONG WINAPI
VectoredHandler(struct _EXCEPTION_POINTERS* ep)
{
    EXCEPTION_RECORD &er = *(ep->ExceptionRecord);
    if (er.ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;
    if (er.ExceptionInformation[0] != 1)
        return EXCEPTION_CONTINUE_SEARCH;

    // Access violation on a write

    PVOID pPool = (PVOID)(buffer + sizeof(buffer) - 1);
    PVOID pNext = DetourCopyInstruction(
                    buffer,              // _In_opt_ PVOID pDst
                    &pPool,              // _Inout_opt_ PVOID * ppDstPool
                    er.ExceptionAddress, // _In_ PVOID pSrc
                    NULL,                // _Out_opt_ PVOID * ppTarget
                    NULL                 // _Out_opt_ LONG * plExtra
                    );

    MessageBoxA(NULL, "Write", NULL, MB_OK);

    return EXCEPTION_CONTINUE_SEARCH;
}

/*static VOID NTAPI
Detour_KiUserExceptionDispatcher(PEXCEPTION_RECORD pExceptRec,
    CONTEXT* pContext)
{
    MessageBoxA(NULL, "Fuck off!", "!", MB_OK);
    Real_KiUserExceptionDispatcher(pExceptRec, pContext);
}*/

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    /*Real_KiUserExceptionDispatcher =
        (VOID(NTAPI*)(IN PEXCEPTION_RECORD, IN PCONTEXT))
        DetourFindFunction("ntdll.dll", "KiUserExceptionDispatcher");

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)Real_KiUserExceptionDispatcher,
        Detour_KiUserExceptionDispatcher);
    DetourTransactionCommit();*/

    LPVOID p = VirtualAlloc(
                    NULL,           // LPVOID lpAddress
                    65536,          // SIZE_T dwSize
                    MEM_COMMIT,     // DWORD  flAllocationType
                    PAGE_READONLY   // DWORD flProtect
                    );

    PVOID vxh = AddVectoredExceptionHandler(0, VectoredHandler);

    *(DWORDLONG*)p = 5;

    if (vxh)
        RemoveVectoredExceptionHandler(vxh);

    return 0;
}
