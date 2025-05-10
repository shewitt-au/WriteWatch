// WriteWatchDLL.cpp
//

#include "pch.h"
#include "WriteWatchDLL.h"
#include <shared_mutex>
#include <vector>
#include <algorithm>
#include <detours.h>
#include "CodeGen.h"

LONG WINAPI VectoredHandler(struct _EXCEPTION_POINTERS* ep);

UINT_PTR g_PageMask;
DWORD g_PageSize;
PVOID g_vxh;
DWORD g_TlsSlot;
std::vector<PVOID> g_Trampolines;
CRITICAL_SECTION g_TrampLock;
const SIZE_T TrampolineSize = 4096;

class MemoryRegion
{
public:
    MemoryRegion(PVOID pMem, SIZE_T size)
    {
        m_start = (ULONG_PTR)pMem;
        m_end = (ULONG_PTR)(m_start +size-1);
    }

    bool Check(PVOID pMem, SIZE_T size) const
    {
        ULONG_PTR s = (ULONG_PTR)pMem;
        ULONG_PTR e = (ULONG_PTR)(s+size-1);
        return s>=m_start && e<=m_end;
    }

    bool CheckStart(PVOID pMem) const
    {
        return (ULONG_PTR)pMem == m_start;
    }

private:
    ULONG_PTR m_start;
    ULONG_PTR m_end;
};

class MemoryAllocator
{
public:
    MemoryAllocator()  {}
    ~MemoryAllocator() {}

    PVOID Alloc(SIZE_T size);
    void Free(PVOID pMem);

    bool Check(PVOID pMem, SIZE_T sz);

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
        //PAGE_READWRITE
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

bool MemoryAllocator::Check(PVOID pMem, SIZE_T sz)
{
    ReadLock w_lock(g_AllocLock);
    auto it = std::find_if(
        m_regions.begin(), m_regions.end(),
        [pMem, sz](const MemoryRegion& r) { return r.Check(pMem, sz); }
    );
    return it != m_regions.end();
}

MemoryAllocator g_memory;

void InitDLL()
{
    // REMOVE THIS: Just here to find code byte sequences.
    /*MEMORY_BASIC_INFORMATION mbi;
    SIZE_T st = VirtualQuery((LPCVOID)0x1111'2222'3333'4444, &mbi, sizeof(mbi));
    if (mbi.State != MEM_COMMIT ||
        !(mbi.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_READONLY | PAGE_READWRITE)))
    {
        DebugBreak();
    }*/

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    g_PageSize = si.dwPageSize;
    g_PageMask = ~(((UINT_PTR)si.dwPageSize)-1);
  
    InitializeCriticalSection(&g_TrampLock);
    g_TlsSlot = TlsAlloc();
    g_vxh = AddVectoredExceptionHandler(0, VectoredHandler);
}

void UninitDLL()
{
    if (g_vxh)
        RemoveVectoredExceptionHandler(g_vxh);
    TlsFree(g_TlsSlot);

    EnterCriticalSection(&g_TrampLock);
    for (auto pTramp : g_Trampolines)
    {
        g_Trampolines.push_back(pTramp);

        VirtualFree(
            pTramp,         // LPVOID lpAddress
            0,              // SIZE_T dwSize
            MEM_RELEASE     // DWORD  dwFreeType
        );
    }
    LeaveCriticalSection(&g_TrampLock);

    DeleteCriticalSection(&g_TrampLock);
}

void InitThread()
{
}

void UninitThread()
{
    PVOID pTrampoline = TlsGetValue(g_TlsSlot);
    if (pTrampoline)
    {
        EnterCriticalSection(&g_TrampLock);
        auto it = std::find(
            g_Trampolines.begin(), g_Trampolines.end(),
            pTrampoline
        );
        if (it != g_Trampolines.end())
            g_Trampolines.erase(it);
        LeaveCriticalSection(&g_TrampLock);

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

        EnterCriticalSection(&g_TrampLock);
        g_Trampolines.push_back(pTrampoline);
        LeaveCriticalSection(&g_TrampLock);

        TlsSetValue(g_TlsSlot, pTrampoline);
    }

    return g_memory.Alloc(size);
}

EXPORTED_FN void __stdcall FreeWatched(PVOID pMem)
{
    g_memory.Free(pMem);
}

void MakeWritable(LPVOID lpAddr, DWORD *pOld)
{
    VirtualProtect(
        (LPVOID)((UINT_PTR)lpAddr & g_PageMask),
        g_PageSize,
        PAGE_READWRITE,
        pOld
        );
}

bool RestoreAndCheck(LPVOID lpAddr, DWORD* pOld)
{
    DWORD old;
    VirtualProtect(
        (LPVOID)((UINT_PTR)lpAddr & g_PageMask),
        g_PageSize,
        *pOld,
        &old
    );

    MEMORY_BASIC_INFORMATION mbi;
    SIZE_T st = VirtualQuery(
        (LPVOID)((UINT_PTR)*(void**)lpAddr & g_PageMask),
        &mbi,
        sizeof(mbi)
        );

     return mbi.State == MEM_COMMIT &&
        (mbi.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_READONLY | PAGE_READWRITE));
}

