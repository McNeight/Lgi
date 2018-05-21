/*
**	FILE:			LgiRes_Dialog.h
**	AUTHOR:			Matthew Allen
**	DATE:			5/8/1999
**	DESCRIPTION:	Dialog Resource Editor
**
**
**	Copyright (C) 1999, Matthew Allen
**		fret@memecode.com
*/

#ifndef __LGIRES_DIALOG_H
#define __LGIRES_DIALOG_H

#include "Res.h"
#include "LgiRes_String.h"

class ResDialog;
class ResDialogUi;

#define UI_CURSOR				0
#define UI_TABLE				1
#define UI_TEXT					2
#define UI_EDITBOX				3
#define UI_CHECKBOX				4
#define UI_BUTTON				5
#define UI_GROUP				6
#define UI_RADIO				7
#define UI_TABS					8
#define	UI_LIST					9
#define UI_COMBO				10
#define UI_TREE					11
#define UI_BITMAP				12
#define UI_PROGRESS				13
#define UI_SCROLL_BAR			14
#define UI_CUSTOM				15
#define UI_TAB					16
#define UI_COLUMN				17
#define UI_CONTROL_TREE			18
#define UI_DIALOG				100

#define GOOBER_SIZE				5
#define GOOBER_BORDER			7

#define GRID_X					7
#define GRID_Y					7

////////////////////////////////////////////////////////////////
#define DECL_DIALOG_CTRL(id)	 \
	int GetType() { return id; } \
	GView *View() { return this; } \
	void OnMouseClick(GMouse &m); \
	void OnMouseMove(GMouse &m); \
	void OnPaint(GSurface *pDC);

#define IMPL_DIALOG_CTRL(cls)	 \
	void cls::OnMouseClick(GMouse &m) { ResDialogCtrl::OnMouseClick(m); } \
	void cls::OnMouseMove(GMouse &m) { ResDialogCtrl::OnMouseMove(m); }

enum DlgSelectMode
{
	SelNone,
	SelSet,
	SelAdd,
};

class ResDialogCtrl : public FieldSource, public ResObject
{
	friend class ResDialog;

protected:
	static int TabDepth;

	void TabString(char *Str);
	void ReadPos(char *Str);
	char *GetRefText();

	GRect Goobers[8];

	ResDialog *Dlg;
	GRect Title;
	GRect Client;
	int DragCtrl;
	GRect DragRgn;
	GdcPt2 DragStart;
	bool MoveCtrl;
	DlgSelectMode SelectMode;
	GRect SelectStart;

	bool AcceptChildren;
	bool Movable;
	bool Vis;
	GAutoString CssClass;
	GAutoString CssStyle;

public:
	ResString *Str;
	typedef GAutoPtr<GViewIterator> ChildIterator;
	
	ResDialogCtrl(ResDialog *dlg, char *CtrlTypeName, GXmlTag *load);
	~ResDialogCtrl();

	const char *GetClass() { return "ResDialogCtrl"; }
	virtual GView *View() = 0;
	ResDialogCtrl *ParentCtrl() { return dynamic_cast<ResDialogCtrl*>(View()->GetParent()); }
	ResDialog *GetDlg() { return Dlg; }

	bool IsContainer() { return AcceptChildren; }
	void OnPaint(GSurface *pDC);
	void OnMouseClick(GMouse &m);
	void OnMouseMove(GMouse &m);
	bool SetPos(GRect &p, bool Repaint = false);
	void StrFromRef(int Id);
	GRect AbsPos();
	bool GetFields(FieldTree &Fields);

	// Copy/Paste translations only
	void CopyText();
	void PasteText();

	virtual int GetType() = 0;
	virtual GRect GetMinSize();
	virtual bool Serialize(FieldTree &Fields);
	virtual void ListChildren(List<ResDialogCtrl> &l, bool Deep = true);
	virtual bool AttachCtrl(ResDialogCtrl *Ctrl, GRect *r = 0);
	virtual GRect *GetChildArea(ResDialogCtrl *Ctrl) { return 0; }
	virtual GRect *GetPasteArea() { return 0; }
	virtual void EnumCtrls(List<ResDialogCtrl> &Ctrls);
	virtual void ShowMe(ResDialogCtrl *Child) {}
};

