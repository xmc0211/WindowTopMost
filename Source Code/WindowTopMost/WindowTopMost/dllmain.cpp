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
#include <ShlObj.h>
#include "uiaccess.h"
#include "TaskDialogEx.h"
#include "Convert.h"
#include "ProcessCtrl.h"
#include "FileSys.h"
#include "FileBinIO.h"

#ifdef _DEBUG
#include <fstream>
#endif

// File name, window class, and corresponding message for transmitting information
#define WND_COLLECTOR_CLASSNAME       TEXT("IAMCollector_7DD48162_2BC7_4073_9CC8_51CC3DB0FF72")
#define WND_HANDLER_CLASSNAME         TEXT("IAMHandler_D06B7428_73FA_4967_B30C_96030ACCF998")
#define WM_PERFORM          (WM_USER + 1)
#define WM_HANDLERHELLO     (WM_USER + 2)
#define WM_COLLECTORHELLO   (WM_USER + 3)
#define WM_COLLECTORSTART   (WM_USER + 4)
#define WM_RESULT           (WM_USER + 5)
#define COLLECTOR_TIMEOUT   (500)

// DLL injection object name
#define COLLECTORIMAGE TEXT("IAMKeyHacker.dll")

// Debug
#ifdef _DEBUG
#define DEBUGLOG(Text) do { std::ofstream Log("D:\\DebugLog_WTM.txt", std::ios::app); Log << Text << std::endl; } while (0)
#define DEBUGERR(Text) DEBUGLOG(Text + (", " + std::to_string((ULONG)GetLastError())))
#else
#define DEBUGLOG(Text) do {  } while (0)
#define DEBUGERR(Text) DEBUGLOG(Text)
#endif

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

namespace IAMHandlerWindow {
    HWND hWnd = NULL;
    BOOL bIsCollectorReady = FALSE;
    IAMCollectorResponse Response = { 0 };
    BOOL bThreadStopped = FALSE;
    HANDLE hWindowLoopThread = NULL;
    void Reset() {
        hWnd = NULL;
        bIsCollectorReady = FALSE;
        Response = { 0 };
        bThreadStopped = FALSE;
        hWindowLoopThread = NULL;
    }
}

BOOL WTMAPI CheckForDll();

// Use WM_COPYDATA to transmit message above WM_USER avoid permission issues (Hide)
BOOL SendMessageEx(HWND hWnd, HWND hSourceWnd, UINT message) {
    COPYDATASTRUCT TempMessage;
    TempMessage.dwData = message;
    TempMessage.lpData = NULL;
    TempMessage.cbData = 0;
    LRESULT bRes = SendMessage(hWnd, WM_COPYDATA, (WPARAM)hSourceWnd, (LPARAM)&TempMessage);
    if (bRes != ERROR_SUCCESS) DEBUGERR("Communication: SendMessage error");
    return bRes == ERROR_SUCCESS;
}

