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
    //PVOID p = (PVOID)(new char[1000]);

    DWORDLONG v = *(DWORDLONG*)p;

    *(DWORDLONG*)p = 1;
    *(DWORDLONG*)p = 2;
    *(DWORDLONG*)p = 3;

    *(DWORDLONG*)0 = 4;

    MessageBoxA(NULL, "Here", NULL, MB_OK);

    return 0;
}
