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
void DeinitThread();
void DeinitDLL();
#endif

EXPORTED_FN void __stdcall Test();
