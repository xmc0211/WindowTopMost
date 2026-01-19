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
#include "../Common Headers/ThreadStopHandler.h"
#include "../Common Headers/WindowTopMost.h"
#include <TlHelp32.h>
#include <Windows.h>
#include <VersionHelpers.h>
#include <ShlObj.h>
#include "uiaccess.h"
#include "Convert.h"

// DLL injection object name
#define IAMWORKER_IMAGENAME TEXT("IAMKeyHacker.dll")

#if defined(__cplusplus)
extern "C" {
#endif

// IAMController hWindow Information
struct tag_IAMControllerWindow {
    HWND hWnd;
    BOOL bIsReady;
    IAMWorkerResponse Response;
    ThreadStopHandler ThreadStop;
    HANDLE hWindowCreateEvent;

    tag_IAMControllerWindow() : hWnd(NULL), bIsReady(FALSE), Response(), hWindowCreateEvent(NULL) {}
    void Reset() {
        hWnd = NULL;
        bIsReady = FALSE;
        hWindowCreateEvent = NULL;
        Response = IAMWorkerResponse();
    }
} IAMControllerWindow;

// Auxiliary function used to check if the IAMWorker backend program has been injected
_Success_(return == TRUE)
BOOL WTMAPI WTMCheckForDll() {
    MODULEENTRY32 Mod{ 0 };
    BOOL bFound = FALSE;
    DWORD nProcessID = 0;

    // Retrieve Explorer.EXE's PID through the taskbar window
    HWND hTaskBarWnd = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (hTaskBarWnd == NULL) return FALSE;
    DWORD nRes = GetWindowThreadProcessId(hTaskBarWnd, &nProcessID);
    if (nRes == NULL) {
        SetLastError(nRes);
        return FALSE;
    }

    // Retrieve all loaded DLLs and compare them one by one
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, nProcessID);
    if (hSnapshot == INVALID_HANDLE_VALUE) return FALSE;

    Mod.dwSize = sizeof(MODULEENTRY32);
    if (!Module32First(hSnapshot, &Mod)) {
        CloseHandle(hSnapshot);
        return FALSE;
    }
    do {
        if (_tcscmp(Mod.szModule, IAMWORKER_IMAGENAME) == 0) {
            bFound = TRUE;
            break;
        }
    } while (Module32Next(hSnapshot, &Mod));
    CloseHandle(hSnapshot);
    return bFound;
}

// Unload IAMWorker
// If the program crashes and leaves IAMWorker in Explorer.EXE, you can call this function directly to clean it up.
_Success_(return == TRUE)
BOOL WTMAPI WTMUnloadWorker() {
    // If already unloaded, do not unload again
    if (!WTMCheckForDll()) {
        SetLastError(ERROR_SUCCESS);
        return TRUE;
    }

    MODULEENTRY32 Mod{ 0 };
    BOOL bFound = FALSE;
    DWORD nProcessID = 0;

    // Retrieve Explorer.EXE's PID through the taskbar window
    HWND hTaskBarWnd = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (hTaskBarWnd == NULL) {
        SetLastError(ERROR_WTM_WORKER_CONTROL);
        return FALSE;
    }
    DWORD nRes = GetWindowThreadProcessId(hTaskBarWnd, &nProcessID);
    if (nRes == NULL) {
        SetLastError(nRes);
        return FALSE;
    }

    Mod.dwSize = sizeof(MODULEENTRY32);
    // Retrieve all loaded DLLs and compare them one by one
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, nProcessID);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        SetLastError(ERROR_WTM_WORKER_CONTROL);
        return FALSE;
    }

    if (!Module32First(hSnapshot, &Mod)) {
        CloseHandle(hSnapshot);
        SetLastError(ERROR_WTM_WORKER_CONTROL);
        return FALSE;
    }
    do {
        if (_tcscmp(Mod.szModule, IAMWORKER_IMAGENAME) == 0) break;
    } while (Module32Next(hSnapshot, &Mod));

    // Get the handle of the target process
    HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, nProcessID);
    if (!hProcess) {
        CloseHandle(hSnapshot);
        SetLastError(ERROR_WTM_WORKER_CONTROL);
        return FALSE;
    }

    // Get FreeLibrary address
    HMODULE hMod = GetModuleHandle(TEXT("Kernel32.DLL"));
    if (hMod == NULL) {
        CloseHandle(hProcess);
        CloseHandle(hSnapshot);
        SetLastError(ERROR_WTM_WORKER_CONTROL);
        return FALSE;
    }
    LPTHREAD_START_ROUTINE pThread = (LPTHREAD_START_ROUTINE)GetProcAddress(hMod, "FreeLibrary");
    if (pThread == NULL) {
        CloseHandle(hProcess);
        CloseHandle(hSnapshot);
        SetLastError(ERROR_WTM_WORKER_CONTROL);
        return FALSE;
    }

    // Call FreeLibrary to uninstall DLL
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pThread, Mod.modBaseAddr, 0, NULL);
    if (!hThread) {
        CloseHandle(hProcess);
        CloseHandle(hSnapshot);
        SetLastError(ERROR_WTM_WORKER_CONTROL);
        return FALSE;
    }

    // Waiting for thread to end
    WaitForSingleObject(hThread, INFINITE);

    // Clear
    CloseHandle(hThread);
    CloseHandle(hProcess);
    CloseHandle(hSnapshot);
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

