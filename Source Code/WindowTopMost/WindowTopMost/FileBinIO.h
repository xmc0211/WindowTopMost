// MIT License
//
// Copyright (c) 2025 FileBinIO - xmc0211 <xmc0211@qq.com>
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

#ifndef FILEBINIO_H
#define FILEBINIO_H

#include <windows.h>
#include <string>
#include <tchar.h>

#if defined(UNICODE)
#define _tstring wstring
#else
#define _tstring string
#endif

enum FB_RESULTS {
    FB_SUCCESS = 0x0, 
    FB_ERROR = 0x1, 
    FB_FILE_NOT_EXIST = 0x2, 
    FB_OUT_OF_RANGE = 0x4, 
    FB_INVAILD_POINTER = 0x8
};

const UINT FB_UI_INF = UINT_MAX;
const ULONG FB_UL_INF = ULONG_MAX;
const ULONGLONG FB_ULL_INF = ULLONG_MAX;

template <typename _Tp>
_Tp FBMax(_Tp x, _Tp y);

template <typename _Tp>
_Tp FBMin(_Tp x, _Tp y);

// LARGE_INTEGER and ULONG interconversion
ULONG FBLIntToUl(LARGE_INTEGER x);
LARGE_INTEGER FBUlToLInt(ULONG x);

// Retrieve the number of bytes in the file (unit: bytes)
LARGE_INTEGER FBGetFileSize(std::_tstring filePath);

// Read files by byte (or disk)
DWORD FBReadFile(std::_tstring lpcFilePath, // File path
    BYTE* lpcData, // Read data segment pointer
    LPDWORD lpdwBytesRead, // Receive read byte pointer
    LONG uiRstart, // Starting address for reading
    DWORD uiRsize = FB_UL_INF // Read the total size (FB_UL_INF represents to the end)
);

// Write files by byte (or disk)
DWORD FBWriteFile(std::_tstring lpcFilePath, // File path
    BYTE* lpcData, // Write data segment pointer
    LPDWORD lpdwBytesWrite, // Receive write byte pointer
    LONG uiWstart, // Starting address for writing
    DWORD uiWsize // Write total size
);

#endif
