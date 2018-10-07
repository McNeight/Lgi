#include "Lgi.h"
#include "GMidi.h"

#define MIDI_MIRROR_IN_TO_OUT		0

#ifdef WINDOWS

#define M_MIDI_IN	(M_USER + 2000)
class GMidiNotifyWnd : public GWindow
{
	GMidi *m;
	
public:
	GMidiNotifyWnd(GMidi *midi)
	{
		m = midi;
		Attach(0);
	}
	
	GMessage::Result OnEvent(GMessage *Msg)
	{
		if (MsgCode(Msg) == M_MIDI_IN)
		{
			m->ParseMidi();
		}
		
		return GView::OnEvent(Msg);
	}
};

#endif

struct GMidiPriv
{
	GMidi *Midi;
	
	#if defined(MAC)
		MIDIClientRef Client;
		MIDIPortRef InPort, OutPort;
		GArray<MIDIDeviceRef> Devs;
		GArray<MIDIEndpointRef> Srcs, Dsts;
		MIDIEndpointRef Dst;
		GArray<uint8> Data;
	#elif defined(WINDOWS)
		HMIDIIN hIn;
		HMIDIOUT hOut;
		MIDIHDR InHdr;
		char InBuf[2 << 10];
		GMidiNotifyWnd Notify;
	#elif defined(LINUX)
		int Hnd;
		GAutoPtr<LThread> Reader;
	#endif
	
	GMidiPriv(GMidi *m)
		#ifdef WINDOWS
		: Notify(m)
		#endif
	{
		Midi = m;

		#if defined(MAC)
		Client = 0;
		Dst = 0;
		#elif defined(WINDOWS)
		hIn = 0;
		hOut = 0;
		#elif defined(LINUX)
		Hnd = -1;
		#endif
	}
};

#if defined(MAC)

	GAutoString PrintMIDIObjectInfo(MIDIObjectRef o)
	{
		CFStringRef pName, pManuf, pModel;
		SInt32 Id;
		MIDIObjectGetStringProperty(o, kMIDIPropertyName, &pName);
		MIDIObjectGetStringProperty(o, kMIDIPropertyManufacturer, &pManuf);
		MIDIObjectGetStringProperty(o, kMIDIPropertyModel, &pModel);
		MIDIObjectGetIntegerProperty(o, kMIDIPropertyUniqueID, &Id);
		GString Name = pName;
		GString Man = pManuf;
		GString Mod = pModel;

		char s[256];
		sprintf(s, "%s, %s, %s, %x", Name.Get(), Man.Get(), Mod.Get(), (uint32)Id);
		printf("%s\n", s);

		CFRelease(pName);
		CFRelease(pManuf);
		CFRelease(pModel);
		
		return GAutoString(NewStr(s));
	}

	void MidiNotify(const MIDINotification *message, void *refCon)
	{
		GMidi *a = (GMidi*)refCon;
		a->OnMidiNotify(message);
	}

	void MidiRead(const MIDIPacketList *pktlist, void *readProcRefCon, void *srcConnRefCon)
	{
		GMidi *a = (GMidi*)readProcRefCon;
		if (a)
		{
			GArray<uint8> *d = &a->d->Data;
			
			// Collect all data from the input stream
			MIDIPacket *packet = (MIDIPacket*)pktlist->packet;
			for (unsigned int j = 0; j < pktlist->numPackets; j++)
			{
				for (unsigned i=0; i<packet->length; i++)
					d->Add(packet->data[i]);
				packet = MIDIPacketNext(packet);
			}

			// Parse into full MIDI packets
			int Len;
			while
			(
				(
					Len = a->GetMidiPacketSize( &(*d)[0], d->Length() )
				)
				> 0
			)
			{
				// Send full packet to the app
				a->OnMidiIn(&(*d)[0], Len);
				
				// Remove the message from the buffer
				if (d->Length() > Len)
				{
					int Remaining = d->Length() - Len;
					if (Remaining > 0)
					{
						memmove(&(*d)[0], &d[Len], Remaining);
						d->Length(Remaining);
					}
				}
				else
				{
					d->Length(0);
					break;
				}
			}
		}
	}

	void GMidi::OnMidiNotify(const MIDINotification *message)
	{
		printf("MIDI notify: %li\n", message->messageID);
	}

