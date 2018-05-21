/*hdr
**      FILE:           GDb-Dbf.h
**      AUTHOR:         Matthew Allen
**      DATE:           8/2/2000
**      DESCRIPTION:    Separated variables implementation of the GDb API
**
**      Copyright (C) 2003 Matthew Allen
**			fret@memecode.com
*/

#include <stdio.h>

#include "Lgi.h"
#include "GDb.h"
#include "GToken.h"
#include "GTextFile.h"

///////////////////////////////////////////////////////////////////
// Forward decl
class SvField;
class SvRecord;
class SvDb;

char *LgiTsvTok(char *&s)
{
	static const char *Ws = " \r\n";
	while (*s && strchr(Ws, *s)) s++;
	if (*s == '\'' || *s == '\"')
	{
		char delim = *s++;
		char *start = s;
		char *end = strchr(s, delim);
		if (!end)
			return NULL;
		s = end + 1;
		if (*s == '\t') s++;
		return NewStr(start, end-start);
	}

	char *start = s;
	while (*s && *s != '\t') s++;
	char *end = s;
	if (start == end)
		return NULL;

	while (end > start && strchr(Ws, end[-1])) end--;
	if (*s == '\t') s++;
	return NewStr(start, end-start);
}

class SvRecordset : public GDbRecordset
{
	friend class SvField;
	
	SvDb *Parent;

	GAutoString FileName;
	GArray<SvField*> F;
	GArray<SvRecord*> R;
	unsigned Cur;
	SvRecord *Temp;
	SvRecord *New;
	bool Dirty;
	bool HasHeaders;

	void Empty();
	void Read();
	void Write();
	SvRecord *Record();

public:
	SvRecordset(SvDb *parent, const char *file, bool Headers);
	~SvRecordset();

	char *Name();
	GDbField &operator [](unsigned Index);
	GDbField &operator [](const char *Name);
	GDbField *InsertField(const char *Name, int Type, int Len, int Index);
	bool DeleteField(GDbField *Fld);
	int Fields();
	bool End();
	bool AddNew();
	bool Edit();
	bool Update();
	void Cancel();
	bool MoveFirst();
	bool MoveNext();
	bool MovePrev();
	bool MoveLast();
	int SeekRecord(int i);
	int RecordIndex();
	int Records();
	bool DeleteRecord();
	char GetSep();
};

class SvDb : public GDb
{
	friend class SvRecordset;

	char Separator;
	bool HasHeaders;
	List<SvRecordset> Tables;

public:
	SvDb(char sep, bool headers)
	{
		Separator = sep;
		HasHeaders = headers;
	}

	~SvDb();
	bool Connect(const char *Init);
	bool Disconnect();
	GDbRecordset *Open(char *Name);
	GDbRecordset *TableAt(int i);
	bool Tsv() { return Separator == '\t'; }
	char GetSep() { return Separator; }
};

class SvRecord
{
	int RawLen;
	GAutoString Raw;

public:
	SvRecordset *Rs;
	int Fields;
	char **Data;

	void EmptyField(int i)
	{
		if (Data &&
			i < Fields)
		{
			char *End = Raw + RawLen;
			if (!Raw || (Data[i] < Raw.Get() || Data[i] > End))
			{
				DeleteArray(Data[i]);
			}
		}
	}

	void Empty()
	{
		if (Data)
		{
			char *End = Raw + RawLen;
			for (int i=0; i<Fields; i++)
			{
				if (!Raw || (Data[i] < Raw.Get() || Data[i] > End))
				{
					DeleteArray(Data[i]);
				}
			}
		}

		Raw.Reset();
	}

	bool SetFields(int i)
	{
		char **b = new char*[i];
		if (b)
		{
			memset(b, 0, sizeof(char*)*i);
			if (Data && Fields > 0)
			{
				int m = MIN(Fields, i);
				memcpy(b, Data, sizeof(char*)*m);
			}
			DeleteArray(Data);
			Data = b;
			Fields = i;

			return true;
		}

		return false;
	}

	SvRecord(SvRecordset *rs)
	{
		Rs = rs;
		RawLen = 0;
		Fields = 0;
		Data = 0;
		SetFields(Rs->Fields());
	}

	SvRecord(SvRecordset *rs, SvRecord *r)
	{
		Rs = rs;
		RawLen = 0;
		Fields = 0;
		Data = 0;
		if (r) *this = *r;
	}

