// MIT License
//
// Copyright (c) 2025-2026 WindowTopMost - xmc0211 <xmc0211@qq.com>
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

#include "../Common Headers/Framework.h"
#include <shlobj_core.h>
#include "mhook-lib/mhook.h"
#include "IAMKeyBlock.h"

// Because there will be no static references from third-party programs, set IAMAPI as IAMIMPORT directly
#define IAMEXPORT __declspec(dllexport)
#define IAMIMPORT __declspec(dllimport)
#define IAMAPI IAMEXPORT WINAPI

// Unpublished API function global object declaration
T_GetWindowBand pGetWindowBand = NULL;
T_CreateWindowInBand pCreateWindowInBand = NULL;
T_CreateWindowInBandEx pCreateWindowInBandEx = NULL;
T_SetWindowBand pSetWindowBand = NULL;
T_NtUserEnableIAMAccess pNtUserEnableIAMAccess = NULL;

// Global variables
HMODULE hParentInstance = NULL; // Parent process (Explorer.EXE) instance handle
HANDLE hCheckThread = NULL;
HWND hCollectWnd = NULL; // Information receiving window handle
BOOL bHookCreated = FALSE; // Has the hook been successfully created
BOOL bCheckThreadStopped = FALSE; // Has thread Check exited
BOOL bHandlerStarted = FALSE; // Has handler window sent WM_COLLECTORSTART

// IAM global variables
ULONG64 nIAMKey = 0; // Saved IAM Access Key
BOOL bIAMKeySaved = FALSE; // Has the IAM key been successfully obtained and saved

// Function declarations