#elif defined(WINDOWS)

	void CALLBACK MidiOutProc(HMIDIOUT hmo, UINT wMsg, MIDI_TYPE dwInstance, MIDI_TYPE wParam, MIDI_TYPE lParam)
	{
		GMidi *a = (GMidi*)dwInstance;
		switch (wMsg)
		{
			case MM_MOM_OPEN:
				// ya ya... we're open for business.
				break;
			case MM_MOM_DONE:
			{
				MIDIHDR *Hdr = (MIDIHDR*)wParam;
				if (!Hdr->dwUser)
					a->OnMidiOut((uchar*)Hdr->lpData, Hdr->dwBufferLength);
				// a->GetLog()->Print("MidiOutProc MM_MOM_DONE\n");
				break;
			}
			default:
			{
				if (a->GetLog())
					a->GetLog()->Print("MidiOutProc 0x%x\n", wMsg);
				break;
			}
		}
	}

	void CALLBACK MidiInProc(HMIDIIN hmi, UINT wMsg, MIDI_TYPE dwInstance, MIDI_TYPE dwParam1, MIDI_TYPE dwParam2)
	{
		GMidi *a = (GMidi*)dwInstance;
		uint8 *b = 0;
		int len = 0;
		uint8 buf[3];

		// LgiTrace("MidiInProc wMsg=%i\n", wMsg);
		switch (wMsg)
		{
			case MM_MIM_OPEN:
				// ya ya... we're open for business.
				break;
			case MM_MIM_CLOSE:
				break;
			case MM_MIM_DATA:
			{
				buf[0] = (uint8) (dwParam2 & 0xff);
				buf[1] = (uint8) ((dwParam2 >> 8) & 0xff);
				buf[2] = (uint8) ((dwParam2 >> 16) & 0xff);
				b = buf;
				
				switch (b[0] >> 4)
				{
					case 0x8:
					case 0x9:
					case 0xa:
					case 0xb:
					case 0xe:
						len = 3;
						break;
					case 0xc:
					case 0xd:
						len = 2;
						break;
					default:
						b = NULL;
						break;
				}
				break;
			}
			case MM_MIM_LONGDATA:
			{
				MIDIHDR *Hdr = (MIDIHDR*)dwParam1;
				b = (uint8*) Hdr->lpData;
				len = Hdr->dwBytesRecorded;
				
				if (len)
					midiInAddBuffer(hmi, Hdr, sizeof(*Hdr));
				break;
			}
			default:
			{
				if (a->GetLog())
					a->GetLog()->Print("MidiInProc 0x%x\n", wMsg);
				break;
			}
		}

		if (b && len > 0)
			a->StoreMidi(b, len);
	}

#elif defined (LINUX)

	#include "errno.h"

	class ReaderThread : public LThread
	{
		GMidiPriv *d;
		bool loop;
		
	public:
		ReaderThread(GMidiPriv *priv) : LThread("MidiReaderThread")
		{
			d = priv;
			loop = true;
			Run();
		}
		
		~ReaderThread()
		{
			loop = false;
			while (!IsExited())
				LgiSleep(1);
		}
		
		int Main()
		{
			while (d->Hnd >= 0 && loop)
			{
				char buf[128];
				int rd = read(d->Hnd, buf, sizeof(buf));
				if (rd > 0)
				{
					d->Midi->OnMidiIn(buf, rd);
				}
			}
			
			return 0;
		}
	};

#endif

