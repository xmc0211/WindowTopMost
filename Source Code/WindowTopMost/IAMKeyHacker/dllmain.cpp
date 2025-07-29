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

#ifdef _DEBUG
#include <fstream>
#endif

#define IAMEXPORT __declspec(dllexport)
#define IAMIMPORT __declspec(dllimport)
#define IAMAPI IAMEXPORT WINAPI

// The program uses files to transfer data.
// WNDCLASSNAME is the window class name. The window title is empty.
// WM_FILECREATED is a custom message.
// IAMDIRECTORYNAME is the temp directory name.
// INFOFILENAME is the name of the information file (the file is saved in %PUBLIC%\\IAMDIRECTORYNAME\\)
// FLAGFILENAME is the flag file name (created after successfully obtaining the IAM key and saved in %PUBLIC%\\IAMDIRECTORYNAME\\)
#define WND_COLLECTOR_CLASSNAME       TEXT("IAMCollector_7DD48162_2BC7_4073_9CC8_51CC3DB0FF72")
#define WND_HANDLER_CLASSNAME         TEXT("IAMHandler_D06B7428_73FA_4967_B30C_96030ACCF998")
#define WM_PERFORM          (WM_USER + 1)
#define WM_HANDLERHELLO     (WM_USER + 2)
#define WM_COLLECTORHELLO   (WM_USER + 3)
#define WM_COLLECTORSTART   (WM_USER + 4)
#define WM_RESULT           (WM_USER + 5)

// Lure EXPLORER Delay between EXE operations (ms)
#define INDUCEDELAY (100ul)

// Debug
#ifdef _DEBUG
#define DEBUGLOG(Text) do { std::ofstream Log("D:\\DebugLog_IAM.txt", std::ios::app); Log << Text << std::endl; } while (0)
#define DEBUGERR(Text) DEBUGLOG(Text + (", " + std::to_string((ULONG)GetLastError())))
#else
#define DEBUGLOG(Text) do {  } while (0)
#define DEBUGERR(Text) DEBUGLOG(Text)
#endif

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
T_GetWindowBand pGetWindowBand = NULL;
T_CreateWindowInBand pCreateWindowInBand = NULL;
T_SetWindowBand pSetWindowBand = NULL;
T_NtUserEnableIAMAccess pNtUserEnableIAMAccess = NULL;

HMODULE hParentInstance = NULL; // Parent process (Explorer.EXE) instance handle
HANDLE hInduceThread = NULL; // Thread instance handles
HANDLE hCheckThread = NULL;
HWND hCollectWnd = NULL; // Information receiving window handle
ULONG64 ullIAMKey = 0; // Saved IAM Access Key
BOOL bHookCreated = FALSE; // Has the hook been successfully created
BOOL bIAMKeySaved = FALSE; // Has the IAM key been successfully obtained and saved
BOOL bInduceShouldStop = FALSE; // Should the Induce thread exit
BOOL bCheckThreadStopped = FALSE; // Has thread Check exited
BOOL bInduceThreadStopped = FALSE; // Has thread Induce exited
BOOL bHandlerStarted = FALSE; // Has handler window sent WM_COLLECTORSTART

// Function declarations