#if defined(__cplusplus)
extern "C" {
#endif

// Handling requests from handler
BOOL IAMHandleRequest(const IAMCollectorRequest* pRequest, IAMCollectorResponse* pResponse) {
    if (pRequest == nullptr || pResponse == nullptr) return FALSE;

    // Get request data
    switch (pRequest->RequestType) {
        case IAM_GetIAMKey: {
            pResponse->ResponseType = IAM_GetIAMKey;
            IAM_GetIAMKey_ResponseArguments* pResponseArguments = &pResponse->Arguments.GetIAMKey;

            // Return IAM key, error if not found
            if (!bIAMKeySaved) {
                pResponseArguments->dwErrorCode = ERROR_NOT_READY;
                pResponseArguments->nIAMKey = NULL;
                DEBUGERR("RequestHandler: Request IAM key but not saved");
                break;
            }
            pResponseArguments->dwErrorCode = ERROR_SUCCESS;
            pResponseArguments->nIAMKey = nIAMKey;
            DEBUGLOG("RequestHandler: Request IAM key success");
            break;
        }
        case IAM_SetWindowBand: {
            pResponse->ResponseType = IAM_SetWindowBand;
            const IAM_SetWindowBand_RequestArguments* pRequestArguments = &pRequest->Arguments.SetWindowBand;
            IAM_SetWindowBand_ResponseArguments* pResponseArguments = &pResponse->Arguments.SetWindowBand;

            // Temporarily enable IAM access and execution
            pNtUserEnableIAMAccess(nIAMKey, TRUE);
            SetLastError(ERROR_SUCCESS);
            pResponseArguments->bStatus = pSetWindowBand(
                pRequestArguments->hWnd, 
                pRequestArguments->hWndInsertAfter, 
                pRequestArguments->dwBand
            );
            pResponseArguments->dwErrorCode = GetLastError();
            pNtUserEnableIAMAccess(nIAMKey, FALSE);

            DEBUGLOG("RequestHandler: Execute SetWindowBand success");
            break;
        }
        case IAM_CreateWindowInBand: {
            pResponse->ResponseType = IAM_CreateWindowInBand;
            const IAM_CreateWindowInBand_RequestArguments* pRequestArguments = &pRequest->Arguments.CreateWindowInBand;
            IAM_CreateWindowInBand_ResponseArguments* pResponseArguments = &pResponse->Arguments.CreateWindowInBand;

            // Temporarily enable IAM access and execution
            pNtUserEnableIAMAccess(nIAMKey, TRUE);
            SetLastError(ERROR_SUCCESS);
            pResponseArguments->hWnd = pCreateWindowInBand(
                pRequestArguments->dwExStyle,
                pRequestArguments->lpClassName,
                pRequestArguments->lpWindowName,
                pRequestArguments->dwStyle,
                pRequestArguments->X,
                pRequestArguments->Y,
                pRequestArguments->nWidth,
                pRequestArguments->nHeight,
                pRequestArguments->hWndParent,
                pRequestArguments->hMenu,
                pRequestArguments->hInstance,
                pRequestArguments->lpParam,
                pRequestArguments->dwBand
            );
            pResponseArguments->dwErrorCode = GetLastError();
            pNtUserEnableIAMAccess(nIAMKey, FALSE);

            DEBUGLOG("RequestHandler: Execute CreateWindowInBand success");
            break;
        }
        case IAM_CreateWindowInBandEx: {
            pResponse->ResponseType = IAM_CreateWindowInBandEx;
            const IAM_CreateWindowInBandEx_RequestArguments* pRequestArguments = &pRequest->Arguments.CreateWindowInBandEx;
            IAM_CreateWindowInBandEx_ResponseArguments* pResponseArguments = &pResponse->Arguments.CreateWindowInBandEx;

            // Temporarily enable IAM access and execution
            pNtUserEnableIAMAccess(nIAMKey, TRUE);
            SetLastError(ERROR_SUCCESS);
            pResponseArguments->hWnd = pCreateWindowInBandEx(
                pRequestArguments->dwExStyle,
                pRequestArguments->lpClassName,
                pRequestArguments->lpWindowName,
                pRequestArguments->dwStyle,
                pRequestArguments->X,
                pRequestArguments->Y,
                pRequestArguments->nWidth,
                pRequestArguments->nHeight,
                pRequestArguments->hWndParent,
                pRequestArguments->hMenu,
                pRequestArguments->hInstance,
                pRequestArguments->lpParam,
                pRequestArguments->dwBand,
                pRequestArguments->Flag
            );
            pResponseArguments->dwErrorCode = GetLastError();
            pNtUserEnableIAMAccess(nIAMKey, FALSE);

            DEBUGLOG("RequestHandler: Execute CreateWindowInBandEx success");
            break;
        }
        default: {
            DEBUGERR("RequestHandler: Unknown request type");
            break;
        }
    }

    return TRUE;
}

// Message receiving window message processing function
LRESULT CALLBACK IAMCollectWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        // WM_COPYDATA message
        case WM_COPYDATA: {
            // Get COPYDATA struct
            PCOPYDATASTRUCT pRequestDataInfo = (PCOPYDATASTRUCT)lParam;

            // Operation request
            if (pRequestDataInfo->dwData == WM_IAM_OPERATION_REQUEST) {
                COPYDATASTRUCT ResponseDataInfo{ 0 };
                IAMCollectorResponse ResponseData;

                DEBUGLOG("Enter Message: WM_IAM_OPERATION_REQUEST");

                // Exit if handler is not start
                if (!bHandlerStarted) {
                    DEBUGERR("Collector: Handler has not start");
                    break;
                }

                // Get request pointer
                const IAMCollectorRequest* pRequestData = reinterpret_cast<const IAMCollectorRequest*>(pRequestDataInfo->lpData);

                // Process request
                BOOL bRes = IAMHandleRequest(pRequestData, &ResponseData);
                if (!bRes) {
                    DEBUGERR("Collector: Can not process request");
                    break;
                }

                // build COPYDATA struct for response
                ResponseData.bCollected = TRUE;
                ResponseDataInfo.dwData = WM_IAM_OPERATION_RESPONSE;
                ResponseDataInfo.cbData = sizeof(ResponseData);
                ResponseDataInfo.lpData = &ResponseData;
                DEBUGLOG("Collector: Process response success");

                // Send response data
                HWND hHandlerWindow = FindWindow(WND_HANDLER_CLASSNAME, NULL);
                if (!hHandlerWindow) {
                    DEBUGERR("Collector: Can not find handler window");
                    return 1;
                }
                SendMessage(hHandlerWindow, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&ResponseDataInfo);

                DEBUGLOG("Exit Message: WM_IAM_OPERATION_REQUEST");
            }
            if (pRequestDataInfo->dwData == WM_COLLECTORHELLO) {
                // First handshake message
                DEBUGLOG("Communication: --> Handler hello");
                HWND hHandlerWindow = FindWindow(WND_HANDLER_CLASSNAME, NULL);
                if (!hHandlerWindow) {
                    DEBUGERR("Collector: Can not find handler window");
                    return 1;
                }
                DEBUGLOG("Communication: <-- Collector hello");
                COPYDATASTRUCT TempMessage{ 0 };
                TempMessage.dwData = WM_HANDLERHELLO;
                TempMessage.lpData = NULL;
                TempMessage.cbData = 0;
                LRESULT bRes = SendMessage(hHandlerWindow, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&TempMessage);
                if (bRes != ERROR_SUCCESS) DEBUGERR("Communication: SendMessage error");
            }
            if (pRequestDataInfo->dwData == WM_COLLECTORSTART) {
                // Third handshake message
                DEBUGLOG("Communication: --> Collector start");
                bHandlerStarted = TRUE;
            }
            break;
        }
        // Close message
        case WM_CLOSE: {
            DestroyWindow(hWnd);
            break;
        }
        // Destroy message
        case WM_DESTROY: {
            PostQuitMessage(0);
            break;
        }
        default: {
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    return 0;
}

// Thread callback function for monitoring SetWindowBand requests
// A hidden window will be created within the function to receive messages.
void CALLBACK IAMCollectorWindowLoopThread(LPVOID lpParam) {
    DEBUGLOG("Enter: IAMCollectorWindowLoopThread");

    // Register Window Class
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = IAMCollectWndProc;
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
        DEBUGLOG("Failure Exit: IAMCollectorWindowLoopThread");
        return;
    }

    // Window initialization, but not display
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
        DEBUGLOG("Failure Exit: IAMCollectorWindowLoopThread");
        return;
    }

    // Reduce permissions to accept WM_COPYDATA
    if (IsUserAnAdmin()) {
        BOOL bRes = ChangeWindowMessageFilterEx(hCollectWnd, WM_COPYDATA, MSGFLT_ADD, NULL);
        if (!bRes) {
            DEBUGERR("Window Creater: ChangeWindowMessageFilterEx error");
            UnregisterClass(WND_COLLECTOR_CLASSNAME, hParentInstance);
            bCheckThreadStopped = TRUE;
            DEBUGLOG("Failure Exit: IAMCollectorWindowLoopThread");
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

    // Cleanup
    UnregisterClass(WND_COLLECTOR_CLASSNAME, hParentInstance);
    bCheckThreadStopped = TRUE;
    DEBUGLOG("Successful Exit: IAMCollectorWindowLoopThread");
    return;
}

// A function modeled after NtUserEnableIAMAccess, used for hook callbacks and obtaining IAM keys
BOOL IAMAPI IAMNtUserEnableIAMAccess_Hook(ULONG64 key, BOOL enable) {
    DEBUGLOG("Enter: IAMNtUserEnableIAMAccess_Hook");

    // Get the return value of the real function
    const BOOL bRes = pNtUserEnableIAMAccess(key, enable);

    // Check if it is successful and if the IAM key has not been set. If it matches, obtain IAM key and uninstall hook
    if (bRes == TRUE && bIAMKeySaved == FALSE) {
        // Save IAM Key
        nIAMKey = key;

        // The caching function is incomplete ...
        //BOOL bBlockRes = IAMKeyWrite(nIAMKey);
        //if (!bBlockRes) {
        //    DEBUGERR("Hook: Can not save IAM key into memory.");

        //    // No impact on usage, continue execution
        //    // return bRes;
        //}

        DEBUGLOG("Hook: Key saved successfully. Key=" + std::to_string(nIAMKey));

        // Set flag as saved
        bIAMKeySaved = TRUE;
        
        // Start the thread and start monitoring SetWindowBand requests in the registry
        hCheckThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)IAMCollectorWindowLoopThread, NULL, NULL, 0);

        // Uninstall the hook to stop monitoring
        if (bHookCreated == TRUE) {
            if (!Mhook_Unhook((PVOID*)&pNtUserEnableIAMAccess)) return FALSE;
            bHookCreated = FALSE; // Update state
        }

        // Error check
        if (hCheckThread == NULL) {
            DEBUGERR("Hook: Can not start thread IAMCollectorWindowLoopThread");
            DEBUGLOG("Failure Exit: IAMNtUserEnableIAMAccess_Hook");
            return bRes;
        }
    }
    
    // Return the value that should be returned
    DEBUGLOG("Successful Exit: IAMNtUserEnableIAMAccess_Hook");
    return bRes;
}

