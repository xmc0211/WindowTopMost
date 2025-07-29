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

#ifndef FILESYS_H
#define FILESYS_H

#include <Windows.h>
#include <string>
#include "FileBinIO.h"

#if defined(UNICODE)
#define _tstring wstring
#define to_tstring to_wstring
#else
#define _tstring string
#define to_tstring to_string
#endif

// Enumeration of file operations
enum FSFILEACTION {
	FS_OPEN,			// Open in default mode
	FS_PRINT,			// Printing (document only)
	FS_EDIT,			// Edit with default editor (if available)
	FS_EXPLORE,			// Open with File Explorer
//	FS_FIND,			// Search (document only) // This feature is not currently available
	FS_PROPERTIES,		// Open the Properties dialog box
	FS_RUNASADMIN,		// Run as administrator (executable files only)
};

// Add or remove file attributes
#define FSAddObjectAttribute(f, a) FSSetObjectAttribute((f), FSGetObjectAttribute((f)) | (a))
#define FSRemoveObjectAttribute(f, a) FSSetObjectAttribute((f), FSGetObjectAttribute((f)) & ~(a))

// Path callback function. Return TRUE to continue enumeration, return False to stop enumeration.
typedef BOOL(CALLBACK* FS_PATH_CALLBACK)(std::_tstring ExistFullPath, LPVOID lpParam);

// Determine whether the file or directory exists
BOOL FSObjectExist(_In_ std::_tstring lpFullPath);
// View file or directory properties
DWORD FSGetObjectAttribute(_In_ std::_tstring lpExistFullPath);
// Set file or directory properties
BOOL FSSetObjectAttribute(_In_ std::_tstring lpExistFullPath, _In_opt_ UINT uFileAttribute);
// Perform operations on files. The available operations can refer to the comments within the enumeration.
HANDLE FSOpenObject(_In_ std::_tstring lpExistFullPath, _In_opt_ FSFILEACTION fAction = FS_OPEN, _In_opt_ std::_tstring lpParameters = NULL, _In_opt_ BOOL bWaitForThread = TRUE, _In_opt_ UINT uShowFlags = SW_SHOWNORMAL);
// Determine whether the path is a file. If it does not exist, return False; if both exist, return bDefault.
BOOL FSObjectIsFile(_In_ std::_tstring lpFullPath);

// Get the current file name
std::_tstring FSGetCurrentFilePath();

// Format the complete path (lpFormat example: dpnx=>C:\Windows\System32\CMD.EXE nx=>CMD.EXE)
std::_tstring FSFormat(_In_ std::_tstring lpFormat, _In_ std::_tstring lpFullPath);

// Create an empty file
BOOL FSCreateFile(_In_ std::_tstring lpFullPath);
// Delete file
BOOL FSDeleteFile(_In_ std::_tstring lpExistFullPath);
// Move file
BOOL FSMoveFile(_In_ std::_tstring lpExistFullPath, _In_ std::_tstring lpNewFullPath, _In_opt_ BOOL bFailIfExists = TRUE);
// Rename file
BOOL FSRenameFile(_In_ std::_tstring lpExistFullPath, _In_ std::_tstring lpNewFileName, _In_opt_ BOOL bFailIfExists = TRUE);
// Copy file
BOOL FSCopyFile(_In_ std::_tstring lpExistFullPath, _In_ std::_tstring lpNewFullPath, _In_opt_ BOOL bFailIfExists = TRUE);

// Create an empty directory
BOOL FSCreateDir(_In_ std::_tstring lpFullPath);
// Delete directory
BOOL FSDeleteDir(_In_ std::_tstring lpFullPath);
// Move directory
BOOL FSMoveDir(_In_ std::_tstring lpExistFullPath, _In_ std::_tstring lpNewFullPath, _In_opt_ BOOL bFailIfExists = TRUE);
// Rename directory
BOOL FSRenameDir(_In_ std::_tstring lpExistFullPath, _In_ std::_tstring lpNewFileName, _In_opt_ BOOL bFailIfExists = TRUE);
// Copy directory
BOOL FSCopyDir(_In_ std::_tstring lpExistFullPath, _In_ std::_tstring lpNewFullPath, _In_opt_ BOOL bFailIfExists = TRUE);

// List the subdirectories under the directory
BOOL FSEnumDir(_In_ std::_tstring lpExistDirPath, _In_ FS_PATH_CALLBACK cbFunc, _In_opt_ LPVOID lpParam);
// List the sub files of the directory at the next level
BOOL FSEnumFile(_In_ std::_tstring lpExistDirPath, _In_ FS_PATH_CALLBACK cbFunc, _In_opt_ LPVOID lpParam);
// List all subdirectories under the directory
BOOL FSEnumAllDir(_In_ std::_tstring lpExistDirPath, _In_ FS_PATH_CALLBACK cbFunc, _In_opt_ LPVOID lpParam);
// List all subfiles in the directory
BOOL FSEnumAllFile(_In_ std::_tstring lpExistDirPath, _In_ FS_PATH_CALLBACK cbFunc, _In_opt_ LPVOID lpParam);

#endif

