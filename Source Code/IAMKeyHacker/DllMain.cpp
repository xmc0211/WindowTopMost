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
#include "../Common Headers/ThreadStopHandler.h"
#include <shlobj_core.h>
#include "mhook-lib/mhook.h"
#include "IAMKeyBlock.h"

// Unpublished API function global object declaration
T_GetWindowBand pGetWindowBand = NULL;
T_CreateWindowInBand pCreateWindowInBand = NULL;
T_CreateWindowInBandEx pCreateWindowInBandEx = NULL;
T_SetWindowBand pSetWindowBand = NULL;
T_NtUserEnableIAMAccess pNtUserEnableIAMAccess = NULL;

// Global variables
HMODULE hParentInstance = NULL; // Parent process (Explorer.EXE) instance handle
HANDLE hCheckThread = NULL;
HWND hWorkerWindow = NULL; // Information receiving window handle
BOOL bHookCreated = FALSE; // Has the hook been successfully created
BOOL bIAMControllerStarted;
ThreadStopHandler ThreadStop;

// IAM global variables
ULONG64 nIAMKey = 0; // Saved IAM Access Key
BOOL bIAMKeySaved = FALSE; // Has the IAM key been successfully obtained and saved

// The structures used to pass parameters to the window message loop thread
// For CreateWindowInBand
typedef struct _IAM_CreateWindowInBand_WindowLoopThreadArguments {
    const IAM_CreateWindowInBand_RequestArguments* pRequestArguments; // Request arguments
    IAM_CreateWindowInBand_ResponseArguments* pResponseArguments; // Response arguments
    LPCWSTR lpClassName; // Class name
    LPCWSTR lpWindowName; // Window name
    HANDLE hWindowCreateEvent; // Create finish event
} IAM_CreateWindowInBand_WindowLoopThreadArguments;
// For CreateWindowInBandEx
typedef struct _IAM_CreateWindowInBandEx_WindowLoopThreadArguments {
    const IAM_CreateWindowInBandEx_RequestArguments* pRequestArguments; // Request arguments
    IAM_CreateWindowInBandEx_ResponseArguments* pResponseArguments; // Response arguments
    LPCWSTR lpClassName; // Class name
    LPCWSTR lpWindowName; // Window name
    HANDLE hWindowCreateEvent; // Create finish event
} IAM_CreateWindowInBandEx_WindowLoopThreadArguments;

// Function declarations

