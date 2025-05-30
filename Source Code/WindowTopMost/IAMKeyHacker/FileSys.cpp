// MIT License
//
// Copyright (c) 2025 FileSys - xmc0211 <xmc0211@qq.com>
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

#include "FileSys.h"
#include <sddl.h>
#include <AclAPI.h>
#include <shellapi.h>
#include <tchar.h>

#pragma comment(lib, "Shell32.lib")

#define substr_p(s, t) substr((s), (t) - (s) + 1);

BOOL FSObjectExist(_In_ std::_tstring lpFullPath) {
	DWORD dwAttr = GetFileAttributes(lpFullPath.c_str());
	BOOL bRes = (dwAttr != INVALID_FILE_ATTRIBUTES);
	return bRes;
}

DWORD FSGetObjectAttribute(_In_ std::_tstring lpExistFullPath) {
	return GetFileAttributes(lpExistFullPath.c_str());
}

BOOL FSSetObjectAttribute(_In_ std::_tstring lpExistFullPath, _In_opt_ UINT uFileAttribute) {
	BOOL bRes = TRUE;
	bRes = (SetFileAttributes(lpExistFullPath.c_str(), uFileAttribute) == INVALID_FILE_ATTRIBUTES);
	return bRes;
}

HANDLE FSOpenObject(_In_ std::_tstring lpExistFullPath, _In_opt_ FSFILEACTION fAction, _In_opt_ std::_tstring lpParameters, _In_opt_ BOOL bWaitForThread, _In_opt_ UINT uShowFlags) {
	BOOL bRes = TRUE;
	std::_tstring Verb = TEXT("open");
	switch (fAction) {
	case FS_OPEN: { Verb = TEXT("open"); break; }
	case FS_EDIT: { Verb = TEXT("edit"); break; }
	case FS_EXPLORE: { Verb = TEXT("explore"); break; }
	//case FS_FIND: { Verb = TEXT("find"); break; }
	case FS_PRINT: { Verb = TEXT("print"); break; }
	case FS_PROPERTIES: { Verb = TEXT("properties"); break; }
	case FS_RUNASADMIN: { Verb = TEXT("runas"); break; }
	}
	if (Verb == TEXT("explore") && FSObjectIsFile(lpExistFullPath)) {
		return FSOpenObject(TEXT("Explorer.EXE"), FS_OPEN, (std::_tstring(TEXT("/SELECT, \"")) + lpExistFullPath + TEXT("\"")).c_str(), bWaitForThread, uShowFlags);
	}
	SHELLEXECUTEINFO ShExecInfo = { 0 };
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_INVOKEIDLIST;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = Verb.c_str();
	ShExecInfo.lpFile = lpExistFullPath.c_str();
	ShExecInfo.lpParameters = lpParameters.c_str();
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = uShowFlags;
	ShExecInfo.hInstApp = NULL;
	ShExecInfo.lpIDList = NULL;
	bRes = ShellExecuteEx(&ShExecInfo);
	if (!bRes || ShExecInfo.hProcess == NULL) return NULL;
	if (bWaitForThread) WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
	return ShExecInfo.hProcess;
}