#if defined(__cplusplus)
extern "C" {
#endif

// The structure of IAMCollector request and response
struct IAMCollectorRequest {
    HWND hWnd;
    HWND hWndInsertAfter;
    DWORD dwBand;
};
struct IAMCollectorResponse {
    BOOL bCollected;
    BOOL bStatus;
    DWORD dwErrorCode;
};

BOOL SendMessageEx(HWND hWnd, HWND hSourceWnd, UINT message);
LRESULT IAMAPI CALLBACK CollectWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void IAMAPI CALLBACK CheckSetWindowBandCallThread(LPVOID lpParam);
BOOL IAMAPI NtUserEnableIAMAccessHook(ULONG64 key, BOOL enable);
void IAMAPI CALLBACK InduceExplorerCallThread(LPVOID lpParam);
BOOL IAMAPI GetIAMAttachMain();
BOOL IAMAPI GetIAMDetachMain();

// Use WM_COPYDATA to transmit message above WM_USER avoid permission issues
BOOL SendMessageEx(HWND hWnd, HWND hSourceWnd, UINT message) {
    COPYDATASTRUCT TempMessage;
    TempMessage.dwData = message;
    TempMessage.lpData = NULL;
    TempMessage.cbData = 0;
    LRESULT bRes = SendMessage(hWnd, WM_COPYDATA, (WPARAM)hSourceWnd, (LPARAM)&TempMessage);
    if (bRes != ERROR_SUCCESS) DEBUGERR("Communication: SendMessage error");
    return bRes == ERROR_SUCCESS;
}

// Message receiving window message processing function
LRESULT IAMAPI CALLBACK CollectWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COPYDATA: {
        PCOPYDATASTRUCT pRequestDataInfo = (PCOPYDATASTRUCT)lParam;
        if (pRequestDataInfo->dwData == WM_PERFORM) {
            if (!bHandlerStarted) break; // Exit if handler is not start
            // Get request data
            DEBUGLOG("Enter Message: WM_PERFORM");
            if (pRequestDataInfo->cbData != sizeof(IAMCollectorRequest)) {
                DEBUGLOG("Collector: Request size is incorrect");
                return 1;
            }
            IAMCollectorRequest* pRequestData = reinterpret_cast<IAMCollectorRequest*>(pRequestDataInfo->lpData);
            DEBUGLOG("Collector: hWnd=" + std::to_string((ULONG64)pRequestData->hWnd));
            DEBUGLOG("Collector: hWndInsertAfter=" + std::to_string((ULONG64)pRequestData->hWndInsertAfter));
            DEBUGLOG("Collector: dwBand=" + std::to_string((ULONG64)pRequestData->dwBand));

            // Temporarily enable IAM access and execution.
            DEBUGLOG("Collector: SetWindowBand Executing");
            pNtUserEnableIAMAccess(ullIAMKey, TRUE);
            SetLastError(ERROR_SUCCESS);
            BOOL bRes = pSetWindowBand(pRequestData->hWnd, pRequestData->hWndInsertAfter, pRequestData->dwBand);
            DWORD dwError = GetLastError();
            pNtUserEnableIAMAccess(ullIAMKey, FALSE);

            // Send response data
            HWND hHandlerWindow = FindWindow(WND_HANDLER_CLASSNAME, NULL);
            if (!hHandlerWindow) {
                DEBUGERR("Collector: Can not find handler window");
                return 1;
            }
            COPYDATASTRUCT ResponseDataInfo;
            IAMCollectorResponse ResponseData;
            ResponseData.bCollected = TRUE;
            ResponseData.bStatus = bRes;
            ResponseData.dwErrorCode = dwError;
            ResponseDataInfo.dwData = WM_RESULT;
            ResponseDataInfo.cbData = sizeof(ResponseData);
            ResponseDataInfo.lpData = &ResponseData;
            DEBUGLOG("Collector: bStatus=" + std::to_string((ULONG64)ResponseData.bStatus));
            DEBUGLOG("Collector: dwErrorCode=" + std::to_string((ULONG64)ResponseData.dwErrorCode));
            SendMessage(hHandlerWindow, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&ResponseDataInfo);

            DEBUGLOG("Exit Message: WM_PERFORM");
        }
        if (pRequestDataInfo->dwData == WM_HANDLERHELLO) {
            DEBUGLOG("Communication: --> Handler hello");
            HWND hHandlerWindow = FindWindow(WND_HANDLER_CLASSNAME, NULL);
            if (!hHandlerWindow) {
                DEBUGERR("Collector: Can not find handler window");
                return 1;
            }
            DEBUGLOG("Communication: <-- Collector hello");
            SendMessageEx(hHandlerWindow, hWnd, WM_COLLECTORHELLO);
        }
        if (pRequestDataInfo->dwData == WM_HANDLERHELLO) {
            DEBUGLOG("Communication: --> Collector start");
            bHandlerStarted = TRUE;
        }
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
void IAMAPI CALLBACK CheckSetWindowBandCallThread(LPVOID lpParam) {
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
    wcex.lpszClassName = WND_COLLECTOR_CLASSNAME;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    if (!RegisterClassEx(&wcex)) {
        DEBUGERR("Window Creater: Window class register error");
        bCheckThreadStopped = TRUE;
        DEBUGLOG("Failure Exit: CheckSetWindowBandCallThread");
        return;
    }

    // Window initialization, but not displayed
    hCollectWnd = CreateWindowEx(
        NULL, 
        WND_COLLECTOR_CLASSNAME,
        TEXT(""),
        WS_OVERLAPPEDWINDOW,
        0, 0, 100, 100, 
        nullptr, 
        nullptr, 
        hParentInstance, 
        nullptr
    );
    if (!hCollectWnd) {
        DEBUGERR("Window Creater: Window create error");
        UnregisterClass(WND_COLLECTOR_CLASSNAME, hParentInstance);
        bCheckThreadStopped = TRUE;
        DEBUGLOG("Failure Exit: CheckSetWindowBandCallThread");
        return;
    }
    if (IsUserAnAdmin()) {
        BOOL bRes = ChangeWindowMessageFilterEx(hCollectWnd, WM_COPYDATA, MSGFLT_ADD, NULL); // Reduce permissions to accept WM_COPYDATA
        if (!bRes) {
            DEBUGERR("Window Creater: ChangeWindowMessageFilterEx error");
            UnregisterClass(WND_COLLECTOR_CLASSNAME, hParentInstance);
            bCheckThreadStopped = TRUE;
            DEBUGLOG("Failure Exit: CheckSetWindowBandCallThread");
            return;
        }
    }
    UpdateWindow(hCollectWnd);
    
    // Main message loop
    DEBUGLOG("Window Creater: Window created successfully. Enter main loop. ");
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnregisterClass(WND_COLLECTOR_CLASSNAME, hParentInstance);
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
        DEBUGLOG("APIHookCB: Key saved successfully. Key=" + std::to_string(ullIAMKey));

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
void IAMAPI CALLBACK InduceExplorerCallThread(LPVOID lpParam) {
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

    DEBUGLOG("Inducer: Start induce.");
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
    
    // Temporarily load dynamic libraries and obtain addresses of undisclosed API functions
    HMODULE hUser32 = GetModuleHandle(TEXT("User32.dll"));
    if (hUser32 == NULL) return FALSE;
    pGetWindowBand = (T_GetWindowBand)GetProcAddress(hUser32, "GetWindowBand"); 
    pCreateWindowInBand = (T_CreateWindowInBand)GetProcAddress(hUser32, "CreateWindowInBand"); 
    pSetWindowBand = (T_SetWindowBand)GetProcAddress(hUser32, "SetWindowBand");
    pNtUserEnableIAMAccess = (T_NtUserEnableIAMAccess)GetProcAddress(hUser32, MAKEINTRESOURCEA(2510));
    if (pGetWindowBand == NULL || pCreateWindowInBand == NULL || pSetWindowBand == NULL || pNtUserEnableIAMAccess == NULL) return FALSE;

    // Create API hooks using MHook and monitor NtUserEnableIAMAccess
    if (!Mhook_SetHook((PVOID*)&pNtUserEnableIAMAccess, NtUserEnableIAMAccessHook)) {
        DEBUGERR("AttachMain: SetHook error.");
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
        while (!bCheckThreadStopped) Sleep(50); // Wait with global variables
    }

    // If the hook is successfully created, use MHook to uninstall the API hook
    if (bHookCreated == TRUE) {
        if (!Mhook_Unhook((PVOID*)&pNtUserEnableIAMAccess)) {
            DEBUGERR("DetachMain: Unhook hook error.");
            return FALSE;
        }
        bHookCreated = FALSE; // Update state
    }

    // Ensure that the Induce thread ends
    bInduceShouldStop = TRUE;
    if (hInduceThread) {
        while (!bInduceThreadStopped) Sleep(50); // Wait with global variables
    }

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