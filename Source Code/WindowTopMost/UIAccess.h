//uiaccess-master from:https://github.com/killtimer0/uiaccess
//source code from:https://www.bilibili.com/video/BV1HCwwegEVp

#pragma once
#include <windows.h>

#ifdef _DEBUG
#include <stdio.h>
#define dbgstart()						\
	do									\
	{									\
		AllocConsole();					\
		freopen("CON", "r", stdin);		\
		freopen("CON", "w", stdout);	\
		freopen("CON", "w", stderr);	\
	} while (0)
#define dbgend()			FreeConsole()
#define dbg(...)			printf(__VA_ARGS__)
#else
#define dbgstart()			((void)0)
#define dbgend()			FALSE
#define dbg(...)			(-1)
#endif

// Return win32 error code
EXTERN_C DWORD PrepareForUIAccess();

EXTERN_C BOOL CheckForUIAccess(DWORD * pdwErr, DWORD * pfUIAccess);