BOOL FSObjectIsFile(_In_ std::_tstring lpFullPath) {
	if (!FSObjectExist(lpFullPath)) return FALSE;
	HANDLE hFile = CreateFile(lpFullPath.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return FALSE;
	CloseHandle(hFile);
	return TRUE;
}


std::_tstring FSGetCurrentFilePath() {
	TCHAR Buffer[MAX_PATH];
	GetModuleFileName(NULL, Buffer, MAX_PATH);
	return std::_tstring(Buffer);
}


std::_tstring FSFormat(_In_ std::_tstring lpFormat, _In_ std::_tstring lpFullPath) {
	std::_tstring op(lpFormat), Path(lpFullPath);
	std::_tstring Res = TEXT("");
	size_t fSize = op.size(), pSize = Path.size();

	// Remove quotation marks
	if (Path[0] == '\"' && Path[pSize - 1] == '\"') {
		Path = Path.substr(1, pSize - 2);
		pSize -= 2;
	}

	// Example
	// 
	// C:\Windows\System32\CMD.EXE
	//  ]                 ]   )
	// D|       P         | N | X
	//
	size_t DrivePoint = Path.find_first_of(':');
	if (DrivePoint == std::_tstring::npos) return TEXT("");
	size_t PathPoint = Path.find_last_of('\\');
	if (PathPoint == std::_tstring::npos) return TEXT("");
	size_t NamePoint = Path.find_last_of('.');
	if (NamePoint == std::_tstring::npos) return TEXT("");

	// Obtain the content of each section based on the lpFormat
	std::_tstring ResDrive = Path.substr_p(0, DrivePoint);
	std::_tstring ResPath = Path.substr_p(DrivePoint + 1, PathPoint);
	std::_tstring ResName = Path.substr_p(PathPoint + 1, NamePoint - 1);
	std::_tstring ResExt = Path.substr(NamePoint);

	// Process and add each character individually
	for (size_t Pt = 0; Pt < fSize; Pt++) {
		switch (op[Pt]) {
		case 'd': case 'D': {
			Res += ResDrive;
			break;
		}
		case 'p': case 'P': {
			Res += ResPath;
			break;
		}
		case 'n': case 'N': {
			Res += ResName;
			break;
		}
		case 'x': case 'X': {
			Res += ResExt;
			break;
		}
		case 'z': case 'Z': {
			Res += std::to_tstring(FBLIntToUl(FBGetFileSize(Path.c_str())));
			break;
		}
		default: Res += op[Pt];
		}
	}
	return Res;
}


BOOL FSCreateFile(_In_ std::_tstring lpFullPath) {
	HANDLE hFile = CreateFile(lpFullPath.c_str(), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return FALSE;
	CloseHandle(hFile);
	return TRUE;
}

BOOL FSDeleteFile(_In_ std::_tstring lpFullPath) {
	BOOL bRes = DeleteFile(lpFullPath.c_str());
	return bRes;
}

BOOL FSMoveFile(_In_ std::_tstring lpExistFullPath, _In_ std::_tstring lpNewFullPath, _In_opt_ BOOL bFailIfExists) {
	if (FSObjectExist(lpNewFullPath)) {
		if (bFailIfExists) return FALSE;
		if (!FSDeleteFile(lpNewFullPath)) return FALSE;
	}
	BOOL bRes = MoveFile(lpExistFullPath.c_str(), lpNewFullPath.c_str());
	return bRes;
}

BOOL FSRenameFile(_In_ std::_tstring lpExistFullPath, _In_ std::_tstring lpNewFileName, _In_opt_ BOOL bFailIfExists) {
	BOOL bRes = TRUE;
	std::_tstring ExistPath = FSFormat(TEXT("dp"), lpExistFullPath);
	if (ExistPath == TEXT("")) return FALSE;
	bRes = FSMoveFile(lpExistFullPath, (ExistPath + lpNewFileName).c_str(), bFailIfExists);
	return bRes;
}

BOOL FSCopyFile(_In_ std::_tstring lpExistFullPath, _In_ std::_tstring lpNewFullPath, _In_opt_ BOOL bFailIfExists) {
	BOOL bRes = CopyFile(lpExistFullPath.c_str(), lpNewFullPath.c_str(), bFailIfExists);
	return bRes;
}


BOOL FSCreateDir(_In_ std::_tstring lpFullPath) {
	BOOL bRes = CreateDirectory(lpFullPath.c_str(), NULL);
	return bRes;
}

BOOL FSDeleteDir(_In_ std::_tstring lpExistFullPath) {
	std::_tstring EnumPath = lpExistFullPath + TEXT("\\*");
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile(EnumPath.c_str(), &findData);
	if (hFind == INVALID_HANDLE_VALUE) return FALSE;
	do {
		std::_tstring NowPath = lpExistFullPath + TEXT("\\") + findData.cFileName;
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (_tcscmp(findData.cFileName, TEXT(".")) == 0 || _tcscmp(findData.cFileName, TEXT("..")) == 0) continue;
			FSDeleteDir(NowPath.c_str());
		}
		else {
			FSDeleteFile(NowPath.c_str());
		}
	} while (FindNextFile(hFind, &findData) != ERROR_SUCCESS);
	if (!RemoveDirectory(lpExistFullPath.c_str())) return FALSE;
	FindClose(hFind);
	return TRUE;
}

BOOL FSMoveDir(_In_ std::_tstring lpExistFullPath, _In_ std::_tstring lpNewFullPath, _In_opt_ BOOL bFailIfExists) {
	std::_tstring EnumPath = lpExistFullPath + TEXT("\\*");
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile(EnumPath.c_str(), &findData);
	if (hFind == INVALID_HANDLE_VALUE) return FALSE;
	if (!FSObjectExist(lpNewFullPath)) {
		if (!FSCreateDir(lpNewFullPath)) return FALSE;
	}
	do {
		std::_tstring NowPath = lpExistFullPath + TEXT("\\") + findData.cFileName;
		std::_tstring NewPath = lpNewFullPath + TEXT("\\") + findData.cFileName;
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (_tcscmp(findData.cFileName, TEXT(".")) == 0 || _tcscmp(findData.cFileName, TEXT("..")) == 0) continue;
			FSMoveDir(NowPath.c_str(), NewPath.c_str(), bFailIfExists);
		}
		else {
			FSMoveFile(NowPath.c_str(), NewPath.c_str(), bFailIfExists);
		}
	} while (FindNextFile(hFind, &findData) != ERROR_SUCCESS);
	if (!RemoveDirectory(lpExistFullPath.c_str())) return FALSE;
	FindClose(hFind);
	return TRUE;
}

