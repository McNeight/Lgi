#if !defined(_GCOM_H_) && defined(WIN32)
#define _GCOM_H_

#ifdef __GNUC__
#include <unknwn.h>
#define COM_NO_WINDOWS_H
#endif
#include <wtypes.h>
#include <oaidl.h>
#include <ShlObj.h>

#include "GContainers.h"
#include "GVariant.h"

template <class T>
class GUnknownImpl : public T
{
	int Count;

	class Interface
	{
	public:
		REFIID iid;
		void *pvObject;

		Interface(REFIID i, void *p) : iid(i)
		{
			pvObject = p;
		}
	};
List<Interface> Interfaces;

protected:
	bool TraceRefs;

	void AddInterface(REFIID iid, void *pvObject)
	{
		Interface *i = new Interface(iid, pvObject);
		if (i)
		{
			Interfaces.Insert(i);
		}
	}

public:
	GUnknownImpl()
	{
		Count = 0;
		TraceRefs = false;
	}

	virtual ~GUnknownImpl()
	{
		Interfaces.DeleteObjects();
	}
	
	// IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject)
	{
		if (memcmp(&IID_IUnknown, &iid, sizeof(IID)) == 0)
		{
			*ppvObject = this;
			AddRef();
			return S_OK;
		}

		for (Interface *i = Interfaces.First(); i; i=Interfaces.Next())
		{
			if (memcmp(&i->iid, &iid, sizeof(IID)) == 0)
			{
				*ppvObject = i->pvObject;
				AddRef();
				return S_OK;
			}
		}

		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef()
	{
		if (TraceRefs)
			LgiTrace("%s:%i - %p::AddRef %i\n", _FL, this, Count+1);
		return ++Count;
	}
	
	ULONG STDMETHODCALLTYPE Release()
	{
		int i = --Count;
		if (TraceRefs)
			LgiTrace("%s:%i - %p::Release %i\n", _FL, this, Count);
		if (i <= 0)
		{
			delete this;
		}
		return  i;
	}

	// Helpers
	BSTR VariantToBstr(VARIANT *v)
	{
		if (v)
		{
			if (v->vt == (VT_VARIANT | VT_BYREF)) v = v->pvarVal;
			
			if (v->vt == VT_BSTR) return v->bstrVal;
			if (v->vt == (VT_BSTR | VT_BYREF)) return *v->pbstrVal;
		}

		return 0;
	}
};

template <class T>
class GDispatchImpl : public T
{
public:
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT __RPC_FAR *pctinfo)
	{
		return S_OK;
	}
    
    HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
	{
		return S_OK;
	}
    
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid,
											LPOLESTR __RPC_FAR *rgszNames,
											UINT cNames,
											LCID lcid,
											DISPID __RPC_FAR *rgDispId)
	{
		return S_OK;
	}
    
    HRESULT STDMETHODCALLTYPE Invoke(		DISPID dispIdMember,
											REFIID riid,
											LCID lcid,
											WORD wFlags,
											DISPPARAMS __RPC_FAR *pDispParams,
											VARIANT __RPC_FAR *pVarResult,
											EXCEPINFO __RPC_FAR *pExcepInfo,
											UINT __RPC_FAR *puArgErr)
	{
		return S_OK;
	}
};

/// This class wraps a IStream in an GStream interface...
class IStreamWrap : public GStream
{
	IStream *s;
	
public:
	GAutoPtr<FILEDESCRIPTORW> Desc;

	IStreamWrap(IStream *str) { s = str; }	
	~IStreamWrap() { Close(); }

	int Open(const char *Str = 0, int Int = 0) { return IsOpen(); }
	bool IsOpen() { return s != NULL; }

	bool GetVariant(const char *Name, GVariant &Value, char *Array = NULL) override
	{
		if (!Desc) return false;
		GDomProperty p = LgiStringToDomProp(Name);
		switch (p)
		{
			case ObjName:
				Value.OwnStr(WideToUtf8(Desc->cFileName));
				return true;
			case ObjLength:
				Value = ((int64) Desc->nFileSizeHigh << 32) | Desc->nFileSizeLow;
				return true;
			default:
				LgiAssert(!"Unknown field.");
				break;
		}
		
		return false;
	}
	
	bool SetVariant(const char *Name, GVariant &Value, char *Array = NULL) override
	{
		LgiAssert(!"Impl me.");
		return false;
	}

	int Close() override
	{
		if (s)
		{
			s->Release();
			s = NULL;
		}
		return 0;
	}

	int64 GetSize() override
	{
		STATSTG sz;
		HRESULT res = s->Stat(&sz, STATFLAG_DEFAULT);
		if (SUCCEEDED(res))
			return sz.cbSize.QuadPart;
		return -1;
	}

