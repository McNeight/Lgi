/// \file
#ifndef _GMUTEX_H_
#define _GMUTEX_H_

/// This is a re-enterant mutex class for thread locking.
class LgiClass LMutex
{
	OsThreadId _Thread;
	OsSemaphore _Sem;
	const char *File;
	int Line;
	#ifdef _DEBUG
	bool _DebugSem;
	#endif
	
	bool _Lock();
	void _Unlock();
	char *_Name;

protected:
	int _Count;

public:
	/// Constructor
	LMutex
	(
		/// Optional name for the semaphore
		const char *name = 0
	);
	virtual ~LMutex();

	/// Lock the semaphore, waiting forever
	///
	/// \returns true if locked successfully.
	bool Lock
	(
		/// The file name of the locker
		const char *file,
		/// The line number of the locker
		int line,
		/// [Optional] No trace to prevent recursion in LgiTrace.
		bool NoTrace = false
	);
	/// \returns true if the semephore was locked in the time
	bool LockWithTimeout
	(
		/// In ms
		int Timeout,
		/// The file of the locker
		const char *file,
		/// The line number of the locker
		int line
	);
	/// Unlocks the semaphore
	void Unlock();

	char *GetName();
	void SetName(const char *s);
	
	#ifdef _DEBUG
	void SetDebug(bool b = true) { _DebugSem = b; }
	int GetCount() { return _Count; }
	#endif
	
	class Auto
	{
	    LMutex *Sem;
	    bool Locked;
	    const char *File;
	    int Line;
	
	public:
	    Auto(LMutex *s, const char *file, int line)
	    {
	        LgiAssert(s != NULL);
	        Locked = (Sem = s) ? Sem->Lock(File = file, Line = line) : 0;
			LgiAssert(Locked);
	    }

	    Auto(LMutex *s, int timeout, const char *file, int line)
	    {
	        LgiAssert(s != NULL);
	        Locked = (Sem = s) ? Sem->LockWithTimeout(timeout, File = file, Line = line) : 0;
	    }
	    
	    ~Auto()
	    {
	        if (Locked) Sem->Unlock();
	    }

		bool GetLocked() { return Locked; }
		const char *GetFile() { return File; }
		int GetLine() { return Line; }
	};
};

#endif
