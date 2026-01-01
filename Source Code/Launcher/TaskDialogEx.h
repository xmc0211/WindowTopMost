// MIT License
//
// Copyright (c) 2025-2026 TaskDialogEx - xmc0211 <xmc0211@qq.com>
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

// * Commctrl 6.0.0.0 can be disabled by #define NOUSING_CEW_CMMCTRLPRAGMA. (may cause exceptions, not recommended)
// * You can disable the automatic execution of commands when clicking hyperlinks by using #define NOHYPERLINK_AUTORUN.

#ifndef TASKDIALOGEX_H
#define TASKDIALOGEX_H

#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <string>
#include <map>
#include "Convert.h"
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Shlwapi.lib")

#ifndef NOUSING_NEW_COMMCTRL_PRAGMA
	#pragma comment(linker,"\"/manifestdependency:type='win32' \
							name = 'Microsoft.Windows.Common-Controls' \
							version = '6.0.0.0' \
							processorArchitecture = '*' \
							publicKeyToken = '6595b64144ccf1df' \
							language = '*'\"")
#endif

#define __RND(l, r) (rand() % (r - l + 1) + l)

#define __REMOVE(ORIG, OPT) (ORIG & (~OPT))
#define __HAVE(ORIG, OPT) (bool)(ORIG & OPT)
#define __ADD(ORIG, OPT) (ORIG | OPT)

const INT __TD_MAX_BUTTON_NUM = 10;
const INT __TD_MAX_RBUTTON_NUM = 10;
const INT __TD_TERMINATE_TIMEOUT = 0;
const INT __TASKDIALOG_CBFUNC_CORRECTARGS[11] = {0, 0, 1, 2, 1, 0, 1, 0, 1, 0, 1};

enum TASKDIALOGMAINICON {
	TDMI_WARNING = -1,
	TDMI_ERROR = -2,
	TDMI_INFORMATION = -3,
	TDMI_SHIELD = -4, 
	TDMI_SHIELD_BLUE = -5,
	TDMI_SHIELDWARNING_YELLOW = -6,
	TDMI_SHIELDERROR_RED = -7,
	TDMI_SHIELDSUCCESS_GREEN = -8,
	TDMI_SHIELD_GREY = -9
};
enum TASKDIALOGFOOTERICON {
	TDFI_WARNING = -1,
	TDFI_ERROR = -2,
	TDFI_INFORMATION = -3,
	TDFI_SHIELD = -4,
	TDFI_SHIELDWARNING = -6,
	TDFI_SHIELDERROR = -7,
	TDFI_SHIELDSUCCESS = -8,
};
enum TASKDIALOGFLAGOPTION {
	TDF_ENABLE = 1,
	TDF_DISABLE = 0
};

typedef void (CALLBACK* TASKDIALOG_FNOPARAM)(HWND hWnd, LPVOID vArg);
typedef void (CALLBACK* TASKDIALOG_FONEPARAM)(HWND hWnd, LONG_PTR uiP1, LPVOID vArg);
typedef void (CALLBACK* TASKDIALOG_FTWOPARAM)(HWND hWnd, LONG_PTR uiP1, LONG_PTR uiP2, LPVOID vArg);

// The parameter structure of some callback functions
struct __CBFUNCARG {
	LONG_PTR lpClass;
	LONG_PTR vUserArg;
	// Constructor function
	__CBFUNCARG();
};

// Universal callback function structure
struct TASKDIALOGCBFUNC {
	int type;
	TASKDIALOG_FNOPARAM nop;
	TASKDIALOG_FONEPARAM op;
	TASKDIALOG_FTWOPARAM tp;
	// Constructor function
	TASKDIALOGCBFUNC();
	TASKDIALOGCBFUNC(TASKDIALOG_FNOPARAM func);
	TASKDIALOGCBFUNC(TASKDIALOG_FONEPARAM func);
	TASKDIALOGCBFUNC(TASKDIALOG_FTWOPARAM func);
	// Destructor function
	~TASKDIALOGCBFUNC();

	// Call existing callback functions
	void Execute(HWND hWnd, LONG_PTR uiP1 = 0, LONG_PTR uiP2 = 0, LONG_PTR vArg = NULL);
};

// Used to store temporary data for TASKDIALOGEX
struct TASKDIALOGTMP {
	HWND hWnd;
	HANDLE hThread;
	LPVOID lpThreadThis;
	INT tdbc, tdrbc;
	INT iButtonID, iRadioIndex;
	INT iX, iY;
	LONG lThResult;
	BOOL bCheckboxIsOK;
	BOOL bOnTop;
	std::wstring WindowTitle, Content, MainInstruction, CheckboxText, Footer;
	std::wstring ExpText, ExpOpenText, ExpCloseText;
	std::wstring ButtonName[__TD_MAX_BUTTON_NUM + 1], RButtonName[__TD_MAX_RBUTTON_NUM + 1];
	TASKDIALOG_BUTTON tdb[__TD_MAX_BUTTON_NUM + 1], tdrb[__TD_MAX_RBUTTON_NUM + 1];
	HICON hMainIcon, hFooterIcon;
	TASKDIALOG_FNOPARAM fCreated;
	TASKDIALOG_FNOPARAM fDestroyed;
	TASKDIALOG_FONEPARAM fBClicked;
	__CBFUNCARG vCBFArg;
	std::map <std::_tstring, std::_tstring> Link;
	// Constructor function
	TASKDIALOGTMP();
	// Copy constructor function
	TASKDIALOGTMP(const TASKDIALOGTMP& other);
	// Destructor function
	~TASKDIALOGTMP();
};

