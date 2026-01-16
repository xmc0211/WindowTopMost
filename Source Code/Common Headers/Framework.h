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

#ifndef FRAMEWORK_H
#define FRAMEWORK_H

#define WIN32_LEAN_AND_MEAN

// Windows Headers
#include <Windows.h>

// STL Headers
#include <string>

#ifdef _DEBUG
#include <fstream>
#endif

// Unpublished API function prototype definition
typedef BOOL(WINAPI* T_GetWindowBand)(
    HWND hWnd,                  // Handle of the window
    LPDWORD pdwBand             // Z-Order Band output
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
    DWORD dwBand                // Window Z-Order Band
    );
typedef HWND(WINAPI* T_CreateWindowInBandEx)(
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
    DWORD dwBand,               // Window Z-Order Band
    DWORD Flag                  // [The meaning of the flag is unclear now. Welcome developers to explore it.]
    );
typedef BOOL(WINAPI* T_SetWindowBand)(
    HWND hWnd,                  // Handle of the window
    HWND hWndInsertAfter,       // Same parameter meaning as SetWindowPos
    DWORD dwBand                // Window Z-Order Band
    );
typedef BOOL(WINAPI* T_NtUserEnableIAMAccess)(
    ULONG64 key,                // IAM access key
    BOOL enable                 // Enable or disable flag
    );

// Collector message (request or response) type, used to distinguish message parameters
enum IAMCollectorMessageType : UINT {
    IAM_NONE = 0, // Default
    IAM_GetIAMKey, // Get IAM Key
	IAM_SetWindowBand, // Call SetWindowBand
	IAM_CreateWindowInBand, // Call CreateWindowInBand
	IAM_CreateWindowInBandEx // Call CreateWindowInBandEx
};

// Debugging output
#ifdef _DEBUG
#define DEBUGLOG(Text) do { std::ofstream Log("D:\\DebugLog_WindowTopMost.txt", std::ios::app); Log << Text << std::endl; } while (0)
#define DEBUGERR(Text) DEBUGLOG(Text + (", " + std::to_string((ULONG)GetLastError())))
#else
#define DEBUGLOG(Text) do {  } while (0)
#define DEBUGERR(Text) DEBUGLOG(Text)
#endif

// The program uses window messages to transfer data.
// WND_COLLEXTOR_CLASSNAME and WND_HANDLER_CLASSNAME is the window class name.
#define WND_COLLECTOR_CLASSNAME         TEXT("IAMCollector_7DD48162_2BC7_4073_9CC8_51CC3DB0FF72")
#define WND_HANDLER_CLASSNAME           TEXT("IAMHandler_D06B7428_73FA_4967_B30C_96030ACCF998")
#define WM_IAM_OPERATION_REQUEST        (WM_USER + 1)
#define WM_HANDLERHELLO                 (WM_USER + 2)
#define WM_COLLECTORHELLO               (WM_USER + 3)
#define WM_COLLECTORSTART               (WM_USER + 4)
#define WM_IAM_OPERATION_RESPONSE       (WM_USER + 5)

// Lure EXPLORER Delay between EXE operations (ms)
#define INDUCE_FOREGROUND_DELAY (100ul)
#define INDUCE_STARTMENU_DELAY (200ul)
#define COLLECTOR_TIMEOUT (2000)

// Call SetWindowBand request arguments
struct IAM_SetWindowBand_RequestArguments {
    HWND hWnd;
    HWND hWndInsertAfter;
    DWORD dwBand;
};
// Call CreateWindowInBand request arguments
struct IAM_CreateWindowInBand_RequestArguments {
    DWORD dwExStyle;
    LPCWSTR lpClassName;
    LPCWSTR lpWindowName;
    DWORD dwStyle;
    int X;
    int Y;
    int nWidth;
    int nHeight;
    HWND hWndParent;
    HMENU hMenu;
    HINSTANCE hInstance;
    LPVOID lpParam;
    DWORD dwBand;
};
// Call CreateWindowInBand request arguments
struct IAM_CreateWindowInBandEx_RequestArguments {
    DWORD dwExStyle;
    LPCWSTR lpClassName;
    LPCWSTR lpWindowName;
    DWORD dwStyle;
    int X;
    int Y;
    int nWidth;
    int nHeight;
    HWND hWndParent;
    HMENU hMenu;
    HINSTANCE hInstance;
    LPVOID lpParam;
    DWORD dwBand;
    DWORD Flag;
};
// Call SetWindowBand response arguments
struct IAM_SetWindowBand_ResponseArguments {
    BOOL bStatus;
    DWORD dwErrorCode;
};
// Call CreateWindowInBand response arguments
struct IAM_CreateWindowInBand_ResponseArguments {
    HWND hWnd;
    DWORD dwErrorCode;
};
// Call CreateWindowInBand response arguments
struct IAM_CreateWindowInBandEx_ResponseArguments {
    HWND hWnd;
    DWORD dwErrorCode;
};
// Get IAM key response arguments
struct IAM_GetIAMKey_ResponseArguments {
    ULONGLONG nIAMKey;
    DWORD dwErrorCode;
};

// The structure of IAMCollector request and response
struct IAMCollectorRequest {
    // Type of the request
    IAMCollectorMessageType RequestType;

    // Request arguments
    union IAMRequestArguments {
        IAM_SetWindowBand_RequestArguments SetWindowBand;
        IAM_CreateWindowInBand_RequestArguments CreateWindowInBand;
        IAM_CreateWindowInBandEx_RequestArguments CreateWindowInBandEx;
    } Arguments;

    IAMCollectorRequest() : RequestType(IAM_NONE), Arguments({ 0 }) {}
};
struct IAMCollectorResponse {
    // Is the request being processed
    BOOL bCollected;

    // Type of the response
    IAMCollectorMessageType ResponseType;

    // Response arguments
    union IAMResponseArguments {
        IAM_SetWindowBand_ResponseArguments SetWindowBand;
        IAM_CreateWindowInBand_ResponseArguments CreateWindowInBand;
        IAM_CreateWindowInBandEx_ResponseArguments CreateWindowInBandEx;
        IAM_GetIAMKey_ResponseArguments GetIAMKey;
    } Arguments;

    IAMCollectorResponse() : ResponseType(IAM_NONE), bCollected(FALSE), Arguments({ 0 }) {}
};

#endif
