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

#include <Windows.h>
#include <shlobj_core.h>
#include <string>
#include "mhook-lib/mhook.h"
#include "FileBinIO.h"
#include "FileSys.h"

#ifdef _DEBUG
#include <fstream>
#endif

#define IAMEXPORT __declspec(dllexport)
#define IAMIMPORT __declspec(dllimport)
#define IAMAPI IAMEXPORT WINAPI

// Declaration of undisclosed API functions
#define DECLAREFUNC(f) T_##f p##f = NULL;
// Obtain the function address and check for a null pointer (return False, initialization failed). Only used in DllMain.
#define GETFUNCADDR(f) {p##f = (T_##f)GetProcAddress(hUser32, #f); if (p##f == NULL) return FALSE; }
#define GETFUNCADDRFROM(f, v) {p##f = (T_##f)(v); if (p##f == NULL) return FALSE; }

// The program uses files to transfer data.
// WNDCLASSNAME is the window class name. The window title is empty.
// WM_FILECREATED is a custom message.
// IAMDIRECTORYNAME is the temp directory name.
// INFOFILENAME is the name of the information file (the file is saved in %PUBLIC%\\IAMDIRECTORYNAME\\)
// FLAGFILENAME is the flag file name (created after successfully obtaining the IAM key and saved in %PUBLIC%\\IAMDIRECTORYNAME\\)
#define WNDCLASSNAME TEXT("IAMCollector_7DD48162_2BC7_4073_9CC8_51CC3DB0FF72")
#define WM_FILECRATED (WM_USER + 1)
#define IAMDIRECTORYNAME TEXT("\\TEMP_7DD48162_2BC7_4073_9CC8_51CC3DB0FF72\\")
#define INFOFILENAME TEXT("IAMInfo_7DD48162_2BC7_4073_9CC8_51CC3DB0FF72.tmp")
#define FLAGFILENAME TEXT("IAMFlag_7DD48162_2BC7_4073_9CC8_51CC3DB0FF72.tmp")

// Lure EXPLORER Delay between EXE operations (ms)
#define INDUCEDELAY (100ul)

// Debug
#ifdef _DEBUG
#define DEBUGLOG(Text) do { std::ofstream Log("C:\\Users\\PUBLIC\\Desktop\\DebugLog_IAM.txt", std::ios::app); Log << Text << std::endl; } while (0)
#else
#define DEBUGLOG(Text) do {  } while (0)
#endif
#define DEBUGERR(Text) DEBUGLOG(Text + std::to_string((ULONG)GetLastError()))