GMidi::GMidi() : LMutex("GMidi")
{
	d = new GMidiPriv(this);

	#if defined(MAC)

		int Devs = MIDIGetNumberOfDevices();
		for (int i = 0; i < Devs; i++)
		{
			MIDIDeviceRef dev = MIDIGetDevice(i);
			d->Devs[i] = dev;
			GAutoString a = PrintMIDIObjectInfo(dev);
			In.New().Reset(NewStr(a));
			Out.New().Reset(NewStr(a));

			int nEnt = MIDIDeviceGetNumberOfEntities(dev);
			// printf("Device %d has %d entities:\n", i, nEnt);
			for (int n = 0; n < nEnt; ++n)
			{
				MIDIEntityRef ent = MIDIDeviceGetEntity(dev, n);
				// printf("	 ");
				// PrintMIDIObjectInfo(ent);
				int nSources = MIDIEntityGetNumberOfSources(ent);
				// printf("	 Entity %d has %d source endpoints:\n", n, nSources);
				for (int k = 0; k < nSources; ++k)
				{
					MIDIEndpointRef sep = MIDIEntityGetSource(ent, k);
					d->Srcs[i] = sep;
					// printf("	   ");
					// PrintMIDIObjectInfo(sep);
				}

				int nDestinations = MIDIEntityGetNumberOfDestinations(ent);
				// printf("	 Entity %d has %d destination endpoints:\n", i, nDestinations);
				for (int l = 0; l < nDestinations; ++l)
				{
					MIDIEndpointRef dep = MIDIEntityGetDestination(ent, l);
					d->Dsts[i] = dep;
					// printf("	   ");
					// PrintMIDIObjectInfo(dep);
				}
			}
		}
	
	#elif defined(WINDOWS)

		UINT InDevs = midiInGetNumDevs();
		UINT OutDevs = midiOutGetNumDevs();
		UINT n;

		MIDIINCAPS InCaps;
		for (n=0; n<InDevs; n++)
		{
			MMRESULT r = midiInGetDevCaps(n, &InCaps, sizeof(InCaps));
			if (r == MMSYSERR_NOERROR)
			{
				#ifdef UNICODE
				In.New().Reset(WideToUtf8(InCaps.szPname));
				#else
				In.New().Reset(NewStr(InCaps.szPname));
				#endif
			}
		}

		MIDIOUTCAPS OutCaps;
		for (n=0; n<OutDevs; n++)
		{
			MMRESULT r = midiOutGetDevCaps(n, &OutCaps, sizeof(OutCaps));
			if (r == MMSYSERR_NOERROR)
			{
				#ifdef UNICODE
				Out.New().Reset(WideToUtf8(OutCaps.szPname));
				#else
				Out.New().Reset(NewStr(OutCaps.szPname));
				#endif
			}
		}
	
	#elif defined(LINUX)
	
		GDirectory Dir;
		const char *Path = "/dev";
		for (int b = Dir.First(Path); b; b = Dir.Next())
		{
			if (!Dir.IsDir() &&
				stristr(Dir.GetName(), "midi"))
			{
				char Full[128];
				LgiMakePath(Full, sizeof(Full), Path, Dir.GetName());
				printf("Midi[%i] = %s\n", In.Length(), Full);
				In.New().Reset(NewStr(Full));
				Out.New().Reset(NewStr(Full));
			}
		}
	
	#endif
}

GMidi::~GMidi()
{
	CloseMidi();
	delete d;
}

bool GMidi::IsMidiOpen()
{
	#if defined(MAC)
	return d->Client != 0;
	#elif defined(WINDOWS)
	return d->hIn != 0;
	#elif defined(LINUX)
	return d->Hnd >= 0;
	#endif
}

int GMidi::GetMidiPacketSize(uint8 *ptr, int len)
{
	if (!ptr || len < 1)
		return 0;

	if (ptr[0] == MIDI_SYSEX_START)
	{
		int i;
		for (i=1; i<len; i++)
		{
			if (ptr[i] == MIDI_SYSEX_END)
				return i + 1;
		}
	}
	else
	{
		switch (ptr[0] >> 4)
		{
			case 0x8:
			case 0x9:
			case 0xa:
			case 0xb:
			case 0xe:
				if (len >= 3)
					return 3;
				break;
			case 0xc:
			case 0xd:
				if (len >= 2)
					return 2;
				break;
			default:
				return 1;
				break;
		}
	}

	return 0;
}

#ifdef WINDOWS

void GMidi::ParseMidi()
{
	if (Lock(_FL))
	{
		while (MidiIn.Length() > 0)
		{
			int len = GetMidiPacketSize(&MidiIn[0], MidiIn.Length());
			if (len > 0)
			{
				OnMidiIn(&MidiIn[0], len);
				int Remaining = MidiIn.Length() - len;
				if (Remaining > 0)
				{
					memmove(&MidiIn[0], &MidiIn[len], Remaining);
					MidiIn.Length(Remaining);
				}
				else
				{
					MidiIn.Length(0);
					break;
				}
			}
			else
			{
				LgiAssert(0);
				break;
			}
		}
		Unlock();
	}
}