	SvRecord(SvRecordset *rs, char16 *In)
	{
		Rs = rs;
		RawLen = 0;
		Fields = 0;
		Data = 0;

		int Flds = 1;
		char16 *Start = In;
		char16 Sep = Rs->GetSep();
		for (RawLen=0; In[RawLen]; )
		{
			if (In[RawLen] == Sep)
			{
				Flds++;
				RawLen++;
			}
			else if (In[RawLen] == '\"')
			{
				RawLen++;
				while (In[RawLen] && In[RawLen] != '\"') RawLen++;
				RawLen++;
			}
			else if (In[RawLen] == '\n' || In[RawLen] == '\r')
			{
				break;
			}
			else
			{
				RawLen++;
			}
		}

		if (RawLen > 0)
		{
			In += RawLen;
			while (*In == '\r' || *In == '\n') In++;
			Raw.Reset(WideToUtf8(Start, RawLen));

			if (Raw && SetFields(Flds))
			{
				char *r = Raw;
				for (int n=0; n<Fields && r; n++)
				{
					while (*r == ' ') r++;

					char *e = r;
					if (*r == '\"')
					{
						e = ++r;
						while (*e && *e != '\"') e++;
						if (*e) *e++ = 0;
					}
					else
					{
						while (*e && *e != '\n' && *e != Sep) e++;
					}
					Data[n] = r;
					if (*e)
					{
						r = e + 1;
						*e = 0;

					}
					else
					{
						break;
					}
				}
			}
			else In = 0;
		}
	}

	~SvRecord()
	{
		DeleteArray(Data);
	}

	SvRecord &operator =(SvRecord &r)
	{
		Empty();
		SetFields(r.Fields);
		for (int i=0; i<Fields; i++)
		{
			Data[i] = NewStr(r.Data[i]);
		}

		return *this;
	}
};

class SvField : public GDbField
{
	int Index;
	GAutoString FldName;
	SvRecordset *Rs;

public:
	SvField(SvRecordset *rs, int i, GAutoString n)
	{
		Index = i;
		Rs = rs;
		FldName = n;
	}

	char *Name()
	{
		return FldName;
	}

	bool Name(char *Str)
	{
		return FldName.Reset(NewStr(Str));
	}

	int Type()
	{
		return GV_STRING;
	}

	bool Type(int NewType)
	{
		return false;
	}

	int Length()
	{
		return 255;
	}

	bool Length(int NewLength)
	{
		return false;
	}

	char *Description()
	{
		return 0;
	}

	bool Description(char *NewDesc)
	{
		return false;
	}

	bool Set(GVariant &v)
	{
		SvRecord *r = Rs->Record();
		if (r && v.Str())
		{
			if (Index < r->Fields ||
				r->SetFields(Index + 1))
			{
				r->EmptyField(Index);
				r->Data[Index] = NewStr(v.Str());
				return true;
			}
		}

		return false;
	}

	GDbField &operator =(char *ns)
	{
		SvRecord *r = Rs->Record();
		if (r)
		{
			if (Index < r->Fields)
			{
				r->EmptyField(Index);
				r->Data[Index] = NewStr(ns);
			}
		}
		
		return *this;		
	}

	operator char*()
	{
		SvRecord *r = Rs->Record();
		if (r)
		{
			if (Index < r->Fields)
			{
				return r->Data[Index];
			}
		}
		
		return 0;
	}

	bool Get(GVariant &v)
	{
		SvRecord *r = Rs->Record();
		if (r)
		{
			if (Index < r->Fields)
			{
				v = r->Data[Index];
				return true;
			}
		}
		
		return false;
	}
};

/////////////////////////////////////////////////////////////////////
SvRecordset::SvRecordset(SvDb *parent, const char *file, bool Headers)
{
	Parent = parent;
	HasHeaders = Headers;
	Temp = 0;
	New = 0;
	Dirty = false;
	FileName.Reset(NewStr(file));
	Read();
}

SvRecordset::~SvRecordset()
{
	if (Dirty)
	{
		Write();
	}
	if (Parent)
	{
		Parent->Tables.Delete(this);
	}

	Empty();
}

char *SvRecordset::Name()
{
	char *d = strrchr(FileName, DIR_CHAR);
	return d ? d + 1 : 0;
}

void SvRecordset::Empty()
{
	F.DeleteObjects();
	R.DeleteObjects();
}

void SvRecordset::Read()
{
	Empty();

	GTextFile f;
	if (f.Open(FileName, O_READ|O_SHARE))
	{
		GArray<char16> Line;
		while (f.GetLine(Line))
		{
			SvRecord *r = new SvRecord(this, &Line[0]);			
			if (F.Length() == 0)
			{
				if (HasHeaders)
				{
					// Headers...
					for (int i=0; i<r->Fields; i++)
					{
						char *Name = r->Data[i];
						GAutoString Field(TrimStr(Name, " \r\t\n\""));
						if (Field)
							F.Add(new SvField(this, i, Field));
					}
					
					DeleteObj(r);
				}
				else
				{
					for (int n=0; n<r->Fields; n++)
					{
						char Name[32];
						sprintf_s(Name, sizeof(Name), "Field%i", n);
						GAutoString a(NewStr(Name));
						F.Add(new SvField(this, n, a));
					}
					
					R.Add(r);
				}
			}
			else
			{
				R.Add(r);
			}
		}
	}
}

