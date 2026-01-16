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

#define __WTM_IS_COMPILING

#include "../Common Headers/Framework.h"
#include "../Common Headers/WindowTopMost.h"
#include <TlHelp32.h>
#include <Windows.h>
#include <VersionHelpers.h>
#include <ShlObj.h>
#include "uiaccess.h"
#include "Convert.h"

// DLL injection object name
#define COLLECTORIMAGE TEXT("IAMKeyHacker.dll")

#if defined(__cplusplus)
extern "C" {
#endif

// Handler Window Information
struct tag_IAMHandlerWindow {
    HWND hWnd;
    BOOL bIsReady;
    IAMCollectorResponse Response;
    BOOL bThreadStopped;
    HANDLE hWindowLoopThread;

    tag_IAMHandlerWindow() : hWnd(NULL), bIsReady(FALSE), Response(), bThreadStopped(FALSE), hWindowLoopThread(NULL) {}
    void Reset() {
        hWnd = NULL;
        bIsReady = FALSE;
        Response = IAMCollectorResponse();
        bThreadStopped = FALSE;
        hWindowLoopThread = NULL;
    }
} IAMHandlerWindow;

// Auxiliary function used to check if the COLLECTORIMAGE backend program has been injected
BOOL WTMAPI WTMCheckForDll() {
    MODULEENTRY32 MOD{ 0 };
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
        if (_tcscmp(MOD.szModule, COLLECTORIMAGE) == 0) {
            bFound = TRUE;
            break;
        }
    } while (Module32Next(hSnapshot, &MOD));
    CloseHandle(hSnapshot);
    return bFound;
}