// Unpublished API function prototype definition
typedef BOOL(WINAPI* T_GetWindowBand)(
    HWND hWnd,
    LPDWORD pdwBand
);
typedef HWND(WINAPI* T_CreateWindowInBand)(
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
typedef BOOL(WINAPI* T_SetWindowBand)(
    HWND hWnd,
    HWND hWndInsertAfter,
    DWORD dwBand
);
typedef BOOL(WINAPI* T_NtUserEnableIAMAccess)(
    ULONG64 key,
    BOOL enable
);

// Unpublished API function global object declaration
DECLAREFUNC(GetWindowBand);
DECLAREFUNC(CreateWindowInBand);
DECLAREFUNC(SetWindowBand);
DECLAREFUNC(NtUserEnableIAMAccess);
HMODULE hParentInstance = NULL; // Parent process (Explorer.EXE) instance handle
HANDLE hInduceThread = NULL; // Thread instance handles
HANDLE hCheckThread = NULL;
HWND hCollectWnd = NULL; // Information receiving window handle
ULONG64 ullIAMKey = 0; // Saved IAM Access Key
std::_tstring InfoFileFullPath = TEXT(""); // Complete path of information file (%PUBLIC%\\IAMDIRECTORYNAME\\INFOFILENAME)
std::_tstring FlagFileFullPath = TEXT(""); // Complete path of flag file (%PUBLIC%\\IAMDIRECTORYNAME\\FLAGFILENAME)
std::_tstring IAMTempDirectoryPath = TEXT(""); // Complete path of temp directory (%PUBLIC%\\IAMDIRECTORYNAME)
BOOL bHookCreated = FALSE; // Has the hook been successfully created
BOOL bIAMKeySaved = FALSE; // Has the IAM key been successfully obtained and saved
BOOL bInduceShouldStop = FALSE; // Should the Induce thread exit
BOOL bCheckThreadStopped = FALSE; // Has thread Check exited
BOOL bInduceThreadStopped = FALSE; // Has thread Induce exited

// Function declarations

#if defined(__cplusplus)
extern "C" {
#endif

ULONG32 IAMAPI UChToUL(UCHAR Buffer[4]);
ULONG64 IAMAPI UChToULL(UCHAR Buffer[8]);
LRESULT IAMAPI CALLBACK CollectWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
VOID IAMAPI CALLBACK CheckSetWindowBandCallThread(LPVOID lpParam);
BOOL IAMAPI NtUserEnableIAMAccessHook(ULONG64 key, BOOL enable);
VOID IAMAPI CALLBACK InduceExplorerCallThread(LPVOID lpParam);
BOOL IAMAPI GetIAMAttachMain();
BOOL IAMAPI GetIAMDetachMain();

// Convert UCHAR[8] to ULONG64, and UCHAR[4] to ULONG32
ULONG64 IAMAPI UChToULL(UCHAR Buffer[8]) {
    ULONG64 ullRes = 0;
    for (INT pt = 0; pt < 8; ++pt) {
        ullRes |= (static_cast<ULONG64>(Buffer[pt]) << (56 - (pt * 8)));
    }
    return ullRes;
}
ULONG32 IAMAPI UChToUL(UCHAR Buffer[4]) {
    ULONG32 ullRes = 0;
    for (INT pt = 0; pt < 4; ++pt) {
        ullRes |= (static_cast<ULONG32>(Buffer[pt]) << (12 - (pt * 4)));
    }
    return ullRes;
}

// Message receiving window message processing function
LRESULT IAMAPI CALLBACK CollectWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_FILECRATED: {
        DEBUGLOG("Enter Message: WM_FILECREATED");
        HWND ArhWnd = NULL;
        HWND ArhWndInsertAfter = NULL;
        DWORD ArdwBand = 0;
        UCHAR Buffer[8] = {0};
        
        // Read data from file
        if (FBReadFile(InfoFileFullPath, Buffer, NULL, 0, 8) != FB_SUCCESS) break;
        ArhWnd = (HWND)UChToULL(Buffer);
        if (FBReadFile(InfoFileFullPath, Buffer, NULL, 8, 8) != FB_SUCCESS) break;
        ArhWndInsertAfter = (HWND)UChToULL(Buffer);
        if (FBReadFile(InfoFileFullPath, Buffer, NULL, 16, 4) != FB_SUCCESS) break;
        ArdwBand = (DWORD)UChToUL(Buffer);

        // Delete files to ensure no traces are left
        DeleteFile(InfoFileFullPath.c_str());

        DEBUGLOG("Collector: hWnd=" + std::to_string((ULONG64)ArhWnd));
        DEBUGLOG("Collector: hWndInsertAfter=" + std::to_string((ULONG64)ArhWndInsertAfter));
        DEBUGLOG("Collector: dwBand=" + std::to_string((ULONG64)ArdwBand));

        // Temporarily enable IAM access and execution. Discard return value (unable to pass).
        DEBUGLOG("Collector: SetWindowBand Executing");
        pNtUserEnableIAMAccess(ullIAMKey, TRUE);
        BOOL bRes = pSetWindowBand(ArhWnd, ArhWndInsertAfter, ArdwBand);
        pNtUserEnableIAMAccess(ullIAMKey, FALSE);

        DEBUGLOG("Exit Message: WM_FILECREATED");
        break;
    }
    case WM_CLOSE: {
        DestroyWindow(hWnd);
        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Thread callback function for monitoring SetWindowBand requests
// A hidden window will be created within the function to receive messages.
VOID IAMAPI CALLBACK CheckSetWindowBandCallThread(LPVOID lpParam) {
    DEBUGLOG("Enter: CheckSetWindowBandCallThread");

    // Register Window Class
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = CollectWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = (HINSTANCE)hParentInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = WNDCLASSNAME;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    if (!RegisterClassEx(&wcex)) {
        DEBUGERR("Window Creater: Window Class Resigter Error: ");
        bCheckThreadStopped = TRUE;
        DEBUGLOG("Failure Exit: CheckSetWindowBandCallThread");
        return;
    }

    // Window initialization, but not displayed
    hCollectWnd = CreateWindowEx(
        NULL, 
        WNDCLASSNAME,
        TEXT(""),
        WS_OVERLAPPEDWINDOW,
        0, 0, 100, 100, 
        nullptr, 
        nullptr, 
        hParentInstance, 
        nullptr
    );
    if (!hCollectWnd) {
        DEBUGERR("Window Creater: Window Create Error: ");
        UnregisterClass(WNDCLASSNAME, hParentInstance);
        bCheckThreadStopped = TRUE;
        DEBUGLOG("Failure Exit: CheckSetWindowBandCallThread");
        return;
    }
    UpdateWindow(hCollectWnd);

    // Create flag file
    BYTE Buffer[4] = { 'S', 'u', 'c', '\0' };
    FBWriteFile(FlagFileFullPath, Buffer, NULL, 0, 4);
    
    // Main message loop
    DEBUGLOG("Window Creater: Window Created Successfully. Enter Main Loop. ");
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnregisterClass(WNDCLASSNAME, hParentInstance);
    bCheckThreadStopped = TRUE;
    DEBUGLOG("Successful Exit: CheckSetWindowBandCallThread");
    return;
}

// A function modeled after NtUserEnableIAMAccess, used for hook callbacks and obtaining IAM keys
BOOL IAMAPI NtUserEnableIAMAccessHook(ULONG64 key, BOOL enable) {
    DEBUGLOG("Enter: NtUserEnableIAMAccessHook");

    // Get the return value of the real function
    const BOOL bRes = pNtUserEnableIAMAccess(key, enable);

    // Check if it is successful and if the IAM key has not been set. If it matches, obtain IAM key and uninstall hook
    if (bRes == TRUE && bIAMKeySaved == FALSE) {

        // Save IAM Key
        ullIAMKey = key;
        DEBUGLOG("APIHookCB: Key Saved Successfully. Key=" + std::to_string(ullIAMKey));

        // Set flag as saved
        bIAMKeySaved = TRUE;
        
        // Start the thread and start monitoring SetWindowBand requests in the registry
        hCheckThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CheckSetWindowBandCallThread, NULL, NULL, 0);

        // Uninstall the hook to stop monitoring
        if (bHookCreated == TRUE) {
            if (!Mhook_Unhook((PVOID*)&pNtUserEnableIAMAccess)) return FALSE;
            bHookCreated = FALSE; // Update state
        }
    }
    
    // Return the value that should be returned
    DEBUGLOG("Successful Exit: NtUserEnableIAMAccessHook");
    return bRes;
}

// A thread callback function used to lure Explorer.EXE
VOID IAMAPI CALLBACK InduceExplorerCallThread(LPVOID lpParam) {
    DEBUGLOG("Enter: InduceExplorerCallThread");

    // Use API functions and FindWindow to obtain handles for the active window and taskbar, and determine null pointers
    HWND hWndForeground = GetForegroundWindow();
    HWND hWndTaskBar = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (hWndForeground == NULL || hWndTaskBar == NULL) {
        DEBUGLOG("Inducer: Taskbar not found.");
        bInduceThreadStopped = TRUE;
        DEBUGLOG("Failure Exit: InduceExplorerCallThread");
        return;
    }

    DEBUGLOG("Inducer: Start Induce.");
    while (!bIAMKeySaved && !bInduceShouldStop) {
        Sleep(INDUCEDELAY); // Give the system some reaction time, the same applies below

        // Set focus to desktop window, ready to lure
        SetForegroundWindow(GetDesktopWindow());
        Sleep(INDUCEDELAY);

        // Set focus to taskbar, trigger recovery event
        SetForegroundWindow(hWndTaskBar);
        Sleep(INDUCEDELAY);

        // Restore the active window to avoid affecting users
        SetForegroundWindow(hWndForeground);
        Sleep(INDUCEDELAY);
    }

    bInduceThreadStopped = TRUE;
    DEBUGLOG("Successful Exit: InduceExplorerCallThread");
    return;
}

// The main function that runs during loading
BOOL IAMAPI GetIAMAttachMain() {

    // Prevent duplicate loading of hooks
    if (bHookCreated == TRUE) return TRUE;

    // Save the complete path of the information file
    TCHAR Buffer[MAX_PATH];
    if (!GetEnvironmentVariable(TEXT("PUBLIC"), Buffer, MAX_PATH)) return FALSE;
    IAMTempDirectoryPath = Buffer;
    IAMTempDirectoryPath = IAMTempDirectoryPath + IAMDIRECTORYNAME;
    FSCreateDir(IAMTempDirectoryPath);
    FSSetObjectAttribute(IAMTempDirectoryPath, FILE_ATTRIBUTE_HIDDEN);
    InfoFileFullPath = Buffer;
    InfoFileFullPath = InfoFileFullPath + IAMDIRECTORYNAME + INFOFILENAME;
    FlagFileFullPath = Buffer;
    FlagFileFullPath = FlagFileFullPath + IAMDIRECTORYNAME + FLAGFILENAME;
    
    // Temporarily load dynamic libraries and obtain addresses of undisclosed API functions
    HMODULE hUser32 = GetModuleHandle(TEXT("User32.dll"));
    if (hUser32 == NULL) return FALSE;
    GETFUNCADDR(GetWindowBand);
    GETFUNCADDR(CreateWindowInBand);
    GETFUNCADDR(SetWindowBand);
    GETFUNCADDRFROM(NtUserEnableIAMAccess, GetProcAddress(hUser32, MAKEINTRESOURCEA(2510)));

    // Create API hooks using MHook and monitor NtUserEnableIAMAccess
    if (!Mhook_SetHook((PVOID*)&pNtUserEnableIAMAccess, NtUserEnableIAMAccessHook)) {
        DEBUGERR("AttachMain: SetHook Error.");
        return FALSE;
    }
    bHookCreated = TRUE; // Update State

    // Create a thread to lure Explorer.EXE to call NtUserEnableIAMAccess, thereby triggering the hook
    hInduceThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)InduceExplorerCallThread, NULL, NULL, 0);
    
    return TRUE;
}