void SvRecordset::Write()
{
	GFile f;
	if (FileName && f.Open(FileName, O_WRITE))
	{
		char s[256];
		int NewLine = strlen(EOL_SEQUENCE);
		f.SetSize(0);

		// Headers
		for (unsigned Idx = 0; Idx < F.Length(); Idx++)
		{
			SvField *Fld = F[Idx];
			char *Name = Fld->Name();
			f.Write(s, sprintf_s(s, sizeof(s), "%s\"%s\"", Idx ? "," : "", Name));
		}
		f.Write((void*)EOL_SEQUENCE, NewLine);
	
		// Records
		for (bool b = MoveFirst(); b; b = MoveNext())
		{
			GVariant v;
			for (unsigned i=0; i<F.Length(); i++)
			{
				if (i)
				{
					f.Write((void*)",", 1);
				}

				if ((*this)[i].Get(v))
				{
					switch (v.Type)
					{
						default:
							break;
						case GV_STRING:
						{
							f.Write((char*)"\"", 1);
							for (char *c = v.Str(); *c; c++)
							{
								if (*c == '\"')
								{
									f.Write((char*)"\"\"\"", 3);
								}
								else f.Write(c, 1);
							}
							f.Write((char*)"\"", 1);
							break;
						}
					}
				}
			}
			f.Write((void*)EOL_SEQUENCE, NewLine);
		}
	}
}

SvRecord *SvRecordset::Record()
{
	if (Temp) return Temp;
	if (New) return New;	
	if (Cur < R.Length())
		return R[Cur];
	return NULL;
}

static GDbField Null;

GDbField &SvRecordset::operator [](unsigned Index)
{
	if (Index < F.Length())
		return *F[Index];

	return Null;
}

GDbField &SvRecordset::operator [](const char *Name)
{
	if (Name)
	{
		for (unsigned i=0; i<F.Length(); i++)
		{
			GDbField *f = F[i];
			if (f->Name() && _stricmp(Name, f->Name()) == 0)
			{
				return *f;
			}
		}
	}
	return Null;
}

GDbField *SvRecordset::InsertField(const char *Name, int Type, int Length, int Index)
{
	if (Index < 0)
	{
		Index = (int)F.Length();
	}

    GAutoString n(NewStr(Name));
	SvField *f = new SvField(this, Index, n);
	if (f)
	{
		F.AddAt(Index, f);
	}
	return f;
}

bool SvRecordset::DeleteField(GDbField *Fld)
{
	return 0;
}

int SvRecordset::Fields()
{
	return (int)F.Length();
}

bool SvRecordset::End()
{
	return Cur > R.Length();
}

bool SvRecordset::AddNew()
{
	Update();
	New = new SvRecord(this);
	return New != 0;
}

bool SvRecordset::Edit()
{
	Update();
	if (Cur < R.Length())
	{
		Temp = new SvRecord(this, R[Cur]);
	}
	return Temp != 0;
}

bool SvRecordset::Update()
{
	if (Temp)
	{
		SvRecord *c = Cur < R.Length() ? R[Cur] : NULL;
		if (c)
		{
			*c = *Temp;
			DeleteObj(Temp);
			Dirty = true;
			return true;
		}
	}
	else if (New)
	{
		R.Add(New);
		New = 0;
		Dirty = true;
		return true;
	}

	return false;
}

void SvRecordset::Cancel()
{
	DeleteObj(Temp);
	DeleteObj(New);
}

bool SvRecordset::MoveFirst()
{
	Cancel();
	Cur = 0;
	return R.Length() > 0;
}

bool SvRecordset::MoveNext()
{
	Cancel();
	Cur++;
	return Cur < R.Length();
}

bool SvRecordset::MovePrev()
{
	Cancel();
	
	if (Cur == 0)
		return false;
	Cur--;
	return Cur < R.Length();
}

bool SvRecordset::MoveLast()
{
	Cancel();

	if (R.Length() == 0)
		return false;
	
	Cur = (int)R.Length()-1;
	return true;
}

int SvRecordset::SeekRecord(int i)
{
	Cancel();
	Cur = i;
	return Cur < R.Length();
}

int SvRecordset::RecordIndex()
{
	return Cur;
}

int SvRecordset::Records()
{
	return (int)R.Length();
}

bool SvRecordset::DeleteRecord()
{
	return false;
}

char SvRecordset::GetSep()
{
	return Parent->GetSep();
}

//////////////////////////////////////////////////////////////////////////
SvDb::~SvDb()
{
	Disconnect();
}

bool SvDb::Connect(const char *Init)
{
	Tables.Insert(new SvRecordset(this, Init, HasHeaders));
	return Tables.First() != 0;
}

bool SvDb::Disconnect()
{
	Tables.DeleteObjects();
	return true;
}

GDbRecordset *SvDb::Open(char *Name)
{
	SvRecordset *rs = new SvRecordset(this, Name, HasHeaders);
	if (rs)
	{
		Tables.Insert(rs);
	}
	return rs;
}

GDbRecordset *SvDb::TableAt(int i)
{
	return Tables[i];
}

///////////////////////////////////////////////////////////////////
GDb *OpenCsvDatabase(char *Path, bool HasHeader)
{
	SvDb *Db = new SvDb(',', HasHeader);
	if (Db && Db->Connect(Path))
	{
		return Db;
	}
	DeleteObj(Db);
	return 0;
}

GDb *OpenTsvDatabase(char *Path, bool HasHeader)
{
	SvDb *Db = new SvDb('\t', HasHeader);
	if (Db && Db->Connect(Path))
	{
		return Db;
	}
	DeleteObj(Db);
	return 0;
}


