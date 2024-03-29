/*
 *  thread_priority_sdl_win32.cpp
 *  AlephOne-OSX
 *
 *  Created by woody on Mon Dec 03 2001.
 *  Copyright (c) 2001 __MyCompanyName__. All rights reserved.
 *
 */

#include	"thread_priority_sdl.h"

#include    <windows.h>
#include    <stdio.h>
#include    <SDL_thread.h>

static bool
TryToReduceMainThreadPriority() {
    static bool isMainThreadPriorityReduced = false;
    
    if(isMainThreadPriorityReduced)
        return true;
    
    HANDLE	theMainThreadH = GetCurrentThread();
    
    if(SetThreadPriority(theMainThreadH, THREAD_PRIORITY_BELOW_NORMAL) == 0)
        return false;
    
    isMainThreadPriorityReduced = true;
    return true;
}

typedef HANDLE (WINAPI *OpenThreadPtrT)(DWORD,BOOL,DWORD);

bool
BoostThreadPriority(SDL_Thread* inThread) {
	bool success = false;
	HMODULE kernel32 = GetModuleHandle("KERNEL32");

	if (kernel32 == NULL)
	{	printf("warning: BoostThreadPriority failed: Could not open KERNEL32.  Network performance may suffer.\n"); }
	else
	{
		OpenThreadPtrT OpenThreadPtr = reinterpret_cast<OpenThreadPtrT>(GetProcAddress(kernel32, "OpenThread"));

		if (OpenThreadPtr == NULL)
		{	printf("warning: BoostThreadPriority failed: No OpenThread (only available on WinME, Win2000, WinXP or better).  Network performance may suffer.\n"); }
		else
		{
            HANDLE theTargetThread = OpenThreadPtr(STANDARD_RIGHTS_REQUIRED | THREAD_SET_INFORMATION, FALSE, SDL_GetThreadID(inThread));

			if (theTargetThread == NULL)
			{	printf("warning: BoostThreadPriority failed: Could not open thread.  Network performance may suffer.\n"); }
			else
			{
                if(SetThreadPriority(theTargetThread, THREAD_PRIORITY_TIME_CRITICAL)
					|| SetThreadPriority(theTargetThread, THREAD_PRIORITY_HIGHEST)
					|| SetThreadPriority(theTargetThread, THREAD_PRIORITY_ABOVE_NORMAL))
							success = true;

				CloseHandle(theTargetThread);
			}
		}

		FreeLibrary(kernel32);
	}

	if (success)
	{	return true; }
	else
	{
		// Supposedly this works under Win98, but it is not documented
		HANDLE theTargetThread = reinterpret_cast<HANDLE>(SDL_GetThreadID(inThread));
		return SetThreadPriority(theTargetThread, THREAD_PRIORITY_TIME_CRITICAL) != 0
			|| SetThreadPriority(theTargetThread, THREAD_PRIORITY_HIGHEST) != 0
			|| SetThreadPriority(theTargetThread, THREAD_PRIORITY_ABOVE_NORMAL) != 0
			|| TryToReduceMainThreadPriority();
	}
}
