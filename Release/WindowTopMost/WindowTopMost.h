// MIT License
//
// Copyright (c) 2025 WindowTopMost - xmc0211 <xmc0211@qq.com>xmc0211
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

// Warning:
// 1. Due to functional requirements, please make sure to use IAMKeyHacker Place the DLL in the same directory as the application.
// 2. Need to ensure Explorer EXE runs normally and stably when called, as it requires injecting DLL into Explorer EXE performs operations.
// 3. It is necessary to ensure that the application can end normally so that the injected DLL can be uninstalled. If it cannot be guaranteed, 
//    please call the WTMUninit function after not using the library function. (Can be called multiple times)
// 4. Do not call this library function too frequently, as the backend DLL may not be able to process messages.

#define WTMEXPORT __declspec(dllexport)
#define WTMIMPORT __declspec(dllimport)

#ifndef WTMDLLFILE
#define WTMAPI WTMIMPORT
#else
#define WTMAPI WTMEXPORT
#endif

#ifndef WTMCONFIG_H
#define WTMCONFIG_H

#include <Windows.h>

// Windows 7 and below versions do not support the use of ZBID. If you want to place the window at the forefront, you can periodically call SetWindowPos to place it at the top.
#if WINVER < 0x0800 || _WIN32_WINNT < 0x0800
#error Win7 and below versions are not applicable to this library. Please consider using 'SetWindowPos'.
#endif

// Some ZBID are only available for Windows 10 and above. Add the UNUSED_ prefix to ZBID that are not supported in the current version.
#if WINVER >= 0x0A00 || _WIN32_WINNT >= 0x0A00
#define WIN10UP(i) i
#else
#define WIN10UP(i) UNUSED_ ## i
#endif

#if defined(__cplusplus)
extern "C" {
#endif

// Z segments that can be used in Windows windows
// The default is ZBID_DESKTOP, with ZBID_UIACCESS at the top. The priority here is sorted from low to high.
enum ZBID : DWORD {
	ZBID_DEFAULT					 = 0,	// Default Z segment£¨ZBID_DESKTOP£©
	ZBID_DESKTOP					 = 1,	// Normal window
	ZBID_IMMERSIVE_RESTRICTED		 = 15,	// ** Generally not used **
	ZBID_IMMERSIVE_BACKGROUND		 = 12,
	ZBID_IMMERSIVE_INACTIVEDOCK		 = 9,	// ** Generally not used **
	ZBID_IMMERSIVE_INACTIVEMOBODY    = 8,	// ** Generally not used **
	ZBID_IMMERSIVE_ACTIVEDOCK		 = 11,	// ** Generally not used **
	ZBID_IMMERSIVE_ACTIVEMOBODY		 = 10,	// ** Generally not used **
	ZBID_IMMERSIVE_APPCHROME		 = 5,	// Task View (Win+Tab menu)
	ZBID_IMMERSIVE_MOGO				 = 6,	// Start menu, Taskbar
	ZBID_IMMERSIVE_SEARCH			 = 13,	// Cortana, Windows search
	ZBID_IMMERSIVE_NOTIFICATION		 = 4,	// Operation Center (Notification Center)
	ZBID_IMMERSIVE_EDGY				 = 7,
	ZBID_SYSTEM_TOOLS				 = 16,	// Top task manager, Alt+Tab menu
	WIN10UP(ZBID_LOCK)				 = 17,	// Lock screen interface
	WIN10UP(ZBID_ABOVELOCK_UX)		 = 18,	// Volume/brightness window in the upper left corner of the screen, real-time playback window
	ZBID_IMMERSIVE_IHM				 = 3,
	ZBID_GENUINE_WINDOWS			 = 14,	// 'Activate Windows' window
	ZBID_UIACCESS					 = 2,	// Screen keyboard, magnifying glass
};


#ifndef WTMDLLFILE

void WTMAPI ULLToByte(ULONG64 ulNum, BYTE Buffer[8]);
void WTMAPI ULToByte(ULONG32 ulNum, BYTE Buffer[4]);

// Check if Dll has been successfully injected
BOOL WTMIMPORT CheckForDll();

// The WindowTopMost initialization function is automatically called in DllMain or can be manually called for initialization.
BOOL WTMIMPORT WTMInit();

// The WindowTopMost uninstallation function is automatically called in DllMain or manually called to complete the uninstallation.
BOOL WTMIMPORT WTMUninit();

// Obtain UIAccess permission by restarting the application
// There is no need to manually call this function.
BOOL WTMAPI EnableUIAccess(BOOL bEnable);

// Create the top-level window using UIAccess and verify if the Z segment is correct. If an error occurs, return NULL.
// The application will restart.
HWND WTMIMPORT CreateTopMostWindowW(
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
);
HWND WTMIMPORT CreateTopMostWindowA(
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
);
#ifdef UNICODE
#define CreateTopMostWindow CreateTopMostWindowW
#else
#define CreateTopMostWindow CreateTopMostWindowA
#endif

// SetWindowBand that does not deny access.
// Regardless of whether the Z-sequence setting is successful or not, if an error occurs, return False, and if a message is successfully sent, return True
BOOL WTMIMPORT SetWindowBandEx(
	_In_opt_ HWND hWnd,
	_In_opt_ HWND hWndInsertAfter,
	_In_opt_ DWORD dwBand
);

// CreateWindowInBand will not deny access and will return NULL if an error occurs.
HWND WTMIMPORT CreateWindowInBandExW(
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
);
HWND WTMIMPORT CreateWindowInBandExA(
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
);
#ifdef UNICODE
#define CreateWindowInBandEx CreateWindowInBandExW
#else
#define CreateWindowInBandEx CreateWindowInBandExA
#endif

// GetWindowBand call entrance, and if an error occurs, returns False.
BOOL WTMIMPORT GetWindowBandEx(
	_In_ HWND hWnd,
	_In_ LPDWORD pdwBand
);

#endif

#if defined(__cplusplus)
}
#endif

#endif
