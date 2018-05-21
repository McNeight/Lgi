#ifndef _GEVENTTARGETTHREAD_H_
#define _GEVENTTARGETTHREAD_H_

#include "LThread.h"
#include "LMutex.h"
#include "LThreadEvent.h"
#include "LCancel.h"

#define PostThreadEvent GEventSinkMap::Dispatch.PostEvent

class LgiClass GEventSinkMap : public LMutex
{
protected:
	GHashTbl<int,GEventSinkI*> ToPtr;
	GHashTbl<void*,int> ToHnd;

public:
	static GEventSinkMap Dispatch;

	GEventSinkMap(int SizeHint = 0) :
		ToPtr(SizeHint, false, 0, NULL),
		ToHnd(SizeHint, false, NULL, 0)
	{
	}

	virtual ~GEventSinkMap()
	{
	}

	int AddSink(GEventSinkI *s)
	{
		if (!s || !Lock(_FL))
			return ToPtr.GetNullKey();

		// Find free handle...
		int Hnd;
		while (ToPtr.Find(Hnd = LgiRand(10000) + 1))
			;

		// Add the new sink
		ToPtr.Add(Hnd, s);
		ToHnd.Add(s, Hnd);

		Unlock();

		return Hnd;
	}

	bool RemoveSink(GEventSinkI *s)
	{
		if (!s || !Lock(_FL))
			return false;

		bool Status = false;
		int Hnd = ToHnd.Find(s);
		if (Hnd > 0)
		{
			Status |= ToHnd.Delete(s);
			Status &= ToPtr.Delete(Hnd);
		}
		else
			LgiAssert(!"Not a member of this sink.");

		Unlock();
		return Status;
	}

	bool RemoveSink(int Hnd)
	{
		if (!Hnd || !Lock(_FL))
			return false;

		bool Status = false;
		void *Ptr = ToPtr.Find(Hnd);
		if (Ptr)
		{
			Status |= ToHnd.Delete(Ptr);
			Status &= ToPtr.Delete(Hnd);
		}
		else
			LgiAssert(!"Not a member of this sink.");

		Unlock();
		return Status;
	}

	bool PostEvent(int Hnd, int Cmd, GMessage::Param a = 0, GMessage::Param b = 0)
	{
		if (!Hnd)
			return false;
		if (!Lock(_FL))
			return false;

		GEventSinkI *s = (GEventSinkI*)ToPtr.Find(Hnd);
		bool Status = false;
		if (s)
			Status = s->PostEvent(Cmd, a, b);
		#if _DEBUG
		else
			// This is not fatal, but we might want to know about it in DEBUG builds:
			LgiTrace("%s:%i - Sink associated with handle '%i' doesn't exist.\n", _FL, Hnd);
		#endif

		Unlock();
		return Status;
	}
	
	bool CancelThread(int Hnd)
	{
		if (!Hnd)
			return false;
		
		if (!Lock(_FL))
			return false;
		
		GEventSinkI *s = (GEventSinkI*)ToPtr.Find(Hnd);
		bool Status = false;
		if (s)
		{
			LCancel *c = dynamic_cast<LCancel*>(s);
			if (c)
			{
				Status = c->Cancel(true);
			}
			else
			{
				LgiTrace("%s:%i - GEventSinkI is not an LCancel object.\n", _FL);
			}
		}
		
		Unlock();
		return Status;
	}
};

class LgiClass GMappedEventSink : public GEventSinkI
{
protected:
	int Handle;
	GEventSinkMap *Map;

public:
	GMappedEventSink()
	{
		Map = NULL;
		Handle = 0;
		SetMap(&GEventSinkMap::Dispatch);
	}

	virtual ~GMappedEventSink()
	{
		SetMap(NULL);
	}

	bool SetMap(GEventSinkMap *m)
	{
		if (Map)
		{
			if (!Map->RemoveSink(this))
				return false;
			Map = 0;
			Handle = 0;
		}
		Map = m;
		if (Map)
		{
			Handle = Map->AddSink(this);
			return Handle > 0;
		}
		return true;
	}

	int GetHandle()
	{
		return Handle;
	}
};

