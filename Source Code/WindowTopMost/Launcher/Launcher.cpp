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

#include "framework.h"
#include <windows.h>
#include <string>
#include <iostream>
#include "WindowTopMost.h"
#include "TaskDialogEx.h"
using namespace std;

#define COPYRIGHT "WindowTopMost Launcher by xmc0211"

#define MAKE(b) {szTitle = szWindowClass = #b; dwBand = b; CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MakeWindowBand, NULL, NULL, 0); }
#define CASE(b) case b: {*Zbid = b; *lpText = TEXT(#b); return TRUE; }

HINSTANCE hProcInstance;
std::_tstring szTitle; // Title bar text
std::_tstring szWindowClass; // Main window class name
ZBID dwCurrentBand;
BOOL isAlwaysOnTop = FALSE; // Is it always on top
INT X = 100, Y = 100;

BOOL DwordToZbid(DWORD dwBand, ZBID* Zbid, std::_tstring* lpText) {
	switch (dwBand) {
	CASE(ZBID_DEFAULT)
	CASE(ZBID_DESKTOP)
	CASE(ZBID_IMMERSIVE_RESTRICTED)
	CASE(ZBID_IMMERSIVE_BACKGROUND)
	CASE(ZBID_IMMERSIVE_INACTIVEDOCK)
	CASE(ZBID_IMMERSIVE_ACTIVEDOCK)
	CASE(ZBID_IMMERSIVE_ACTIVEMOBODY)
	CASE(ZBID_IMMERSIVE_APPCHROME)
	CASE(ZBID_IMMERSIVE_MOGO)
	CASE(ZBID_IMMERSIVE_SEARCH)
	CASE(ZBID_IMMERSIVE_INACTIVEMOBODY)
	CASE(ZBID_IMMERSIVE_NOTIFICATION)
	CASE(ZBID_IMMERSIVE_EDGY)
	CASE(ZBID_SYSTEM_TOOLS)
	CASE(ZBID_LOCK)
	CASE(ZBID_ABOVELOCK_UX)
	CASE(ZBID_IMMERSIVE_IHM)
	CASE(ZBID_GENUINE_WINDOWS)
	CASE(ZBID_UIACCESS)
	}
	*Zbid = ZBID_DEFAULT;
	*lpText = TEXT("ZBID_DEFAULT");
	return FALSE;
}

// Window message processing function
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_CLOSE: {
		DestroyWindow(hWnd);
		break;
	}
	case WM_DESTROY: {
		PostQuitMessage(0);
		break;
	}
	case WM_TIMER: {
		UINT uTimerID = (UINT)wParam;
		if (uTimerID == 1) {
			SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
	}
	default: return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void MakeWindowBand(LPVOID lpParam) {

	// Register window class
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hProcInstance;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass.c_str();
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&wcex);

	// Window initialization
	HWND hWnd = CreateWindowEx(NULL, szWindowClass.c_str(), szTitle.c_str(), WS_OVERLAPPEDWINDOW,
		X, Y, 500, 200, nullptr, nullptr, hProcInstance, nullptr);
	if (!hWnd) return;
	ShowWindow(hWnd, SW_SHOW);
	SetWindowBandEx(hWnd, 0, dwCurrentBand);
	if (isAlwaysOnTop) SetTimer(hWnd, 1, 20, NULL);
	UpdateWindow(hWnd);

	MSG msg;
	// Main message loop
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return;
}

void Show_CreateWindowInBand() {
	system("cls");
	cout << COPYRIGHT << endl;
	cout << R"(
=============================================================================
A = ZBID_DEFAULT                               Default Z segment（ZBID_DESKTOP）
B = ZBID_DESKTOP                               Normal window
P = ZBID_IMMERSIVE_RESTRICTED                  ** Generally not used **
M = ZBID_IMMERSIVE_BACKGROUND                  
J = ZBID_IMMERSIVE_INACTIVEDOCK                ** Generally not used **
I = ZBID_IMMERSIVE_INACTIVEMOBODY              ** Generally not used **
L = ZBID_IMMERSIVE_ACTIVEDOCK                  ** Generally not used **
K = ZBID_IMMERSIVE_ACTIVEMOBODY                ** Generally not used **
F = ZBID_IMMERSIVE_APPCHROME                   Task View (Win+Tab menu)
G = ZBID_IMMERSIVE_MOGO                        Start menu, taskbar
N = ZBID_IMMERSIVE_SEARCH                      Cortana, Windows search
E = ZBID_IMMERSIVE_NOTIFICATION                Operation Center (Notification Center)
H = ZBID_IMMERSIVE_EDGY                        
Q = ZBID_SYSTEM_TOOLS                          Top task manager, Alt+Tab menu
R = ZBID_LOCK (Win 10 up)                      Lock screen interface
S = ZBID_ABOVELOCK_UX (Win 10 up)              real-time playback window
D = ZBID_IMMERSIVE_IHM                         
O = ZBID_GENUINE_WINDOWS                       'Activate Windows' window
C = ZBID_UIACCESS                              Screen keyboard, magnifying glass
=============================================================================
The order of priority from low to high is (replicable): ABPMJILKFGNEHQRSDOC
Program will display windows in alphabetical order.
 >)";
	std::string Input;
	cin >> Input;
	cout << R"(Do you want to enable 'Window always at top'? (0/1)
