// MIT License
//
// Copyright (c) 2025 WindowTopMost - xmc0211 <xmc0211@qq.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#define WTMDLLFILE
#include "WindowTopMost.h"
#include <TlHelp32.h>
#include <Windows.h>
#include <VersionHelpers.h>
#include "uiaccess.h"
#include "TaskDialogEx.h"
#include "Convert.h"
#include "ProcessCtrl.h"
#include "FileBinIO.h"
#include "FileSys.h"

#ifdef _DEBUG
#include <fstream>
#endif

// File name, window class, and corresponding message for transmitting information
#define WNDCLASSNAME TEXT("IAMCollector_7DD48162_2BC7_4073_9CC8_51CC3DB0FF72")
#define WM_FILECRATED (WM_USER + 1)
#define IAMDIRECTORYNAME TEXT("\\TEMP_7DD48162_2BC7_4073_9CC8_51CC3DB0FF72\\")
#define INFOFILENAME TEXT("IAMInfo_7DD48162_2BC7_4073_9CC8_51CC3DB0FF72.tmp")
#define FLAGFILENAME TEXT("IAMFlag_7DD48162_2BC7_4073_9CC8_51CC3DB0FF72.tmp")

// DLL injection object name
#define INJECTOBJECT TEXT("IAMKeyHacker.dll")

// Debug
#ifdef _DEBUG
#define DEBUGLOG(Text) do { std::ofstream Log("C:\\Users\\PUBLIC\\Desktop\\DebugLog_WTM.txt", std::ios::app); Log << Text << std::endl; } while (0)
#else
#define DEBUGLOG(Text) do {  } while (0)
#endif
#define DEBUGERR(Text) DEBUGLOG(Text + std::to_string((ULONG)GetLastError()))

// Prototype of each function
typedef BOOL(WINAPI* GetWindowBand)(
	HWND hWnd,
	LPDWORD pdwBand
	);
typedef HWND(WINAPI* CreateWindowInBand)(
	DWORD dwExStyle,
	LPCWSTR lpClassName,
	LPCWSTR lpWindowName,
	DWORD dwStyle,
	int X,
	int Y,
	int nWidth,
	int nHeight,
	HWND hWndParent,
	HMENU hMenu,
	HINSTANCE hInstance,
	LPVOID lpParam,
	DWORD dwBand
	);
typedef BOOL(WINAPI* SetWindowBand)(
	HWND hWnd, 
	HWND hWndInsertAfter,
	DWORD dwBand
	);
typedef BOOL(WINAPI* NtUserEnableIAMAccess)(
    ULONG64 key,
    BOOL enable
    );


#if defined(__cplusplus)
extern "C" {
#endif

// Adjust process privilage (Hide)
DWORD WINAPI AdjustPrivilege(_In_ LPCTSTR lpPrivilegeName, _In_opt_ BOOL bEnable) {
    DEBUGLOG("Enter: AdjustPrivilege");

    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES NewState;
    LUID luidPrivilegeLUID;
    DWORD dwErr = ERROR_SUCCESS;
    HANDLE hProcess = GetCurrentProcess();
    if (hProcess == NULL) {
        dwErr = GetLastError();
        goto EXIT;
    }
    if (!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        dwErr = GetLastError();
        goto EXIT;
    }
    if (bEnable == FALSE) {
        if (!AdjustTokenPrivileges(hToken, TRUE, NULL, 0, NULL, NULL)) dwErr = GetLastError();
        goto EXIT;
    }
    LookupPrivilegeValue(NULL, lpPrivilegeName, &luidPrivilegeLUID);
    NewState.PrivilegeCount = 1;
    NewState.Privileges[0].Luid = luidPrivilegeLUID;
    NewState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!AdjustTokenPrivileges(hToken, FALSE, &NewState, 0, NULL, NULL)) dwErr = GetLastError();
EXIT:
    if (hToken) CloseHandle(hToken);
    if (dwErr == ERROR_SUCCESS) {
        DEBUGLOG("Successful Exit: AdjustPrivilege");
    }
    else {
        DEBUGERR("Failture Exit: AdjustPrivilege");
    }
    return dwErr;
}

