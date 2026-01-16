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

#ifndef IAMKEYBLOCK_H
#define IAMKEYBLOCK_H

#include <Windows.h>

// IAM block data's magic number, used to confirm valid memory blocks
#define IAM_KEY_BLOCK_MAGIC ((ULONGLONG)(0xDCFE09BA65872143))

// IAM key data struct
struct IAMBlockData {
    ULONGLONG Magic; // Magic number, used for data verification
    ULONGLONG IAMKey; // IAM key data
};

// Find saved IAM key in memory
BOOL IAMKeyFind(IAMBlockData** ppBlock);

// Write key to memory. (If it already exists, overwrite it)
BOOL IAMKeyWrite(ULONGLONG Key);

// Get key from memory.
BOOL IAMKeyRead(ULONGLONG* pKey);

#endif
