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

#include "Framework.h"
#include <windows.h>
#include <string>
#include <iostream>
#include <cassert>
#include "../Common Headers/ThreadStopHandler.h"
#include "../Common Headers/WindowTopMost.h"
#include "Convert.h"
using namespace std;

#define COPYRIGHT "WindowTopMost Launcher by xmc0211"

#define MAKE(b) {szTitle = szWindowClass = #b; dwBand = b; CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MakeWindowBand, NULL, NULL, 0); }
#define CASE(b) case b: {*Zbid = b; *lpText = TEXT(#b); return TRUE; }
#undef max
#undef min

HINSTANCE hProcInstance;
CRITICAL_SECTION hCurrentWindowSection;
HANDLE hCurrentWindowFinishEvent;
HWND hLastWnd = NULL;
struct _CurrentWindowInformation {
	std::_tstring szTitle; // Title bar text
	std::_tstring szWindowClass; // Main window class name
	ZBID dwCurrentBand;
	BOOL bIsAlwaysOnTop; // Is it always on top
	INT X, Y;
	DWORD nFlag;
	LONGLONG nThreadCount;
	_CurrentWindowInformation() : dwCurrentBand(ZBID_DEFAULT), bIsAlwaysOnTop(FALSE), X(0), Y(0), nFlag(0), nThreadCount(0) {}
} CurrentWindow;

void Pause() {
	getchar();
}

DWORD PrintErrorMessage(DWORD ErrorCode) {
	if (ErrorCode == -1) return 0;
	if (WTMIsWTMError(ErrorCode)) {
#ifdef UNICODE
		wcout << std::wstring(WTMGetErrorDescription(ErrorCode));
#else
		cout << std::string(WTMGetErrorDescription(ErrorCode));
#endif
		return 0;
	}
	LPSTR buffer = nullptr;
	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS;

	DWORD result = ::FormatMessageA(
		flags,
		nullptr,
		ErrorCode,
		0x0409,
		reinterpret_cast<LPSTR>(&buffer),
		0,
		nullptr
	);

	std::string message;
	if (result > 0 && buffer) {
		message.assign(buffer);
		LocalFree(buffer);
	}
	else {
		message = "Unknown error " + std::to_string(ErrorCode) + ".";
	}
	cout << message;
	return ErrorCode;
}

