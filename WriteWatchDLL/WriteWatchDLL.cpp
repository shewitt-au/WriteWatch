// WriteWatchDLL.cpp
//

#include "pch.h"
#include "WriteWatchDLL.h"
#include <shared_mutex>
#include <vector>
#include <algorithm>

DWORD g_TlsSlot;

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
            pTrampoline,    // LPVOID lpAddress
            0,              // SIZE_T dwSize
            MEM_RELEASE     // DWORD  dwFreeType
        );
    }
}

EXPORTED_FN PVOID __stdcall AllocWatched(SIZE_T size)
{
    return g_memory.Alloc(size);
}
