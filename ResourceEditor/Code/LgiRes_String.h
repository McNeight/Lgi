/*
**	FILE:			LgiRes_String.h
**	AUTHOR:			Matthew Allen
**	DATE:			10/9/1999
**	DESCRIPTION:	Dialog Resource Editor
**
**
**	Copyright (C) 1999, Matthew Allen
**		fret@memecode.com
*/

#ifndef __LGIRES_STRING_H
#define __LGIRES_STRING_H

#include "Res.h"
#include "LList.h"

////////////////////////////////////////////////////////////////
class ResStringGroup;
class ResStringUi;

////////////////////////////////////////////////////////////////
class StrLang
{
	GLanguageId Lang;
	char *Str;

public:
	StrLang();
	~StrLang();

	GLanguageId GetLang();
	void SetLang(GLanguageId i);
	char *&GetStr();
	void SetStr(const char *s);
	bool IsEnglish();

	bool operator ==(GLanguageId LangId);
	bool operator !=(GLanguageId LangId);
};

class ResString : public LListItem, public FieldSource
{
	friend class ResStringGroup;
	friend class ResStringView;

protected:
	ResStringGroup *Group;

	char RefStr[16];
	char IdStr[16];
	char **GetIndex(int i);
	StrLang *GetLang(GLanguageId i);
	GAutoString Define;		// #define used in code to reference
	int Ref;			// globally unique
	int Id;				// numerical value used in code to reference

public:
	char *Tag;			// Optional component tag, for turning off features.
	List<StrLang> Items;
	GView *UpdateWnd;

	ResString(ResStringGroup *group, int init_ref = -1);
	~ResString();
	ResString &operator =(ResString &s);
	ResStringGroup *GetGroup() { return Group; }

	// Data
	int GetRef() { return Ref; }
	int SetRef(int r);
	int GetId() { return Id; }
	int SetId(int id);
	int NewId(); // Creates a new Id based on #define
	char *Get(GLanguageId Lang = 0);
	void Set(const char *s, GLanguageId Lang = 0);
	void UnDuplicate();
	int Compare(LListItem *To, int Field);
	void CopyText();
	void PasteText();
	char *GetDefine() { return Define; }
	void SetDefine(const char *s);

	// Item
	int GetCols();
	char *GetText(int i);
	void OnMouseClick(GMouse &m);

	// Fields
	bool GetFields(FieldTree &Fields);
	bool Serialize(FieldTree &Fields);

	// Persistence
	bool Test(ErrorCollection *e);
	bool Read(GXmlTag *t, SerialiseContext &Ctx);
	bool Write(GXmlTag *t, SerialiseContext &Ctx);
};

#define RESIZE_X1			0x0001
#define RESIZE_Y1			0x0002
#define RESIZE_X2			0x0004
#define RESIZE_Y2			0x0008

class ResStringGroup : public Resource, public LList
{
	friend class ResString;
	friend class ResStringView;
	friend class ResStringUi;

protected:
	ResStringUi *Ui;
	List<ResString> Strs;
	GArray<GLanguage*> Lang;
	GArray<GLanguage*> Visible;
	int SortCol;
	int SortAscend;

	void UpdateColumns();
	void OnColumnClick(int Col, GMouse &m);

public:
	ResStringGroup(AppWnd *w, int type = TYPE_STRING);
	~ResStringGroup();

	GView *Wnd() { return dynamic_cast<GView*>(this); }
	bool Attach(GViewI *Parent) { return LList::Attach(Parent); }
	ResStringGroup *GetStringGroup() { return this; }

	// Methods
	void Create(GXmlTag *load, SerialiseContext *ctx);
	ResString *CreateStr(bool User = false);
	void DeleteStr(ResString *Str);
	ResString *FindName(char *Name);
	ResString *FindRef(int Ref);
	int FindId(int Id, List<ResString*> &Strs);
	int UniqueRef();
	int UniqueId(char *Define = 0);
	int OnCommand(int Cmd, int Event, OsView hWnd);
	void SetLanguages();
	int GetLanguages() { return Lang.Length(); }
	GLanguage *GetLanguage(int i) { return i < Lang.Length() ? Lang[i] : 0; }
	GLanguage *GetLanguage(GLanguageId Id);
	int GetLangIdx(GLanguageId Id);
	void AppendLanguage(GLanguageId Id);
	void DeleteLanguage(GLanguageId Id);
	void Sort();
	void RemoveUnReferenced();
	List<ResString> *GetStrs() { return &Strs; }
	void OnShowLanguages();

	// Clipboard access
	void DeleteSel();
	void Copy(bool Delete = false);
	void Paste();

	// Resource
	GView *CreateUI();
	void OnRightClick(GSubMenu *RClick);
	void OnCommand(int Cmd);
	bool Test(ErrorCollection *e);
	bool Read(GXmlTag *t, SerialiseContext &Ctx);
	bool Write(GXmlTag *t, SerialiseContext &Ctx);
	ResStringGroup *IsStringGroup() { return this; }

	// LList
	void OnItemClick(LListItem *Item, GMouse &m);
	void OnItemSelect(GArray<LListItem*> &Items);
};

class ResStringUi : public GLayout
{
	GToolBar *Tools;
	ResStringGroup *StringGrp;
	GStatusBar *Status;
	GStatusPane *StatusInfo;

public:
	ResStringUi(ResStringGroup *Res);
	~ResStringUi();

	void OnPaint(GSurface *pDC);
	void PourAll();
	void OnPosChange();
	void OnCreate();
	GMessage::Result OnEvent(GMessage *Msg);
};

class LangDlg : public GDialog
{
	GCombo *Sel;
	List<GLanguage> Langs;

public:
	GLanguage *Lang;

	LangDlg(GView *parent, List<GLanguage> &l, int Init = -1);
	int OnNotify(GViewI *Ctrl, int Flags);
};

////////////////////////////////////////////////////////////////
#endif