// Used to convert ULL or UL to a BYTE array and store information for use
void WTMAPI ULLToByte(_In_opt_ ULONG64 ulNum, _In_ BYTE Buffer[8]) {
    memset(Buffer, 0, sizeof Buffer);
    for (int i = 0; i < 8; ++i) {
        Buffer[i] = static_cast<BYTE>((ulNum >> (56 - (i * 8))) & 0xFF);
    }
    return;
}
void WTMAPI ULToByte(_In_opt_ ULONG32 ulNum, _In_ BYTE Buffer[4]) {
    memset(Buffer, 0, sizeof Buffer);
    for (int i = 0; i < 4; ++i) {
        Buffer[i] = static_cast<BYTE>((ulNum >> (24 - (i * 8))) & 0xFF);
    }
    return;
}

// Auxiliary function used to check if the INJECTOBJECT backend program has been injected
BOOL WTMAPI CheckForDll() {
    MODULEENTRY32 MOD;
    BOOL bFound = FALSE;
    DWORD dwPid = 0;

    // Get the Explorer responsible for the desktop EXE's PID
    HWND hWndTaskBar = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (hWndTaskBar == NULL) return FALSE;
    GetWindowThreadProcessId(hWndTaskBar, &dwPid);
    if (dwPid == 0) return FALSE;

    // Retrieve all loaded DLLs and compare them one by one
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPid);
    if (hSnapshot == INVALID_HANDLE_VALUE) return FALSE;
    
    MOD.dwSize = sizeof(MODULEENTRY32);
    if (!Module32First(hSnapshot, &MOD)) {
        CloseHandle(hSnapshot);
        return FALSE;
    }
    do {
        if (_tcscmp(MOD.szModule, INJECTOBJECT) == 0) {
            bFound = TRUE;
            break;
        }
    } while (Module32Next(hSnapshot, &MOD));
    CloseHandle(hSnapshot);
    return bFound;
}

// Check if the operating system meets the requirements
BOOL WTMAPI WTMCheckEnvironment() {
    DEBUGLOG("Enter: WTMCheckEnvironment");

    // Check Windows OS Version
    if (!IsWindows8OrGreater()) {
        DEBUGLOG("Check: Windows Version Error.");
        DEBUGLOG("Failure Exit: WTMCheckEnvironment");
        return FALSE;
    }
    DEBUGLOG("Check: Windows Version OK. (VER >= 8)");

    // Check the dll
    std::_tstring AppPath = FSFormat(TEXT("DP"), FSGetCurrentFilePath());
    if (!FSObjectExist(AppPath + INJECTOBJECT)) {
        DEBUGLOG("Check: DLL Not Exist.");
        DEBUGLOG("Failure Exit: WTMCheckEnvironment");
        return FALSE;
    }
    DEBUGLOG("Check: DLL Existing.");

    // Find the taskbar window
    HWND hWndTaskBar = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (hWndTaskBar == NULL) {
        DEBUGLOG("Check: Taskbar Not Found.");
        DEBUGLOG("Failure Exit: WTMCheckEnvironment");
        return FALSE;
    }
    DEBUGLOG("Check: Taskbar found.");

    DEBUGLOG("Successful Exit: WTMCheckEnvironment");
    return TRUE;
}