#define RESIZE_X1			0x0001
#define RESIZE_Y1			0x0002
#define RESIZE_X2			0x0004
#define RESIZE_Y2			0x0008

class ResDialog : public Resource, public GLayout, public ResFactory
{
	friend class ResDialogCtrl;
	friend class ResDialogUi;
	friend class CtrlDlg;
	friend class CtrlTabs;
	friend class CtNode;

protected:
	static int SymbolRefs;
	static ResStringGroup *Symbols;
	static bool CreateSymbols;

	ResDialogUi *Ui;
	List<ResDialogCtrl> Selection;
	GRect DlgPos;
	
	int DragGoober;
	int *DragX, *DragY;
	int DragOx, DragOy;
	GRect DragRgn;
	ResDialogCtrl *DragCtrl;
	void DrawSelection(GSurface *pDC);

public:
	ResDialog(AppWnd *w, int type = TYPE_DIALOG);
	~ResDialog();
	void Create(GXmlTag *load, SerialiseContext *Ctx);

	const char *GetClass() { return "ResDialog"; }
	GView *Wnd() { return dynamic_cast<GView*>(this); }
	static void AddLanguage(GLanguageId Id);
	ResDialog *IsDialog() { return this; }
	void EnumCtrls(List<ResDialogCtrl> &Ctrls);

	// GObj overrides
	char *Name();
	bool Name(const char *n);

	// Factory
	char *StringFromRef(int Ref);
	ResObject *CreateObject(GXmlTag *Tag, ResObject *Parent);

	int Res_GetStrRef(ResObject *Obj);
	bool Res_SetStrRef(ResObject *Obj, int Id, ResReadCtx *Ctx);
	void Res_SetPos(ResObject *Obj, int x1, int y1, int x2, int y2);
	void Res_SetPos(ResObject *Obj, char *s);
	GRect Res_GetPos(ResObject *Obj);
	void Res_Attach(ResObject *Obj, ResObject *Parent);
	bool Res_GetChildren(ResObject *Obj, List<ResObject> *l, bool Deep);
	void Res_Append(ResObject *Obj, ResObject *Parent);
	bool Res_GetItems(ResObject *Obj, List<ResObject> *l);
	bool Res_GetProperties(ResObject *Obj, GDom *Props);
	bool Res_SetProperties(ResObject *Obj, GDom *Props);
	GDom *Res_GetDom(ResObject *Obj);

	// Implementation
	int CurrentTool();
	ResDialogCtrl *CreateCtrl(GXmlTag *Tag);
	ResDialogCtrl *CreateCtrl(int Tool, GXmlTag *load);
	bool IsSelected(ResDialogCtrl *Ctrl);
	bool IsDraging();
	void SnapPoint(GdcPt2 *p, ResDialogCtrl *From);
	void SnapRect(GRect *r, ResDialogCtrl *From);
	void MoveSelection(int Dx, int Dy);
	void SelectRect(ResDialogCtrl *Parent, GRect *r, bool ClearPrev = true);
	void SelectNone();
	void SelectCtrl(ResDialogCtrl *c);
	void CleanSymbols();
	ResString *CreateSymbol();
	void OnShowLanguages();

	// Copy/Paste whole controls
	void Delete();
	void Copy(bool Delete = false);
	void Paste();

	// Methods
	GView *CreateUI();
	void OnMouseClick(GMouse &m);
	void OnMouseMove(GMouse &m);
	bool OnKey(GKey &k);
	void OnSelect(ResDialogCtrl *Wnd, bool ClearPrev = true);
	void OnDeselect(ResDialogCtrl *Wnd);
	void OnRightClick(GSubMenu *RClick);
	void OnCommand(int Cmd);
	int OnCommand(int Cmd, int Event, OsView hWnd);
	void OnLanguageChange();

	void _Paint(GSurface *pDC = NULL, GdcPt2 *Offset = NULL, GRegion *Update = NULL);

	bool Test(ErrorCollection *e);
	bool Read(GXmlTag *Tag, SerialiseContext &Ctx);
	bool Write(GXmlTag *Tag, SerialiseContext &Ctx);
};

class ResDialogUi : public GLayout
{
	friend class ResDialog;

