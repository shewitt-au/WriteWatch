// WriteWatchDLL.cpp
//

#include "pch.h"
#include "WriteWatchDLL.h"
#include <shared_mutex>
#include <vector>
#include <algorithm>
#include <detours.h>

LONG WINAPI VectoredHandler(struct _EXCEPTION_POINTERS* ep);

PVOID g_vxh;
DWORD g_TlsSlot;
const SIZE_T TrampolineSize = 4096;

class MemoryRegion
{
public:
    MemoryRegion(PVOID pMem, SIZE_T size)
    {
        m_start = (DWORD_PTR)pMem;
        m_end = (DWORD_PTR)((DWORD_PTR)pMem +size-1);
    }

    bool Check(PVOID pMem, SIZE_T size) const
    {
        DWORD_PTR s = (DWORD_PTR)pMem;
        DWORD_PTR e = (DWORD_PTR)((DWORD_PTR)pMem+size-1);
        return s>=m_start && e<=m_end;
    }

    bool CheckStart(PVOID pMem) const
    {
        return (DWORD_PTR)pMem == m_start;
    }

private:
    DWORD_PTR m_start;
    DWORD_PTR m_end;
};

class MemoryAllocator
{
public:
    MemoryAllocator()  {}
    ~MemoryAllocator() {}

    PVOID Alloc(SIZE_T size);
    void Free(PVOID pMem);

private:
    typedef std::shared_mutex Lock;
    typedef std::unique_lock<Lock>  WriteLock;
    typedef std::shared_lock<Lock>  ReadLock;

    static Lock g_AllocLock;
    std::vector<MemoryRegion> m_regions;
};

MemoryAllocator::Lock MemoryAllocator::g_AllocLock;

PVOID MemoryAllocator::Alloc(SIZE_T size)
{
    PVOID p = VirtualAlloc(
        NULL,           // LPVOID lpAddress
        size,           // SIZE_T dwSize
        MEM_COMMIT,     // DWORD  flAllocationType
        PAGE_READONLY   // DWORD flProtect
        );
    MEMORY_BASIC_INFORMATION mbi;
    VirtualQuery(p, &mbi, sizeof(MEMORY_BASIC_INFORMATION));

    if (p)
    {
        WriteLock w_lock(g_AllocLock);
        m_regions.push_back(MemoryRegion(p, mbi.RegionSize));
    }

    return p;
}

void MemoryAllocator::Free(PVOID pMem)
{
    VirtualFree(
        pMem,       // LPVOID lpAddress
        0,          // SIZE_T dwSize
        MEM_RELEASE // DWORD  dwFreeType
    );

    WriteLock w_lock(g_AllocLock);
    auto it = std::find_if(
                    m_regions.begin(), m_regions.end(),
                    [pMem](const MemoryRegion& r) { return r.CheckStart(pMem); }
                    );
    if (it != m_regions.end())
        m_regions.erase(it);
}

MemoryAllocator g_memory;

void InitDLL()
{
    g_TlsSlot = TlsAlloc();
    g_vxh = AddVectoredExceptionHandler(0, VectoredHandler);
}

void UninitDLL()
{
    TlsFree(g_TlsSlot);
    if (g_vxh)
        RemoveVectoredExceptionHandler(g_vxh);
}

void InitThread()
{
}

void UninitThread()
{
    PVOID pTrampoline = TlsGetValue(g_TlsSlot);
    if (pTrampoline)
    {
        VirtualFree(
            pTrampoline,    // LPVOID lpAddress
            0,              // SIZE_T dwSize
            MEM_RELEASE     // DWORD  dwFreeType
        );
    }
}

EXPORTED_FN PVOID __stdcall AllocWatched(SIZE_T size)
{
    // Allocate the trampoline on demand.
    // The DLL_THREAD_DETACH notification is only called for
    // threads loaded after we are, not pre-existing ones.
    PVOID pTrampoline = TlsGetValue(g_TlsSlot);
    if (!pTrampoline)
    {
        PVOID pTrampoline = VirtualAlloc(
            NULL,                   // LPVOID lpAddress
            4096,                   // SIZE_T dwSize
            MEM_COMMIT,             // DWORD  flAllocationType
            PAGE_EXECUTE_READWRITE  // DWORD flProtect
        );

        TlsSetValue(g_TlsSlot, pTrampoline);
    }

    return g_memory.Alloc(size);
}

EXPORTED_FN void __stdcall FreeWatched(PVOID pMem)
{
    g_memory.Free(pMem);
}

LONG WINAPI
VectoredHandler(struct _EXCEPTION_POINTERS* ep)
{
    EXCEPTION_RECORD& er = *(ep->ExceptionRecord);
    if (er.ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;
    if (er.ExceptionInformation[0] != 1)
        return EXCEPTION_CONTINUE_SEARCH;

    // Access violation on a write

    PVOID pTrampoline = TlsGetValue(g_TlsSlot);
    LPVOID pPool = (PVOID)((char*)pTrampoline + TrampolineSize-1);
    LPVOID pNext = DetourCopyInstruction(
        pTrampoline,         // _In_opt_ PVOID pDst
        &pPool,              // _Inout_opt_ PVOID * ppDstPool
        er.ExceptionAddress, // _In_ PVOID pSrc
        NULL,                // _Out_opt_ PVOID * ppTarget
        NULL                 // _Out_opt_ LONG * plExtra
    );

    ep->ContextRecord->Rip = (DWORD64)pNext;

    return EXCEPTION_CONTINUE_EXECUTION;
}