// WindowTopMost initialization function. It will be automatically called in DllMain or manually initialized.
BOOL WTMAPI WTMInit() {
    DEBUGLOG("Enter: WTMInit");

    // Inject IAMKeyHacker DLL to Explorer EXE to start the backend program
    
    // If already injected, do not inject again
    if (CheckForDll()) {
        DEBUGLOG("Successful Exit: WTMInit (been initialized)");
        return TRUE;
    }

    // Check the environment
    if (!WTMCheckEnvironment()) {
        DEBUGLOG("Failure Exit: WTMInit");
        return FALSE;
    }

    // Elevation
    if (AdjustPrivilege(SE_DEBUG_NAME, TRUE) != ERROR_SUCCESS) {
        DEBUGLOG("Failure Exit: WTMInit");
        return FALSE;
    }

    // Get the complete path of the DLL
    TCHAR ModulePathBuffer[MAX_PATH];
    GetModuleFileName(NULL, ModulePathBuffer, MAX_PATH);
    std::_tstring ModuleFullPath(ModulePathBuffer);
    size_t lastBackslashPos = ModuleFullPath.find_last_of(TEXT("\\"));
    std::_tstring dllPath = ModuleFullPath.substr(0, lastBackslashPos + 1) + INJECTOBJECT;
    DEBUGLOG("Injector: DllPath=" + LPT2LPC(dllPath));

    // Find an Explorer responsible for the desktop EXE's PID, get ready to inject into the process
    DWORD processId;
    HWND hWndTaskBar;
    hWndTaskBar = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (hWndTaskBar == NULL) return FALSE;
    GetWindowThreadProcessId(hWndTaskBar, &processId);
    if (processId == 0) return FALSE;
    DEBUGLOG("Injector: Explorer.EXE Pid=" + std::to_string((ULONG)processId));
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        DEBUGERR("Injector: OpenProcess failed: ");
        DEBUGLOG("Failure Exit: WTMInit");
        return FALSE;
    }
    DEBUGLOG("Injector: OpenProcess success");

    // Allocate memory within the process
    LPVOID pRemoteMemory = VirtualAllocEx(hProcess, NULL, (dllPath.size() + 1) * sizeof(TCHAR), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!pRemoteMemory) {
        DEBUGERR("Injector: VirtualAllocEx failed: ");
        CloseHandle(hProcess);
        DEBUGLOG("Failure Exit: WTMInit");
        return FALSE;
    }
    DEBUGLOG("Injector: VirtualAllocEx success");

    // Write DLL path
    BOOL success = WriteProcessMemory(hProcess, pRemoteMemory, dllPath.c_str(), (dllPath.size() + 1) * sizeof(TCHAR), NULL);
    if (!success) {
        DEBUGERR("Injector: WriteProcessMemory failed: ");
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        DEBUGLOG("Failure Exit: WTMInit");
        return FALSE;
    }
    DEBUGLOG("Injector: WriteProcessMemory success");

    // Create remote thread to load DLL
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibrary, pRemoteMemory, 0, NULL);
    if (!hThread) {
        DEBUGERR("Injector: CreateRemoteThread failed: ");
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        DEBUGLOG("Failure Exit: WTMInit");
        return FALSE;
    }
    DEBUGLOG("Injector: CreateRemoteThread success");

    // Waiting for thread to end
    WaitForSingleObject(hThread, INFINITE);

    // Clear
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    // Check again
    if (!CheckForDll()) return FALSE;
    DEBUGLOG("Successful Exit: WTMInit");

    return TRUE;
}

// WindowTopMost uninstallation function. It will be automatically called in DllMain or manually called to complete the uninstallation.
BOOL WTMAPI WTMUninit() {
    // From Explorer Uninstall IAMKeyHacker in EXE DLL to close the background program

    // If already uninstalled, do not uninstall again
    if (!CheckForDll()) return TRUE;

    MODULEENTRY32 MOD;
    BOOL bFound = FALSE;
    DWORD dwPid = 0;

    // Get the Explorer responsible for the desktop EXE's PID
    HWND hWndTaskBar = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (hWndTaskBar == NULL) return FALSE;
    GetWindowThreadProcessId(hWndTaskBar, &dwPid);
    if (dwPid == 0) return FALSE;

    MOD.dwSize = sizeof(MODULEENTRY32);
    // Retrieve all loaded DLLs and compare them one by one
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPid);
    if (hSnapshot == INVALID_HANDLE_VALUE) return FALSE;

    if (!Module32First(hSnapshot, &MOD)) {
        CloseHandle(hSnapshot);
        return FALSE;
    }
    do {
        if (_tcscmp(MOD.szModule, INJECTOBJECT) == 0) break;
    } while (Module32Next(hSnapshot, &MOD));
    
    // Get the handle of the target process
    HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, dwPid);
    if (!hProcess) {
        CloseHandle(hSnapshot);
        return FALSE;
    }

    // Get FreeLibrary address
    HMODULE hMod = GetModuleHandle(TEXT("Kernel32.dll"));
    if (hMod == NULL) {
        CloseHandle(hProcess);
        CloseHandle(hSnapshot);
        return FALSE;
    }
    LPTHREAD_START_ROUTINE pThread = (LPTHREAD_START_ROUTINE)GetProcAddress(hMod, "FreeLibrary");
    if (pThread == NULL) {
        CloseHandle(hProcess);
        CloseHandle(hSnapshot);
        return FALSE;
    }

    // Call FreeLibrary to uninstall DLL
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pThread, MOD.modBaseAddr, 0, NULL);
    if (!hThread) {
        CloseHandle(hProcess);
        CloseHandle(hSnapshot);
        return FALSE;
    }

    // Waiting for thread to end
    WaitForSingleObject(hThread, INFINITE);

    // Clear
    CloseHandle(hThread);
    CloseHandle(hProcess);
    CloseHandle(hSnapshot);

    return TRUE;
}