	GToolBar *Tools;
	ResDialog *Dialog;
	GStatusBar *Status;
	GStatusPane *StatusInfo;

public:
	ResDialogUi(ResDialog *Res);
	~ResDialogUi();

	const char *GetClass() { return "ResDialogUi"; }
	void OnPaint(GSurface *pDC);
	void PourAll();
	void OnPosChange();
	void OnCreate();
	GMessage::Result OnEvent(GMessage *Msg);

	int CurrentTool();
	void SelectTool(int i);
};

///////////////////////////////////////////////////////////////////////
class CtrlDlg : public ResDialogCtrl, public GView
{
public:
	CtrlDlg(ResDialog *dlg, GXmlTag *load);

	DECL_DIALOG_CTRL(UI_DIALOG)
	const char *GetClass() { return "CtrlDlg"; }

	GRect &GetClient(bool InClientSpace = true);
	void _Paint(GSurface *pDC = NULL, GdcPt2 *Offset = NULL, GRegion *Update = NULL);
};

class CtrlTable : public ResDialogCtrl, public GDom, public GView
{
	class CtrlTablePrivate *d;

public:
	CtrlTable(ResDialog *dlg, GXmlTag *load);
	~CtrlTable();

	DECL_DIALOG_CTRL(UI_TABLE)
	const char *GetClass() { return "CtrlTable"; }

	bool GetFields(FieldTree &Fields);
	bool Serialize(FieldTree &Fields);

	void SetAttachCell(class ResTableCell *c);
	bool AttachCtrl(ResDialogCtrl *Ctrl, GRect *r = 0);
	void OnChildrenChanged(GViewI *Wnd, bool Attaching);
	GRect *GetPasteArea();
	GRect *GetChildArea(ResDialogCtrl *Ctrl);
	void Layout();
	void UnMerge(class ResTableCell *Cell);
	void Fix();
	void InsertRow(int y);
	void EnumCtrls(List<ResDialogCtrl> &Ctrls);

	bool GetVariant(const char *Name, GVariant &Value, char *Array = 0);
	bool SetVariant(const char *Name, GVariant &Value, char *Array = 0);
};

class CtrlText : public ResDialogCtrl, public GView
{
public:
	CtrlText(ResDialog *dlg, GXmlTag *load);

	DECL_DIALOG_CTRL(UI_TEXT)
	const char *GetClass() { return "CtrlText"; }
};

class CtrlEditbox : public ResDialogCtrl, public GView
{
	bool Password;
	bool MultiLine;

public:
	CtrlEditbox(ResDialog *dlg, GXmlTag *load);

	DECL_DIALOG_CTRL(UI_EDITBOX)
	const char *GetClass() { return "CtrlEditbox"; }

	bool GetFields(FieldTree &Fields);
	bool Serialize(FieldTree &Fields);
};

class CtrlCheckbox : public ResDialogCtrl, public GView
{
public:
	CtrlCheckbox(ResDialog *dlg, GXmlTag *load);

	DECL_DIALOG_CTRL(UI_CHECKBOX)
	const char *GetClass() { return "CtrlCheckbox"; }
};

class CtrlButton : public ResDialogCtrl, public GView
{
	GString Image;

public:
	CtrlButton(ResDialog *dlg, GXmlTag *load);

	DECL_DIALOG_CTRL(UI_BUTTON)
	const char *GetClass() { return "CtrlButton"; }
	bool GetFields(FieldTree &Fields);
	bool Serialize(FieldTree &Fields);
};

class CtrlGroup : public ResDialogCtrl, public GView
{
public:
	CtrlGroup(ResDialog *dlg, GXmlTag *load);

	DECL_DIALOG_CTRL(UI_GROUP)
	const char *GetClass() { return "CtrlGroup"; }
};

class CtrlRadio : public ResDialogCtrl, public GView
{
	GSurface *Bmp;

public:
	CtrlRadio(ResDialog *dlg, GXmlTag *load);
	~CtrlRadio();

	DECL_DIALOG_CTRL(UI_RADIO)
	const char *GetClass() { return "CtrlRadio"; }
};

class CtrlTab : public ResDialogCtrl, public GView
{
public:
	CtrlTab(ResDialog *dlg, GXmlTag *load);