// Adjust process privilage
DWORD IAMAdjustPrivilege(LPCTSTR lpPrivilegeName, BOOL bEnable) {
    DEBUGLOG("Enter: IAMAdjustPrivilege");

    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES NewState{ 0 };
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
        DEBUGLOG("Successful Exit: IAMAdjustPrivilege");
    }
    else {
        DEBUGERR("Failture Exit: IAMAdjustPrivilege");
    }
    return dwErr;
}

// A function used to lure Explorer.EXE call NtUserEnableIAMAccess
BOOL IAMInduceExplorer() {
    DEBUGLOG("Enter: IAMInduceExplorer");

    // Use API functions and FindWindow to obtain handles for the active window and taskbar, and determine null pointers
    HWND hWndForeground = GetForegroundWindow();
    HWND hWndTaskBar = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    HWND hWndDesktop = GetDesktopWindow();
    if (hWndForeground == NULL || hWndTaskBar == NULL || hWndDesktop == NULL) {
        DEBUGERR("Inducer: Window not found. [Foreground/Taskbar/Desktop]");
        DEBUGLOG("Failure Exit: IAMInduceExplorer");
        return FALSE;
    }

    DEBUGLOG("Inducer: Start induce.");
    UINT nTryCount = 0;
    // Option 1: Call SetForegroundWindow
    for (nTryCount = 0; nTryCount < 3 && !bIAMKeySaved; nTryCount++) { // Try up to 3 times to avoid affecting users
        BOOL bSetForegroundSuccess = TRUE;
        DEBUGLOG("Inducer: Perform option 1");

        // Using the console window to bypass the restrictions of SetForegroundWindow
        if (nTryCount > 0) {
            AllocConsole();
            auto hWndConsole = GetConsoleWindow();
            SetWindowPos(hWndConsole, 0, 0, 0, 0, 0, SWP_NOACTIVATE);
            FreeConsole();
        }

        Sleep(INDUCE_FOREGROUND_DELAY); // Give the system some reaction time, the same applies below

        // Set focus to desktop window, ready to lure
        bSetForegroundSuccess &= SetForegroundWindow(hWndDesktop);
        if (!bSetForegroundSuccess) DEBUGERR("Inducer: SetForegroundWindow failed. [Desktop]");
        Sleep(INDUCE_FOREGROUND_DELAY);

        // Set focus to taskbar, trigger recovery event
        bSetForegroundSuccess &= SetForegroundWindow(hWndTaskBar);
        if (!bSetForegroundSuccess) DEBUGERR("Inducer: SetForegroundWindow failed. [Taskbar]");
        Sleep(INDUCE_FOREGROUND_DELAY);

        // Restore the active window to avoid affecting users
        bSetForegroundSuccess &= SetForegroundWindow(hWndForeground);
        if (!bSetForegroundSuccess) DEBUGERR("Inducer: SetForegroundWindow failed. [Foreground]");
        Sleep(INDUCE_FOREGROUND_DELAY);
    }
    // Option 2: Toggle start menu (Ctrl + Esc)
    for (nTryCount = 0; nTryCount < 2 && !bIAMKeySaved; nTryCount++) { // Try up to 2 times to avoid affecting users
        INPUT Input = { 0 };
        DEBUGLOG("Inducer: Perform option 2");

        Input.type = INPUT_KEYBOARD;
        Input.ki.wVk = VK_LCONTROL;
        SendInput(1, &Input, sizeof(INPUT)); // Ctrl down

        Sleep(INDUCE_STARTMENU_DELAY);

        for (UINT nEscCount = 0; nEscCount < 2; nEscCount++) {
            ZeroMemory(&Input, sizeof(INPUT));
            Input.type = INPUT_KEYBOARD;
            Input.ki.wVk = VK_ESCAPE;
            SendInput(1, &Input, sizeof(INPUT)); // Esc down

            Sleep(INDUCE_STARTMENU_DELAY);

            // Release the second key
            ZeroMemory(&Input, sizeof(INPUT));
            Input.type = INPUT_KEYBOARD;
            Input.ki.wVk = VK_ESCAPE;
            Input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &Input, sizeof(INPUT)); // Esc up

            Sleep(INDUCE_STARTMENU_DELAY);
        }

        ZeroMemory(&Input, sizeof(INPUT));
        Input.type = INPUT_KEYBOARD;
        Input.ki.wVk = VK_LCONTROL;
        Input.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &Input, sizeof(INPUT)); // Ctrl up

        Sleep(INDUCE_STARTMENU_DELAY * 2); // Waiting for explorer's response
    }
    // Option 3: Toggle start menu (Win)
    for (nTryCount = 0; nTryCount < 2 && !bIAMKeySaved; nTryCount++) { // Try up to 2 times to avoid affecting users
        INPUT Input = { 0 };
        DEBUGLOG("Inducer: Perform option 3");

        for (UINT nWinCount = 0; nWinCount < 2; nWinCount++) {
            ZeroMemory(&Input, sizeof(INPUT));
            Input.type = INPUT_KEYBOARD;
            Input.ki.wVk = VK_LWIN;
            SendInput(1, &Input, sizeof(INPUT)); // LWin down

            Sleep(INDUCE_STARTMENU_DELAY);

            ZeroMemory(&Input, sizeof(INPUT));
            Input.type = INPUT_KEYBOARD;
            Input.ki.wVk = VK_LWIN;
            Input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &Input, sizeof(INPUT)); // LWin up
        }

        Sleep(INDUCE_STARTMENU_DELAY * 2); // Waiting for explorer's response
    }
    if (!bIAMKeySaved) {
        DEBUGLOG("Failure Exit: IAMInduceExplorer");
        return FALSE;
    }

    DEBUGLOG("Successful Exit: IAMInduceExplorer");
    return TRUE;
}