static LONG WINAPI
VectoredHandler(struct _EXCEPTION_POINTERS* ep)
{
    EXCEPTION_RECORD& er = *(ep->ExceptionRecord);
    if (er.ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;
    if (er.ExceptionInformation[0] != 1)
        return EXCEPTION_CONTINUE_SEARCH;

    // Access violation on a write
    //

    // Is it one of our's?
    PVOID pAccess = (PVOID)ep->ExceptionRecord->ExceptionInformation[1];
    if (!g_memory.Check(pAccess, sizeof(DWORD_PTR)))
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    // Yes
    //

    PVOID pTrampoline = TlsGetValue(g_TlsSlot);
    PVOID pPool = (PVOID)((char*)pTrampoline + TrampolineSize - 1);

    CodeGen g((uint8_t*)pTrampoline);

    // Add a vector to jump back to user code.
    auto resume = g.value((DWORD64)0);

    // Add vector to MakeWritable
    auto mw = g.pointer((ULONG_PTR)MakeWritable);
    // Space for a DWORD (old memory protection) .
    auto oldprotect = g.variable(sizeof(DWORD));

    // Add vector to RestoreAndCheck
    auto rc = g.pointer((ULONG_PTR)RestoreAndCheck);

    auto pEntryPoint = g.get_next();

    // Save volatile regs
    g.push_volatile_regs();

    // Call MakeWritable
    g.param1((UINT_PTR)pAccess);
    g.param2((UINT_PTR)oldprotect);
    g.call_indirect(mw);

    // Restore volatile regs
    g.pop_volatile_regs();

    // Copy the faulting instruction.
    auto pCC = g.get_next();
    PVOID pNext = DetourCopyInstruction(
        pCC,                 // _In_opt_ PVOID pDst
        &pPool,              // _Inout_opt_ PVOID * ppDstPool
        er.ExceptionAddress, // _In_ PVOID pSrc
        NULL,                // _Out_opt_ PVOID * ppTarget
        NULL                 // _Out_opt_ LONG * plExtra
        );
    // pNext is in the source. Translate to trampoline.
    PVOID pTn = (PVOID)((DWORD_PTR)pCC+(DWORD_PTR)pNext-(DWORD_PTR)(er.ExceptionAddress));
    g.set_next((uint8_t*)pTn);

    // Populate return to user code vector.
    *(DWORD64*)resume = (DWORD64)pNext;

    // Save volatile regs
    g.push_volatile_regs();

    // Call RestoreAndCheck
    g.param1((UINT_PTR)pAccess);
    g.param2((UINT_PTR)oldprotect);
    g.call_indirect(rc);

    g.test_eax_eax();
    g.jnz(1);

    // 0 - Bad pointer
    g.pop_volatile_regs();
    g.breakpoint();
    g.jmp_indirect((ULONG_PTR)resume);

    // 1 - Good pointer
    g.label(1);
    g.pop_volatile_regs();
    g.jmp_indirect((ULONG_PTR)resume);

    g.patch();
    
    ep->ContextRecord->Rip = (DWORD64)pEntryPoint;

    return EXCEPTION_CONTINUE_EXECUTION;
}