// Unload COLLECTORIMAGE
BOOL UnloadCollector() {
    // If already unloaded, do not unload again
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

// Message handler window message processing function
LRESULT CALLBACK HandlerWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COPYDATA: {
        PCOPYDATASTRUCT pResponseDataInfo = (PCOPYDATASTRUCT)lParam;
        if (pResponseDataInfo->dwData == WM_RESULT) {
            // Get response data
            DEBUGLOG("Enter Message: WM_RESULT");

            if (pResponseDataInfo->cbData != sizeof(IAMCollectorResponse)) {
                DEBUGLOG("Handler: Response size is incorrect");
                return 1;
            }
            IAMCollectorResponse* pRequestData = reinterpret_cast<IAMCollectorResponse*>(pResponseDataInfo->lpData);
            DEBUGLOG("Handler: bStatus=" + std::to_string((ULONG64)pRequestData->bStatus));
            DEBUGLOG("Handler: dwErrorCode=" + std::to_string((ULONG64)pRequestData->dwErrorCode));
            IAMHandlerWindow::Response = *pRequestData;
            IAMHandlerWindow::Response.bCollected = TRUE;

            DEBUGLOG("Exit Message: WM_RESULT");
        }
        if (pResponseDataInfo->dwData == WM_COLLECTORHELLO) {
            IAMHandlerWindow::bIsCollectorReady = TRUE;
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
DWORD WTMAPI CALLBACK IAMHandlerWindowThread(LPVOID lpParam) {
    DEBUGLOG("Enter: IAMHandlerWindowThread");

    // Register Window Class
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = HandlerWndProc;
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
        IAMHandlerWindow::bThreadStopped = TRUE;
        DEBUGLOG("Failure Exit: IAMHandlerWindowThread");
        return 0;
    }

    // Window initialization, but not displayed
    IAMHandlerWindow::hWnd = CreateWindowEx(
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
    if (!IAMHandlerWindow::hWnd) {
        DEBUGERR("Window Creater: Window create error");
        UnregisterClass(WND_HANDLER_CLASSNAME, hInstance);
        IAMHandlerWindow::bThreadStopped = TRUE;
        DEBUGLOG("Failure Exit: IAMHandlerWindowThread");
        return 0;
    }
    if (IsUserAnAdmin()) {
        BOOL bRes = ChangeWindowMessageFilterEx(IAMHandlerWindow::hWnd, WM_COPYDATA, MSGFLT_ADD, NULL); // Reduce permissions to accept WM_COPYDATA
        if (!bRes) {
            DEBUGERR("Window Creater: ChangeWindowMessageFilterEx error");
            UnregisterClass(WND_HANDLER_CLASSNAME, hInstance);
            IAMHandlerWindow::bThreadStopped = TRUE;
            DEBUGLOG("Failure Exit: IAMHandlerWindowThread");
            return 0;
        }
    }
    UpdateWindow(IAMHandlerWindow::hWnd);

    // Main message loop
    DEBUGLOG("Window Creater: Window created successfully. enter main loop. ");
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnregisterClass(WND_HANDLER_CLASSNAME, hInstance);
    IAMHandlerWindow::bThreadStopped = TRUE;
    DEBUGLOG("Successful Exit: CheckSetWindowBandCallThread");
    return 0;
}

// Auxiliary function used to check if the COLLECTORIMAGE backend program has been injected
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
        if (_tcscmp(MOD.szModule, COLLECTORIMAGE) == 0) {
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
        DEBUGLOG("Check: Windows version error.");
        DEBUGLOG("Failure Exit: WTMCheckEnvironment");
        return FALSE;
    }
    DEBUGLOG("Check: Windows version OK. (VER >= 8)");

    // Check the dll
    std::_tstring AppPath = FSFormat(TEXT("DP"), FSGetCurrentFilePath());
    if (!FSObjectExist(AppPath + COLLECTORIMAGE)) {
        DEBUGLOG("Check: DLL not exist.");
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
    std::_tstring dllPath = ModuleFullPath.substr(0, lastBackslashPos + 1) + COLLECTORIMAGE;
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

    // Create handler window and wait (use mutex)
    IAMHandlerWindow::hWindowLoopThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)IAMHandlerWindowThread, NULL, NULL, 0);
    if (!IAMHandlerWindow::hWindowLoopThread) {
        DEBUGERR("Communication: Can not create handler window");
        DEBUGLOG("Failure Exit: WTMInit");
        return FALSE;
    }
    DEBUGLOG("Communication: Wait for window create");
    while (!IAMHandlerWindow::hWnd) {
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
    SendMessageEx(hCollectorWnd, IAMHandlerWindow::hWnd, WM_HANDLERHELLO);

    // 2. Collector hello (wait for flag bCollectorResponsed)
    DEBUGLOG("Communication: <-- Collector hello waiting");
    for (nTime = 0, bIsReady = FALSE; !bIsReady && nTime <= COLLECTOR_TIMEOUT; nTime += 50) {
        if (IAMHandlerWindow::bIsCollectorReady) bIsReady = TRUE;
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
    SendMessageEx(hCollectorWnd, IAMHandlerWindow::hWnd, WM_COLLECTORSTART);

    DEBUGLOG("Successful Exit: WTMInit");

    return TRUE;
}

// WindowTopMost uninstallation function. You must call it in your program. 
BOOL WTMAPI WTMUninit() {
    // From Explorer Uninstall IAMKeyHacker in EXE DLL to close the background program

    // If already uninstalled, do not uninstall again
    if (!CheckForDll()) return TRUE;

    // Unload COLLECTORIMAGE
    if (!UnloadCollector()) return FALSE;

    // Close handler window and wait
    SendMessage(IAMHandlerWindow::hWnd, WM_CLOSE, 0, 0);
    while (!IAMHandlerWindow::bThreadStopped) {
        Sleep(50);
    }
    IAMHandlerWindow::Reset();

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

// SetWindowBand that does not deny access.
BOOL WTMAPI SetWindowBandEx(
	_In_opt_ HWND hWnd,
	_In_opt_ HWND hWndInsertAfter,
	_In_opt_ DWORD dwBand
)  {
    if (!CheckForDll()) return FALSE;

    // Wait for the backend to be ready (check if the flag file has been generated)
    while (!IAMHandlerWindow::bIsCollectorReady) {
        Sleep(50);
    }

    // Get collection window handle
    HWND hCollectorWnd = FindWindow(WND_COLLECTOR_CLASSNAME, NULL);
    if (hCollectorWnd == NULL) return FALSE;

    // Send window information
    COPYDATASTRUCT RequestDataInfo;
    IAMCollectorRequest RequestData;
    RequestData.hWnd = hWnd;
    RequestData.hWndInsertAfter = hWndInsertAfter;
    RequestData.dwBand = dwBand;
    RequestDataInfo.dwData = WM_PERFORM;
    RequestDataInfo.cbData = sizeof(RequestData);
    RequestDataInfo.lpData = &RequestData;
    DEBUGLOG("Handler: hWnd=" + std::to_string((ULONG64)RequestData.hWnd));
    DEBUGLOG("Handler: hWndInsertAfter=" + std::to_string((ULONG64)RequestData.hWndInsertAfter));
    DEBUGLOG("Handler: dwBand=" + std::to_string((ULONG64)RequestData.dwBand));
    SendMessage(hCollectorWnd, WM_COPYDATA, NULL, (LPARAM)&RequestDataInfo);

    // Collect response
    UINT nTime = 0;
    BOOL bIsReady = FALSE;
    for (nTime = 0, bIsReady = FALSE; !bIsReady && nTime <= COLLECTOR_TIMEOUT; nTime += 50) {
        if (IAMHandlerWindow::Response.bCollected) bIsReady = TRUE;
        Sleep(50);
    }
    if (!bIsReady) {
        DEBUGERR("Handler: Reaponse timeout");
        DEBUGLOG("Failure Exit: SetWindowBandEx");
        return FALSE;
    }

    // Set error
    SetLastError(IAMHandlerWindow::Response.dwErrorCode);
    BOOL bRes = IAMHandlerWindow::Response.bStatus;
    IAMHandlerWindow::Response.bCollected = FALSE;
    return bRes;
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
    if (!SetWindowBandEx(hWnd, NULL, dwBand)) {
        DestroyWindow(hWnd);
        return NULL;
    }
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

        break;

    case DLL_PROCESS_DETACH:

        // Unload COLLECTORIMAGE
        if (!UnloadCollector()) DEBUGERR("Unload COLLECTORIMAGE error");

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