// Obtain UIAccess permission by restarting the application
// There is no need to manually call this function.
BOOL WTMAPI EnableUIAccess(BOOL bEnable) {
    DWORD dwErr = 0;
    if (bEnable == TRUE) {
        // Call third-party functions to gain privileges (Thanks for Them)
        dwErr = PrepareForUIAccess();
        return dwErr == ERROR_SUCCESS;
    }
    else {
        // Restart yourself to relinquish permissions
        DWORD fUIAccess = 0;
        if (CheckForUIAccess(&dwErr, &fUIAccess)) {
            if (fUIAccess) return TRUE;
        }
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        GetStartupInfo(&si);
        if (CreateProcess(NULL, GetCommandLine(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            ExitProcess(0);
            return TRUE;
        }
        return FALSE;
    }
}

// Create the top-level window using UIAccess and verify if the Z segment is correct. If an error occurs, return NULL.
// The application will restart.
HWND WTMAPI CreateTopMostWindowW(
    _In_ DWORD dwExStyle,
    _In_ LPCWSTR lpClassName,
    _In_ LPCWSTR lpWindowName,
    _In_ DWORD dwStyle,
    _In_ int X,
    _In_ int Y,
    _In_ int nWidth,
    _In_ int nHeight,
    _In_opt_ HWND hWndParent,
    _In_opt_ HMENU hMenu,
    _In_ HINSTANCE hInstance,
    _In_opt_ LPVOID lpParam
) {
    if (lpClassName == NULL || lpWindowName == NULL) return NULL;
	// Get function address
	HMODULE hUser32 = GetModuleHandle(TEXT("User32.dll"));
	if (hUser32 == NULL) return NULL;
	GetWindowBand pGetWindowBand = (GetWindowBand)GetProcAddress(hUser32, "GetWindowBand");
	CreateWindowInBand pCreateWindowInBand = (CreateWindowInBand)GetProcAddress(hUser32, "CreateWindowInBand");
	SetWindowBand pSetWindowBand = (SetWindowBand)GetProcAddress(hUser32, "SetWindowBand");
	if (pGetWindowBand == NULL || pCreateWindowInBand == NULL || pSetWindowBand == NULL) return NULL;

	// Prepare UIAccess
	DWORD dwErr = PrepareForUIAccess();
	if (ERROR_SUCCESS != dwErr) return NULL;

	HWND Window = pCreateWindowInBand(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam, ZBID_UIACCESS);
	if (Window == NULL) return NULL;

	// Verify Z sequence segment
	DWORD bandCheck;
	if (pGetWindowBand(Window, &bandCheck)) {
		if (bandCheck != ZBID_UIACCESS) return NULL;
	}
	else return NULL;
	return Window;
}
HWND WTMAPI CreateTopMostWindowA(
    _In_ DWORD dwExStyle,
    _In_ LPCSTR lpClassName,
    _In_ LPCSTR lpWindowName,
    _In_ DWORD dwStyle,
    _In_ int X,
    _In_ int Y,
    _In_ int nWidth,
    _In_ int nHeight,
    _In_opt_ HWND hWndParent,
    _In_opt_ HMENU hMenu,
    _In_ HINSTANCE hInstance,
    _In_opt_ LPVOID lpParam
) {
    // Convert to LPCWTR and call again
    HWND hWnd = NULL;
    if (lpClassName == NULL || lpWindowName == NULL) return NULL;
    hWnd = CreateTopMostWindowW(dwExStyle, LPC2LPW(lpClassName).c_str(), LPC2LPW(lpWindowName).c_str(), dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    return hWnd;
}

// SetWindowBand that does not deny access, returns false if an error occurs.
BOOL WTMAPI SetWindowBandEx(
	_In_opt_ HWND hWnd,
	_In_opt_ HWND hWndInsertAfter,
	_In_opt_ DWORD dwBand
) {
    if (!CheckForDll()) return FALSE;
    
    // Obtain information file address
    TCHAR DirBuffer[MAX_PATH];
    std::_tstring InfoFileFullPath;
    std::_tstring FlagFileFullPath;
    if (!GetEnvironmentVariable(TEXT("PUBLIC"), DirBuffer, MAX_PATH)) return FALSE;
    InfoFileFullPath = DirBuffer;
    InfoFileFullPath = InfoFileFullPath + IAMDIRECTORYNAME + INFOFILENAME;
    FlagFileFullPath = DirBuffer;
    FlagFileFullPath = FlagFileFullPath + IAMDIRECTORYNAME + FLAGFILENAME;

    // Wait for the backend to be ready (check if the flag file has been generated)
    while (!FSObjectExist(FlagFileFullPath)) {
        Sleep(50);
    }

    // Get collection window handle
    HWND hCollectorWnd = FindWindow(WNDCLASSNAME, NULL);
    if (hCollectorWnd == NULL) return FALSE;

    // Write information to a file
    UCHAR Buffer[8] = { 0 };
    ULLToByte((ULONG64)hWnd, Buffer);
    FBWriteFile(InfoFileFullPath.c_str(), Buffer, NULL, 0, 8);
    ULLToByte((ULONG64)hWndInsertAfter, Buffer);
    FBWriteFile(InfoFileFullPath.c_str(), Buffer, NULL, 8, 8);
    ULToByte((ULONG32)dwBand, Buffer);
    FBWriteFile(InfoFileFullPath.c_str(), Buffer, NULL, 16, 4);

    // Send window information
    SendMessage(hCollectorWnd, WM_FILECRATED, 0, 0);

    return TRUE;
}

// CreateWindowInBand will not deny access and will return NULL if an error occurs.
HWND WTMAPI CreateWindowInBandExW(
    _In_ DWORD dwExStyle,
    _In_opt_ LPCWSTR lpClassName,
    _In_opt_ LPCWSTR lpWindowName,
    _In_ DWORD dwStyle,
    _In_ int X,
    _In_ int Y,
    _In_ int nWidth,
    _In_ int nHeight,
    _In_opt_ HWND hWndParent,
    _In_opt_ HMENU hMenu,
    _In_ HINSTANCE hInstance,
    _In_opt_ LPVOID lpParam,
    _In_ ZBID dwBand
) {
    if (!CheckForDll()) return NULL;
    HWND hWnd = NULL;
    hWnd = CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    if (hWnd == NULL) return NULL;
    SetWindowBandEx(hWnd, NULL, dwBand);
    return hWnd;
}
HWND WTMAPI CreateWindowInBandExA(
    _In_ DWORD dwExStyle,
    _In_opt_ LPCSTR lpClassName,
    _In_opt_ LPCSTR lpWindowName,
    _In_ DWORD dwStyle,
    _In_ int X,
    _In_ int Y,
    _In_ int nWidth,
    _In_ int nHeight,
    _In_opt_ HWND hWndParent,
    _In_opt_ HMENU hMenu,
    _In_ HINSTANCE hInstance,
    _In_opt_ LPVOID lpParam,
    _In_ ZBID dwBand
) {
    if (!CheckForDll()) return NULL;
    HWND hWnd = NULL;
    hWnd = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    if (hWnd == NULL) return NULL;
    SetWindowBandEx(hWnd, NULL, dwBand);
    return hWnd;
}

// GetWindowBand call entrance, and if an error occurs, returns False.
BOOL WTMAPI GetWindowBandEx(
    _In_ HWND hWnd,
    _In_ LPDWORD pdwBand
) {
    if (pdwBand == NULL) return FALSE;
    // Get function address
    HMODULE hUser32 = GetModuleHandle(TEXT("User32.dll"));
    if (hUser32 == NULL) return FALSE;
    GetWindowBand pGetWindowBand = (GetWindowBand)GetProcAddress(hUser32, "GetWindowBand");
    if (pGetWindowBand == NULL) return FALSE;
    BOOL bRes = pGetWindowBand(hWnd, pdwBand);
    return bRes;
}

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:

        // Disable DLL_THREAD_ATTACH and DLL_THREAD_DETACH events to avoid multiple entries into the main function
        DisableThreadLibraryCalls(hModule);

        // Call initialization function
        WTMInit();

        break;

    case DLL_PROCESS_DETACH:

        // Call the unload function
        WTMUninit();

        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}


#if defined(__cplusplus)
}
#endif