Warning: If enabled, the ZBID_DESKTOP segment window may flicker with other windows such as 
ZBID_DEFAULT and affect system performance. Please be cautious when opening this option.
 >)";
	cin >> isAlwaysOnTop;
	cout << "Executing..." << endl;
	X = Y = 100;
	for (size_t Ptr = 0; Ptr < Input.size(); Ptr++) {
		Sleep(1000);
		ZBID Band;
		std::_tstring Text;
		if (!DwordToZbid(Input[Ptr] - 'A', &Band, &Text)) continue;
		szTitle = szWindowClass = Text;
		dwCurrentBand = Band;
		X += 32;
		Y += 32;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MakeWindowBand, 0, 0, 0);
	}
	return;
}

void Show_SetWindowBand() {
	system("cls");
	cout << COPYRIGHT << endl;
	cout << R"(
Press any key, and the program will select the window under the mouse after 5 seconds.

Press any key to continue...)";
	getchar(); getchar();
	cout << endl << "Please move your mouse to target window..." << endl << endl;
	Sleep(5000);
	POINT Pnt;
	GetCursorPos(&Pnt);
	HWND hWnd = WindowFromPoint(Pnt);
	DWORD dwBand = 0;
	ZBID Band;
	std::_tstring Message;
	if (hWnd) {
		system("cls");
		cout << COPYRIGHT << endl;
		cout << R"(
=============================================================================
A = ZBID_DEFAULT                               Default Z segment（ZBID_DESKTOP）
B = ZBID_DESKTOP                               Normal window
P = ZBID_IMMERSIVE_RESTRICTED                  ** Generally not used **
M = ZBID_IMMERSIVE_BACKGROUND                  
J = ZBID_IMMERSIVE_INACTIVEDOCK                ** Generally not used **
I = ZBID_IMMERSIVE_INACTIVEMOBODY              ** Generally not used **
L = ZBID_IMMERSIVE_ACTIVEDOCK                  ** Generally not used **
K = ZBID_IMMERSIVE_ACTIVEMOBODY                ** Generally not used **
F = ZBID_IMMERSIVE_APPCHROME                   Task View (Win+Tab menu)
G = ZBID_IMMERSIVE_MOGO                        Start menu, taskbar
N = ZBID_IMMERSIVE_SEARCH                      Cortana, Windows search
E = ZBID_IMMERSIVE_NOTIFICATION                Operation Center (Notification Center)
H = ZBID_IMMERSIVE_EDGY                        
Q = ZBID_SYSTEM_TOOLS                          Top task manager, Alt+Tab menu
R = ZBID_LOCK (Win 10 up)                      Lock screen interface
S = ZBID_ABOVELOCK_UX (Win 10 up)              real-time playback window
D = ZBID_IMMERSIVE_IHM                         
O = ZBID_GENUINE_WINDOWS                       'Activate Windows' window
C = ZBID_UIACCESS                              Screen keyboard, magnifying glass
=============================================================================
The order of priority from low to high is (replicable): ABPMJILKFGNEHQRSDOC
Program will set Z order band as the first input character.

INFO: Windows that have been upgraded ZBID may automatically top up.
 >)";
		std::string Input;
		std::_tstring Text = TEXT("");
		cin >> Input;
		if (Input.size() >= 1) {
			if (DwordToZbid(Input[0] - 'A', &Band, &Text)) {
				if (!SetWindowBandEx(hWnd, 0, Band)) {
					cout << "Sorry, Can not set window Z-order Band. (" << GetLastError() << ")" << endl;
				}
			}
			else cout << "Sorry, the provided character is invalid." << endl;
		}
		else cout << "Sorry, the provided string is empty." << endl;
	}
	else cout << "Sorry, the window handle is invaild." << endl;
	cout << R"(
Press any key to return...)";
	getchar(); getchar();
	return;
}

void Show_GetWindowBand() {
	system("cls");
	cout << COPYRIGHT << endl;
	cout << R"(
Press any key, and the program will display the ZBID of the window under the mouse after 5 seconds.

Press any key to continue...)";
	getchar(); getchar();
	cout << endl << "Please move your mouse to target window..." << endl << endl;
	Sleep(5000);
	POINT Pnt;
	GetCursorPos(&Pnt);
	HWND hWnd = WindowFromPoint(Pnt);
	DWORD dwBand = 0;
	ZBID Band;
	std::_tstring Message;
	if (hWnd) {
		GetWindowBandEx(hWnd, &dwBand);
		DwordToZbid(dwBand, &Band, &Message);
		cout << "=============================================================================" << endl <<
			"The WindowBand of this window is " << dwBand << " (" << LPT2LPC(Message) << ")" << endl;
		cout << "=============================================================================" << endl;
	}
	else {
		cout << "Sorry, the window handle is invaild." << endl;
	}
	cout << R"(
Press any key to return...)";
	getchar();
	return;
}

int main() {
	WTMInit();
	while (1) {
		INT opt;
		system("cls");
		cout << COPYRIGHT << endl;
		if (!WTMCheckEnvironment()) {
			cout << R"(
Sorry, your system is not supported!
+ Windows 7 and below versions do not support.
+ Please make sure to place IAMKeyHacker.DLL in the same directory as the application.
+ Ensure Explorer.EXE runs normally and stably when called, as it requires injecting DLL into Explorer.EXE performs operations.
)"; 
			cout << R"(
Press any key to exit...)";
			getchar();
			return 1;
		}
		cout << R"(
Select a function:
1. CreateWindowInBand
2. GetWindowBand
3. SetWindowBand
0. Exit
 >)";
		cin >> opt;
		if (opt == 0) break;
		if (opt == 1) Show_CreateWindowInBand();
		if (opt == 2) Show_GetWindowBand();
		if (opt == 3) Show_SetWindowBand();
	}
	WTMUninit();
	return 0;
}