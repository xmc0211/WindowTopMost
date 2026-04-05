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

#ifndef WTM_FRAMEWORK_H
#define WTM_FRAMEWORK_H

#define WIN32_LEAN_AND_MEAN

// Windows Headers
#include <Windows.h>

// STL Headers
#include <string>

#ifdef _DEBUG
#include <fstream>
#endif

#include "ErrorCodes.h"

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
    DWORD dwInternalFlag        // [The meaning of this flag is unclear now. For more information, Please review file "CreateWindowInBand-dwInternalFlag-informations.md".]
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
enum IAMWorkerMessageType : UINT {
    IAM_NONE = 0, // Default
    IAM_GetIAMKey, // Get IAM Key
	IAM_SetWindowBand, // Call SetWindowBand
	IAM_CreateWindowInBand, // Call CreateWindowInBand
	IAM_CreateWindowInBandEx // Call CreateWindowInBandEx
};

// Debugging output
#ifdef _DEBUG
#define DEBUGLOG(Text) do { DWORD __nErrorTemp = GetLastError(); std::ofstream Log("D:\\WTM_DebugLog.txt", std::ios::app); Log << Text << std::endl; SetLastError(__nErrorTemp); } while (0)
#define DEBUGERR(Text) DEBUGLOG(Text + (", " + std::to_string((ULONG)GetLastError())))
#else
#define DEBUGLOG(Text) do {  } while (0)
#define DEBUGERR(Text) DEBUGLOG(Text)
#endif

// The program uses window messages to transfer data.
// WND_COLLEXTOR_CLASSNAME and IAMCONTROLLER_WINDOW_CLASSNAME is the window class name.
#define IAMWORKER_WINDOW_CLASSNAME      TEXT("IAMWorker_7DD48162_2BC7_4073_9CC8_51CC3DB0FF72")
#define IAMCONTROLLER_WINDOW_CLASSNAME  TEXT("IAMController_D06B7428_73FA_4967_B30C_96030ACCF998")
#define WM_IAM_OPERATION_REQUEST        (WM_USER + 1)
#define WM_IAMCONTROLLERHELLO           (WM_USER + 2)
#define WM_IAMWORKERHELLO               (WM_USER + 3)
#define WM_IAMWORKERSTART               (WM_USER + 4)
#define WM_IAM_OPERATION_RESPONSE       (WM_USER + 5)

// Lure EXPLORER Delay between EXE operations (ms)
#define INDUCE_FOREGROUND_DELAY         (100ul)
#define INDUCE_STARTMENU_DELAY          (50ul)
#define IAMWORKER_TIMEOUT               (2000ul)

// The maximum length of the window name and window class name obtained from actual testing
#define MAX_WINDOW_NAME                 (0x00010000) // 65536 Characters (not bytes)
#define MAX_WINDOW_CLASS_NAME           (0x00000100) // 256 Characters

// The structor of extended data information
//                        [PCOPYDATASTRUCT]->
// +-------------------+--------------------------------------------+
// | Request Arguments | Extended Data [strings, dynamic values...] |
// +-------------------+--------------------------------------------+
// When sending message WM_COPYDATA, some memory space will be added after the structure
// to store one or more extended data. When extended data is required in the request, 
// the offset and data size of the extended data relative to the structure header will 
// be passed.
typedef struct _IAMExtendedDataInformation {
    SIZE_T nOffset; // The offset of the extended data relative to the structure IAMWorkerRequest or IAMWorkerResponse
    SIZE_T nSize; // The data size of the extended data relative to the structure IAMWorkerRequest or IAMWorkerResponse
} IAMExtendedDataInformation;

// Call SetWindowBand request arguments
typedef struct _IAM_SetWindowBand_RequestArguments {
    HWND hWnd;
    HWND hWndInsertAfter;
    DWORD dwBand;
} IAM_SetWindowBand_RequestArguments;

// Call CreateWindowInBand request arguments
typedef struct _IAM_CreateWindowInBand_RequestArguments {
    DWORD dwExStyle;
    IAMExtendedDataInformation lpClassName; // The string will saved in extended data
    IAMExtendedDataInformation lpWindowName; // The string will saved in extended data
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
} IAM_CreateWindowInBand_RequestArguments;

// Call CreateWindowInBand request arguments
typedef struct _IAM_CreateWindowInBandEx_RequestArguments {
    DWORD dwExStyle;
    IAMExtendedDataInformation lpClassName; // The string will saved in extended data
    IAMExtendedDataInformation lpWindowName; // The string will saved in extended data
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
    DWORD dwInternalFlag;
} IAM_CreateWindowInBandEx_RequestArguments;

// Call SetWindowBand response arguments
typedef struct _IAM_SetWindowBand_ResponseArguments {
    BOOL bStatus;
    DWORD dwErrorCode;
} IAM_SetWindowBand_ResponseArguments;

// Call CreateWindowInBand response arguments
typedef struct _IAM_CreateWindowInBand_ResponseArguments {
    HWND hWnd;
    DWORD dwErrorCode;
} IAM_CreateWindowInBand_ResponseArguments;

// Call CreateWindowInBandEx response arguments
typedef struct _IAM_CreateWindowInBandEx_ResponseArguments {
    HWND hWnd;
    DWORD dwErrorCode;
} IAM_CreateWindowInBandEx_ResponseArguments;

// Get IAM key response arguments
typedef struct _IAM_GetIAMKey_ResponseArguments {
    ULONGLONG nIAMKey;
    DWORD dwErrorCode;
} IAM_GetIAMKey_ResponseArguments;

// The structure of IAMWorker request and response
typedef struct _IAMWorkerRequest {
    // Type of the request
    IAMWorkerMessageType RequestType;

    // Size of extended data
    SIZE_T nExtendedDataSize;

    // Request arguments
    union IAMRequestArguments {
        IAM_SetWindowBand_RequestArguments SetWindowBand;
        IAM_CreateWindowInBand_RequestArguments CreateWindowInBand;
        IAM_CreateWindowInBandEx_RequestArguments CreateWindowInBandEx;
    } Arguments;

    _IAMWorkerRequest() : RequestType(IAM_NONE), nExtendedDataSize(0) {
        ZeroMemory(&Arguments, sizeof(Arguments));
    }
} IAMWorkerRequest;
typedef struct _IAMWorkerResponse {
    // Is the request being processed
    BOOL bCollected;

    // Type of the response
    IAMWorkerMessageType ResponseType;

    // Size of extended data
    SIZE_T nExtendedDataSize;

    // Response arguments
    union IAMResponseArguments {
        IAM_SetWindowBand_ResponseArguments SetWindowBand;
        IAM_CreateWindowInBand_ResponseArguments CreateWindowInBand;
        IAM_CreateWindowInBandEx_ResponseArguments CreateWindowInBandEx;
        IAM_GetIAMKey_ResponseArguments GetIAMKey;
    } Arguments;

    _IAMWorkerResponse() : bCollected(FALSE), ResponseType(IAM_NONE), nExtendedDataSize(0) {
        ZeroMemory(&Arguments, sizeof(Arguments));
    }
} IAMWorkerResponse;

#endif