BOOL FSRenameDir(_In_ std::_tstring lpExistFullPath, _In_ std::_tstring lpNewFileName, _In_opt_ BOOL bFailIfExists) {
	BOOL bRes = TRUE;
	size_t Pos = lpExistFullPath.find_last_of('\\');
	if (Pos == std::_tstring::npos) return FALSE;
	std::_tstring ExistPath = lpExistFullPath.substr(0, Pos + 1);
	bRes = FSMoveDir(lpExistFullPath, (ExistPath + lpNewFileName).c_str(), bFailIfExists);
	return bRes;
}

BOOL FSCopyDir(_In_ std::_tstring lpExistFullPath, _In_ std::_tstring lpNewFullPath, _In_opt_ BOOL bFailIfExists) {
	std::_tstring EnumPath = lpExistFullPath + TEXT("\\*");
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile(EnumPath.c_str(), &findData);
	if (hFind == INVALID_HANDLE_VALUE) return FALSE;
	if (!FSObjectExist(lpNewFullPath)) {
		if (!FSCreateDir(lpNewFullPath)) return FALSE;
	}
	do {
		std::_tstring NowPath = lpExistFullPath + TEXT("\\") + findData.cFileName;
		std::_tstring NewPath = lpNewFullPath + TEXT("\\") + findData.cFileName;
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (_tcscmp(findData.cFileName, TEXT(".")) == 0 || _tcscmp(findData.cFileName, TEXT("..")) == 0) continue;
			FSCopyDir(NowPath.c_str(), NewPath.c_str(), bFailIfExists);
		}
		else {
			FSCopyFile(NowPath.c_str(), NewPath.c_str(), bFailIfExists);
		}
	} while (FindNextFile(hFind, &findData) != ERROR_SUCCESS);
	FindClose(hFind);
	return TRUE;
}


BOOL FSEnumDir(_In_ std::_tstring lpExistDirPath, _In_ FS_PATH_CALLBACK cbFunc, _In_opt_ LPVOID lpParam) {
	std::_tstring EnumPath = lpExistDirPath + TEXT("\\*");
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile(EnumPath.c_str(), &findData);
	if (hFind == INVALID_HANDLE_VALUE) return FALSE;
	do {
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (_tcscmp(findData.cFileName, TEXT(".")) == 0 || _tcscmp(findData.cFileName, TEXT("..")) == 0) continue;
			if (!cbFunc(std::_tstring(lpExistDirPath) + TEXT("\\") + findData.cFileName, lpParam)) break;
		}
	} while (FindNextFile(hFind, &findData) != ERROR_SUCCESS);
	FindClose(hFind);
	return TRUE;
}

BOOL FSEnumFile(_In_ std::_tstring lpExistDirPath, _In_ FS_PATH_CALLBACK cbFunc, _In_opt_ LPVOID lpParam) {
	std::_tstring EnumPath = std::_tstring(lpExistDirPath) + TEXT("\\*");
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile(EnumPath.c_str(), &findData);
	if (hFind == INVALID_HANDLE_VALUE) return FALSE;
	do {
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			if (!cbFunc(std::_tstring(lpExistDirPath) + TEXT("\\") + findData.cFileName, lpParam)) break;
		}
	} while (FindNextFile(hFind, &findData) != ERROR_SUCCESS);
	FindClose(hFind);
	return TRUE;
}

BOOL FSEnumAllDir(_In_ std::_tstring lpExistDirPath, _In_ FS_PATH_CALLBACK cbFunc, _In_opt_ LPVOID lpParam) {
	std::_tstring EnumPath = std::_tstring(lpExistDirPath) + TEXT("\\*");
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile(EnumPath.c_str(), &findData);
	if (hFind == INVALID_HANDLE_VALUE) return FALSE;
	do {
		std::_tstring NowPath = std::_tstring(lpExistDirPath) + TEXT("\\") + findData.cFileName;
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (_tcscmp(findData.cFileName, TEXT(".")) == 0 || _tcscmp(findData.cFileName, TEXT("..")) == 0) continue;
			if (!cbFunc(NowPath, lpParam)) break;
			FSEnumAllDir(NowPath.c_str(), cbFunc, lpParam);
		}
	} while (FindNextFile(hFind, &findData) != ERROR_SUCCESS);
	FindClose(hFind);
	return TRUE;
}

BOOL FSEnumAllFile(_In_ std::_tstring lpExistDirPath, _In_ FS_PATH_CALLBACK cbFunc, _In_opt_ LPVOID lpParam) {
	std::_tstring EnumPath = std::_tstring(lpExistDirPath) + TEXT("\\*");
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile(EnumPath.c_str(), &findData);
	if (hFind == INVALID_HANDLE_VALUE) return FALSE;
	do {
		std::_tstring NowPath = std::_tstring(lpExistDirPath) + TEXT("\\") + findData.cFileName;
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (_tcscmp(findData.cFileName, TEXT(".")) == 0 || _tcscmp(findData.cFileName, TEXT("..")) == 0) continue;
			FSEnumAllFile(NowPath.c_str(), cbFunc, lpParam);
		}
		else {
			if (!cbFunc(NowPath, lpParam)) break;
		}
	} while (FindNextFile(hFind, &findData) != ERROR_SUCCESS);
	FindClose(hFind);
	return TRUE;
}