BOOL DwordToZbid(DWORD dwBand, ZBID* Zbid, std::_tstring* lpText) {
	switch (dwBand) {
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
	EnterCriticalSection(&hCurrentWindowSection);

	// Register window class
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW/* | CS_GLOBALCLASS*/;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hProcInstance;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = CurrentWindow.szWindowClass.c_str();
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	RegisterClassEx(&wcex);

	// Window initialization

	HWND hWnd = CreateWindowEx(NULL, CurrentWindow.szWindowClass.c_str(), CurrentWindow.szTitle.c_str(), WS_OVERLAPPEDWINDOW,
		CurrentWindow.X, CurrentWindow.Y, 500, 200, nullptr, nullptr, hProcInstance, nullptr);
	if (!hWnd) {
		DWORD nError = GetLastError();
		cout << endl << "Sorry, Can not create window in Z-order Band " << LPT2LPC(CurrentWindow.szWindowClass) << ". (" << nError << ")" << endl;
		PrintErrorMessage(nError);
		cout << endl;

		InterlockedDecrement64(&CurrentWindow.nThreadCount);
		if (CurrentWindow.nThreadCount == 0) SetEvent(hCurrentWindowFinishEvent);
		LeaveCriticalSection(&hCurrentWindowSection);

		return;
	}
	ShowWindow(hWnd, SW_SHOW);
	WTMSetWindowBand(hWnd, 0, CurrentWindow.dwCurrentBand);
	if (CurrentWindow.bIsAlwaysOnTop) SetTimer(hWnd, 1, 20, NULL);
	UpdateWindow(hWnd);

	// If call original API, Explorer.EXE can not find the window class. I don't know why?
	/*HWND hWnd = WTMCreateWindowInBand(NULL, CurrentWindow.szWindowClass.c_str(), CurrentWindow.szTitle.c_str(), WS_OVERLAPPEDWINDOW,
		CurrentWindow.X, CurrentWindow.Y, 500, 200, nullptr, nullptr, hProcInstance, nullptr, CurrentWindow.dwCurrentBand);
	if (!hWnd) {
		DWORD nError = GetLastError();
		cout << endl << "Sorry, Can not create window in Z-order Band " << CurrentWindow.dwCurrentBand << ". (" << nError << ")" << endl;
		PrintErrorMessage(nError);
		cout << endl;

		CurrentWindow.bHasError = TRUE;
		CurrentWindow.ThreadStop.LeaveThread();
		return;
	}
	ShowWindow(hWnd, SW_SHOW);
	if (CurrentWindow.bIsAlwaysOnTop) SetTimer(hWnd, 1, 20, NULL);
	UpdateWindow(hWnd);*/

	InterlockedDecrement64(&CurrentWindow.nThreadCount);
	if (CurrentWindow.nThreadCount == 0) SetEvent(hCurrentWindowFinishEvent);
	LeaveCriticalSection(&hCurrentWindowSection);

	// Main message loop
	MSG Msg;
	while (GetMessage(&Msg, nullptr, 0, 0)) {
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	
	return;
}

void PrintZBIDs() {
	cout << R"(
Please input Z-order Band:
+---------------------------------------------------------------------------------------+
|     ZBID_DEFAULT                       [0]     Default Z segment (just means default) |
| B = ZBID_DESKTOP                       [1]     Normal window                          |
| P = ZBID_IMMERSIVE_RESTRICTED          [15]    ** Generally not used **               |
| M = ZBID_IMMERSIVE_BACKGROUND          [12]                                           |
| J = ZBID_IMMERSIVE_INACTIVEDOCK        [9]     ** Generally not used **               |
| I = ZBID_IMMERSIVE_INACTIVEMOBODY      [8]     ** Generally not used **               |
| L = ZBID_IMMERSIVE_ACTIVEDOCK          [11]    ** Generally not used **               |
| K = ZBID_IMMERSIVE_ACTIVEMOBODY        [10]    ** Generally not used **               |
| F = ZBID_IMMERSIVE_APPCHROME           [5]     Task View (Win+Tab menu)               |
| G = ZBID_IMMERSIVE_MOGO                [6]     Start menu, taskbar                    |
| N = ZBID_IMMERSIVE_SEARCH              [13]    Cortana, Windows search                |
| E = ZBID_IMMERSIVE_NOTIFICATION        [4]     Operation Center (Notification Center) |
| H = ZBID_IMMERSIVE_EDGY                [7]                                            |
| Q = ZBID_SYSTEM_TOOLS                  [16]    Top task manager, Alt+Tab menu         |
| R = ZBID_LOCK (Win 10 up)              [17]    Lock screen interface                  |
| S = ZBID_ABOVELOCK_UX (Win 10 up)      [18]    real-time playback window              |
| D = ZBID_IMMERSIVE_IHM                 [3]                                            |
| O = ZBID_GENUINE_WINDOWS               [14]    'Activate Windows' window              |
| C = ZBID_UIACCESS                      [2]     Screen keyboard, magnifying glass      |
+---------------------------------------------------------------------------------------+
The order of priority from low to high is (replicable): BPMJILKFGNEHQRSDOC
Program will set Z-order band as the input string.)" << endl;
}

void PrintWindowZBID(HWND hWnd) {
	DWORD dwBand = 0;
	ZBID Band;
	std::_tstring Message;
	if (hWnd) {
		if (!WTMGetWindowBand(hWnd, &dwBand)) {
			cout << "Sorry, we can not get the Z-Order band about this window." << endl;
		}
		else {
			DwordToZbid(dwBand, &Band, &Message);
			cout << endl << "=============================================================================" << endl <<
				"The WindowBand of this window is " << dwBand << " (" << LPT2LPC(Message) << ")" <<
				endl << "=============================================================================" << endl;
		}
	}
	else {
		cout << "Sorry, the window handle is invaild." << endl;
	}
}

void Show_SetWindowBand() {
	system("cls");
	cout << COPYRIGHT << endl;
	BOOL bUseLast = FALSE;
	if (IsWindow(hLastWnd)) {
		cout << R"(
Do you want to use last selected window? (0/1)
 >)";
		cin >> bUseLast;
		cin.ignore(numeric_limits<streamsize>::max(), '\n');
	}
	HWND hWnd = hLastWnd;
	if (bUseLast && !IsWindow(hLastWnd)) {
		bUseLast = FALSE; // Check again
		cout << R"(
Sorry, last selected window is invalid. Please select new window.
)";
	}
	if (!bUseLast) {
		cout << R"(
Press any key, and the program will select the window under the mouse after 5 seconds.

Press any key to continue...)";
		Pause();
		cout << endl << "Please move your mouse to target window..." << endl << endl;
		Sleep(5000);
		POINT Pnt;
		GetCursorPos(&Pnt);
		hWnd = WindowFromPoint(Pnt);
	}
	DWORD dwBand = 0;
	ZBID Band;
	std::_tstring Message;
	if (hWnd) {
		system("cls");
		cout << COPYRIGHT << endl;
		PrintZBIDs();
		cout << R"( >)";
		std::string Input;
		BOOL bBringToTop = FALSE;
		std::_tstring Text = TEXT("");
		cin >> Input;
		cin.ignore(numeric_limits<streamsize>::max(), '\n');
		cout << R"(
Do you want this window to be at the top? (0/1)
 >)";
		cin >> bBringToTop;
		cin.ignore(numeric_limits<streamsize>::max(), '\n');
		if (Input.size() >= 1) {
			if (DwordToZbid(Input[0] - 'A', &Band, &Text)) {
				if (!WTMSetWindowBand(hWnd, 0, Band)) {
					DWORD nError = GetLastError();
					cout << endl << "Sorry, Can not set window Z-order band. (" << GetLastError() << ")" << endl;
					PrintErrorMessage(nError);
					cout << endl;
				}
				else {
					if (bBringToTop) SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
					else SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
					cout << "Set window Z-order band success." << endl;
				}
			}
			else cout << "Sorry, the provided character is invalid." << endl;
		}
		else cout << "Sorry, the provided string is empty." << endl;
	}
	else cout << "Sorry, the window handle is invaild." << endl;
	cout << R"(
