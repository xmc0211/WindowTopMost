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

#include "ThreadStopHandler.h"

ThreadStopHandler::ThreadStopHandler() : Section({ 0 }), nActiveThreadCount(0) {

}
ThreadStopHandler::~ThreadStopHandler() {
	Uninit(); // Uninit at last
}

// Init handler
DWORD ThreadStopHandler::Init() {
	if (!InitializeCriticalSectionAndSpinCount(&Section, 4000)) {
		return TSH_ERROR_INIT_CRITICALSECTION;
	}

	nActiveThreadCount = 0;

	// Create manual reset events
	hStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
	hStopFinishEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

	if (hStopEvent == NULL || hStopFinishEvent == NULL) {
		// Release created resources
		if (hStopEvent != NULL) {
			CloseHandle(hStopEvent);
			hStopEvent = NULL;
		}
		if (hStopFinishEvent != NULL) {
			CloseHandle(hStopFinishEvent);
			hStopFinishEvent = NULL;
		}
		DeleteCriticalSection(&Section);
		return TSH_ERROR_CREATE_EVENT;
	}

	return TSH_SUCCESS;
}

// Uninit handler
void ThreadStopHandler::Uninit() {
	// Stop all threads
	StopAllThread();

	// Close events
	if (hStopEvent != NULL) {
		ResetEvent(hStopEvent);
		CloseHandle(hStopEvent);
		hStopEvent = NULL;
	}
	if (hStopFinishEvent != NULL) {
		ResetEvent(hStopFinishEvent);
		CloseHandle(hStopFinishEvent);
		hStopFinishEvent = NULL;
	}

	// Delete section
	BOOL bCritSecInitialized = (Section.DebugInfo != NULL);
	if (bCritSecInitialized) {
		DeleteCriticalSection(&Section);
		ZeroMemory(&Section, sizeof(CRITICAL_SECTION));
	}

	nActiveThreadCount = 0;
}

// Enter a thread, called upon thread startup
void ThreadStopHandler::EnterThread() {
	if (ThreadShouldStop()) return;

	// Add thread count
	InterlockedIncrement64(&nActiveThreadCount);

	// Secondary inspection stop event
	if (ThreadShouldStop()) {
		InterlockedDecrement64(&nActiveThreadCount);
		return;
	}
}

// Leave a thread, called upon thread stop
void ThreadStopHandler::LeaveThread() {
	LONGLONG nAfterDecrement = InterlockedDecrement64(&nActiveThreadCount);
	// Prevent counting to negative numbers
	if (nAfterDecrement < 0) {
		InterlockedIncrement64(&nActiveThreadCount);
		return;
	}

	// Trigger completion event only when the stop signal has been triggered and all threads have exited
	if (ThreadShouldStop() && nAfterDecrement == 0) {
		EnterCriticalSection(&Section); // Protect 'SetEvent'
		SetEvent(hStopFinishEvent);
		LeaveCriticalSection(&Section);
	}
}

// Query whether the thread should stop
BOOL ThreadStopHandler::ThreadShouldStop() {
	// Query event state
	if (hStopEvent == NULL) {
		return FALSE;
	}
	return (WaitForSingleObject(hStopEvent, 0) == WAIT_OBJECT_0);
}

// Stop all thread
DWORD ThreadStopHandler::StopAllThread() {
	if (hStopEvent == NULL || hStopFinishEvent == NULL) {
		return TSH_ERROR_CREATE_EVENT;
	}

	EnterCriticalSection(&Section);
	BOOL bSetEventOk = SetEvent(hStopEvent);
	LONGLONG nCurrentThreadCount = nActiveThreadCount;
	LeaveCriticalSection(&Section);

	if (!bSetEventOk) {
		return TSH_ERROR_SET_EVENT;
	}

	// No active threads, return directly
	if (nCurrentThreadCount <= 0) {
		return TSH_SUCCESS;
	}

	// Waiting for all threads to stop
	DWORD nRes = WaitForSingleObject(hStopFinishEvent, WAIT_THREAD_STOP_TIMEOUT);
	if (nRes != WAIT_OBJECT_0) {
		return TSH_ERROR_WAIT_EVENT;
	}

	return TSH_SUCCESS;
}