// Unload COLLECTORIMAGE
BOOL WTMUnloadCollector() {
    // If already unloaded, do not unload again
    if (!WTMCheckForDll()) return TRUE;

    MODULEENTRY32 MOD{ 0 };
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
        if (_tcscmp(MOD.szModule, COLLECTORIMAGE) == 0) break;
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

// Message handler window message processing function
LRESULT CALLBACK WTMHandlerWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COPYDATA: {
        PCOPYDATASTRUCT pResponseDataInfo = (PCOPYDATASTRUCT)lParam;
        if (pResponseDataInfo->dwData == WM_IAM_OPERATION_RESPONSE) {
            // Get response data
            DEBUGLOG("Enter Message: WM_IAM_OPERATION_RESPONSE");

            IAMCollectorResponse* pRequestData = reinterpret_cast<IAMCollectorResponse*>(pResponseDataInfo->lpData);
            memcpy(&IAMHandlerWindow.Response, pRequestData, sizeof(IAMCollectorResponse));
            IAMHandlerWindow.Response.bCollected = TRUE;

            DEBUGLOG("Exit Message: WM_IAM_OPERATION_RESPONSE");
        }
        if (pResponseDataInfo->dwData == WM_HANDLERHELLO) {
            IAMHandlerWindow.bIsReady = TRUE;
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

// A hidden window will be created within the function to receive messages.
DWORD CALLBACK WTMHandlerWindowThread(LPVOID lpParam) {
    DEBUGLOG("Enter: WTMHandlerWindowThread");

    // Register Window Class
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WTMHandlerWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = WND_HANDLER_CLASSNAME;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    if (!RegisterClassEx(&wcex)) {
        DEBUGERR("Window Creater: Window class register error");
        IAMHandlerWindow.bThreadStopped = TRUE;
        DEBUGLOG("Failure Exit: WTMHandlerWindowThread");
        return 0;
    }

    // Window initialization, but not display
    IAMHandlerWindow.hWnd = CreateWindowEx(
        NULL,
        WND_HANDLER_CLASSNAME,
        TEXT(""),
        WS_OVERLAPPEDWINDOW,
        0, 0, 100, 100,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );
    if (!IAMHandlerWindow.hWnd) {
        DEBUGERR("Window Creater: Window create error");
        UnregisterClass(WND_HANDLER_CLASSNAME, hInstance);
        IAMHandlerWindow.bThreadStopped = TRUE;
        DEBUGLOG("Failure Exit: WTMHandlerWindowThread");
        return 0;
    }

    // Reduce permissions to accept WM_COPYDATA
    if (IsUserAnAdmin()) {
        BOOL bRes = ChangeWindowMessageFilterEx(IAMHandlerWindow.hWnd, WM_COPYDATA, MSGFLT_ADD, NULL); // Reduce permissions to accept WM_COPYDATA
        if (!bRes) {
            DEBUGERR("Window Creater: ChangeWindowMessageFilterEx error");
            UnregisterClass(WND_HANDLER_CLASSNAME, hInstance);
            IAMHandlerWindow.bThreadStopped = TRUE;
            DEBUGLOG("Failure Exit: WTMHandlerWindowThread");
            return 0;
        }
    }

    UpdateWindow(IAMHandlerWindow.hWnd);

    // Main message loop
    DEBUGLOG("Window Creater: Window created successfully. Enter main loop. ");
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    UnregisterClass(WND_HANDLER_CLASSNAME, hInstance);
    IAMHandlerWindow.bThreadStopped = TRUE;
    DEBUGLOG("Successful Exit: CheckSetWindowBandCallThread");
    return 0;
}

// Check if the operating system meets the requirements
BOOL WTMAPI WTMCheckEnvironment() {
    DEBUGLOG("Enter: WTMCheckEnvironment");

    // Check Windows OS Version
    if (!IsWindows8OrGreater()) {
        DEBUGLOG("Check: Windows version error.");
        DEBUGLOG("Failure Exit: WTMCheckEnvironment");
        return FALSE;
    }
    DEBUGLOG("Check: Windows version OK. (VER >= 8)");

    // Check the dll

    // Get the current directory
    TCHAR Buffer[MAX_PATH];
    GetModuleFileName(NULL, Buffer, MAX_PATH);
    std::_tstring AppPath(Buffer);
    size_t DrivePoint = AppPath.find_first_of(':');
    if (DrivePoint == std::_tstring::npos) return FALSE;
    size_t PathPoint = AppPath.find_last_of('\\');
    if (PathPoint == std::_tstring::npos) return FALSE;
    AppPath = AppPath.substr(0, DrivePoint + 1) + AppPath.substr(DrivePoint + 1, PathPoint - DrivePoint);

    if (GetFileAttributes((AppPath + COLLECTORIMAGE).c_str()) == INVALID_FILE_ATTRIBUTES) {
        DEBUGLOG("Check: DLL not exist or it contains error.");
        DEBUGLOG("Failure Exit: WTMCheckEnvironment");
        return FALSE;
    }
    DEBUGLOG("Check: DLL existing.");

    // Find the taskbar window
    HWND hWndTaskBar = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (hWndTaskBar == NULL) {
        DEBUGLOG("Check: Taskbar not found.");
        DEBUGLOG("Failure Exit: WTMCheckEnvironment");
        return FALSE;
    }
    DEBUGLOG("Check: Taskbar found.");

    DEBUGLOG("Successful Exit: WTMCheckEnvironment");
    return TRUE;
}

// WindowTopMost initialization function. You must call it in your program.
BOOL WTMAPI WTMInit() {
    DEBUGLOG("Enter: WTMInit");

    

    // Inject COLLECTORIMAGE to Explorer EXE to start the backend program
    
    // If already injected, uninstall and inject again
    if (WTMCheckForDll()) {
        DEBUGLOG("Injector: Collector image is already injected. Inject again.");
        if (!WTMUnloadCollector()) {
            DEBUGERR("Injector: Can not unload collector");
            DEBUGLOG("Failure Exit: WTMInit");
            return FALSE;
        }
    }

    // Check the environment
    if (!WTMCheckEnvironment()) {
        DEBUGLOG("Failure Exit: WTMInit");
        return FALSE;
    }

    // Get the complete path of the DLL
    TCHAR ModulePathBuffer[MAX_PATH];
    GetModuleFileName(NULL, ModulePathBuffer, MAX_PATH);
    std::_tstring ModuleFullPath(ModulePathBuffer);
    size_t lastBackslashPos = ModuleFullPath.find_last_of(TEXT("\\"));
    std::_tstring dllPath = ModuleFullPath.substr(0, lastBackslashPos + 1) + COLLECTORIMAGE;
    DEBUGLOG("Injector: DllPath=" + LPT2LPC(dllPath));

    // Find an Explorer responsible for the desktop EXE's PID, get ready to inject into the process
    DWORD processId;
    HWND hWndTaskBar;
    hWndTaskBar = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (hWndTaskBar == NULL) {
        DEBUGERR("Injector: Taskbar not found");
        DEBUGLOG("Failure Exit: WTMInit");
        return FALSE;
    }
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
    if (!WTMCheckForDll()) return FALSE;

    // Create handler window and wait (use mutex)
    IAMHandlerWindow.hWindowLoopThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WTMHandlerWindowThread, NULL, NULL, 0);
    if (!IAMHandlerWindow.hWindowLoopThread) {
        DEBUGERR("Communication: Can not create handler window");
        DEBUGLOG("Failure Exit: WTMInit");
        return FALSE;
    }
    DEBUGLOG("Communication: Wait for window create");
    while (!IAMHandlerWindow.hWnd) {
        Sleep(50);
    }
    DEBUGLOG("Communication: Window created");

    // Shaking hands with collector window
    UINT nTime = 0;
    BOOL bIsReady = FALSE;
    HWND hCollectorWnd = NULL;
    for (nTime = 0, bIsReady = FALSE; !bIsReady && nTime <= COLLECTOR_TIMEOUT; nTime += 50) {
        hCollectorWnd = FindWindow(WND_COLLECTOR_CLASSNAME, NULL);
        if (hCollectorWnd) bIsReady = TRUE;
        Sleep(50);
    }
    if (!bIsReady) {
        DEBUGERR("Communication: Can not find collector window, timeout");
        DEBUGLOG("Failure Exit: WTMInit");
        return FALSE;
    }
    DEBUGLOG("Communication: Collector window found");

    // 1. Handler hello
    DEBUGLOG("Communication: --> Handler hello");
    COPYDATASTRUCT TempMessage{ 0 };
    TempMessage.dwData = WM_COLLECTORHELLO;
    TempMessage.lpData = NULL;
    TempMessage.cbData = 0;
    LRESULT bRes = SendMessage(hCollectorWnd, WM_COPYDATA, (WPARAM)IAMHandlerWindow.hWnd, (LPARAM)&TempMessage);
    if (bRes != ERROR_SUCCESS) {
        DEBUGERR("Communication: Can not send WM_COLLECTORHELLO message");
        DEBUGLOG("Failure Exit: WTMInit");
        return FALSE;
    }

    // 2. Collector hello (wait for flag bCollectorResponsed)
    DEBUGLOG("Communication: <-- Collector hello waiting");
    for (nTime = 0, bIsReady = FALSE; !bIsReady && nTime <= COLLECTOR_TIMEOUT; nTime += 50) {
        if (IAMHandlerWindow.bIsReady) bIsReady = TRUE;
        Sleep(50);
    }
    if (!bIsReady) {
        DEBUGERR("Communication: Collector hello timeout");
        DEBUGLOG("Failure Exit: WTMInit");
        return FALSE;
    }
    DEBUGLOG("Communication: Collector hello received");

    // 3. Collector Start
    DEBUGLOG("Communication: --> Collector start");
    TempMessage.dwData = WM_COLLECTORSTART;
    TempMessage.lpData = NULL;
    TempMessage.cbData = 0;
    bRes = SendMessage(hCollectorWnd, WM_COPYDATA, (WPARAM)IAMHandlerWindow.hWnd, (LPARAM)&TempMessage);
    if (bRes != ERROR_SUCCESS) {
        DEBUGERR("Communication: Can not send WM_COLLECTORSTART message");
        DEBUGLOG("Failure Exit: WTMInit");
        return FALSE;
    }

    DEBUGLOG("Successful Exit: WTMInit");

    return TRUE;
}

// WindowTopMost uninstallation function. You must call it in your program. 
BOOL WTMAPI WTMUninit() {
    // From Explorer Uninstall IAMKeyHacker in EXE DLL to close the background program

    // If already uninstalled, do not uninstall again
    if (!WTMCheckForDll()) return TRUE;

    // Unload COLLECTORIMAGE
    if (!WTMUnloadCollector()) return FALSE;

    // Close handler window and wait
    SendMessage(IAMHandlerWindow.hWnd, WM_CLOSE, 0, 0);
    while (!IAMHandlerWindow.bThreadStopped) {
        Sleep(50);
    }
    IAMHandlerWindow.Reset();

    return TRUE;
}

// Obtain UIAccess permission by restarting the application
// There is no need to manually call this function.
BOOL WTMAPI WTMEnableUIAccess(BOOL bEnable) {
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
HWND WTMAPI WTMCreateUIAccessWindowW(
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

    // Temporarily load dynamic libraries and obtain addresses of undisclosed API functions
    BOOL bIsUser32Loaded = FALSE;
    HMODULE hUser32 = GetModuleHandle(TEXT("User32.dll"));
    if (hUser32 == NULL) {
        hUser32 = LoadLibrary(TEXT("User32.dll"));
        if (hUser32 == NULL) {
            DEBUGERR("CreateUIAccessWindow: Can not get handle of User32.dll");
            DEBUGLOG("Failure Exit: WTMCreateUIAccessWindowW");
            return FALSE;
        }
        bIsUser32Loaded = TRUE;
    }
    T_GetWindowBand pGetWindowBand = (T_GetWindowBand)GetProcAddress(hUser32, "GetWindowBand");
    T_CreateWindowInBand pCreateWindowInBand = (T_CreateWindowInBand)GetProcAddress(hUser32, "CreateWindowInBand");
    if (pGetWindowBand == NULL || pCreateWindowInBand == NULL) {
        DEBUGERR("CreateUIAccessWindow: Can not get all function addresses");
        DEBUGLOG("Failure Exit: WTMCreateUIAccessWindowW");
        return FALSE;
    }
    if (bIsUser32Loaded) {
        BOOL bRes = FreeLibrary(hUser32);
        if (bRes != ERROR_SUCCESS) {
            DEBUGERR("CreateUIAccessWindow: Can not free User32.dll");
            DEBUGLOG("Failure Exit: WTMCreateUIAccessWindowW");
            return FALSE;
        }
    }

	// Prepare UIAccess
	DWORD dwErr = PrepareForUIAccess();
    if (ERROR_SUCCESS != dwErr) {
        DEBUGERR("CreateUIAccessWindow: Can not get UIAccess");
        DEBUGLOG("Failure Exit: WTMCreateUIAccessWindowW");
        return NULL;
    }

	HWND Window = pCreateWindowInBand(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam, ZBID_UIACCESS);
    if (Window == NULL) {
        DEBUGERR("CreateUIAccessWindow: CreateWindowInBand failed");
        DEBUGLOG("Failure Exit: WTMCreateUIAccessWindowW");
        return NULL;
    }

	// Verify Z sequence segment
	DWORD bandCheck;
	if (pGetWindowBand(Window, &bandCheck)) {
        if (bandCheck != ZBID_UIACCESS) {
            DestroyWindow(Window);
            DEBUGERR("CreateUIAccessWindow: Window created but Z-Order band is incorrect");
            DEBUGLOG("Failure Exit: WTMCreateUIAccessWindowW");
            return NULL;
        }
	}
    else {
        DEBUGERR("CreateUIAccessWindow: GetWindowBand failed");
        DEBUGLOG("Failure Exit: WTMCreateUIAccessWindowW");
        return NULL;
    }
	return Window;
}
HWND WTMAPI WTMCreateUIAccessWindowA(
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
    hWnd = WTMCreateUIAccessWindowW(dwExStyle, LPC2LPW(lpClassName).c_str(), LPC2LPW(lpWindowName).c_str(), dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    return hWnd;
}

// SetWindowBand that does not deny access.
BOOL WTMAPI WTMSetWindowBand(
	_In_opt_ HWND hWnd,
	_In_opt_ HWND hWndInsertAfter,
	_In_opt_ DWORD dwBand
)  {
    if (!WTMCheckForDll()) {
        SetLastError(ERROR_NOT_READY);
        return FALSE;
    }

    UINT nTime = 0;
    BOOL bIsReady = FALSE;
    
    // Wait for the backend to be ready (check if the flag file has been generated)
    for (nTime = 0, bIsReady = FALSE; !bIsReady && nTime <= COLLECTOR_TIMEOUT; nTime += 50) {
        if (IAMHandlerWindow.bIsReady) bIsReady = TRUE;
        Sleep(50);
    }
    if (!bIsReady) {
        DEBUGERR("Handler: Collector ready timeout");
        DEBUGLOG("Failure Exit: WTMSetWindowBand");
        SetLastError(ERROR_TIMEOUT);
        return FALSE;
    }
    
    // Get collection window handle
    HWND hCollectorWnd = FindWindow(WND_COLLECTOR_CLASSNAME, NULL);
    if (hCollectorWnd == NULL) {
        SetLastError(ERROR_NOT_READY);
        return FALSE;
    }

    // Send window information
    COPYDATASTRUCT RequestDataInfo{ 0 };
    IAMCollectorRequest RequestData;
    IAM_SetWindowBand_RequestArguments* pRequestArguments = &RequestData.Arguments.SetWindowBand;
    IAM_SetWindowBand_ResponseArguments* pResponseArguments = &IAMHandlerWindow.Response.Arguments.SetWindowBand;
    RequestData.RequestType = IAM_SetWindowBand;
    pRequestArguments->hWnd = hWnd;
    pRequestArguments->hWndInsertAfter = hWndInsertAfter;
    pRequestArguments->dwBand = dwBand;
    RequestDataInfo.dwData = WM_IAM_OPERATION_REQUEST;
    RequestDataInfo.cbData = sizeof(RequestData);
    RequestDataInfo.lpData = &RequestData;
    SendMessage(hCollectorWnd, WM_COPYDATA, NULL, (LPARAM)&RequestDataInfo);

    // Collect response
    for (nTime = 0, bIsReady = FALSE; !bIsReady && nTime <= COLLECTOR_TIMEOUT; nTime += 50) {
        if (IAMHandlerWindow.Response.bCollected) bIsReady = TRUE;
        Sleep(50);
    }
    if (!bIsReady) {
        DEBUGERR("Handler: Response timeout");
        DEBUGLOG("Failure Exit: WTMSetWindowBand");
        SetLastError(ERROR_TIMEOUT);
        return FALSE;
    }

    // Set error and status
    SetLastError(pResponseArguments->dwErrorCode);
    BOOL bRes = pResponseArguments->bStatus;
    IAMHandlerWindow.Response.bCollected = FALSE;
    return bRes;
}

// CreateWindowInBand will not deny access and will return NULL if an error occurs.
HWND WTMAPI WTMCreateWindowInBandW(
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
    if (!WTMCheckForDll()) return NULL;
    HWND hWnd = NULL;
    hWnd = CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    if (hWnd == NULL) return NULL;
    if (!WTMSetWindowBand(hWnd, NULL, dwBand)) {
        DestroyWindow(hWnd);
        return NULL;
    }
    return hWnd;
}
HWND WTMAPI WTMCreateWindowInBandA(
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
    if (!WTMCheckForDll()) return NULL;
    HWND hWnd = NULL;
    hWnd = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    if (hWnd == NULL) return NULL;
    WTMSetWindowBand(hWnd, NULL, dwBand);
    return hWnd;
}

// GetWindowBand call entrance, and if an error occurs, returns False.
BOOL WTMAPI WTMGetWindowBand(
    _In_ HWND hWnd,
    _In_ LPDWORD pdwBand
) {
    if (pdwBand == NULL) return FALSE;
    // Temporarily load dynamic libraries and obtain addresses of undisclosed API functions
    BOOL bIsUser32Loaded = FALSE;
    HMODULE hUser32 = GetModuleHandle(TEXT("User32.dll"));
    if (hUser32 == NULL) {
        hUser32 = LoadLibrary(TEXT("User32.dll"));
        if (hUser32 == NULL) {
            DEBUGERR("WTMInit: Can not get handle of User32.dll");
            DEBUGLOG("Failure Exit: WTMInit");
            return FALSE;
        }
        bIsUser32Loaded = TRUE;
    }
    T_GetWindowBand pGetWindowBand = (T_GetWindowBand)GetProcAddress(hUser32, "GetWindowBand");
    if (pGetWindowBand == NULL) {
        DEBUGERR("WTMInit: Can not get all function addresses");
        DEBUGLOG("Failure Exit: WTMInit");
        return FALSE;
    }
    if (bIsUser32Loaded) {
        BOOL bRes = FreeLibrary(hUser32);
        if (bRes != ERROR_SUCCESS) {
            DEBUGERR("WTMInit: Can not free User32.dll");
            DEBUGLOG("Failure Exit: WTMInit");
            return FALSE;
        }
    }
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

        break;

    case DLL_PROCESS_DETACH:

        // Unload COLLECTORIMAGE
        if (!WTMUnloadCollector()) DEBUGERR("Unload COLLECTORIMAGE error");

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