#if defined(__cplusplus)
extern "C" {
#endif

// Window message loop functions
_Success_(return == ERROR_SUCCESS)
DWORD CALLBACK IAMCreateWindowInBand_WindowMainLoopThread(LPVOID lpArguments) {
    ThreadStop.EnterThread();

    // Extract arguments and check
    if (lpArguments == nullptr) {
        ThreadStop.LeaveThread();
        return ERROR_INVALID_PARAMETER;
    }
    IAM_CreateWindowInBand_WindowLoopThreadArguments* pArguments = reinterpret_cast<IAM_CreateWindowInBand_WindowLoopThreadArguments*>(lpArguments);
    if (pArguments->pResponseArguments == nullptr ||
        pArguments->pRequestArguments == nullptr ||
        pArguments->lpClassName == nullptr ||
        pArguments->lpWindowName == nullptr ||
        pArguments->hWindowCreateEvent == NULL
        ) {
        ThreadStop.LeaveThread();
        return ERROR_INVALID_PARAMETER;
    }

    // Temporarily enable IAM access and execution
    if (!pNtUserEnableIAMAccess(nIAMKey, TRUE)) {
        pArguments->pResponseArguments->hWnd = NULL;
        pArguments->pResponseArguments->dwErrorCode = ERROR_WTM_IAM_PERMISSION;
        SetEvent(pArguments->hWindowCreateEvent);
        ThreadStop.LeaveThread();
        return ERROR_WTM_IAM_PERMISSION;
    }
    SetLastError(ERROR_SUCCESS);
    pArguments->pResponseArguments->hWnd = pCreateWindowInBand(
        pArguments->pRequestArguments->dwExStyle,
        pArguments->lpClassName,
        pArguments->lpWindowName,
        pArguments->pRequestArguments->dwStyle,
        pArguments->pRequestArguments->X,
        pArguments->pRequestArguments->Y,
        pArguments->pRequestArguments->nWidth,
        pArguments->pRequestArguments->nHeight,
        pArguments->pRequestArguments->hWndParent,
        pArguments->pRequestArguments->hMenu,
        pArguments->pRequestArguments->hInstance,
        pArguments->pRequestArguments->lpParam,
        pArguments->pRequestArguments->dwBand
    );
    pArguments->pResponseArguments->dwErrorCode = GetLastError();
    if (!pNtUserEnableIAMAccess(nIAMKey, FALSE)) {
        pArguments->pResponseArguments->hWnd = NULL;
        pArguments->pResponseArguments->dwErrorCode = ERROR_WTM_IAM_PERMISSION;
        SetEvent(pArguments->hWindowCreateEvent);
        ThreadStop.LeaveThread();
        return ERROR_WTM_IAM_PERMISSION;
    }

    // The creation of the event notification window has been completed
    if (!SetEvent(pArguments->hWindowCreateEvent)) {
        ThreadStop.LeaveThread();
        return ERROR_INTERNAL_ERROR;
    }

    // Failed to create window and return directly
    if (pArguments->pResponseArguments->dwErrorCode != ERROR_SUCCESS) {
        ThreadStop.LeaveThread();
        return ERROR_SUCCESS;
    }

    // Main message loop
    MSG Msg;
    while (TRUE) {
        if (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE)) {
            if (Msg.hwnd == pArguments->pResponseArguments->hWnd) {
                if (Msg.message == WM_QUIT) break;
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
    }

    ThreadStop.LeaveThread();
    return ERROR_SUCCESS;
}
_Success_(return == ERROR_SUCCESS)
DWORD CALLBACK IAMCreateWindowInBandEx_WindowMainLoopThread(LPVOID lpArguments) {
    ThreadStop.EnterThread();

    // Extract arguments and check
    if (lpArguments == nullptr) {
        ThreadStop.LeaveThread();
        return ERROR_INVALID_PARAMETER;
    }
    IAM_CreateWindowInBandEx_WindowLoopThreadArguments* pArguments = reinterpret_cast<IAM_CreateWindowInBandEx_WindowLoopThreadArguments*>(lpArguments);
    if (pArguments->pResponseArguments == nullptr ||
        pArguments->pRequestArguments == nullptr ||
        pArguments->lpClassName == nullptr ||
        pArguments->lpWindowName == nullptr ||
        pArguments->hWindowCreateEvent == NULL
        ) {
        ThreadStop.LeaveThread();
        return ERROR_INVALID_PARAMETER;
    }

    // Temporarily enable IAM access and execution
    if (!pNtUserEnableIAMAccess(nIAMKey, TRUE)) {
        pArguments->pResponseArguments->hWnd = NULL;
        pArguments->pResponseArguments->dwErrorCode = ERROR_WTM_IAM_PERMISSION;
        SetEvent(pArguments->hWindowCreateEvent);
        ThreadStop.LeaveThread();
        return ERROR_WTM_IAM_PERMISSION;
    }
    SetLastError(ERROR_SUCCESS);
    pArguments->pResponseArguments->hWnd = pCreateWindowInBandEx(
        pArguments->pRequestArguments->dwExStyle,
        pArguments->lpClassName,
        pArguments->lpWindowName,
        pArguments->pRequestArguments->dwStyle,
        pArguments->pRequestArguments->X,
        pArguments->pRequestArguments->Y,
        pArguments->pRequestArguments->nWidth,
        pArguments->pRequestArguments->nHeight,
        pArguments->pRequestArguments->hWndParent,
        pArguments->pRequestArguments->hMenu,
        pArguments->pRequestArguments->hInstance,
        pArguments->pRequestArguments->lpParam,
        pArguments->pRequestArguments->dwBand,
        pArguments->pRequestArguments->dwInternalFlag
    );
    pArguments->pResponseArguments->dwErrorCode = GetLastError();
    if (!pNtUserEnableIAMAccess(nIAMKey, FALSE)) {
        pArguments->pResponseArguments->hWnd = NULL;
        pArguments->pResponseArguments->dwErrorCode = ERROR_WTM_IAM_PERMISSION;
        SetEvent(pArguments->hWindowCreateEvent);
        ThreadStop.LeaveThread();
        return ERROR_WTM_IAM_PERMISSION;
    }

    // The creation of the event notification window has been completed
    if (!SetEvent(pArguments->hWindowCreateEvent)) {
        ThreadStop.LeaveThread();
        return ERROR_INTERNAL_ERROR;
    }

    // Failed to create window and return directly
    if (pArguments->pResponseArguments->dwErrorCode != ERROR_SUCCESS) {
        ThreadStop.LeaveThread();
        return ERROR_SUCCESS;
    }

    // Main message loop
    MSG Msg;
    while (TRUE) {
        if (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE)) {
            if (Msg.hwnd == pArguments->pResponseArguments->hWnd) {
                if (Msg.message == WM_QUIT) break;
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
    }

    ThreadStop.LeaveThread();
    return ERROR_SUCCESS;
}

// Get string pointer (allocate new) from extended data
_Success_(return == TRUE)
BOOL IAMGetStringFromExtendedData(
    _In_        ULONG_PTR pBaseAddress,     // Original memory starting address
    _In_        SIZE_T nValidByteSize,      // The number of valid bytes in raw memory
    _In_opt_    SIZE_T nByteOffset,         // Byte offset of string in raw memory
    _In_        SIZE_T nCharCount,          // The number of characters to be extracted, including the terminator
    _Inout_     PWCHAR pBuffer,             // The buffer provided by the caller
    _In_        SIZE_T nBufferCharCount      // Characters count of buffer zone
) {
    if (pBaseAddress == NULL || pBuffer == nullptr) return FALSE;
    if (nBufferCharCount == 0 || nCharCount == 0 || nBufferCharCount < nCharCount) return FALSE; // Buffer overflow judgment

    // Verify the original memory boundary
    const SIZE_T nRequiredByteSize = nCharCount * sizeof(WCHAR);
    // Verify access interval
    if (nByteOffset > nValidByteSize || nByteOffset + nRequiredByteSize > nValidByteSize) return FALSE;

    // Safely read raw memory
    const PWCHAR pSourceAddress = reinterpret_cast<const PWCHAR>(pBaseAddress + nByteOffset);

    // Copy to the caller buffer and add a terminator
    for (SIZE_T i = 0; i < nCharCount - 1; ++i) {
        pBuffer[i] = pSourceAddress[i];
    }

    // Add terminator
    pBuffer[nCharCount - 1] = L'\0';

    return TRUE;
}

// Handling requests from IAMController
_Success_(return == TRUE)
BOOL IAMHandleRequest(
    _In_        const IAMWorkerRequest* pRequest, 
    _In_        SIZE_T nRequestSize, 
    _Out_       IAMWorkerResponse** ppResponse, 
    _Out_opt_   SIZE_T* pResponseSize
) {
    if (pRequest == nullptr || ppResponse == nullptr || pResponseSize == nullptr) return FALSE;

    // Allocate response data memory and set basic size
    // *ppResponse is valid regardless of whether the function is successful or not, and needs to be manually released by the caller.
    *ppResponse = reinterpret_cast<IAMWorkerResponse*>(malloc(sizeof(IAMWorkerResponse)));
    *pResponseSize = sizeof(IAMWorkerResponse);
    if (*ppResponse == nullptr) return FALSE;
    ZeroMemory(*ppResponse, sizeof(IAMWorkerResponse));

    // Get request data
    switch (pRequest->RequestType) {
        case IAM_GetIAMKey: {
            (*ppResponse)->ResponseType = IAM_GetIAMKey;
            IAM_GetIAMKey_ResponseArguments* pResponseArguments = &(*ppResponse)->Arguments.GetIAMKey;

            // Return IAM key, error if not found
            if (!bIAMKeySaved) {
                SetLastError(ERROR_WTM_IAM_KEY_HAS_NOT_OBTAINED);
                DEBUGERR("IAMHandleRequest: Request IAM key but not saved");
                return FALSE;
            }
            pResponseArguments->dwErrorCode = ERROR_SUCCESS;
            pResponseArguments->nIAMKey = nIAMKey;
            DEBUGLOG("IAMHandleRequest: Request IAM key success");
            break;
        }
        case IAM_SetWindowBand: {
            (*ppResponse)->ResponseType = IAM_SetWindowBand;
            const IAM_SetWindowBand_RequestArguments* pRequestArguments = &pRequest->Arguments.SetWindowBand;
            IAM_SetWindowBand_ResponseArguments* pResponseArguments = &(*ppResponse)->Arguments.SetWindowBand;

            DEBUGLOG("IAMHandleRequest: Ready for executing SetWindowBand");

            // Temporarily enable IAM access and execution
            if (!pNtUserEnableIAMAccess(nIAMKey, TRUE)) {
                DEBUGERR("IAMHandleRequest: Can not enable IAM access");
                SetLastError(ERROR_WTM_IAM_PERMISSION);
                return FALSE;
            }
            SetLastError(ERROR_SUCCESS);
            pResponseArguments->bStatus = pSetWindowBand(
                pRequestArguments->hWnd, 
                pRequestArguments->hWndInsertAfter, 
                pRequestArguments->dwBand
            );
            pResponseArguments->dwErrorCode = GetLastError();
            if (!pNtUserEnableIAMAccess(nIAMKey, FALSE)) {
                DEBUGERR("IAMHandleRequest: Can not disable IAM access");
                SetLastError(ERROR_WTM_IAM_PERMISSION);
                return FALSE;
            }

            DEBUGLOG("IAMHandleRequest: Execute SetWindowBand success");
            break;
        }
        case IAM_CreateWindowInBand: {
            (*ppResponse)->ResponseType = IAM_CreateWindowInBand;
            const IAM_CreateWindowInBand_RequestArguments* pRequestArguments = &pRequest->Arguments.CreateWindowInBand;
            IAM_CreateWindowInBand_ResponseArguments* pResponseArguments = &(*ppResponse)->Arguments.CreateWindowInBand;

            // Retrieve string information
            SIZE_T nClassNameByteSize = pRequestArguments->lpClassName.nSize;
            SIZE_T nWindowNameByteSize = pRequestArguments->lpWindowName.nSize;

            // Check if the size is aligned with WCHAR
            if (nClassNameByteSize % sizeof(WCHAR) != 0 ||
                nWindowNameByteSize % sizeof(WCHAR) != 0
            ) {
                SetLastError(ERROR_WTM_COMMUNICATION_CODING);
                DEBUGERR("IAMHandleRequest: Unexpected string length");
                return FALSE;
            }

            SIZE_T nClassNameCharCount = nClassNameByteSize / sizeof(WCHAR);
            SIZE_T nWindowNameCharCount = nWindowNameByteSize / sizeof(WCHAR);

            // Prevent long strings and handle string length errors in advance
            if (nClassNameByteSize > MAX_WINDOW_CLASS_NAME ||
                nWindowNameByteSize > MAX_WINDOW_NAME
            ) {
                SetLastError(ERROR_INVALID_PARAMETER);
                DEBUGERR("IAMHandleRequest: String too long");
                return FALSE;
            }

            // Allocate strings' memory
            LPWSTR lpClassName = reinterpret_cast<LPWSTR>(malloc(nClassNameByteSize));
            if (lpClassName == nullptr) {
                *pResponseSize = 0;
                return FALSE;
            }
            ZeroMemory(lpClassName, nClassNameByteSize);
            LPWSTR lpWindowName = reinterpret_cast<LPWSTR>(malloc(nWindowNameByteSize));
            if (lpWindowName == nullptr) {
                free(lpClassName);
                *pResponseSize = 0;
                return FALSE;
            }
            ZeroMemory(lpWindowName, nWindowNameByteSize);

            // Expand strings
            if (!IAMGetStringFromExtendedData(
                (ULONG_PTR)pRequest,
                nRequestSize,
                pRequestArguments->lpClassName.nOffset,
                nClassNameCharCount,
                lpClassName,
                nClassNameCharCount
            )) {
                free(lpClassName);
                free(lpWindowName);
                SetLastError(ERROR_WTM_COMMUNICATION_CODING);
                DEBUGERR("IAMHandleRequest: Can not expand lpClassName");
                return FALSE;
            }
            if (!IAMGetStringFromExtendedData(
                (ULONG_PTR)pRequest,
                nRequestSize,
                pRequestArguments->lpWindowName.nOffset,
                nWindowNameCharCount,
                lpWindowName,
                nWindowNameCharCount
            )) {
                free(lpClassName);
                free(lpWindowName);
                SetLastError(ERROR_WTM_COMMUNICATION_CODING);
                DEBUGERR("IAMHandleRequest: Can not expand lpWindowName");
                return FALSE;
            }

            DEBUGLOG("IAMHandleRequest: Ready for executing CreateWindowInBand");

            // Create an event and waiting for thread to complete window creation
            HANDLE hWindowCreateEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (hWindowCreateEvent == nullptr) {
                free(lpClassName);
                free(lpWindowName);
                SetLastError(ERROR_WTM_COMMUNICATION_PROCESSING);
                return FALSE;
            }
            IAM_CreateWindowInBand_WindowLoopThreadArguments WindowLoopArguments = {
                pRequestArguments,
                pResponseArguments,
                lpClassName,
                lpWindowName,
                hWindowCreateEvent
            };
            HANDLE hWindowCreateThread = CreateThread(NULL, 0, IAMCreateWindowInBand_WindowMainLoopThread, reinterpret_cast<LPVOID>(&WindowLoopArguments), 0, 0);

            // Wait for window creation
            WaitForSingleObject(hWindowCreateEvent, INFINITE);

            free(lpClassName);
            free(lpWindowName);

            DEBUGLOG("IAMHandleRequest: Execute CreateWindowInBand success");
            break;
        }
        case IAM_CreateWindowInBandEx: {
            (*ppResponse)->ResponseType = IAM_CreateWindowInBandEx;
            const IAM_CreateWindowInBandEx_RequestArguments* pRequestArguments = &pRequest->Arguments.CreateWindowInBandEx;
            IAM_CreateWindowInBandEx_ResponseArguments* pResponseArguments = &(*ppResponse)->Arguments.CreateWindowInBandEx;

            // Retrieve string information
            SIZE_T nClassNameByteSize = pRequestArguments->lpClassName.nSize;
            SIZE_T nWindowNameByteSize = pRequestArguments->lpWindowName.nSize;

            // Check if the size is aligned with WCHAR
            if (nClassNameByteSize % sizeof(WCHAR) != 0 ||
                nWindowNameByteSize % sizeof(WCHAR) != 0
                ) {
                SetLastError(ERROR_WTM_COMMUNICATION_CODING);
                DEBUGERR("IAMHandleRequest: Unexpected string length");
                return FALSE;
            }

            SIZE_T nClassNameCharCount = nClassNameByteSize / sizeof(WCHAR);
            SIZE_T nWindowNameCharCount = nWindowNameByteSize / sizeof(WCHAR);

            // Prevent long strings and handle string length errors in advance
            if (nClassNameByteSize > MAX_WINDOW_CLASS_NAME ||
                nWindowNameByteSize > MAX_WINDOW_NAME
                ) {
                SetLastError(ERROR_INVALID_PARAMETER);
                DEBUGERR("IAMHandleRequest: String too long");
                return FALSE;
            }

            // Allocate strings' memory
            LPWSTR lpClassName = reinterpret_cast<LPWSTR>(malloc(nClassNameByteSize));
            if (lpClassName == nullptr) {
                *pResponseSize = 0;
                return FALSE;
            }
            ZeroMemory(lpClassName, nClassNameByteSize);
            LPWSTR lpWindowName = reinterpret_cast<LPWSTR>(malloc(nWindowNameByteSize));
            if (lpWindowName == nullptr) {
                free(lpClassName);
                *pResponseSize = 0;
                return FALSE;
            }
            ZeroMemory(lpWindowName, nWindowNameByteSize);

            // Expand strings
            if (!IAMGetStringFromExtendedData(
                (ULONG_PTR)pRequest,
                nRequestSize,
                pRequestArguments->lpClassName.nOffset,
                nClassNameCharCount,
                lpClassName,
                nClassNameCharCount
            )) {
                free(lpClassName);
                free(lpWindowName);
                SetLastError(ERROR_WTM_COMMUNICATION_CODING);
                DEBUGERR("IAMHandleRequest: Can not expand lpClassName");
                return FALSE;
            }
            if (!IAMGetStringFromExtendedData(
                (ULONG_PTR)pRequest,
                nRequestSize,
                pRequestArguments->lpWindowName.nOffset,
                nWindowNameCharCount,
                lpWindowName,
                nWindowNameCharCount
            )) {
                free(lpClassName);
                free(lpWindowName);
                SetLastError(ERROR_WTM_COMMUNICATION_CODING);
                DEBUGERR("IAMHandleRequest: Can not expand lpWindowName");
                return FALSE;
            }

            DEBUGLOG("IAMHandleRequest: Ready for executing CreateWindowInBandEx");

            // Create an event and waiting for thread to complete window creation
            HANDLE hWindowCreateEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (hWindowCreateEvent == nullptr) {
                free(lpClassName);
                free(lpWindowName);
                SetLastError(ERROR_WTM_COMMUNICATION_PROCESSING);
                return FALSE;
            }
            IAM_CreateWindowInBandEx_WindowLoopThreadArguments WindowLoopArguments = {
                pRequestArguments,
                pResponseArguments,
                lpClassName,
                lpWindowName,
                hWindowCreateEvent
            };
            HANDLE hWindowCreateThread = CreateThread(NULL, 0, IAMCreateWindowInBandEx_WindowMainLoopThread, reinterpret_cast<LPVOID>(&WindowLoopArguments), 0, 0);

            // Wait for window creation
            WaitForSingleObject(hWindowCreateEvent, INFINITE);

            free(lpClassName);
            free(lpWindowName);

            DEBUGLOG("IAMHandleRequest: Execute CreateWindowInBandEx success");
            break;
        }
        default: {
            SetLastError(ERROR_WTM_COMMUNICATION_PROCESSING);
            DEBUGERR("IAMHandleRequest: Unknown request type");
            return FALSE;
        }
    }

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

// Message receiving window message processing function
_Success_(return == ERROR_SUCCESS)
LRESULT CALLBACK IAMWorkerWndProc(
    _In_        HWND hWnd, 
    _In_opt_    UINT message, 
    _In_opt_    WPARAM wParam, 
    _In_opt_    LPARAM lParam
) {
    switch (message) {
        // WM_COPYDATA message
        case WM_COPYDATA: {
            // Get COPYDATA struct
            PCOPYDATASTRUCT pRequestDataInfo = (PCOPYDATASTRUCT)lParam;

            // Operation request
            if (pRequestDataInfo->dwData == WM_IAM_OPERATION_REQUEST) {
                COPYDATASTRUCT ResponseDataInfo{ 0 };
                IAMWorkerResponse* pResponseData = nullptr;

                DEBUGLOG("IAMWorkerWndProc: Enter Message: WM_IAM_OPERATION_REQUEST");

                // Exit if IAMController is not start
                if (!bIAMControllerStarted) {
                    DEBUGLOG("IAMWorkerWndProc: IAMController has not start");
                    return ERROR_WTM_COMMUNICATION_MESSAGING;
                }

                // Check size and get request pointer
                if (pRequestDataInfo->cbData < sizeof(IAMWorkerRequest)) {
                    DEBUGLOG("IAMWorkerWndProc: Unexpected request data size");
                    return ERROR_WTM_COMMUNICATION_CODING;
                }
                const IAMWorkerRequest* pRequestData = reinterpret_cast<const IAMWorkerRequest*>(pRequestDataInfo->lpData);
                if (pRequestData == nullptr) return ERROR_WTM_COMMUNICATION_CODING;

                // Check total data size
                if (sizeof(IAMWorkerRequest) + pRequestData->nExtendedDataSize != pRequestDataInfo->cbData) {
                    DEBUGLOG("IAMWorkerWndProc: Unexpected request extended data size");
                    return ERROR_WTM_COMMUNICATION_CODING;
                }

                // Process request
                SIZE_T nResponseSize = 0;
                BOOL bRes = IAMHandleRequest(pRequestData, pRequestDataInfo->cbData, &pResponseData, &nResponseSize); // it will allocate memory for pResponseData anyway
                if (!bRes || nResponseSize < sizeof(IAMWorkerResponse)) {
                    DWORD nErrorCode = GetLastError();
                    if (nErrorCode == ERROR_SUCCESS) {
                        nErrorCode = ERROR_WTM_COMMUNICATION_PROCESSING;
                    }
                    DEBUGERR("IAMWorkerWndProc: Can not process request");
                    free(pResponseData);
                    return nErrorCode;
                }

                // build COPYDATA struct for response
                pResponseData->bCollected = TRUE;
                ResponseDataInfo.dwData = WM_IAM_OPERATION_RESPONSE;
                ResponseDataInfo.cbData = nResponseSize;
                ResponseDataInfo.lpData = pResponseData;
                DEBUGLOG("IAMWorkerWndProc: Process response success");

                // Send response data
                HWND hIAMControllerWindow = FindWindow(IAMCONTROLLER_WINDOW_CLASSNAME, NULL);
                if (!hIAMControllerWindow) {
                    DEBUGERR("IAMWorkerWndProc: Can not find IAMController window");
                    free(pResponseData);
                    return ERROR_WTM_COMMUNICATION_MESSAGING;
                }
                LRESULT nRes = SendMessage(hIAMControllerWindow, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&ResponseDataInfo);
                if (nRes != ERROR_SUCCESS) {
                    SetLastError(nRes);
                    DEBUGLOG("IAMWorkerWndProc: SendMessage in WM_COPYDATA error");
                    free(pResponseData);
                    return nRes;
                }

                free(pResponseData);
                DEBUGLOG("IAMWorkerWndProc: Exit Message: WM_IAM_OPERATION_REQUEST");
            }
            if (pRequestDataInfo->dwData == WM_IAMWORKERHELLO) {
                // First handshake message
                DEBUGLOG("IAMWorkerWndProc: --> IAMController hello");
                HWND hIAMControllerWindow = FindWindow(IAMCONTROLLER_WINDOW_CLASSNAME, NULL);
                if (!hIAMControllerWindow) {
                    DEBUGERR("IAMWorkerWndProc: Can not find IAMController window");
                    return ERROR_WTM_COMMUNICATION_MESSAGING;
                }
                DEBUGLOG("IAMWorkerWndProc: <-- Collector hello");
                COPYDATASTRUCT TempMessage{ 0 };
                TempMessage.dwData = WM_IAMCONTROLLERHELLO;
                TempMessage.lpData = NULL;
                TempMessage.cbData = 0;
                LRESULT nRes = SendMessage(hIAMControllerWindow, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&TempMessage);
                if (nRes != ERROR_SUCCESS) {
                    SetLastError(nRes);
                    DEBUGERR("IAMWorkerWndProc: SendMessage error");
                    return nRes;
                }
            }
            if (pRequestDataInfo->dwData == WM_IAMWORKERSTART) {
                // Third handshake message
                DEBUGLOG("IAMWorkerWndProc: --> Collector start");
                bIAMControllerStarted = TRUE;
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
    return ERROR_SUCCESS;
}

// Thread callback function for monitoring SetWindowBand requests
// A hidden window will be created within the function to receive messages.
void CALLBACK IAMWorkerWindowLoopThread(
    _In_opt_    LPVOID lpParam
) {
    ThreadStop.EnterThread();

    // Register Window Class
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = IAMWorkerWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = (HINSTANCE)hParentInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = IAMWORKER_WINDOW_CLASSNAME;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    if (!RegisterClassEx(&wcex)) {
        DEBUGERR("IAMWorkerWindowLoopThread: Window class register error");
        ThreadStop.LeaveThread();
        return;
    }

    // Window initialization, but not display
    hWorkerWindow = CreateWindowEx(
        NULL, 
        IAMWORKER_WINDOW_CLASSNAME,
        TEXT(""),
        WS_OVERLAPPEDWINDOW,
        0, 0, 100, 100, 
        nullptr, 
        nullptr, 
        hParentInstance, 
        nullptr
    );
    if (!hWorkerWindow) {
        DEBUGERR("IAMWorkerWindowLoopThread: Window create error");
        UnregisterClass(IAMWORKER_WINDOW_CLASSNAME, hParentInstance);
        ThreadStop.LeaveThread();
        return;
    }

    // Reduce permissions to accept WM_COPYDATA
    if (IsUserAnAdmin()) {
        BOOL bRes = ChangeWindowMessageFilterEx(hWorkerWindow, WM_COPYDATA, MSGFLT_ADD, NULL);
        if (!bRes) {
            DEBUGERR("IAMWorkerWindowLoopThread: ChangeWindowMessageFilterEx error");
            UnregisterClass(IAMWORKER_WINDOW_CLASSNAME, hParentInstance);
            ThreadStop.LeaveThread();
            return;
        }
    }

    UpdateWindow(hWorkerWindow);
    
    // Main message loop
    DEBUGLOG("IAMWorkerWindowLoopThread: Window created successfully. Enter main loop. ");
    MSG Msg;
    while (TRUE) {
        if (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE)) {
            if (Msg.message == WM_QUIT) break;
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }

    // Cleanup
    UnregisterClass(IAMWORKER_WINDOW_CLASSNAME, hParentInstance);
    ThreadStop.LeaveThread();
    return;
}

// A function modeled after NtUserEnableIAMAccess, used for hook callbacks and obtaining IAM keys
_Success_(return == TRUE)
BOOL WINAPI IAMNtUserEnableIAMAccess_Hook(
    _In_opt_    ULONG64 key, 
    _In_opt_    BOOL enable
) {
    DEBUGLOG("IAMNtUserEnableIAMAccess_Hook: Hook function has been called");

    // Get the return value of the real function
    const BOOL bResOriginal = pNtUserEnableIAMAccess(key, enable);

    // Check if it is successful and if the IAM key has not been set. If it matches, obtain IAM key and uninstall hook
    if (bResOriginal == TRUE && bIAMKeySaved == FALSE) {
        // Save IAM Key
        nIAMKey = key;
        BOOL bRes = IAMKeyWrite(nIAMKey);
        if (!bRes) {
            DEBUGERR("IAMNtUserEnableIAMAccess_Hook: Can not save IAM key into memory.");

            // No impact on usage, continue execution
            // return bResOriginal;
        }

        // Set flag as saved
        bIAMKeySaved = TRUE;
        
        // Start the thread and start monitoring requests
        hCheckThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)IAMWorkerWindowLoopThread, NULL, NULL, 0);

        // Uninstall the hook to stop monitoring
        if (bHookCreated == TRUE) {
            if (!Mhook_Unhook((PVOID*)&pNtUserEnableIAMAccess)) return FALSE;
            bHookCreated = FALSE; // Update state
        }

        // Error check
        if (hCheckThread == NULL) {
            DEBUGERR("IAMNtUserEnableIAMAccess_Hook: Can not start thread IAMWorkerWindowLoopThread");
            return bResOriginal;
        }
    }
    
    // Return the value that should be returned
    return bResOriginal;
}

// A function used to lure Explorer.EXE call NtUserEnableIAMAccess
_Success_(return == TRUE)
BOOL IAMInduceExplorer() {
    // Use API functions and FindWindow to obtain handles for the active window and taskbar, and determine null pointers
    HWND hWndForeground = GetForegroundWindow();
    HWND hWndTaskBar = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    HWND hWndDesktop = GetDesktopWindow();
    if (hWndForeground == NULL || hWndTaskBar == NULL || hWndDesktop == NULL) {
        DEBUGERR("IAMInduceExplorer: Window not found. [Foreground/Taskbar/Desktop]");
        return FALSE;
    }

    DEBUGLOG("IAMInduceExplorer: Start induce.");
    UINT nTryCount = 0;
    // Option 1: Call SetForegroundWindow
    for (nTryCount = 0; nTryCount < 3 && !bIAMKeySaved; nTryCount++) { // Try up to 3 times to avoid affecting users
        BOOL bSetForegroundSuccess = TRUE;
        DEBUGLOG("IAMInduceExplorer: Perform option 1");

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
        if (!bSetForegroundSuccess) DEBUGERR("IAMInduceExplorer: SetForegroundWindow failed. [Desktop]");
        Sleep(INDUCE_FOREGROUND_DELAY);

        // Set focus to taskbar, trigger recovery event
        bSetForegroundSuccess &= SetForegroundWindow(hWndTaskBar);
        if (!bSetForegroundSuccess) DEBUGERR("IAMInduceExplorer: SetForegroundWindow failed. [Taskbar]");
        Sleep(INDUCE_FOREGROUND_DELAY);

        // Restore the active window to avoid affecting users
        bSetForegroundSuccess &= SetForegroundWindow(hWndForeground);
        if (!bSetForegroundSuccess) DEBUGERR("IAMInduceExplorer: SetForegroundWindow failed. [Foreground]");
        Sleep(INDUCE_FOREGROUND_DELAY);
    }
    // Option 2: Toggle start menu (Ctrl + Esc)
    for (nTryCount = 0; nTryCount < 2 && !bIAMKeySaved; nTryCount++) { // Try up to 2 times to avoid affecting users
        INPUT Input = { 0 };
        DEBUGLOG("IAMInduceExplorer: Perform option 2");

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
        DEBUGLOG("IAMInduceExplorer: Perform option 3");

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
        DEBUGLOG("IAMInduceExplorer: Induce failed");
        return FALSE;
    }

    return TRUE;
}

// The main function that runs during loading
void CALLBACK IAMAttachMain(
    _In_opt_    ULONG_PTR lpParam
) {
    // Prevent duplicate loading of hooks
    if (bHookCreated == TRUE) return;

    // Init thread stop handler
    if (ThreadStop.Init() != TSH_SUCCESS) {
        DEBUGERR("IAMAttachMain: Can not init ThreadStopHandler");
        return;
    }
    
    // Temporarily load dynamic libraries and obtain addresses of undisclosed API functions
    HMODULE hUser32 = GetModuleHandle(TEXT("User32.DLL"));
    if (hUser32 == NULL) return;
    pGetWindowBand = (T_GetWindowBand)GetProcAddress(hUser32, "GetWindowBand"); 
    pCreateWindowInBand = (T_CreateWindowInBand)GetProcAddress(hUser32, "CreateWindowInBand");
    pCreateWindowInBandEx = (T_CreateWindowInBandEx)GetProcAddress(hUser32, "CreateWindowInBandEx");
    pSetWindowBand = (T_SetWindowBand)GetProcAddress(hUser32, "SetWindowBand");
    pNtUserEnableIAMAccess = (T_NtUserEnableIAMAccess)GetProcAddress(hUser32, MAKEINTRESOURCEA(2510)); // Import by ordinal
    if (
        pGetWindowBand == NULL || 
        pCreateWindowInBand == NULL || 
        pCreateWindowInBandEx == NULL || 
        pSetWindowBand == NULL || 
        pNtUserEnableIAMAccess == NULL) {
        DEBUGERR("IAMAttachMain: Can not get all function addresses");
        return;
    }

    // Attempt to read the saved IAM access key
    ULONGLONG nSavedIAMKey = 0;
    if (IAMKeyRead(&nSavedIAMKey)) {
        // If found, save the key and directly launch the window
        nIAMKey = nSavedIAMKey;
        bIAMKeySaved = TRUE;

        // Start the thread and start monitoring requests
        hCheckThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)IAMWorkerWindowLoopThread, NULL, NULL, 0);
        if (hCheckThread != NULL) return;

        // If failed, retrieve the key again
        DEBUGERR("IAMNtUserEnableIAMAccess_Hook: Can not start thread IAMWorkerWindowLoopThread when find IAM access key");
    }

    // Create API hooks using MHook and monitor NtUserEnableIAMAccess
    bHookCreated = TRUE; // Update State
    if (!Mhook_SetHook((PVOID*)&pNtUserEnableIAMAccess, IAMNtUserEnableIAMAccess_Hook)) {
        DEBUGERR("IAMAttachMain: Can not set API hook");
        return;
    }

    // Lure Explorer.EXE to call NtUserEnableIAMAccess, thereby triggering the hook
    if (!IAMInduceExplorer()) {
        DEBUGERR("IAMAttachMain: Can not induce Explorer.EXE");
        return;
    }

    return;
}

// The main function that runs during uninstallation
void CALLBACK IAMDetachMain(
    _In_opt_    ULONG_PTR lpParam
) {
    // Destroy the window
    LRESULT nRes = SendMessage(hWorkerWindow, WM_CLOSE, 0, 0);
    if (nRes != ERROR_SUCCESS) {
        DEBUGERR("IAMDetachMain: Can not send close message to IAMController window.");
        return;
    }

    // Uninit thread stop handler
    if (ThreadStop.StopAllThread() != TSH_SUCCESS) {
        DEBUGERR("IAMDetachMain: Can not stop threads.");
        return;
    }
    ThreadStop.Uninit();

    // If the hook is successfully created, use MHook to uninstall the API hook
    if (bHookCreated == TRUE) {
        if (!Mhook_Unhook((PVOID*)&pNtUserEnableIAMAccess)) {
            DEBUGERR("IAMDetachMain: Unhook hook error.");
            return;
        }
        bHookCreated = FALSE; // Update state
    }

    return;
}

BOOL APIENTRY DllMain(
    HMODULE hModule, 
    DWORD ul_reason_for_call, 
    LPVOID lpReserved
) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        SetLastError(0);
        DEBUGLOG("DllMain: DLL Already");

        // Set caller instance handle
        hParentInstance = (HINSTANCE)hModule;

        // Disable DLL_THREAD_ATTACH and DLL_THREAD_DETACH events to avoid multiple entries into the main function
        DisableThreadLibraryCalls(hParentInstance);

        // Call the attach main function by using APC
        if (!QueueUserAPC(IAMAttachMain, GetCurrentThread(), NULL)) {
            DEBUGLOG("DllMain: Can not queue APC while attacting");
            return FALSE;
        }

        // Put the process into an alertable state
        SleepEx(50, TRUE);

        break;
    }
    case DLL_PROCESS_DETACH: {
        // Call the detach main function by using APC
        if (!QueueUserAPC(IAMDetachMain, GetCurrentThread(), NULL)) {
            DEBUGLOG("DllMain: Can not queue APC while detacting");
            return FALSE;
        }

        // Put the process into an alertable state
        SleepEx(50, TRUE);

        DEBUGLOG("DllMain: DLL Unloaded");
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