Press any key to return...)";
	Pause();
	return;
}

void Show_GetWindowBand() {
	system("cls");
	cout << COPYRIGHT << endl;
	BOOL bUseLast = FALSE;
	if (IsWindow(hLastWnd)) {
		cout << R"(
Do you want to use last selected window? (0/1)
 >)";
		cin >> bUseLast;
		cin.ignore(numeric_limits<streamsize>::max(), '\n');
	}
	HWND hWnd = hLastWnd;
	if (bUseLast && !IsWindow(hLastWnd)) {
		bUseLast = FALSE; // Check again
		cout << R"(
Sorry, last selected window is invalid. Please select new window.
)";
	}
	if (!bUseLast) {
		cout << R"(
Press any key, and the program will display the ZBID of the window under the mouse after 5 seconds.

Press any key to continue...)";
		Pause();
		cout << endl << "Please move your mouse to target window..." << endl << endl;
		Sleep(5000);
		POINT Pnt;
		GetCursorPos(&Pnt);
		hWnd = WindowFromPoint(Pnt);
	}
	PrintWindowZBID(hWnd);
	hLastWnd = hWnd;
	cout << R"(
Press any key to return...)";
	Pause();
	return;
}

void Show_CreateWindowInBand() {
	system("cls");
	cout << COPYRIGHT << endl;

	PrintZBIDs();
	cout << R"( >)";
	std::string Input;
	cin >> Input;
	cin.ignore(numeric_limits<streamsize>::max(), '\n');

	cout << R"(Do you want to enable 'Window always at top'? (0/1)
Warning: If enabled, the ZBID_DESKTOP segment window may flicker with other windows such as 
ZBID_IMMERSIVE_RESTRICTED and affect system performance. Please be cautious when opening 
this option.
 >)";
	cin >> CurrentWindow.bIsAlwaysOnTop;
	cin.ignore(numeric_limits<streamsize>::max(), '\n');

	CurrentWindow.nThreadCount = 0;
	for (size_t Ptr = 0; Ptr < Input.size(); Ptr++) {
		ZBID Band;
		std::_tstring Text;
		if (DwordToZbid(Input[Ptr] - 'A', &Band, &Text)) CurrentWindow.nThreadCount++;
	}

	if (CurrentWindow.nThreadCount) {
		CurrentWindow.X = CurrentWindow.Y = 100;
		hCurrentWindowFinishEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (hCurrentWindowFinishEvent == NULL) return;
		InitializeCriticalSection(&hCurrentWindowSection);

		cout << "Executing..." << endl;

		for (size_t Ptr = 0; Ptr < Input.size(); Ptr++) {
			Sleep(1000);
			ZBID Band;
			std::_tstring Text;
			if (!DwordToZbid(Input[Ptr] - 'A', &Band, &Text)) continue;

			EnterCriticalSection(&hCurrentWindowSection); // Protect global variables using critical section
			CurrentWindow.szTitle = CurrentWindow.szWindowClass = Text;
			CurrentWindow.dwCurrentBand = Band;
			CurrentWindow.X += 32;
			CurrentWindow.Y += 32;
			LeaveCriticalSection(&hCurrentWindowSection);

			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MakeWindowBand, 0, 0, 0);
		}

		WaitForSingleObject(hCurrentWindowFinishEvent, INFINITE);
		DeleteCriticalSection(&hCurrentWindowSection);
		CloseHandle(hCurrentWindowFinishEvent);
	}

	cout << R"(