	DECL_DIALOG_CTRL(UI_TAB)
	const char *GetClass() { return "CtrlTab"; }

	void ListChildren(List<ResDialogCtrl> &l, bool Deep);
};

class CtrlTabs : public ResDialogCtrl, public GView
{
	friend class CtrlTab;

	int Current;

public:
	List<CtrlTab> Tabs;

	CtrlTabs(ResDialog *dlg, GXmlTag *load);
	~CtrlTabs();

	DECL_DIALOG_CTRL(UI_TABS)
	const char *GetClass() { return "CtrlTabs"; }

	void ListChildren(List<ResDialogCtrl> &l, bool Deep);
	void Empty();
	void ToTab();
	void FromTab();

	GRect GetMinSize();
	void EnumCtrls(List<ResDialogCtrl> &Ctrls);
	void ShowMe(ResDialogCtrl *Child);
};

class ListCol : public ResDialogCtrl, public GView
{
public:
	GRect &r() { return GetPos(); }

	DECL_DIALOG_CTRL(UI_COLUMN)
	const char *GetClass() { return "ListCol"; }

	ListCol(ResDialog *dlg, GXmlTag *load, char *Str = 0, int Width = 50);
};

class CtrlList : public ResDialogCtrl, public GView
{
	int DragCol;

public:
	List<ListCol> Cols;

	CtrlList(ResDialog *dlg, GXmlTag *load);
	~CtrlList();

	DECL_DIALOG_CTRL(UI_LIST)
	const char *GetClass() { return "CtrlList"; }

	void ListChildren(List<ResDialogCtrl> &l, bool Deep);
	void Empty();
};

class CtrlComboBox : public ResDialogCtrl, public GView
{
public:
	CtrlComboBox(ResDialog *dlg, GXmlTag *load);

	DECL_DIALOG_CTRL(UI_COMBO)
	const char *GetClass() { return "CtrlComboBox"; }
};

class CtrlScrollBar : public ResDialogCtrl, public GView
{
public:
	CtrlScrollBar(ResDialog *dlg, GXmlTag *load);

	DECL_DIALOG_CTRL(UI_SCROLL_BAR)
	const char *GetClass() { return "CtrlScrollBar"; }
};

class CtrlTree : public ResDialogCtrl, public GView
{
public:
	CtrlTree(ResDialog *dlg, GXmlTag *load);

	DECL_DIALOG_CTRL(UI_TREE)
	const char *GetClass() { return "CtrlTree"; }
};

class CtrlBitmap : public ResDialogCtrl, public GView
{
public:
	CtrlBitmap(ResDialog *dlg, GXmlTag *load);

	DECL_DIALOG_CTRL(UI_BITMAP)
	const char *GetClass() { return "CtrlBitmap"; }
};

class CtrlProgress : public ResDialogCtrl, public GView
{
public:
	CtrlProgress(ResDialog *dlg, GXmlTag *load);

	DECL_DIALOG_CTRL(UI_PROGRESS)
	const char *GetClass() { return "CtrlProgress"; }
};

class CtrlCustom : public ResDialogCtrl, public GView
{
	char *Control;

public:
	CtrlCustom(ResDialog *dlg, GXmlTag *load);
	~CtrlCustom();

	DECL_DIALOG_CTRL(UI_CUSTOM)
	const char *GetClass() { return "CtrlCustom"; }

	bool GetFields(FieldTree &Fields);
	bool Serialize(FieldTree &Fields);
};

class CtrlControlTree : public ResDialogCtrl, public GTree, public GDom
{
	class CtrlControlTreePriv *d;

public:
	CtrlControlTree(ResDialog *dlg, GXmlTag *load);
	~CtrlControlTree();

	DECL_DIALOG_CTRL(UI_CONTROL_TREE)
	const char *GetClass() { return "CtrlControlTree"; }

	bool GetFields(FieldTree &Fields);
	bool Serialize(FieldTree &Fields);

	bool GetVariant(const char *Name, GVariant &Value, char *Array = 0);
	bool SetVariant(const char *Name, GVariant &Value, char *Array = 0);
};

////////////////////////////////////////////////////////////////
#endif