// Add a string to the extended data
_Success_(return == TRUE)
BOOL WTMAddStringToExtendedData(
    _Inout_     ULONG_PTR pBaseAddress,     // Original memory starting address
    _In_        SIZE_T nValidByteSize,      // The number of valid bytes in raw memory
    _In_        SIZE_T nByteOffset,         // The byte offset you want to add string
    _In_opt_    LPCWSTR pString,            // The string you want to add
    _In_        SIZE_T nCharCount           // The length of the string to be added, including the terminator
) {
    if (pBaseAddress == NULL || pString == nullptr) return FALSE;
    if (nCharCount == 0 || nValidByteSize == 0) return FALSE;

    // Check termination symbol
    if (pString[nCharCount - 1] != L'\0') return FALSE;

    // Verify access interval
    SIZE_T nRequiredByteSize = nCharCount * sizeof(WCHAR);
    if (nByteOffset > nValidByteSize || nByteOffset + nRequiredByteSize > nValidByteSize) return FALSE;

    // Convert the original memory address to a writable pointer
    PWCHAR pAppendStart = reinterpret_cast<PWCHAR>(pBaseAddress + nByteOffset);

    // Write append string to original memory
    for (SIZE_T i = 0; i < nCharCount; ++i) {
        pAppendStart[i] = pString[i];
    }

    return TRUE;
}