	int64 SetSize(int64 Size) override
	{
		ULARGE_INTEGER sz;
		sz.QuadPart = Size;
		HRESULT res = s->SetSize(sz);
		return SUCCEEDED(res) ? Size : -1;
	}

	int64 GetPos() override
	{
		LARGE_INTEGER i;
		ULARGE_INTEGER o;
		i.QuadPart = 0;
		HRESULT res = s->Seek(i, STREAM_SEEK_CUR, &o);
		if (SUCCEEDED(res))
			return o.QuadPart;
		return -1;
	}

	int64 SetPos(int64 Pos) override
	{
		LARGE_INTEGER i;
		ULARGE_INTEGER o;
		i.QuadPart = Pos;
		HRESULT res = s->Seek(i, STREAM_SEEK_SET, &o);
		if (SUCCEEDED(res))
			return o.QuadPart;
		return -1;
	}

	ssize_t Read(void *Ptr, ssize_t Size, int Flags = 0) override
	{
		ULONG Rd = 0;
		HRESULT res = s->Read(Ptr, Size, &Rd);
		return SUCCEEDED(res) ? Rd : 0;
	}
	
	ssize_t Write(const void *Ptr, ssize_t Size, int Flags = 0) override
	{
		ULONG Wr = 0;
		HRESULT res = s->Write(Ptr, Size, &Wr);
		return SUCCEEDED(res) ? Wr : 0;
	}

	GStreamI *Clone() override
	{
		return new IStreamWrap(s);
	}
};

/// This class wraps a GStream in an IStream interface...
class GStreamWrap : public GUnknownImpl<IStream>
{
	bool Own;
	GStream *s;

public:
	GStreamWrap(GStream *src, bool own = true)
	{
		s = src;
		Own = own;
		TraceRefs = true;
		AddInterface(IID_IStream, (IStream*)this);
	}
	
	~GStreamWrap()
	{
		if (Own)
			delete s;
	}
	
	HRESULT STDMETHODCALLTYPE Read(void *pv, ULONG cb, ULONG *pcbRead)
	{
		if (!s || !pv) return E_INVALIDARG;

		ULONG TotalRd = 0;
		uint8 *Ptr = (uint8*)pv;
		ULONG i = 0;
		while (i < cb)
		{
			int Remaining = cb - i;
			int Rd = s->Read(Ptr + i, Remaining);
			
			// LgiTrace("Read(%i)=%i\n", cb, Rd);
			
			if (Rd > 0)
				i += Rd;				
			else
				break;
		}

		if (pcbRead)
			*pcbRead = i;
		return S_OK;
	}
	
	HRESULT STDMETHODCALLTYPE Write(const void *pv, ULONG cb, ULONG *pcbWritten)
	{
		if (!s || !pv) return E_INVALIDARG;
		int Wr = s->Write(pv, cb);
		if (pcbWritten) *pcbWritten = Wr;
		return S_OK;
	}    

	HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
	{
		if (!s) return E_INVALIDARG;
		
		int64 NewPos = -1;
		switch (dwOrigin)
		{
			case STREAM_SEEK_SET:
				NewPos = s->SetPos(dlibMove.QuadPart);
				break;
			case STREAM_SEEK_CUR:
				NewPos = s->SetPos(s->GetPos() + dlibMove.QuadPart);
				break;
			case STREAM_SEEK_END:
				NewPos = s->SetPos(s->GetSize() - dlibMove.QuadPart);
				break;
			// default: return E_INVALIDARG;
		}

		if (NewPos < 0)
			return E_NOTIMPL;
		
		if (plibNewPosition)
			plibNewPosition->QuadPart = NewPos;
		return S_OK;
	}
			
	HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize)
	{
		if (!s) return E_INVALIDARG;
		if (s->SetSize(libNewSize.QuadPart) != libNewSize.QuadPart)
			return E_FAIL;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Stat(__RPC__out STATSTG *pstatstg, DWORD grfStatFlag)
	{
		if (!pstatstg)
			return E_INVALIDARG;
		
		GVariant Name;
		if (pstatstg->pwcsName &&
			s->GetValue("FileName", Name))
		{
			GAutoWString w(Utf8ToWide(Name.Str()));
			Strcpy(pstatstg->pwcsName, 256, w.Get());
		}
		
		pstatstg->type = STGTY_STREAM;
		pstatstg->cbSize.QuadPart = s->GetSize();
		
		return S_OK;
	}
	
	#define NotImplemented { LgiAssert(!"Function not implemented."); return E_NOTIMPL; }
	
	HRESULT STDMETHODCALLTYPE CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten) NotImplemented
	HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags) NotImplemented
	HRESULT STDMETHODCALLTYPE Revert() NotImplemented
	HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) NotImplemented
	HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) NotImplemented
	HRESULT STDMETHODCALLTYPE Clone(__RPC__deref_out_opt IStream **ppstm) NotImplemented
};

#endif
