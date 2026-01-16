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

#include "IAMKeyBlock.h"

// Find saved IAM key in memory
BOOL IAMKeyFind(IAMBlockData** ppBlock) {
    ULONG_PTR pAddress = 0;
    MEMORY_BASIC_INFORMATION MemoryInformation = { 0 };

    if (ppBlock == nullptr) return FALSE;

    // Distinguish the upper limit of user mode addresses between 64-bit and 32-bit
#ifdef _WIN64
    const ULONGLONG nAddressLimit = 0x00007FFFFFFFFFFF;
#else
    const ULONGLONG nAddressLimit = 0x80000000;
#endif

    // Traverse the virtual address space
    while (pAddress < nAddressLimit && VirtualQueryEx(GetCurrentProcess(), (LPVOID)pAddress, &MemoryInformation, sizeof(MemoryInformation))) {
        // Search criteria: Submitted, read-write, private memory
        if (MemoryInformation.State == MEM_COMMIT && MemoryInformation.Protect == PAGE_READWRITE && MemoryInformation.Type == MEM_PRIVATE) {
            // Temporarily convert to structure pointer
            IAMBlockData* pData = reinterpret_cast<IAMBlockData*>(MemoryInformation.BaseAddress);

            // Verify magic number and confirm valid memory blocks
            if (pData->Magic == IAM_KEY_BLOCK_MAGIC) {
                *ppBlock = pData;
                return TRUE;
            }
        }

        // Push to the next memory area
        pAddress = (ULONG_PTR)MemoryInformation.BaseAddress + MemoryInformation.RegionSize;
    }

    // No existing memory block found
    *ppBlock = NULL;
    return FALSE;
}

// Write key to memory. (If it already exists, overwrite it)
BOOL IAMKeyWrite(ULONGLONG Key) {
    // Attempt to search for existing memory blocks
    IAMBlockData* pData;
    BOOL bRes = IAMKeyFind(&pData);

    if (bRes) {
        // Find the memory block and overwrite it
        pData->IAMKey = Key;
        return TRUE;
    }

    // If memory block cannot be found or accessed, create a new data block
    pData = (IAMBlockData*)VirtualAlloc(
        NULL,
        sizeof(IAMBlockData),
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (pData == NULL) return FALSE;

    // Save data
    pData->Magic = IAM_KEY_BLOCK_MAGIC;
    pData->IAMKey = Key;

    return TRUE;
}

// Get key from memory.
BOOL IAMKeyRead(ULONGLONG* pKey) {
    if (pKey == nullptr) return FALSE;

    // Attempt to search for existing memory blocks
    IAMBlockData* pData;
    BOOL bRes = IAMKeyFind(&pData);

    if (bRes) {
        // Find the memory block, return IAM key
        *pKey = pData->IAMKey;
        return TRUE;
    }

    // Unable to find data block, return error
    return FALSE;
}
