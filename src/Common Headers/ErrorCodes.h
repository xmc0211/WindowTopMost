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

#ifndef WTM_ERRORCODES_H
#define WTM_ERRORCODES_H

#include <Windows.h>

// Error code definations

// To distinguish between errors returned by the real API and those returned 
// by the library function, a library specific error code is defined.

#define ERROR_WTM__HEAD								(0x3F00)

//
// ERROR_WTM_COMMUNICATION_CODING
// 
// An exception occurred while encoding/decoding data for the communication between IAMController and IAMWorker.
//
#define ERROR_WTM_COMMUNICATION_CODING				(0x3F01)
#define ERROR_WTM_COMMUNICATION_CODING__DESC		"An exception occurred while encoding/decoding data for the communication between IAMController and IAMWorker."

//
// ERROR_WTM_COMMUNICATION_MESSAGING
// 
// An exception occurred while sending/processing a message for the communication between IAMController and IAMWorker.
//
#define ERROR_WTM_COMMUNICATION_MESSAGING			(0x3F02)
#define ERROR_WTM_COMMUNICATION_MESSAGING__DESC		"An exception occurred while sending/collecting a message for the communication between IAMController and IAMWorker."

//
// ERROR_WTM_IAM_PERMISSION
// 
// Unable to obtain IAM access permission.
//
#define ERROR_WTM_IAM_PERMISSION					(0x3F03)
#define ERROR_WTM_IAM_PERMISSION__DESC				"Unable to obtain IAM access permission."

//
// ERROR_WTM_IAM_KEY_NOT_OBTAINED
// 
// IAM access key have not obtained.
//
#define ERROR_WTM_IAM_KEY_HAS_NOT_OBTAINED			(0x3F04)
#define ERROR_WTM_IAM_KEY_HAS_NOT_OBTAINED__DESC	"IAM access key has not obtained."

//
// ERROR_WTM_WORKER_HAS_NOT_LOADED
// 
// WTMInit has not been called yet or it has failed.
//
#define ERROR_WTM_WORKER_HAS_NOT_LOADED				(0x3F05)
#define ERROR_WTM_WORKER_HAS_NOT_LOADED__DESC		"WTMInit has not been called yet or it has failed."

//
// ERROR_WTM_COMMUNICATION_PROCESSING
// 
// An exception occurred while processing a message for the communication between IAMController and IAMWorker.
//
#define ERROR_WTM_COMMUNICATION_PROCESSING			(0x3F06)
#define ERROR_WTM_COMMUNICATION_PROCESSING__DESC	"An exception occurred while processing a message for the communication between IAMController and IAMWorker."

//
// ERROR_WTM_WORKER_CONTROL
// 
// An error occurred while loading or unloading IAMWorker.
//
#define ERROR_WTM_WORKER_CONTROL					(0x3F07)
#define ERROR_WTM_WORKER_CONTROL__DESC				"An error occurred while loading or unloading IAMWorker."

//
// ERROR_WTM_ENVIRONMENT
// 
// This library cannot run in the current environment.
//
#define ERROR_WTM_ENVIRONMENT						(0x3F08)
#define ERROR_WTM_ENVIRONMENT__DESC					"This library cannot run in the current environment."

#define ERROR_WTM__TAIL								(0x3F09)

// Determine whether the error code belongs to the library
BOOL WTMIsWTMError(DWORD nErrorCode) {
	return nErrorCode > ERROR_WTM__HEAD && nErrorCode < ERROR_WTM__TAIL;
}

// Obtain error description based on error code
LPCTSTR WTMGetErrorDescription(DWORD nErrorCode) {
#define EDMITEM(ErrorCode) case ErrorCode: return TEXT(ErrorCode ## __DESC)
	switch (nErrorCode) {
	EDMITEM(ERROR_WTM_COMMUNICATION_CODING);
	EDMITEM(ERROR_WTM_COMMUNICATION_MESSAGING);
	EDMITEM(ERROR_WTM_IAM_PERMISSION);
	EDMITEM(ERROR_WTM_IAM_KEY_HAS_NOT_OBTAINED);
	EDMITEM(ERROR_WTM_WORKER_HAS_NOT_LOADED);
	EDMITEM(ERROR_WTM_COMMUNICATION_PROCESSING);
	EDMITEM(ERROR_WTM_WORKER_CONTROL);
	EDMITEM(ERROR_WTM_ENVIRONMENT);
	}
	return TEXT("Unknown error.");
#undef EDMITEM
}

#endif