// The main function that runs during uninstallation
BOOL IAMAPI GetIAMDetachMain() {

    // Destroy the window and let the thread check end
    SendMessage(hCollectWnd, WM_CLOSE, 0, 0);
    if (hCheckThread) {
        //WaitForSingleObject(hCheckThread, INFINITE); // NOTE: Unknown bug, changing it will cause a dead loop
        while (!bCheckThreadStopped) Sleep(50); // Wait with global variables
    }

    // If the hook is successfully created, use MHook to uninstall the API hook
    if (bHookCreated == TRUE) {
        if (!Mhook_Unhook((PVOID*)&pNtUserEnableIAMAccess)) {
            DEBUGERR("DetachMain: Unhook hook Error.");
            return FALSE;
        }
        bHookCreated = FALSE; // Update state
    }

    // Ensure that the Induce thread ends
    bInduceShouldStop = TRUE;
    if (hInduceThread) {
        // WaitForSingleObject(hInduceThread, INFINITE); // NOTE: Unknown bug, changing it will cause a dead loop
        while (!bInduceThreadStopped) Sleep(50); // Wait with global variables
    }

    // Delete flag file and the directory
    FSDeleteFile(FlagFileFullPath);
    FSDeleteDir(IAMTempDirectoryPath.c_str());

    return TRUE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {

        SetLastError(0);
        DEBUGLOG("IAMKeyHacker: DLL Already");

        // Set caller instance handle
        hParentInstance = (HINSTANCE)hModule;

        // Disable DLL_THREAD_ATTACH and DLL_THREAD_DETACH events to avoid multiple entries into the main function
        DisableThreadLibraryCalls(hParentInstance);

        // Call the main function and handle exceptions
        if (!GetIAMAttachMain()) {
            DEBUGLOG("IAMKeyHacker: AttachMain Error");
            return FALSE;
        }

        break;
    }
    case DLL_PROCESS_DETACH: {

        // Call the main function and handle exceptions
        if (!GetIAMDetachMain()) {
            DEBUGLOG("IAMKeyHacker: DetachMain Error");
            return FALSE;
        }

        DEBUGLOG("IAMKeyHacker: DLL Unloaded");
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    default: break;
    }
    return TRUE;
}

#if defined(__cplusplus)
}
#endif