// Encapsulation of TASKDIALOGCONIG
class TASKDIALOGEX {
protected:
	TASKDIALOGCONFIG tdc;
	TASKDIALOGCBFUNC cbf[11];
public:
	TASKDIALOGTMP tmp;
protected:
	// Refresh information
	void Update();
	// Preset callback function
	static HRESULT CALLBACK __cbFunc(HWND hWnd, UINT uiNoti, WPARAM wParam, LPARAM lParam, LONG_PTR lpArg);
	// Preset thread running function, using LPVOID as the instance pointer of the class
	static DWORD WINAPI __thFunc(LPVOID lpParam);
public:
	// Constructor function
	TASKDIALOGEX();
	// Copy constructor function
	TASKDIALOGEX(const TASKDIALOGEX& other);
	// Destructor function
	~TASKDIALOGEX();

	TASKDIALOGCONFIG GetCurrentConfig();
	TASKDIALOGCBFUNC GetCurrentCBFunc(INT iIndx);

	/**** Pre run functions ***/
	// Set properties (set or remove multiple available '|' connections)
	void SetAttrib(INT iFlag, TASKDIALOGFLAGOPTION iOpt = TDF_ENABLE);
	// Set window width
	void SetWidth(UINT uiWidth);
	// Set window title
	void SetWindowTitle(std::_tstring lpcWindowTitle);
	// Set content
	void SetContent(std::_tstring lpcContent);
	// Set main instruction
	void SetMainInstruction(std::_tstring lpcMainIntruction);
	// Set main icon (using preset: TDMI_*)
	void SetMainIcon(TASKDIALOGMAINICON iIconID);
	// Set main icon (custom, HICON)
	void SetMainIcon(HICON hIcon);
	// Set main icon (custom, specify DLL name and icon index)
	void SetMainIcon(std::_tstring lpcDllName, INT iIconIndx);
	// Set bottom text body
	void SetFooter(std::_tstring lpcFooter);
	// Set bottom text icon (using preset : TDFI_*)
	void SetFooterIcon(TASKDIALOGFOOTERICON iIconID);
	// Set bottom text icon (custom, HICON)
	void SetFooterIcon(HICON hIcon);
	// Set bottom text icon (custom, specify DLL name and icon index)
	void SetFooterIcon(std::_tstring lpcDllName, INT iIconIndx);
	// Add button (using preset)
	void AddButton(INT tcbfSet);
	// Add button (custom name)
	void AddButton(INT iButtonID, std::_tstring lpcName, std::_tstring lpcCont = TEXT(""));
	// Set default button
	void SetDefaultButton(INT iButtonIndx);
	// Add radio button
	void AddRadioButton(INT iButtonID, std::_tstring lpcName);
	// Set default radio button
	void SetDefaultRadioButton(INT iButtonIndx);
	// Set checkbox text
	void SetCheckboxText(std::_tstring lpcCheckboxText);
	// Set more information text, open prompts, and retract prompts
	void SetExpanded(std::_tstring lpcText, std::_tstring lpcOpenText, std::_tstring lpcCloseText);
	// Set the callback function corresponding to the message
	void SetCallbackFunc(UINT uiNoti, TASKDIALOGCBFUNC fCBFunc);
	// Set the parameter pointer passed to the callback function
	void SetCallbackArg(LPVOID vArg);
	// Set the initial coordinates of the window
	void SetPoint(INT iX, INT iY);
	// Set whether the window is at the top
	void SetIsOnTop(BOOL bIsOnTop);
	// Create hyperlink (ID is an identifier used only for callback functions to distinguish hyperlinks)
	std::_tstring CreateHyperlink(std::_tstring lpId, std::_tstring lpText, std::_tstring lpCmd = TEXT(""));

	/*** Runtime function ***/
	// Display
	LONG Show();
	// Display in new thread
	void ShowInNewThread();
	// Waiting for the open thread to end and obtaining the return value
	LONG WaitThreadEnd() const;
	// Get window handle (NULL before display)
	HWND GetHWND() const;
	// Send a message to the displayed window (only for window in new threads)
	void SendWndMessage(UINT uiMsg, WPARAM wParam = 0, LPARAM lParam = 0) const;
	// Determine if the window is still running (only for window in new threads)
	BOOL IsActive() const;
	// Place the window at the most top level
	void TopMost() const;

	/**** After running the function ****/
	// Obtain the ID of the pressed button
	INT GetButtonID() const;
	// Obtain the ID of the selected radio button
	INT GetRadioButtonID() const;
	// Get checkbox selection message (FALSE:not selected, TRUE:selected)
	BOOL GetCheckboxState() const;
};

// Quick call in the form of a Message Box
INT TaskDialogEx(std::_tstring MainInstruction, std::_tstring Content, TASKDIALOGMAINICON MainIcon, INT Buttons);

#endif