// Message IAMController window message processing function
_Success_(return == ERROR_SUCCESS)
LRESULT CALLBACK WTMControllerWndProc(
    _In_        HWND hWnd, 
    _In_opt_    UINT message, 
    _In_opt_    WPARAM wParam, 
    _In_opt_    LPARAM lParam
) {
    switch (message) {
    case WM_COPYDATA: {
        PCOPYDATASTRUCT pResponseDataInfo = (PCOPYDATASTRUCT)lParam;
        if (pResponseDataInfo->dwData == WM_IAM_OPERATION_RESPONSE) {
            // Get response data
            DEBUGLOG("WTMControllerWndProc: Enter Message: WM_IAM_OPERATION_RESPONSE");

            IAMWorkerResponse* pRequestData = reinterpret_cast<IAMWorkerResponse*>(pResponseDataInfo->lpData);
            memcpy(&IAMControllerWindow.Response, pRequestData, sizeof(IAMWorkerResponse));
            IAMControllerWindow.Response.bCollected = TRUE;

            DEBUGLOG("WTMControllerWndProc: Exit Message: WM_IAM_OPERATION_RESPONSE");
        }
        if (pResponseDataInfo->dwData == WM_IAMCONTROLLERHELLO) {
            IAMControllerWindow.bIsReady = TRUE;
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

    return ERROR_SUCCESS;
}

// A hidden window will be created within the function to receive messages.
_Success_(return == ERROR_SUCCESS)
DWORD CALLBACK WTMControllerWindowThread(
    _In_opt_    LPVOID lpParam
) {
    IAMControllerWindow.ThreadStop.EnterThread();

    // Register hWindow Class
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WTMControllerWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = IAMCONTROLLER_WINDOW_CLASSNAME;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    if (!RegisterClassEx(&wcex)) {
        DEBUGERR("WTMControllerWindowThread: hWindow class register error");
        IAMControllerWindow.ThreadStop.LeaveThread();
        return 0;
    }

    // hWindow initialization, but not display
    IAMControllerWindow.hWnd = CreateWindowEx(
        NULL,
        IAMCONTROLLER_WINDOW_CLASSNAME,
        TEXT(""),
        WS_OVERLAPPEDWINDOW,
        0, 0, 100, 100,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );
    if (!IAMControllerWindow.hWnd) {
        DEBUGERR("WTMControllerWindowThread: hWindow create error");
        UnregisterClass(IAMCONTROLLER_WINDOW_CLASSNAME, hInstance);
        IAMControllerWindow.ThreadStop.LeaveThread();
        return 0;
    }

    // Reduce permissions to accept WM_COPYDATA
    if (IsUserAnAdmin()) {
        BOOL bRes = ChangeWindowMessageFilterEx(IAMControllerWindow.hWnd, WM_COPYDATA, MSGFLT_ADD, NULL); // Reduce permissions to accept WM_COPYDATA
        if (!bRes) {
            DEBUGERR("WTMControllerWindowThread: ChangeWindowMessageFilterEx error");
            UnregisterClass(IAMCONTROLLER_WINDOW_CLASSNAME, hInstance);
            IAMControllerWindow.ThreadStop.LeaveThread();
            return 0;
        }
    }

    SetEvent(IAMControllerWindow.hWindowCreateEvent);
    UpdateWindow(IAMControllerWindow.hWnd);

    // Main message loop
    DEBUGLOG("WTMControllerWindowThread: hWindow created successfully. Enter main loop. ");
    MSG Msg;
    while (TRUE) {
        if (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE)) {
            if (Msg.message == WM_QUIT) break;
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }

    // Cleanup
    UnregisterClass(IAMCONTROLLER_WINDOW_CLASSNAME, hInstance);
    IAMControllerWindow.ThreadStop.LeaveThread();
    return 0;
}

// Check if the operating system meets the requirements
_Success_(return == TRUE)
BOOL WTMAPI WTMCheckEnvironment() {
    // Check Windows OS Version
    if (!IsWindows8OrGreater()) {
        SetLastError(ERROR_WTM_ENVIRONMENT);
        DEBUGERR("WTMCheckEnvironment: Windows version error.");
        return FALSE;
    }

    // Check the IAMWorker DLL

    // Get the current directory
    TCHAR Buffer[MAX_PATH];
    GetModuleFileName(NULL, Buffer, MAX_PATH);
    std::_tstring AppPath(Buffer);
    size_t DrivePoint = AppPath.find_first_of(':');
    if (DrivePoint == std::_tstring::npos) return FALSE;
    size_t PathPoint = AppPath.find_last_of('\\');
    if (PathPoint == std::_tstring::npos) return FALSE;
    AppPath = AppPath.substr(0, DrivePoint + 1) + AppPath.substr(DrivePoint + 1, PathPoint - DrivePoint);

    if (GetFileAttributes((AppPath + IAMWORKER_IMAGENAME).c_str()) == INVALID_FILE_ATTRIBUTES) {
        DEBUGERR("WTMCheckEnvironment: DLL not exist or it contains error.");
        SetLastError(ERROR_WTM_ENVIRONMENT);
        return FALSE;
    }

    // Find the taskbar window
    HWND hTaskBarWnd = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (hTaskBarWnd == NULL) {
        DEBUGERR("WTMCheckEnvironment: Taskbar not found.");
        SetLastError(ERROR_WTM_ENVIRONMENT);
        return FALSE;
    }

    DEBUGLOG("WTMCheckEnvironment: Environment OK");
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

// WindowTopMost initialization function. You must call it in your program.
_Success_(return == TRUE)
BOOL WTMAPI WTMInit() {

    // Inject IAMWorker to Explorer.EXE to start the backend program
    
    // If already injected, uninstall and inject again
    if (WTMCheckForDll()) {
        DEBUGLOG("WTMInit: IAMWorker image is already injected. Inject again.");
        if (!WTMUnloadWorker()) {
            DEBUGERR("WTMInit: Can not unload IAMWorker");
            return FALSE;
        }
    }

    // Check the environment
    if (!WTMCheckEnvironment()) {
        return FALSE;
    }

    // Get the complete path of the DLL
    TCHAR ModulePathBuffer[MAX_PATH];
    GetModuleFileName(NULL, ModulePathBuffer, MAX_PATH);
    std::_tstring ModuleFullPath(ModulePathBuffer);
    size_t lastBackslashPos = ModuleFullPath.find_last_of(TEXT("\\"));
    std::_tstring dllPath = ModuleFullPath.substr(0, lastBackslashPos + 1) + IAMWORKER_IMAGENAME;

    // Find Explorer.EXE's PID, get ready to inject into the process
    DWORD nProcessID;
    HWND hTaskBarWnd = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (hTaskBarWnd == NULL) {
        DEBUGERR("WTMInit: Taskbar not found");
        SetLastError(ERROR_WTM_WORKER_CONTROL);
        return FALSE;
    }
    DWORD nRes = GetWindowThreadProcessId(hTaskBarWnd, &nProcessID);
    if (nRes == NULL) {
        SetLastError(nRes);
        return FALSE;
    }
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, nProcessID);
    if (!hProcess) {
        DEBUGERR("WTMInit: OpenProcess failed");
        SetLastError(ERROR_WTM_WORKER_CONTROL);
        return FALSE;
    }

    // Allocate memory within the process
    LPVOID pRemoteMemory = VirtualAllocEx(hProcess, NULL, (dllPath.size() + 1) * sizeof(TCHAR), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!pRemoteMemory) {
        DEBUGERR("WTMInit: VirtualAllocEx failed");
        CloseHandle(hProcess);
        SetLastError(ERROR_WTM_WORKER_CONTROL);
        return FALSE;
    }

    // Write DLL path
    BOOL success = WriteProcessMemory(hProcess, pRemoteMemory, dllPath.c_str(), (dllPath.size() + 1) * sizeof(TCHAR), NULL);
    if (!success) {
        DEBUGERR("WTMInit: WriteProcessMemory failed");
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        SetLastError(ERROR_WTM_WORKER_CONTROL);
        return FALSE;
    }

    // Create remote thread to load DLL
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibrary, pRemoteMemory, 0, NULL);
    if (!hThread) {
        DEBUGERR("WTMInit: CreateRemoteThread failed");
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        SetLastError(ERROR_WTM_WORKER_CONTROL);
        return FALSE;
    }

    // Waiting for thread to end
    WaitForSingleObject(hThread, INFINITE);

    // Clear
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    // Check again
    if (!WTMCheckForDll()) {
        return FALSE;
    }

    // Init ThreadStopHandler and event
    IAMControllerWindow.hWindowCreateEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (IAMControllerWindow.hWindowCreateEvent == NULL) return FALSE;
    if (IAMControllerWindow.ThreadStop.Init() != TSH_SUCCESS) {
        DEBUGERR("WTMInit: Can not init ThreadStopHandler");
        SetLastError(ERROR_WTM_COMMUNICATION_MESSAGING);
        return FALSE;
    }

    // Create IAMController window and wait (use mutex)
    HANDLE hWindowLoopThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WTMControllerWindowThread, NULL, NULL, 0);
    if (!hWindowLoopThread) {
        DEBUGERR("WTMInit: Can not create IAMController window");
        SetLastError(ERROR_WTM_COMMUNICATION_MESSAGING);
        return FALSE;
    }
    DEBUGLOG("WTMInit: Wait for window create");
    WaitForSingleObject(IAMControllerWindow.hWindowCreateEvent, INFINITE);
    CloseHandle(IAMControllerWindow.hWindowCreateEvent);
    DEBUGLOG("WTMInit: hWindow created");

    // Shaking hands with IAMWorker window
    UINT nTime = 0;
    BOOL bIsReady = FALSE;
    HWND hWorkerWindow = NULL;
    for (nTime = 0, bIsReady = FALSE; !bIsReady && nTime <= IAMWORKER_TIMEOUT; nTime += 50) {
        hWorkerWindow = FindWindow(IAMWORKER_WINDOW_CLASSNAME, NULL);
        if (hWorkerWindow) bIsReady = TRUE;
        Sleep(50);
    }
    if (!bIsReady) {
        SetLastError(ERROR_WTM_COMMUNICATION_MESSAGING);
        DEBUGERR("WTMInit: Can not find IAMWorker window, timeout");
        return FALSE;
    }
    DEBUGLOG("WTMInit: IAMWorker window found");

    // 1. IAMController hello
    DEBUGLOG("WTMInit: --> IAMController hello");
    COPYDATASTRUCT TempMessage{ 0 };
    TempMessage.dwData = WM_IAMWORKERHELLO;
    TempMessage.lpData = NULL;
    TempMessage.cbData = 0;
    LRESULT bRes = SendMessage(hWorkerWindow, WM_COPYDATA, (WPARAM)IAMControllerWindow.hWnd, (LPARAM)&TempMessage);
    if (bRes != ERROR_SUCCESS) {
        DEBUGERR("WTMInit: Can not send WM_IAMWORKERHELLO message");
        SetLastError(bRes);
        return FALSE;
    }

    // 2. IAMWorker hello (wait for flag bCollectorResponsed)
    DEBUGLOG("WTMInit: <-- IAMWorker hello waiting");
    for (nTime = 0, bIsReady = FALSE; !bIsReady && nTime <= IAMWORKER_TIMEOUT; nTime += 50) {
        if (IAMControllerWindow.bIsReady) bIsReady = TRUE;
        Sleep(50);
    }
    if (!bIsReady) {
        SetLastError(ERROR_WTM_COMMUNICATION_MESSAGING);
        DEBUGERR("WTMInit: IAMWorker hello timeout");
        return FALSE;
    }
    DEBUGLOG("WTMInit: IAMWorker hello received");

    // 3. IAMWorker Start
    DEBUGLOG("WTMInit: --> IAMWorker start");
    TempMessage.dwData = WM_IAMWORKERSTART;
    TempMessage.lpData = NULL;
    TempMessage.cbData = 0;
    bRes = SendMessage(hWorkerWindow, WM_COPYDATA, (WPARAM)IAMControllerWindow.hWnd, (LPARAM)&TempMessage);
    if (bRes != ERROR_SUCCESS) {
        DEBUGERR("WTMInit: Can not send WM_IAMWORKERSTART message");
        SetLastError(bRes);
        return FALSE;
    }

    DEBUGLOG("WTMInit: Success");

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

// WindowTopMost uninstallation function. You must call it in your program. 
_Success_(return == TRUE)
BOOL WTMAPI WTMUninit() {
    // Uninstall IAMWorker From Explorer.EXE to close the background program

    // If already uninstalled, do not uninstall again
    if (!WTMCheckForDll()) {
        return TRUE;
    }

    // Unload IAMWorker
    if (!WTMUnloadWorker()) {
        return FALSE;
    }

    // Close IAMController window
    LRESULT nRes = SendMessage(IAMControllerWindow.hWnd, WM_CLOSE, 0, 0);
    if (nRes != ERROR_SUCCESS) {
        DEBUGERR("WTMUninit: Can not send close message to IAMController window.");
        SetLastError(nRes);
        return FALSE;
    }

    // Uninit controller window
    if (IAMControllerWindow.ThreadStop.StopAllThread() != TSH_SUCCESS) {
        DEBUGERR("WTMUninit: Can not stop threads.");
        SetLastError(ERROR_WTM_COMMUNICATION_MESSAGING);
        return FALSE;
    }
    IAMControllerWindow.ThreadStop.Uninit();
    IAMControllerWindow.Reset();

    DEBUGLOG("WTMUninit: Success");

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

// Obtain UIAccess permission by restarting the application
// There is no need to manually call this function.
_Success_(return == TRUE)
BOOL WTMAPI WTMEnableUIAccess(
    _In_opt_    BOOL bEnable
) {
    DWORD dwErr = 0;
    if (bEnable == TRUE) {
        // Call third-party functions to gain privileges (Thanks for Them)
        dwErr = PrepareForUIAccess();
        SetLastError(dwErr);
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
            ExitProcess(ERROR_SUCCESS);
            exit(0);

            // I just hope the program won't run here ...
            SetLastError(ERROR_INTERNAL_ERROR);
            return FALSE;
        }
        return FALSE;
    }
}

// Create the top-level window using UIAccess and verify if the Z segment is correct. If an error occurs, return NULL.
// The application will restart.
_Success_(return != NULL)
HWND WTMAPI WTMCreateUIAccessWindowW(
    _In_        DWORD dwExStyle,
    _In_        LPCWSTR lpClassName,
    _In_        LPCWSTR lpWindowName,
    _In_        DWORD dwStyle,
    _In_        int X,
    _In_        int Y,
    _In_        int nWidth,
    _In_        int nHeight,
    _In_opt_    HWND hWndParent,
    _In_opt_    HMENU hMenu,
    _In_        HINSTANCE hInstance,
    _In_opt_    LPVOID lpParam
) {
    if (lpClassName == NULL || lpWindowName == NULL) return NULL;

    // Temporarily load dynamic libraries and obtain addresses of undisclosed API functions
    HMODULE hUser32 = GetModuleHandle(TEXT("User32.DLL"));
    if (hUser32 == NULL) return NULL;
    T_GetWindowBand pGetWindowBand = (T_GetWindowBand)GetProcAddress(hUser32, "GetWindowBand");
    T_CreateWindowInBand pCreateWindowInBand = (T_CreateWindowInBand)GetProcAddress(hUser32, "CreateWindowInBand");
    if (pGetWindowBand == NULL || pCreateWindowInBand == NULL) {
        DEBUGERR("WTMCreateUIAccessWindowW: Can not get all function addresses");
        SetLastError(ERROR_INTERNAL_ERROR);
        return NULL;
    }

	// Prepare UIAccess
	DWORD dwErr = PrepareForUIAccess();
    if (ERROR_SUCCESS != dwErr) {
        SetLastError(dwErr);
        DEBUGERR("WTMCreateUIAccessWindowW: Can not get UIAccess");
        return NULL;
    }

	HWND hWindow = pCreateWindowInBand(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam, ZBID_UIACCESS);
    if (hWindow == NULL) {
        DEBUGERR("WTMCreateUIAccessWindowW: CreateWindowInBand failed");
        return NULL;
    }

	// Verify Z sequence segment
	DWORD bandCheck;
    BOOL bRes = pGetWindowBand(hWindow, &bandCheck);
    if (bRes) {
        if (bandCheck != ZBID_UIACCESS) {
            DestroyWindow(hWindow);
            SetLastError(ERROR_INTERNAL_ERROR);
            DEBUGERR("WTMCreateUIAccessWindowW: Window created but Z-Order band is incorrect");
            return NULL;
        }
    }
    else return NULL;

    SetLastError(ERROR_SUCCESS);
	return hWindow;
}
_Success_(return != NULL)
HWND WTMAPI WTMCreateUIAccessWindowA(
    _In_        DWORD dwExStyle,
    _In_        LPCSTR lpClassName,
    _In_        LPCSTR lpWindowName,
    _In_        DWORD dwStyle,
    _In_        int X,
    _In_        int Y,
    _In_        int nWidth,
    _In_        int nHeight,
    _In_opt_    HWND hWndParent,
    _In_opt_    HMENU hMenu,
    _In_        HINSTANCE hInstance,
    _In_opt_    LPVOID lpParam
) {
    // Convert to LPCWTR and call again
    HWND hWnd = NULL;
    if (lpClassName == NULL || lpWindowName == NULL) return NULL;
    hWnd = WTMCreateUIAccessWindowW(
        dwExStyle, 
        LPC2LPW(lpClassName).c_str(), 
        LPC2LPW(lpWindowName).c_str(), 
        dwStyle, 
        X, 
        Y, 
        nWidth, 
        nHeight, 
        hWndParent, 
        hMenu, 
        hInstance, 
        lpParam
    );
    return hWnd;
}

// SetWindowBand that does not deny access.
// It will return the actual result of SetWindowBand and set error code.
_Success_(return == TRUE)
BOOL WTMAPI WTMSetWindowBand(
	_In_opt_    HWND hWnd,
	_In_opt_    HWND hWndInsertAfter,
	_In_opt_    DWORD dwBand
)  {
    if (!WTMCheckForDll()) {
        SetLastError(ERROR_WTM_WORKER_HAS_NOT_LOADED);
        return FALSE;
    }

    UINT nTime = 0;
    BOOL bIsReady = FALSE;
    
    // Wait for the backend to be ready (check if the flag file has been generated)
    for (nTime = 0, bIsReady = FALSE; !bIsReady && nTime <= IAMWORKER_TIMEOUT; nTime += 50) {
        if (IAMControllerWindow.bIsReady) bIsReady = TRUE;
        Sleep(50);
    }
    if (!bIsReady) {
        SetLastError(ERROR_WTM_COMMUNICATION_MESSAGING);
        DEBUGERR("WTMSetWindowBand: IAMWorker ready timeout");
        return FALSE;
    }
    
    // Get collection window handle
    HWND hCollectorWnd = FindWindow(IAMWORKER_WINDOW_CLASSNAME, NULL);
    if (hCollectorWnd == NULL) {
        SetLastError(ERROR_WTM_COMMUNICATION_MESSAGING);
        return FALSE;
    }

    // Send window information
    COPYDATASTRUCT RequestDataInfo{ 0 };
    IAMWorkerRequest RequestData;
    IAM_SetWindowBand_RequestArguments* pRequestArguments = &RequestData.Arguments.SetWindowBand;
    IAM_SetWindowBand_ResponseArguments* pResponseArguments = &IAMControllerWindow.Response.Arguments.SetWindowBand;
    RequestData.RequestType = IAM_SetWindowBand;
    pRequestArguments->hWnd = hWnd;
    pRequestArguments->hWndInsertAfter = hWndInsertAfter;
    pRequestArguments->dwBand = dwBand;
    RequestDataInfo.dwData = WM_IAM_OPERATION_REQUEST;
    RequestDataInfo.cbData = sizeof(IAMWorkerRequest);
    RequestDataInfo.lpData = &RequestData;
    LRESULT nRes = SendMessage(hCollectorWnd, WM_COPYDATA, NULL, (LPARAM)&RequestDataInfo);
    if (nRes != ERROR_SUCCESS) {
        SetLastError(nRes);
        DEBUGERR("WTMSetWindowBand: Can not send message to IAMWorker window");
        return FALSE;
    }

    // Collect response
    for (nTime = 0, bIsReady = FALSE; !bIsReady && nTime <= IAMWORKER_TIMEOUT; nTime += 50) {
        if (IAMControllerWindow.Response.bCollected) bIsReady = TRUE;
        Sleep(50);
    }
    if (!bIsReady) {
        SetLastError(ERROR_WTM_COMMUNICATION_MESSAGING);
        DEBUGERR("WTMSetWindowBand: Response timeout");
        return FALSE;
    }

    // Set error and status
    SetLastError(pResponseArguments->dwErrorCode);
    BOOL bRes = pResponseArguments->bStatus;
    IAMControllerWindow.Response.bCollected = FALSE;
    return bRes;
}

// CreateWindowInBand that does not deny access.
// It will return NULL if an error occurs.
_Success_(return != NULL)
HWND WTMAPI WTMCreateWindowInBandW(
    _In_        DWORD dwExStyle,
    _In_        LPCWSTR lpClassName,
    _In_        LPCWSTR lpWindowName,
    _In_        DWORD dwStyle,
    _In_        int X,
    _In_        int Y,
    _In_        int nWidth,
    _In_        int nHeight,
    _In_opt_    HWND hWndParent,
    _In_opt_    HMENU hMenu,
    _In_        HINSTANCE hInstance,
    _In_opt_    LPVOID lpParam,
    _In_        ZBID dwBand
) {
    if (lpClassName == NULL || lpWindowName == NULL) return NULL;

    // Prevent long strings and handle string length errors in advance
    SIZE_T nClassNameCharCount = wcslen(lpClassName) + 1; // including the terminator
    SIZE_T nWindowNameCharCount = wcslen(lpWindowName) + 1; // including the terminator
    SIZE_T nClassNameByteSize = nClassNameCharCount * sizeof(WCHAR);
    SIZE_T nWindowNameByteSize = nWindowNameCharCount * sizeof(WCHAR);

    // Verify DLL status
    if (!WTMCheckForDll()) {
        SetLastError(ERROR_WTM_WORKER_HAS_NOT_LOADED);
        return FALSE;
    }

    // Check string lengths
    if (nClassNameCharCount > MAX_WINDOW_CLASS_NAME ||
        nWindowNameCharCount > MAX_WINDOW_NAME
    ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    UINT nTime = 0;
    BOOL bIsReady = FALSE;

    // Wait for the backend to be ready (check if the flag file has been generated)
    for (nTime = 0, bIsReady = FALSE; !bIsReady && nTime <= IAMWORKER_TIMEOUT; nTime += 50) {
        if (IAMControllerWindow.bIsReady) bIsReady = TRUE;
        Sleep(50);
    }
    if (!bIsReady) {
        SetLastError(ERROR_WTM_COMMUNICATION_MESSAGING);
        DEBUGERR("WTMCreateWindowInBandW: IAMWorker ready timeout");
        return FALSE;
    }

    // Get collection window handle
    HWND hCollectorWnd = FindWindow(IAMWORKER_WINDOW_CLASSNAME, NULL);
    if (hCollectorWnd == NULL) {
        DEBUGERR("WTMCreateWindowInBandW: Can not connect to IAMWorker");
        SetLastError(ERROR_WTM_COMMUNICATION_MESSAGING);
        return FALSE;
    }

    // Set the structure and various pointers
    COPYDATASTRUCT RequestDataInfo{ 0 };
    SIZE_T nRequestDataSize = sizeof(IAMWorkerRequest) + nClassNameByteSize + nWindowNameByteSize;
    IAMWorkerRequest* pRequestData = reinterpret_cast<IAMWorkerRequest*>(malloc(nRequestDataSize)); // For extended strings
    if (pRequestData == nullptr) return FALSE;
    ZeroMemory(pRequestData, nRequestDataSize);

    IAM_CreateWindowInBand_RequestArguments* pRequestArguments = &pRequestData->Arguments.CreateWindowInBand;
    IAM_CreateWindowInBand_ResponseArguments* pResponseArguments = &IAMControllerWindow.Response.Arguments.CreateWindowInBand;
    pRequestData->RequestType = IAM_CreateWindowInBand;
    pRequestData->nExtendedDataSize = nClassNameByteSize + nWindowNameByteSize;

    // Send window information
    pRequestArguments->dwExStyle = dwExStyle;
    pRequestArguments->dwStyle = dwStyle;
    pRequestArguments->X = X;
    pRequestArguments->Y = Y;
    pRequestArguments->nWidth = nWidth;
    pRequestArguments->nHeight = nHeight;
    pRequestArguments->hWndParent = hWndParent;
    pRequestArguments->hMenu = hMenu;
    pRequestArguments->hInstance = hInstance;
    pRequestArguments->lpParam = lpParam;
    pRequestArguments->dwBand = dwBand;

    // Set extended strings
    pRequestArguments->lpClassName.nOffset = sizeof(IAMWorkerRequest);
    pRequestArguments->lpClassName.nSize = nClassNameByteSize;
    if (!WTMAddStringToExtendedData(
        (ULONG_PTR)pRequestData,
        nRequestDataSize,
        pRequestArguments->lpClassName.nOffset,
        lpClassName,
        nClassNameCharCount
    )) {
        free(pRequestData);
        SetLastError(ERROR_WTM_COMMUNICATION_CODING);
        DEBUGERR("WTMCreateWindowInBandW: Can not append class name string to extended strings");
        return FALSE;
    }
    pRequestArguments->lpWindowName.nOffset = sizeof(IAMWorkerRequest) + nClassNameByteSize;
    pRequestArguments->lpWindowName.nSize = nWindowNameByteSize;
    if (!WTMAddStringToExtendedData(
        (ULONG_PTR)pRequestData,
        nRequestDataSize,
        pRequestArguments->lpWindowName.nOffset,
        lpWindowName,
        nWindowNameCharCount
    )) {
        free(pRequestData);
        SetLastError(ERROR_WTM_COMMUNICATION_CODING);
        DEBUGERR("WTMCreateWindowInBandW: Can not append window name string to extended strings");
        return FALSE;
    }

    // Build message structure and send message
    RequestDataInfo.dwData = WM_IAM_OPERATION_REQUEST;
    RequestDataInfo.cbData = nRequestDataSize;
    RequestDataInfo.lpData = pRequestData;
    LRESULT nRes = SendMessage(hCollectorWnd, WM_COPYDATA, NULL, (LPARAM)&RequestDataInfo);
    if (nRes != ERROR_SUCCESS) {
        free(pRequestData);
        SetLastError(nRes);
        DEBUGERR("WTMCreateWindowInBandW: Can not send message to IAMWorker window");
        return FALSE;
    }

    free(pRequestData);

    // Collect response
    for (nTime = 0, bIsReady = FALSE; !bIsReady && nTime <= IAMWORKER_TIMEOUT; nTime += 50) {
        if (IAMControllerWindow.Response.bCollected) bIsReady = TRUE;
        Sleep(50);
    }
    if (!bIsReady) {
        SetLastError(ERROR_WTM_COMMUNICATION_MESSAGING);
        DEBUGERR("WTMCreateWindowInBandW: Response timeout");
        return FALSE;
    }

    // Set error and status
    SetLastError(pResponseArguments->dwErrorCode);
    HWND hWnd = pResponseArguments->hWnd;
    IAMControllerWindow.Response.bCollected = FALSE;
    return hWnd;
}
_Success_(return != NULL)
HWND WTMAPI WTMCreateWindowInBandA(
    _In_        DWORD dwExStyle,
    _In_        LPCSTR lpClassName,
    _In_        LPCSTR lpWindowName,
    _In_        DWORD dwStyle,
    _In_        int X,
    _In_        int Y,
    _In_        int nWidth,
    _In_        int nHeight,
    _In_opt_    HWND hWndParent,
    _In_opt_    HMENU hMenu,
    _In_        HINSTANCE hInstance,
    _In_opt_    LPVOID lpParam,
    _In_        ZBID dwBand
) {
    // Convert to LPCWTR and call again
    HWND hWnd = NULL;
    if (lpClassName == NULL || lpWindowName == NULL) return NULL;
    hWnd = WTMCreateWindowInBandW(
        dwExStyle, 
        LPC2LPW(lpClassName).c_str(), 
        LPC2LPW(lpWindowName).c_str(), 
        dwStyle, 
        X, 
        Y, 
        nWidth, 
        nHeight, 
        hWndParent, 
        hMenu, 
        hInstance, 
        lpParam,
        dwBand
    );
    return hWnd;
}

// CreateWindowInBand that does not deny access.
// It will return NULL if an error occurs.
_Success_(return != NULL)
HWND WTMAPI WTMCreateWindowInBandExW(
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
    _In_ ZBID dwBand,
    _In_opt_ DWORD dwInternalFlag
) {
    if (lpClassName == NULL || lpWindowName == NULL) return NULL;

    // Prevent long strings and handle string length errors in advance
    SIZE_T nClassNameCharCount = wcslen(lpClassName) + 1; // including the terminator
    SIZE_T nWindowNameCharCount = wcslen(lpWindowName) + 1; // including the terminator
    SIZE_T nClassNameByteSize = nClassNameCharCount * sizeof(WCHAR);
    SIZE_T nWindowNameByteSize = nWindowNameCharCount * sizeof(WCHAR);

    // Verify DLL status
    if (!WTMCheckForDll()) {
        SetLastError(ERROR_WTM_WORKER_HAS_NOT_LOADED);
        return FALSE;
    }

    // Check string lengths
    if (nClassNameCharCount > MAX_WINDOW_CLASS_NAME ||
        nWindowNameCharCount > MAX_WINDOW_NAME
        ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    UINT nTime = 0;
    BOOL bIsReady = FALSE;

    // Wait for the backend to be ready (check if the flag file has been generated)
    for (nTime = 0, bIsReady = FALSE; !bIsReady && nTime <= IAMWORKER_TIMEOUT; nTime += 50) {
        if (IAMControllerWindow.bIsReady) bIsReady = TRUE;
        Sleep(50);
    }
    if (!bIsReady) {
        SetLastError(ERROR_WTM_COMMUNICATION_MESSAGING);
        DEBUGERR("WTMCreateWindowInBandExW: IAMWorker ready timeout");
        return FALSE;
    }

    // Get collection window handle
    HWND hCollectorWnd = FindWindow(IAMWORKER_WINDOW_CLASSNAME, NULL);
    if (hCollectorWnd == NULL) {
        DEBUGERR("WTMCreateWindowInBandExW: Can not connect to IAMWorker");
        SetLastError(ERROR_WTM_COMMUNICATION_MESSAGING);
        return FALSE;
    }

    // Set the structure and various pointers
    COPYDATASTRUCT RequestDataInfo{ 0 };
    SIZE_T nRequestDataSize = sizeof(IAMWorkerRequest) + nClassNameByteSize + nWindowNameByteSize;
    IAMWorkerRequest* pRequestData = reinterpret_cast<IAMWorkerRequest*>(malloc(nRequestDataSize)); // For extended strings
    if (pRequestData == nullptr) return FALSE;
    ZeroMemory(pRequestData, nRequestDataSize);

    IAM_CreateWindowInBandEx_RequestArguments* pRequestArguments = &pRequestData->Arguments.CreateWindowInBandEx;
    IAM_CreateWindowInBandEx_ResponseArguments* pResponseArguments = &IAMControllerWindow.Response.Arguments.CreateWindowInBandEx;
    pRequestData->RequestType = IAM_CreateWindowInBandEx;
    pRequestData->nExtendedDataSize = nClassNameByteSize + nWindowNameByteSize;

    // Send window information
    pRequestArguments->dwExStyle = dwExStyle;
    pRequestArguments->dwStyle = dwStyle;
    pRequestArguments->X = X;
    pRequestArguments->Y = Y;
    pRequestArguments->nWidth = nWidth;
    pRequestArguments->nHeight = nHeight;
    pRequestArguments->hWndParent = hWndParent;
    pRequestArguments->hMenu = hMenu;
    pRequestArguments->hInstance = hInstance;
    pRequestArguments->lpParam = lpParam;
    pRequestArguments->dwBand = dwBand;
    pRequestArguments->dwInternalFlag = dwInternalFlag;

    // Set extended strings
    pRequestArguments->lpClassName.nOffset = sizeof(IAMWorkerRequest);
    pRequestArguments->lpClassName.nSize = nClassNameByteSize;
    if (!WTMAddStringToExtendedData(
        (ULONG_PTR)pRequestData,
        nRequestDataSize,
        pRequestArguments->lpClassName.nOffset,
        lpClassName,
        nClassNameCharCount
    )) {
        free(pRequestData);
        SetLastError(ERROR_WTM_COMMUNICATION_CODING);
        DEBUGERR("WTMCreateWindowInBandExW: Can not append class name string to extended strings");
        return FALSE;
    }
    pRequestArguments->lpWindowName.nOffset = sizeof(IAMWorkerRequest) + nClassNameByteSize;
    pRequestArguments->lpWindowName.nSize = nWindowNameByteSize;
    if (!WTMAddStringToExtendedData(
        (ULONG_PTR)pRequestData,
        nRequestDataSize,
        pRequestArguments->lpWindowName.nOffset,
        lpWindowName,
        nWindowNameCharCount
    )) {
        free(pRequestData);
        SetLastError(ERROR_WTM_COMMUNICATION_CODING);
        DEBUGERR("WTMCreateWindowInBandExW: Can not append window name string to extended strings");
        return FALSE;
    }

    // Build message structure and send message
    RequestDataInfo.dwData = WM_IAM_OPERATION_REQUEST;
    RequestDataInfo.cbData = nRequestDataSize;
    RequestDataInfo.lpData = pRequestData;
    LRESULT nRes = SendMessage(hCollectorWnd, WM_COPYDATA, NULL, (LPARAM)&RequestDataInfo);
    if (nRes != ERROR_SUCCESS) {
        free(pRequestData);
        SetLastError(nRes);
        DEBUGERR("WTMCreateWindowInBandExW: Can not send message to IAMWorker window");
        return FALSE;
    }

    free(pRequestData);

    // Collect response
    for (nTime = 0, bIsReady = FALSE; !bIsReady && nTime <= IAMWORKER_TIMEOUT; nTime += 50) {
        if (IAMControllerWindow.Response.bCollected) bIsReady = TRUE;
        Sleep(50);
    }
    if (!bIsReady) {
        SetLastError(ERROR_WTM_COMMUNICATION_MESSAGING);
        DEBUGERR("WTMCreateWindowInBandExW: Response timeout");
        return FALSE;
    }

    // Set error and status
    SetLastError(pResponseArguments->dwErrorCode);
    HWND hWnd = pResponseArguments->hWnd;
    IAMControllerWindow.Response.bCollected = FALSE;
    return hWnd;
}
_Success_(return != NULL)
HWND WTMAPI WTMCreateWindowInBandExA(
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
    _In_ ZBID dwBand,
    _In_opt_ DWORD dwInternalFlag
) {
    // Convert to LPCWTR and call again
    HWND hWnd = NULL;
    if (lpClassName == NULL || lpWindowName == NULL) return NULL;
    hWnd = WTMCreateWindowInBandExW(
        dwExStyle,
        LPC2LPW(lpClassName).c_str(),
        LPC2LPW(lpWindowName).c_str(),
        dwExStyle,
        X,
        Y,
        nWidth,
        nHeight,
        hWndParent,
        hMenu,
        hInstance,
        lpParam,
        dwBand,
        dwInternalFlag
    );
    return hWnd;
}

// GetWindowBand call entrance, and if an error occurs, returns False.
_Success_(return == TRUE)
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
        if (hUser32 == NULL) return FALSE;
        bIsUser32Loaded = TRUE;
    }
    T_GetWindowBand pGetWindowBand = (T_GetWindowBand)GetProcAddress(hUser32, "GetWindowBand");
    if (pGetWindowBand == NULL) {
        DEBUGERR("WTMGetWindowBand: Can not get all function addresses");
        return FALSE;
    }
    if (pGetWindowBand == NULL) return FALSE;
    BOOL bRes = pGetWindowBand(hWnd, pdwBand);
    DWORD nTempError = GetLastError();
    if (bIsUser32Loaded) {
        if (!FreeLibrary(hUser32)) return FALSE;
    }
    SetLastError(nTempError);
    return bRes;
}

// Get IAM Access key, and if an error occurs, returns 0.
_Success_(return == TRUE)
BOOL WTMAPI WTMGetIAMKey(
    _In_ ULONGLONG* pIAMKey
) {
    if (pIAMKey == nullptr) return FALSE;

    // Check for DLL status
    if (!WTMCheckForDll()) {
        SetLastError(ERROR_WTM_WORKER_HAS_NOT_LOADED);
        return FALSE;
    }

    UINT nTime = 0;
    BOOL bIsReady = FALSE;

    // Wait for the backend to be ready (check if the flag file has been generated)
    for (nTime = 0, bIsReady = FALSE; !bIsReady && nTime <= IAMWORKER_TIMEOUT; nTime += 50) {
        if (IAMControllerWindow.bIsReady) bIsReady = TRUE;
        Sleep(50);
    }
    if (!bIsReady) {
        SetLastError(ERROR_WTM_COMMUNICATION_MESSAGING);
        DEBUGERR("WTMGetIAMKey: IAMWorker ready timeout");
        return FALSE;
    }

    // Get collection window handle
    HWND hCollectorWnd = FindWindow(IAMWORKER_WINDOW_CLASSNAME, NULL);
    if (hCollectorWnd == NULL) {
        DEBUGERR("WTMGetIAMKey: Can not find IAMWorker window");
        SetLastError(ERROR_WTM_COMMUNICATION_MESSAGING);
        return FALSE;
    }

    // Send window information
    COPYDATASTRUCT RequestDataInfo{ 0 };
    IAMWorkerRequest RequestData;
    IAM_GetIAMKey_ResponseArguments* pResponseArguments = &IAMControllerWindow.Response.Arguments.GetIAMKey;
    RequestData.RequestType = IAM_GetIAMKey;
    RequestDataInfo.dwData = WM_IAM_OPERATION_REQUEST;
    RequestDataInfo.cbData = sizeof(RequestData);
    RequestDataInfo.lpData = &RequestData;
    LRESULT nRes = SendMessage(hCollectorWnd, WM_COPYDATA, NULL, (LPARAM)&RequestDataInfo);
    if (nRes != ERROR_SUCCESS) {
        SetLastError(nRes);
        DEBUGERR("WTMGetIAMKey: Can not send message to IAMWorker window");
        return FALSE;
    }

    // Collect response
    for (nTime = 0, bIsReady = FALSE; !bIsReady && nTime <= IAMWORKER_TIMEOUT; nTime += 50) {
        if (IAMControllerWindow.Response.bCollected) bIsReady = TRUE;
        Sleep(50);
    }
    if (!bIsReady) {
        SetLastError(ERROR_WTM_COMMUNICATION_MESSAGING);
        DEBUGERR("WTMGetIAMKey: Response timeout");
        return 0;
    }

    // Set error and status
    SetLastError(pResponseArguments->dwErrorCode);
    ULONGLONG nIAMKey = pResponseArguments->nIAMKey;
    IAMControllerWindow.Response.bCollected = FALSE;
    *pIAMKey = nIAMKey;

    return TRUE;
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

        // Unload IAMWorker
        if (!WTMUnloadWorker()) {
            DEBUGERR("DllMain: Unload IAMWorker error");
        }

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