#ifndef _GTHREADEVENT_H_
#define _GTHREADEVENT_H_

#if defined(CARBON)
	#define USE_MACH_SEM		1
	#include <mach/task.h>
	#include <mach/semaphore.h>
#elif defined(LINUX)
    #define USE_POSIX_SEM		1
    #include <semaphore.h>
#endif

class LgiClass LThreadEvent : public GBase
{
	uint32 LastError;
	#if USE_MACH_SEM
		task_t Task;
		semaphore_t Sem;
    #elif USE_POSIX_SEM
        sem_t Local;
        sem_t *Sem;
	#elif defined(POSIX)
        pthread_cond_t Cond;
        pthread_mutex_t Mutex;
	#elif defined(WIN32)
        HANDLE Event;
	#endif

public:
	enum WaitStatus
	{
		WaitError,
		WaitTimeout,
		WaitSignaled,
	};
	
	LThreadEvent(const char *name = NULL);
	~LThreadEvent();

	#if USE_MACH_SEM
	semaphore_t Handle() { return Sem; }
    #elif USE_POSIX_SEM
    sem_t *Handle() { return Sem; }
	#elif defined(POSIX)
	int Handle() { return -1; } // ?
	#elif defined(WIN32)
    HANDLE Handle() { return Event; }
	#endif
		
	bool IsOk();
	bool Signal();
	WaitStatus Wait(int32 Timeout = -1);
	uint32 GetError();
};

#endif