// The main function that runs during loading
void CALLBACK IAMAttachMain(ULONG_PTR lpParam) {
    DEBUGLOG("Enter: IAMAttachMain");

    // Prevent duplicate loading of hooks
    if (bHookCreated == TRUE) {
        return;
    }

    // Adjust process privilage
    if (IAMAdjustPrivilege(SE_DEBUG_NAME, TRUE) != ERROR_SUCCESS) {
        DEBUGERR("AttachMain: Can not adjust process privilage");
        return;
    }
    
    // Temporarily load dynamic libraries and obtain addresses of undisclosed API functions
    BOOL bIsUser32Loaded = FALSE;
    HMODULE hUser32 = GetModuleHandle(TEXT("User32.dll"));
    if (hUser32 == NULL) {
        hUser32 = LoadLibrary(TEXT("User32.dll"));
        if (hUser32 == NULL) {
            DEBUGERR("AttachMain: Can not get handle of User32.dll");
            return;
        }
        bIsUser32Loaded = TRUE;
    }
    pGetWindowBand = (T_GetWindowBand)GetProcAddress(hUser32, "GetWindowBand"); 
    pCreateWindowInBand = (T_CreateWindowInBand)GetProcAddress(hUser32, "CreateWindowInBand");
    pCreateWindowInBandEx = (T_CreateWindowInBandEx)GetProcAddress(hUser32, "CreateWindowInBandEx");
    pSetWindowBand = (T_SetWindowBand)GetProcAddress(hUser32, "SetWindowBand");
    pNtUserEnableIAMAccess = (T_NtUserEnableIAMAccess)GetProcAddress(hUser32, MAKEINTRESOURCEA(2510));
    if (
        pGetWindowBand == NULL || 
        pCreateWindowInBand == NULL || 
        pCreateWindowInBandEx == NULL || 
        pSetWindowBand == NULL || 
        pNtUserEnableIAMAccess == NULL) {
        DEBUGERR("AttachMain: Can not get all function addresses");
        return;
    }
    if (bIsUser32Loaded) {
        BOOL bRes = FreeLibrary(hUser32);
        if (bRes != ERROR_SUCCESS) {
            DEBUGERR("AttachMain: Can not free User32.dll");
            return;
        }
    }

    // Create API hooks using MHook and monitor NtUserEnableIAMAccess
    if (!Mhook_SetHook((PVOID*)&pNtUserEnableIAMAccess, IAMNtUserEnableIAMAccess_Hook)) {
        DEBUGERR("AttachMain: Can not set API hook");
        return;
    }
    bHookCreated = TRUE; // Update State

    // Lure Explorer.EXE to call NtUserEnableIAMAccess, thereby triggering the hook
    // Due to system limitations, threads in DllMain cannot run asynchronously, so they run synchronously directly.
    if (!IAMInduceExplorer()) {
        DEBUGERR("AttachMain: Can not induce Explorer.EXE");
        return;
    }

    return;
}