/// This class is a worker thread that accepts messages on it's GEventSinkI interface.
/// To use, sub class and implement the OnEvent handler.
class LgiClass GEventTargetThread :
	public LThread,
	public LMutex,
	public GMappedEventSink,
	public GEventTargetI // Sub-class has to implement OnEvent
{
	GArray<GMessage*> Msgs;
	LThreadEvent Event;
	bool Loop;

	// This makes the event name unique on windows to 
	// prevent multiple instances clashing.
	GString ProcessName(GString obj, const char *desc)
	{
		OsProcessId process = LgiGetCurrentProcess();
		OsThreadId thread = GetCurrentThreadId();
		GString s;
		s.Printf("%s.%s.%i.%i", obj.Get(), desc, process, thread);
		return s;
	}

protected:
	int PostTimeout;
	size_t Processing;
	uint32 TimerMs; // Milliseconds between timer ticks.
	uint64 TimerTs; // Time for next tick

public:
	GEventTargetThread(GString Name) :
		LThread(Name + ".Thread"),
		LMutex(Name + ".Mutex"),
		Event(ProcessName(Name, "Event"))
	{
		Loop = true;
		PostTimeout = 1000;
		Processing = 0;
		TimerMs = 0;
		TimerTs = 0;

		Run();
	}
	
	virtual ~GEventTargetThread()
	{
		EndThread();
	}
	
	/// Set or clear a timer. Every time the timer expires, the function
	/// OnPulse is called. Until SetPulse() is called.
	bool SetPulse(uint32 Ms = 0)
	{
		TimerMs = Ms;
		TimerTs = Ms ? LgiCurrentTime() + Ms : 0;
		return Event.Signal();
	}
	
	/// Called roughly every 'TimerMs' milliseconds.
	/// Be aware however that OnPulse is called from the worker thread, not your main
	/// GUI thread. So best to send a message or something thread safe.
	virtual void OnPulse() {}

	void EndThread()
	{
		if (Loop)
		{
			// We can't be locked here, because GEventTargetThread::Main needs
			// to lock to check for messages...
			Loop = false;
			
			if (GetCurrentThreadId() == LThread::GetId())
			{
				// Being called from within the thread, in which case we can't signal 
				// the event because we'll be stuck in this loop and not waitin on it.
				#ifdef _DEBUG
				LgiTrace("%s:%i - EndThread called from inside thread.\n", _FL);
				#endif
			}
			else
			{			
				Event.Signal();
				
				uint64 Start = LgiCurrentTime();
				
				while (!IsExited())
				{
					LgiSleep(10);
					
					uint64 Now = LgiCurrentTime();
					if (Now - Start > 2000)
					{
						#ifdef LINUX
						int val = 1111;
						int r = sem_getvalue(Event.Handle(), &val);

						printf("%s:%i - EndThread() hung waiting for %s to exit (caller.thread=%i, worker.thread=%i, event=%i, r=%i, val=%i).\n",
							_FL, LThread::GetName(),
							GetCurrentThreadId(),
							GetId(),
							Event.Handle(),
							r,
							val);
						#else
						printf("%s:%i - EndThread() hung waiting for %s to exit (caller.thread=0x%x, worker.thread=0x%x, event=%p).\n",
							_FL, LThread::GetName(),
							(int)GetCurrentThreadId(),
							(int)GetId(),
							(void*)Event.Handle());
						#endif
						
						Start = Now;
					}
				}
			}
		}
	}

	size_t GetQueueSize()
	{
		return Msgs.Length() + Processing;
	}

	bool PostEvent(int Cmd, GMessage::Param a = 0, GMessage::Param b = 0)
	{
		if (!Loop)
			return false;
		if (!Lock(_FL))
			return false;
		
		Msgs.Add(new GMessage(Cmd, a, b));
		Unlock();
		
		return Event.Signal();
	}
	
	int Main()
	{
		while (Loop)
		{
			int WaitLength = -1;
			if (TimerTs != 0)
			{
				uint64 Now = LgiCurrentTime();
				if (TimerTs > Now)
				{
					WaitLength = (int) (TimerTs - Now);
				}
				else
				{
					OnPulse();
					if (TimerMs)
					{
						TimerTs = Now + TimerMs;
						WaitLength = (int) TimerTs;
					}
					else WaitLength = -1;
				}
			}
			
			LThreadEvent::WaitStatus s = Event.Wait(WaitLength);
			if (s == LThreadEvent::WaitSignaled)
			{
				GArray<GMessage*> m;
				if (Lock(_FL))
				{
					if (Msgs.Length())
					{
						m = Msgs;
						Msgs.Length(0);
					}
					Unlock();
				}

				Processing = m.Length();
				for (unsigned i=0; Loop && i < m.Length(); i++)
				{
					Processing--;
					OnEvent(m[i]);
				}
				m.DeleteObjects();
			}
			else if (s == LThreadEvent::WaitError)
			{
				LgiTrace("%s:%i - Event.Wait failed.\n", _FL);
				break;
			}
		}
		
		return 0;
	}

	template<typename T>
	bool PostObject(int Hnd, int Cmd, GAutoPtr<T> A)
	{
		uint64 Start = LgiCurrentTime();
		bool Status;
		while (!(Status = GEventSinkMap::Dispatch.PostEvent(Hnd, Cmd, (GMessage::Param) A.Get())))
		{
			LgiSleep(2);
			if (LgiCurrentTime() - Start >= PostTimeout) break;
		}
		if (Status)
			A.Release();
		return Status;
	}

	template<typename T>
	bool PostObject(int Hnd, int Cmd, GMessage::Param A, GAutoPtr<T> B)
	{
		uint64 Start = LgiCurrentTime();
		bool Status;
		while (!(Status = GEventSinkMap::Dispatch.PostEvent(Hnd, Cmd, A, (GMessage::Param) B.Get())))
		{
			LgiSleep(2);
			if (LgiCurrentTime() - Start >= PostTimeout) break;
		}
		if (Status)
			B.Release();
		return Status;
	}

	template<typename T>
	bool PostObject(int Hnd, int Cmd, GAutoPtr<T> A, GAutoPtr<T> B)
	{
		uint64 Start = LgiCurrentTime();
		bool Status;
		while (!(Status = GEventSinkMap::Dispatch.PostEvent(Hnd, Cmd, (GMessage::Param) A.Get(), (GMessage::Param) B.Get())))
		{
			LgiSleep(2);
			if (LgiCurrentTime() - Start >= PostTimeout) break;
		}
		if (Status)
		{
			A.Release();
			B.Release();
		}
		return Status;
	}
};



#endif