Press any key to continue...)";
	Pause();

	return;
}

void Show_GetIAMKey() {
	system("cls");
	cout << COPYRIGHT << endl;
	ULONGLONG nIAMKey = 0;
	if (!WTMGetIAMKey(&nIAMKey)) {
		DWORD nError = GetLastError();
		cout << endl << "Sorry, Can not get IAM access key. (" << nError << ")" << endl;
		PrintErrorMessage(nError);
		cout << endl;
		cout << R"(
Press any key to return...)";
		Pause();
		return;
	}
	cout << endl << "=============================================================================" << endl <<
		"The IAM access key is " << hex << nIAMKey << dec << " (" << nIAMKey << ")" << 
			endl << "=============================================================================" << endl;
	cout << R"(
Press any key to return...)";
	Pause();
	return;
}


int UserInputMain() {
	system("cls");
	cout << COPYRIGHT << endl;
	if (!WTMCheckEnvironment()) {
		cout << R"(
Sorry, your system is not supported by this program.
+ Windows 7 and below versions do not support.
+ Please make sure to place IAMKeyHacker.DLL in the same directory as the application.
+ Ensure Explorer.EXE runs normally and stably when called, as it requires injecting IAMWorker into Explorer.EXE performs operations.
)";
		cout << R"(
Press any key to exit...)";
		Pause();
		return 1;
	}
	if (!WTMInit()) {
		cout << R"(
Sorry, we can not init IAMController.
An error may have occurred during initialization or it may have been intercepted by security softwares.
)";
		cout << R"(
Press any key to exit...)";
		Pause();
		return 1;
	}
	DWORD Result = 0;
	while (1) {
		INT opt;

		// Try to get IAM key to determine whether IAMWorker is working properly
		ULONGLONG nIAMKey;
		if (!WTMGetIAMKey(&nIAMKey)) {
			cout << R"(
Sorry, we can not connect to IAMWorker.
An error may have occurred during initialization or it may have been intercepted by security softwares.
)";
			cout << R"(
Press any key to exit...)";
			Pause();
			Result = 1;
			break;
		}

		cout << R"(
Select a function:
1. GetWindowBand
2. SetWindowBand
3. CreateWindowInBand
4. GetIAMKey
5. IAMController reload [Only for use in emergency situations]
0. Exit
 >)";
		cin >> opt;
		cin.ignore(numeric_limits<streamsize>::max(), '\n');
		if (opt == 0) {
			Result = 0;
			break;
		}
		if (opt == 1) Show_GetWindowBand();
		if (opt == 2) Show_SetWindowBand();
		if (opt == 3) Show_CreateWindowInBand();
		if (opt == 4) Show_GetIAMKey();
		if (opt == 5) {
			if (!WTMUninit()) {
				DWORD nError = GetLastError();
				cout << endl << "Sorry, we can not uninit IAMController. (" << nError << ")" << endl;
				PrintErrorMessage(nError);
				cout << endl;
			}
			else {
				if (!WTMInit()) {
					DWORD nError = GetLastError();
					cout << endl << "Sorry, we can not init IAMController again. (" << nError << ")" << endl;
					PrintErrorMessage(nError);
					cout << endl;
				}
				else {
					cout << endl << "Reload IAMController success." << endl;
				}
			}
			cout << R"(
Press any key to continue...)";
			Pause();
		}
		system("cls");
		cout << COPYRIGHT << endl;
		if (!WTMCheckEnvironment()) {
			cout << R"(
Sorry, abnormal environment state was found during operation.
Please ensure Explorer.EXE runs normally and stably.
)";
			cout << R"(
Press any key to exit...)";
			Pause();
			Result = 1;
			break;
		}
	}
	if (!WTMUninit()) {
		cout << R"(
Sorry, we can not uninit IAMController.
An error may have occurred during uninitialization.
You may need to restart the program to uninstall the remaining IAMWorker in Explorer.EXE.
)";
		cout << R"(
Press any key to exit...)";
		Pause();
		return 1;
	}
	return Result;
}

