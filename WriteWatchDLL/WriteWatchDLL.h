// WriteWatchDLL.h
//
#pragma once

#ifdef WDL_EXPORTS
#define EXPORTED_FN __declspec(dllexport)
#else
#define EXPORTED_FN __declspec(dllimport)
#endif

#ifdef WDL_EXPORTS
void InitDLL();
void InitThread();
void UninitThread();
void UninitDLL();
#endif

EXPORTED_FN PVOID __stdcall AllocWatched(SIZE_T size);
EXPORTED_FN void __stdcall FreeWatched(PVOID pMem);