void GMidi::StoreMidi(uint8 *ptr, int len)
{
	if (Lock(_FL))
	{
		MidiIn.Add(ptr, len);
		Unlock();
		
		#ifdef WINDOWS
		d->Notify.PostEvent(M_MIDI_IN);
		#endif
	}
}

#endif

bool GMidi::Connect(int InIdx, int OutIdx, GAutoString *ErrorMsg)
{
	if (IsMidiOpen())
	{
		if (ErrorMsg)
			ErrorMsg->Reset(NewStr("Already connected."));
		return false;
	}

	bool Status = false;
	
	#ifdef MAC
	
		CFStringRef Name = CFSTR("AxeFoot");
		OSStatus e = MIDIClientCreate(Name, MidiNotify, this, &d->Client);
		if (e)
		{
			if (GetLog())
				GetLog()->Print("%s:%i - MIDIClientCreate failed with %i\n", _FL, e);
		}
		else
		{
			e = MIDIInputPortCreate(d->Client, CFSTR("AxeFootInput"), MidiRead, this, &d->InPort);
			if (e)
			{
				if (GetLog())
					GetLog()->Print("%s:%i - MIDIInputPortCreate failed with %i\n", _FL, e);
			}
			else
			{
				e = MIDIOutputPortCreate(d->Client, CFSTR("AxeFootOutput"), &d->OutPort);
				if (e)
				{
					if (GetLog())
						GetLog()->Print("%s:%i - MIDIOutputPortCreate failed with %i\n", _FL, e);
				}
				else
				{
					MIDIEndpointRef Src = d->Srcs[InIdx];
					d->Dst = d->Dsts[OutIdx];

					e = MIDIPortConnectSource(d->InPort, Src, this);
					if (e)
					{
						if (GetLog())
							GetLog()->Print("%s:%i - MIDIPortConnectSource failed with %i\n", _FL, e);
					}
					else
					{
						Status = true;
						if (GetLog())
							GetLog()->Print("Created midi client ok.\n");
					}
				}
			}
		}

	#elif defined(WINDOWS)

		MMRESULT r = midiOutOpen(&d->hOut, OutIdx, (MIDI_TYPE)MidiOutProc, (MIDI_TYPE)this, CALLBACK_FUNCTION);
		if (r != MMSYSERR_NOERROR)
		{
			OnError("midiOutOpen", ErrorMsg, r, _FL);
		}
		else
		{
			r = midiInOpen(&d->hIn, InIdx, (MIDI_TYPE)MidiInProc, (MIDI_TYPE)this, CALLBACK_FUNCTION);
			if (r != MMSYSERR_NOERROR)
			{
				OnError("midiInOpen", ErrorMsg, r, _FL);
			}
			else
			{
				ZeroObj(d->InHdr);
				d->InHdr.lpData = d->InBuf;
				d->InHdr.dwBufferLength = sizeof(d->InBuf);

				r = midiInPrepareHeader(d->hIn, &d->InHdr, sizeof(d->InHdr));
				if (r != MMSYSERR_NOERROR)
				{
					OnError("midiInPrepareHeader", ErrorMsg, r, _FL);
				}
				else
				{
					r = midiInAddBuffer(d->hIn, &d->InHdr, sizeof(d->InHdr));
					if (r != MMSYSERR_NOERROR && GetLog())
						GetLog()->Print("%s:%i - midiInAddBuffer failed with %i\n", _FL, r);
				}

				r = midiInStart(d->hIn);
				if (r != MMSYSERR_NOERROR)
				{
					OnError("midiInStart", ErrorMsg, r, _FL);
				}
				else
				{
					Status = true;
					if (GetLog())
						GetLog()->Print("Created midi connections ok.\n");
				}
			}
		}

	#elif defined(LINUX)

		if (InIdx != OutIdx)
		{
			if (ErrorMsg)
				ErrorMsg->Reset(NewStr("Different indexes not supported yet."));
		}
		else if (InIdx < 0 || InIdx >= In.Length())
		{
			if (ErrorMsg)
				ErrorMsg->Reset(NewStr("Invalid index."));
		}
		else
		{				
			d->Hnd = open(In[InIdx], O_RDWR);
			if (IsMidiOpen())
			{
				Status = true;
				d->Reader.Reset(new ReaderThread(d));
			}
			else if (ErrorMsg)
			{
				int e = errno;
				GAutoString msg = LgiErrorCodeToString(e);
				GString s;
				s.Printf("Error opening the device: %i (%s)\n", e, msg.Get());
				ErrorMsg->Reset(NewStr(s));
			}
		}

	#endif
	
	return Status;
}

