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

#include "FileBinIO.h"

template <typename _Tp>
_Tp FBMax(_Tp x, _Tp y) {return x > y ? x : y; }

template <typename _Tp>
_Tp FBMin(_Tp x, _Tp y) { return x > y ? y : x; }

// LARGE_INTEGER and ULONG interconversion
ULONG FBLIntToUl(LARGE_INTEGER x) {
    ULONG ulValue;
    ulValue = (ULONG)x.QuadPart;
    return ulValue;
}

LARGE_INTEGER FBUlToLInt(ULONG x) {
    LARGE_INTEGER liValue{};
    liValue.QuadPart = (LONGLONG)x;
    return liValue;
}

// Retrieve the number of bytes in the file (unit: bytes)
LARGE_INTEGER FBGetFileSize(std::_tstring filePath) {
    LARGE_INTEGER fileSize;
    fileSize.QuadPart = 0;

    // 打开文件以读取其大小
    HANDLE hFile = CreateFile(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) return fileSize;

    // 获取文件大小
    if (!GetFileSizeEx(hFile, &fileSize)) {
        // 关闭文件句柄
        CloseHandle(hFile);
        return fileSize;
    }

    // 关闭文件句柄
    CloseHandle(hFile);
    return fileSize;
}

// Read files by byte (or disk)
DWORD FBReadFile(std::_tstring lpcFilePath,
    BYTE* lpcData,
    LPDWORD lpdwBytesRead,
    LONG uiRstart,
    DWORD uiRsize
) {
    LARGE_INTEGER liFileSize = FBGetFileSize(lpcFilePath);

    // Identify the possibility of overflow and avoid it
    if (lpcData == NULL) return FB_INVAILD_POINTER;
    if (uiRsize != FB_UL_INF && 1UL * sizeof(UCHAR) * uiRsize > FBLIntToUl(liFileSize)) return FB_OUT_OF_RANGE;

    // Create file object
    HANDLE hFile = CreateFile(lpcFilePath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FB_ERROR;

    // Number of bytes read
    DWORD dwBytesRead;

    // Set Start Position
    if (SetFilePointer(hFile, uiRstart, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        CloseHandle(hFile);
        return FB_ERROR;
    }

    // Read the file and close the handle
    BOOL result = ReadFile(hFile, lpcData, FBMin <ULONG> (uiRsize, FBLIntToUl(liFileSize)), &dwBytesRead, NULL);
    CloseHandle(hFile);

    // Processing read failed
    if (!result) return FB_ERROR;

    if (lpdwBytesRead != NULL) *lpdwBytesRead = dwBytesRead;
    return FB_SUCCESS;
}

// Write files by byte (or disk)
DWORD FBWriteFile(std::_tstring lpcFilePath,
    BYTE* lpcData,
    LPDWORD lpdwBytesWrite,
    LONG uiWstart,
    DWORD uiWsize
) {
    // Identify null pointers and avoid them
    if (lpcData == NULL) return FB_INVAILD_POINTER;

    // Create file object
    HANDLE hFile = CreateFile(lpcFilePath.c_str(), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FB_ERROR;

    // Number of bytes written
    DWORD dwBytesWrite;

    // Set Start Position
    if (SetFilePointer(hFile, uiWstart, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        CloseHandle(hFile);
        return FB_ERROR;
    }

    // Write file and close handle
    BOOL result = WriteFile(hFile, lpcData, uiWsize, &dwBytesWrite, NULL);
    CloseHandle(hFile);

    // Processing write failed
    if (!result) return FB_ERROR;

    if (lpdwBytesWrite != NULL) *lpdwBytesWrite = dwBytesWrite;
    return FB_SUCCESS;
}
