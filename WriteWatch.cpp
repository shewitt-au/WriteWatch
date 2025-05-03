// WriteWatch.cpp
//

#include "framework.h"
#include "WriteWatchDLL/WriteWatchDLL.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    PVOID p = AllocWatched(10);

    DWORDLONG v = *(DWORDLONG*)p;

    *(DWORDLONG*)p = 1;
    *(DWORDLONG*)p = 2;
    *(DWORDLONG*)p = 3;

    FreeWatched(p);

    return 0;
}
