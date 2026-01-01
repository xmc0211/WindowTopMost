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

#include "ProcessCtrl.h"

PROCESS::PROCESS() : Pid(0), Name(TEXT("")), Path(TEXT("")), AccountName(TEXT("")), DomainName(TEXT("")), PE() {}
PROCESS::~PROCESS() {}
THREAD::THREAD() : Tid(0), ParentPid(0), TE() {}
THREAD::~THREAD() {}

typedef BOOL(CALLBACK* PROC_ENUM_CALLBACK)(PROCESS Tsk, LPVOID lParam);
typedef BOOL(CALLBACK* THRE_ENUM_CALLBACK)(THREAD Tre, LPVOID lParam);
typedef BOOL(CALLBACK* WND_ENUM_CALLBACK)(HWND hWnd, LPVOID lParam);


BOOL __GetAccountNameFromSid(PSID pSid, std::_tstring& accountName, std::_tstring& domainName) {
	DWORD cchName = 256;
	DWORD cchDomain = 256;
	TCHAR szName[256];
	TCHAR szDomain[256];
	SID_NAME_USE eUse;
	if (LookupAccountSid(NULL, pSid, szName, &cchName, szDomain, &cchDomain, &eUse)) {
		accountName = szName;
		domainName = szDomain;
		return TRUE;
	}
	return FALSE;
}

BOOL CALLBACK __TerminateProcessPidCB(PROCESS Tsk, LPVOID lParam) {
	BOOL bRet = TRUE;
	HANDLE hProcess = NULL;
	DWORD* ObjPid = reinterpret_cast <DWORD*> (lParam);
	if (*ObjPid != Tsk.Pid) return TRUE;
	hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, Tsk.Pid);
	if (hProcess == NULL) return TRUE;
	bRet = TerminateProcess(hProcess, 1);
	CloseHandle(hProcess);
	return bRet;
}
BOOL CALLBACK __TerminateProcessNameCB(PROCESS Tsk, LPVOID lParam) {
	BOOL bRet = TRUE;
	HANDLE hProcess = NULL;
	std::_tstring* ObjName = reinterpret_cast <std::_tstring*> (lParam);
	if (*ObjName != Tsk.Name) return TRUE;
	hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, Tsk.Pid);
	if (hProcess == NULL) return FALSE;
	bRet = TerminateProcess(hProcess, 1);
	CloseHandle(hProcess);
	return bRet;
}
BOOL CALLBACK __SuspendThreadCB(THREAD Tre, LPVOID lParam) {
	BOOL bRet = TRUE;
	HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, Tre.Tid);
	if (thread != NULL) {
		if (SuspendThread(thread) == (DWORD) -1) bRet = FALSE;
		CloseHandle(thread);
	} else bRet = FALSE;
	return bRet;
}
BOOL CALLBACK __ResumeThreadCB(THREAD Tre, LPVOID lParam) {
	BOOL bRet = TRUE;
	HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, Tre.Tid);
	if (thread != NULL) {
		if (ResumeThread(thread) == (DWORD) -1) bRet = FALSE;
		CloseHandle(thread);
	} else bRet = FALSE;
	return bRet;
}

BOOL EnumProc(PROC_ENUM_CALLBACK fCb, LPVOID lpParam) {
	BOOL bRet = TRUE;
	PROCESSENTRY32 pe{};
	pe.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE) return FALSE;

	if (Process32First(hProcessSnap, &pe)) {
		do {
			std::_tstring strProcessName = pe.szExeFile;
			std::_tstring strProcessPath;
			std::_tstring tn(pe.szExeFile);
			PROCESS obj;
			obj.PE = pe;
			obj.Pid = pe.th32ProcessID;
			obj.Name = tn;
			// 获取进程的完整路径
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
			if (!hProcess) continue;
			TCHAR szProcessPath[MAX_PATH] = { 0 };
			if (GetModuleFileNameEx(hProcess, NULL, szProcessPath, MAX_PATH)) {
				std::_tstring spath(szProcessPath);
				obj.Path = spath;
			}
			HANDLE hToken;
			if (OpenProcessToken(hProcess, TOKEN_READ, &hToken)) {
				DWORD dwSize = 0;
				GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
				PTOKEN_USER pTokenUser = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), 0, dwSize);
				if (GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize)) {
					PSID pid = pTokenUser->User.Sid;
					__GetAccountNameFromSid(pid, obj.AccountName, obj.DomainName);
				}
				HeapFree(GetProcessHeap(), 0, pTokenUser);
				CloseHandle(hToken);
			}
			CloseHandle(hProcess);
			bRet &= fCb(obj, lpParam);
		} while (Process32Next(hProcessSnap, &pe)); // 继续遍历
	}
	CloseHandle(hProcessSnap); // 关闭快照句柄
	return bRet;
}
BOOL EnumThre(DWORD dwPid, THRE_ENUM_CALLBACK fCb, LPVOID lpParam) {
	BOOL bRet = TRUE;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (snapshot == INVALID_HANDLE_VALUE) return FALSE;
	THREADENTRY32 threadEntry = { 0 };
	threadEntry.dwSize = sizeof(THREADENTRY32);
	if (!Thread32First(snapshot, &threadEntry)) {
		CloseHandle(snapshot);
		return FALSE;
	}
	do {
		if (threadEntry.th32OwnerProcessID == dwPid) {
			THREAD obj;
			obj.TE = threadEntry;
			obj.ParentPid = dwPid;
			obj.Tid = threadEntry.th32ThreadID;
			bRet &= fCb(obj, lpParam);
		}
	} while (Thread32Next(snapshot, &threadEntry));

	CloseHandle(snapshot);
	return bRet;
}
BOOL EnumWnd(DWORD dwPid, WND_ENUM_CALLBACK fCb, LPVOID lpParam) {
	HWND hWnd = NULL;
	BOOL bRet = TRUE;
	DWORD lpdwProcessId;
	while ((hWnd = FindWindowEx(NULL, hWnd, NULL, NULL)) != NULL) {
		GetWindowThreadProcessId(hWnd, &lpdwProcessId);
		if (lpdwProcessId == dwPid) bRet &= fCb(hWnd, lpParam);
	}
	return bRet;
}