DWORD CommandLineMain(int argc, char* argv[]) {
	if (argc <= 1) return 0x57;
	if (!strcmp(argv[1], "-?") || !strcmp(argv[1], "/?") || !strcmp(argv[1], "-h") || !strcmp(argv[1], "/h") || !strcmp(argv[1], "-help") || !strcmp(argv[1], "--help")) {
		if (argc != 2) return 0x57;

		// Output help

		cout << COPYRIGHT << endl;
		cout << R"(
Set/Get Window Z-order Band.

Launcher.EXE [/?] [/Set | /Get] hWnd-Number <arguments>

    /?      Show this help message.

    /Set    Set window Z-order band.
            Prompt:   Launcher.EXE /Set hWnd-Number hWndInsertAfter-Number dwBand [/Top]
            Output: Error information.

    /Get    Get window Z-order band.
            Prompt:
                Launcher.EXE /Get hWnd-Number
            Output: Window Z-order band. (or Error information)

	/Unload	Attempt to uninstall IAMWorker remaining in Explorer.EXE due to crash or other reason

    Numbers are represented as unsigned decimal integers.

Examples:
Launcher.EXE /?
Launcher.EXE /Set 1234567 0 16 /Top
Launcher.EXE /Set 1000090 0 1
Launcher.EXE /Get 1020304
Launcher.EXE /Unload
)";
		return -1;
	}
	if (!WTMCheckEnvironment()) {
		return 0x32;
	}
	if (!strcmp(argv[1], "/Unload") || !strcmp(argv[1], "-Unload") || !strcmp(argv[1], "/unload") || !strcmp(argv[1], "-unload")) {
		WTMUninit();
		return GetLastError();
	}
	if (argc < 3) return 0x57;
	HWND hWnd = NULL;
	try {
		UINT hWndValue = std::stoul(argv[2]);
		hWnd = reinterpret_cast<HWND>(static_cast<UINT_PTR>(hWndValue));
	}
	catch (const std::exception& e) {
		return 0x57;
	}
	if (!strcmp(argv[1], "/Get") || !strcmp(argv[1], "-Get") || !strcmp(argv[1], "/get") || !strcmp(argv[1], "-get")) {
		if (argc != 3) return 0x57;
		if (!IsWindow(hWnd)) return 0x578;

		// GetWindowBand

		SetLastError(0);
		DWORD WindowBand = 0;
		if (!WTMGetWindowBand(hWnd, &WindowBand)) return GetLastError();
		cout << WindowBand << endl;

		return -1;
	}
	else if (!strcmp(argv[1], "/Set") || !strcmp(argv[1], "-Set") || !strcmp(argv[1], "/set") || !strcmp(argv[1], "-set")) {
		if (!(argc == 5 || (argc == 6 && (!strcmp(argv[5], "/Top") || !strcmp(argv[5], "-Top") || !strcmp(argv[5], "/top") || !strcmp(argv[5], "-top"))))) return 0x57;
		if (!IsWindow(hWnd)) return 0x578;
		HWND hWndInsertAfter = 0;
		DWORD dwBand = 0;
		try {
			UINT hWndValue = std::stoul(argv[3]);
			hWndInsertAfter = reinterpret_cast<HWND>(static_cast<UINT_PTR>(hWndValue));
		}
		catch (const std::exception& e) {
			return 0x57;
		}
		try {
			dwBand = std::stoul(argv[4]);
		}
		catch (const std::exception& e) {
			return 0x57;
		}

		// SetWindowBand

		if (!WTMInit()) return 0x54F;
		// Try to get IAM key to determine whether IAMWorker is working properly
		ULONGLONG nIAMKey;
		if (!WTMGetIAMKey(&nIAMKey)) return 0x54F;

		WTMSetWindowBand(hWnd, hWndInsertAfter, dwBand);
		DWORD dwTempError = GetLastError();
		if (!WTMUninit()) return 0x54F;
		if (argc == 5) SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		else SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		return dwTempError;
	}
	return 0x57;
}

int main(int argc, char* argv[]) {
	HANDLE hMutex = CreateMutex(NULL, FALSE, _T("Global\\WindowTopMost_LauncherRunning_D92F50F2_D1CB_41FD_9F26_EDFAE2D0292C"));
	if (!hMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
		cout << R"(
Sorry, another Launcher.EXE is currently running.
)";
		cout << R"(
Press any key to exit...)";
		Pause();
		return 0;
	}
	INT Result = 0;
	if (argc > 1) Result = PrintErrorMessage(CommandLineMain(argc, argv));
	else Result = UserInputMain();
	ReleaseMutex(hMutex);
	CloseHandle(hMutex);
	return Result;
}