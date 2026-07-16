// MIT License
//
// Copyright (c) 2025-2026 ThreadStopHandler - xmc0211 <xmc0211@qq.com>
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

#ifndef THREADSTOPHANDLER_H
#define THREADSTOPHANDLER_H

#include <Windows.h>
#include <string>
#include <cstdint>

// Thread stop timeout (ms)
#define WAIT_THREAD_STOP_TIMEOUT        (2000ul)

// Error code definations
#define TSH_SUCCESS						(0x0)
#define TSH_ERROR_INIT_CRITICALSECTION	(0x1)
#define TSH_ERROR_CREATE_EVENT			(0x2)
#define TSH_ERROR_SET_EVENT				(0x3)
#define TSH_ERROR_WAIT_EVENT			(0x4)

// An assistance structure for stopping threads
// This structure is used to protect all threads from ending normally when the program exits.
struct ThreadStopHandler {
	CRITICAL_SECTION Section; // Critical section
	LONGLONG nActiveThreadCount; // Active thread count
	HANDLE hStopEvent = NULL; // Event handle to stop all threads
	HANDLE hStopFinishEvent = NULL; // Event handle to brodcast stop finish

	ThreadStopHandler();
	~ThreadStopHandler();
	DWORD Init(); // Init handler
	void Uninit(); // Uninit handler
	void EnterThread(); // Enter a thread, called upon thread startup
	void LeaveThread(); // Leave a thread, called upon thread stop
	BOOL ThreadShouldStop(); // Query whether the thread should stop
	DWORD StopAllThread(); // Stop all threads
};

#endif