BOOL FindPid(std::_tstring lpName, DWORD* dwRet) {
	BOOL bRet = TRUE;
	PROCESSENTRY32 pe{};
	pe.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE) return FALSE;

	if (Process32First(hProcessSnap, &pe)) {
		do {
			if (pe.szExeFile == lpName) {
				*dwRet = pe.th32ProcessID;
				break;
			}
		} while (Process32Next(hProcessSnap, &pe));
	} else bRet = FALSE;
	CloseHandle(hProcessSnap);
	return bRet;
}
BOOL FindPE(DWORD dwPid, PROCESS* Ret) {
	PROCESSENTRY32 pe{};
	if (Ret == NULL) return FALSE;
	pe.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE) return -1;

	if (Process32First(hProcessSnap, &pe)) {
		do {
			if (pe.th32ProcessID == dwPid) {
				std::_tstring strProcessName = pe.szExeFile;
				std::_tstring strProcessPath;
				std::_tstring tn(pe.szExeFile);
				PROCESS obj;
				obj.PE = pe;
				obj.Pid = pe.th32ProcessID;
				obj.Name = tn;
				HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
				if (!hProcess) continue;
				TCHAR szProcessPath[MAX_PATH] = { 0 };
				if (GetModuleFileNameEx(hProcess, NULL, szProcessPath, MAX_PATH)) {
					std::_tstring spath(szProcessPath);
					obj.Path = spath;
				}
				HANDLE hToken;
				if (OpenProcessToken(hProcess, TOKEN_READ, &hToken)) {
					DWORD dwSize = 0;
					GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
					PTOKEN_USER pTokenUser = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), 0, dwSize);
					if (GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize)) {
						PSID pid = pTokenUser->User.Sid;
						__GetAccountNameFromSid(pid, obj.AccountName, obj.DomainName);
					}
					HeapFree(GetProcessHeap(), 0, pTokenUser);
					CloseHandle(hToken);
				}
				CloseHandle(hProcess);
				*Ret = obj;
				break;
			}
		} while (Process32Next(hProcessSnap, &pe)); // 继续遍历
	}
	CloseHandle(hProcessSnap); // 关闭快照句柄
	return TRUE;
}

BOOL TerminateProcPid(DWORD dwPid) {
	DWORD Pid = dwPid;
	LPVOID param = reinterpret_cast <LPVOID> (&Pid);
	return EnumProc(__TerminateProcessPidCB, param);
}
BOOL TerminateProcName(std::_tstring lpName) {
	std::_tstring Name = lpName;
	LPVOID param = reinterpret_cast <LPVOID> (&Name);
	return EnumProc(__TerminateProcessNameCB, param);
}

BOOL SuspendProc(DWORD dwPid) {
	return EnumThre(dwPid, __SuspendThreadCB, NULL);
}
BOOL ResumeProc(DWORD dwPid) {
	return EnumThre(dwPid, __ResumeThreadCB, NULL);
}

BOOL CrashProc(DWORD dwPid) {
	HANDLE p = OpenProcess(PROCESS_ALL_ACCESS, 0, dwPid);
	if (!p) return FALSE;
#pragma warning(push)
#pragma warning(disable:6387) // Function requirement
	HANDLE hTh = CreateRemoteThread(p, NULL, 0, NULL, NULL, 0, NULL);
#pragma warning(pop)
	if (!hTh) return FALSE;
	CloseHandle(hTh);
	return TRUE;
}