// The main function that runs during uninstallation
void CALLBACK IAMDetachMain(ULONG_PTR lpParam) {
    DEBUGLOG("Enter: IAMDetachMain");

    // Destroy the window and let the thread check end
    SendMessage(hCollectWnd, WM_CLOSE, 0, 0);
    if (hCheckThread) {
        while (!bCheckThreadStopped) Sleep(50); // Wait with global variables
    }

    // If the hook is successfully created, use MHook to uninstall the API hook
    if (bHookCreated == TRUE) {
        if (!Mhook_Unhook((PVOID*)&pNtUserEnableIAMAccess)) {
            DEBUGERR("DetachMain: Unhook hook error.");
            return;
        }
        bHookCreated = FALSE; // Update state
    }

    return;
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

        // Call the attach main function by using APC
        if (!QueueUserAPC(IAMAttachMain, GetCurrentThread(), NULL)) {
            DEBUGLOG("IAMKeyHacker: Can not queue APC while attacting");
            return FALSE;
        }

        // Put the process into an alertable state
        SleepEx(50, TRUE);

        break;
    }
    case DLL_PROCESS_DETACH: {
        // Call the detach main function by using APC
        if (!QueueUserAPC(IAMDetachMain, GetCurrentThread(), NULL)) {
            DEBUGLOG("IAMKeyHacker: Can not queue APC while detacting");
            return FALSE;
        }

        // Put the process into an alertable state
        SleepEx(50, TRUE);

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