void GMidi::CloseMidi()
{
	#if defined(MAC)

		if (d->Client)
		{
			MIDIClientDispose(d->Client);
			d->Client = 0;
		}

	#elif defined(WINDOWS)

		if (d->hIn)
		{
			midiInReset(d->hIn);
			midiInUnprepareHeader(d->hIn, &d->InHdr, sizeof(d->InHdr));
			midiInClose(d->hIn);
			d->hIn = 0;
		}
		if (d->hOut)
		{
			midiOutReset(d->hOut);
			midiOutClose(d->hOut);
			d->hOut = 0;
		}

	#elif defined(LINUX)

		if (d->Hnd >= 0)
		{
			close(d->Hnd);
			d->Hnd = -1;
		}

	#endif
}

void GMidi::OnMidiIn(uint8 *midi, int midi_len)
{
	#if MIDI_MIRROR_IN_TO_OUT
	SendMidi(p, len);
	#endif
}

void GMidi::SendMidi(uint8 *ptr, int len, bool quiet)
{
	LgiAssert(ptr != NULL);
	if (!ptr)
		return;

	#if defined(WINDOWS)
	
		if (d->hOut)
		{
			if (len <= 3)
			{
				DWORD dwMsg = 0;
				uint8 *b = (uint8*) &dwMsg;
				memcpy(b, ptr, len);
				MMRESULT r = midiOutShortMsg(d->hOut, dwMsg);
				if (r != 0)
				{
					if (GetLog())
						GetLog()->Print("midiOutShortMsg failed with %i\n", r);
				}

				if (!quiet)
					OnMidiOut(ptr, len);
			}
			else
			{
				MIDIHDR Hdr;
				ZeroObj(Hdr);
				Hdr.dwUser = quiet;
				Hdr.lpData = (LPSTR) ptr;
				Hdr.dwBufferLength = len;
				midiOutPrepareHeader(d->hOut, &Hdr, sizeof(Hdr));

				MMRESULT r = midiOutLongMsg(d->hOut, &Hdr, sizeof(Hdr));
				if (r != MMSYSERR_NOERROR)
				{
					if (GetLog())
						GetLog()->Print("midiOutLongMsg failed with %i\n", r);
				}

				midiOutUnprepareHeader(d->hOut, &Hdr, sizeof(Hdr));
			}
		}

	#elif defined MAC

		if (d->OutPort)
		{
			struct MIDIPacketList p;
			p.numPackets = 1;
			p.packet[0].timeStamp = 0;
			p.packet[0].length = len;
			memcpy(p.packet[0].data, ptr, len);
			OSStatus e = MIDISend(d->OutPort, d->Dst, &p);
			if (e)
			{
				if (GetLog())
					GetLog()->Print("%s:%i - MIDISend failed %e\n", _FL, e);
			}
			else if (!quiet)
			{
				if (GetLog())
					OnMidiOut(ptr, len);
			}
		}

	#elif defined LINUX

		if (IsMidiOpen())
		{
			int wr = write(d->Hnd, ptr, len);
			if (wr < len)
				printf("%s:%i - Warning: failed to write all bytes %i %i\n", _FL, len, wr);
			if (wr > 0 && !quiet)
				OnMidiOut(ptr, wr);
		}

	#endif
}

void GMidi::OnMidiOut(uint8 *p, int len)
{
	if (GetLog())
	{
		GetLog()->Print("Out: ");
		for (int i=0; i<len; i++)
			GetLog()->Print("%02.2x ", p[i]);
		GetLog()->Print("\n");
	}

	#if MIDI_MIRROR_IN_TO_OUT
	SendMidi(p, len);
	#endif
}

void GMidi::OnError(char *Func, GAutoString *ErrorMsg, uint32 Code, char *File, int Line)
{
	char m[256];
	sprintf_s(m, sizeof(m), "%s failed with %i", Func, Code);
	if (ErrorMsg)
		ErrorMsg->Reset(NewStr(m));
	if (GetLog())
		GetLog()->Print("%s:%i - %s\n", File, Line, m);
}
