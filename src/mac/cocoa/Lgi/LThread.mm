#include "Lgi.h"
#include "LgiOsDefs.h"
#include "LMutex.h"
#include "GStringClass.h"
#include "LThread.h"
#include <errno.h>

#include <pthread.h>
#ifndef _PTHREAD_H
#error "Pthreads not included"
#endif

OsThreadId GetCurrentThreadId()
{
	uint64_t tid = 0;
	pthread_threadid_np(NULL, &tid);
	return tid;
}

////////////////////////////////////////////////////////////////////////////
void *ThreadEntryPoint(void *i)
{
	if (i)
	{
		LThread *Thread = (LThread*) i;
		
		// Make sure we have finished executing the setup
		while (Thread->State == LThread::THREAD_INIT)
		{
			LgiSleep(5);
		}
		
		pthread_detach(Thread->hThread);
		
		// Do thread's work
		Thread->OnBeforeMain();
		Thread->ReturnValue = Thread->Main();
		Thread->OnAfterMain();
		
		// mark thread over...
		Thread->State = LThread::THREAD_EXITED;
		
		if (Thread->DeleteOnExit)
		{
			DeleteObj(Thread);
		}
		
		pthread_exit(0);
	}
	return 0;
}

LThread::LThread(const char *name)
{
	State = THREAD_INIT;
	ReturnValue = -1;
	hThread = 0;
	DeleteOnExit = false;
	Priority = ThreadPriorityNormal;
}

LThread::~LThread()
{
	if (!IsExited())
	{
		Terminate();
	}
}

int LThread::ExitCode()
{
	return ReturnValue;
}

bool LThread::IsExited()
{
	return State == THREAD_EXITED;
}

void LThread::Run()
{
	if (!hThread)
	{
		State = THREAD_INIT;
		
		static int Creates = 0;
		int e;
		if (!(e = pthread_create(&hThread, NULL, ThreadEntryPoint, (void*)this)))
		{
			State = THREAD_RUNNING;
			Creates++;
			
			if (Priority != ThreadPriorityNormal)
			{
				int policy;
				sched_param param;
				e = pthread_getschedparam(hThread, &policy, &param);
				int min_pri = sched_get_priority_min(policy);
				int max_pri = sched_get_priority_max(policy);
				switch (Priority)
				{
					case ThreadPriorityIdle:
						param.sched_priority = min_pri;
						break;
					case ThreadPriorityNormal:
						break;
					case ThreadPriorityHigh:
						param.sched_priority = max_pri;
						break;
					case ThreadPriorityRealtime:
						param.sched_priority = max_pri;
						break;
				}
				e = pthread_setschedparam(hThread, policy, &param);
			}
		}
		else
		{
			const char *Err = "(unknown)";
			switch (e)
			{
				case EAGAIN: Err = "EAGAIN"; break;
				case EINVAL: Err = "EINVAL"; break;
				case EPERM: Err = "EPERM"; break;
				case ENOMEM: Err = "ENOMEM"; break;
			}
			printf(	"%s,%i - pthread_create failed with the error %i (%s) (After %i creates)\n",
				   _FL, e, Err, Creates);
			
			State = THREAD_EXITED;
		}
	}
}

void LThread::Terminate()
{
	if (hThread &&
		pthread_cancel(hThread) == 0)
	{
		State = THREAD_EXITED;
	}
}

int LThread::Main()
{
	return 0;
}

