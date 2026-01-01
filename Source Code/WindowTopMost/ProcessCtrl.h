// MIT License
//
// Copyright (c) 2025-2026 ProcessCtrl - xmc0211 <xmc0211@qq.com>
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

#ifndef PROCESSCTRL_H
#define PROCESSCTRL_H

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <string>

#if defined(UNICODE)
#define _tstring wstring
#else
#define _tstring string
#endif

// Process and thread information
struct PROCESS {
	DWORD Pid;
	std::_tstring Name; // process name
	std::_tstring Path; // file path
	std::_tstring AccountName; // account name
	std::_tstring DomainName; // domain name

	PROCESSENTRY32 PE;

	// Constructor function
	PROCESS();
	// Destructor function
	~PROCESS();
};
struct THREAD {
	DWORD Tid;
	DWORD ParentPid;

	THREADENTRY32 TE;

	// Constructor function
	THREAD();
	// Destructor function
	~THREAD();
};

typedef BOOL(CALLBACK* PROC_ENUM_CALLBACK)(PROCESS Tsk, LPVOID lParam);
typedef BOOL(CALLBACK* THRE_ENUM_CALLBACK)(THREAD Tre, LPVOID lParam);
typedef BOOL(CALLBACK* WND_ENUM_CALLBACK)(HWND hWnd, LPVOID lParam);

// Specify callback function to enumerate all processes. The callback function returning false will cause the entire function to return false.
BOOL EnumProc(PROC_ENUM_CALLBACK fCb, LPVOID lpParam);
// List all child threads of the specified process. The callback function returning false will cause the entire function to return false.
BOOL EnumThre(DWORD dwPid, THRE_ENUM_CALLBACK fCb, LPVOID lpParam);
// List all windows of the specified process. The callback function returning false will cause the entire function to return false.
BOOL EnumWnd(DWORD dwPid, WND_ENUM_CALLBACK fCb, LPVOID lpParam);

// Find the first Pid (smallest Pid) using the process name
BOOL FindPid(std::_tstring lpName, DWORD* dwRet);
// Using Pid to obtain process details
BOOL FindPE(DWORD dwPid, PROCESS* Ret);

// End the process using PID
BOOL TerminateProcPid(DWORD dwPid);
// End the process using the process name
BOOL TerminateProcName(std::_tstring lpName);

// Suspend process using PID
BOOL SuspendProc(DWORD dwPid);
// Resume process using PID
BOOL ResumeProc(DWORD dwPid);

// Causing the process to crash
BOOL CrashProc(DWORD dwPid);


#endif
