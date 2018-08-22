#include "Lgi.h"
#include "GDataGrid.h"
#include "GEdit.h"
#include "GCombo.h"
#include "GClipBoard.h"

enum GDataGridControls
{
	IDC_DELETE = 2000,
	IDC_COPY,
	IDC_EDIT,
};
#define M_DELETE_LATER		(M_USER+2000)
#define CELL_EDGE			2

struct GDataGridPriv
{
	GDataGrid *This;
	int Col;
	GView *e;
	GView *DeleteLater;
	LListItem *Cur;
	bool Dirty, PosDirty;
	GArray<GDataGrid::GDataGridFlags> Flags;
	GArray<GVariant> ColumnArgs;
	LListItem *NewRecord;
	GDataGrid::ItemFactory Factory;
	void *UserData;
	GDataGrid::ItemArray Dropped;
	GDataGrid::IndexArray Deleted;
	char *SrcFmt;
	char *AcceptFmt;

	GDataGridPriv(GDataGrid *t);
	void Save();
	void UpdatePos();
	void Create(int NewCol = -1);
	void MoveCell(int dx);
	void Invalidate();

	LListItem *NewItem()
	{
		if (Factory)
			return Factory(UserData);
		
		return new LListItem;
	}
};

class GDataGridEdit : public GEdit
{
	GDataGridPriv *d;

public:
	GDataGridEdit(GDataGridPriv *data, int id, int x, int y, int cx, int cy, char *txt) :
		GEdit(id, x, y, cx, cy, txt)
	{
		d = data;
	}

	bool OnKey(GKey &k)
	{
		if (!GEdit::OnKey(k) && k.Down())
		{
			if (k.IsChar)
			{
			}
			else
			{
				switch (k.vkey)
				{
					case VK_RETURN:
					{
						LListItem *s = d->This->GetSelected();
						int Idx = s ? d->This->IndexOf(s) : -1;
						d->Save();
						if (Idx >= 0)
						{
							s->Select(false);
							if ((s = d->This->ItemAt(Idx + 1)))
								s->Select(true);

						}
						break;
					}
					case VK_LEFT:
					{
						GRange r = GetSelectionRange();
						if (r.Len && GetCaret() ==  0)
						{
							d->MoveCell(-1);
							return true;
						}
						break;
					}
					case VK_RIGHT:
					{
						GRange r = GetSelectionRange();
						if (r.Len == 0)
							break;

						char16 *Txt = NameW();
						r.Len = Txt ? StrlenW(Txt) : 0;
						int Cursor = GetCaret();
						if (Cursor >= r.Len)
						{
							d->MoveCell(1);
							return true;
						}
						break;
					}
					case VK_PAGEUP:
					case VK_PAGEDOWN:
					case VK_UP:
					case VK_DOWN:
					{
						bool b = d->This->OnKey(k);
						d->Invalidate();
						return b;
						break;
					}
				}
			}
		}

		return false;
	}
};

class GDataGridCombo : public GCombo
{
	GDataGridPriv *d;

public:
	GDataGridCombo(GDataGridPriv *priv, int id, GRect &rc) : GCombo(id, rc.x1, rc.y1, rc.X(), rc.Y(), 0)
	{
		d = priv;
	}
	
	bool OnKey(GKey &k)
	{
		if (!k.IsChar && k.Down())
		{
			if (k.c16 == VK_LEFT)
			{
				d->MoveCell(-1);
			}
			else if (k.c16 == VK_RIGHT)
			{
				d->MoveCell(1);
			}
		}
		
		return GCombo::OnKey(k);
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////
GDataGridPriv::GDataGridPriv(GDataGrid *t)
{
	Factory = 0;
	UserData = 0;
	SrcFmt = AcceptFmt = 0;
	DeleteLater = NULL;

	NewRecord = 0;
	Dirty = false;
	PosDirty = false;
	Cur = 0;
	This = t;
	Col = 0;
	e = 0;
}

void GDataGridPriv::Save()
{
	if (Dirty && Cur && e)
	{
		if (Flags[Col] & GDataGrid::GDG_INTEGER)
		{
			int64 OldVal = e->Value();
			if (OldVal < 0 && Cur == NewRecord)
				return;

			char t[64];
			sprintf(t, LPrintfInt64, OldVal);
			Cur->SetText(t, Col);
		}
		else
		{
			char *OldTxt = e->Name();
			if (!OldTxt && Cur == NewRecord)
				return;

			Cur->SetText(OldTxt, Col);
		}

		if (Cur == NewRecord)
		{
			This->Insert(NewRecord = NewItem());
		}
	}
}

void GDataGridPriv::UpdatePos()
{
	if (e && Cur)
	{
		GRect *r = Cur->GetPos(Col);
		if (r)
		{
			GRect rc = *r;

			if (Flags[Col] & GDataGrid::GDG_INTEGER)
			{
				rc.Offset(2, 0);
			}
			else
			{
				rc.x2--;
				rc.y2--;
			}

			e->SetPos(rc);
			e->Focus(true);
			
			
			PosDirty = false;
		}
	}
}

void GDataGridPriv::Invalidate()
{
	if (e)
	{
		GRect p = e->GetPos();
		p.Size(-CELL_EDGE, -CELL_EDGE);
		This->Invalidate(&p);
	}
}

void GDataGridPriv::MoveCell(int Dx)
{
	for (int i=Col+Dx; i>=0 && i<This->GetColumns(); i+=Dx)
	{
		if (!(Flags[i] & GDataGrid::GDG_READONLY))
		{
			Create(i);
			break;
		}
	}
}

void GDataGridPriv::Create(int NewCol)
{
	if (!This->IsAttached())
		return;

	LListItem *i = This->GetSelected();
	if (!i)
		return;

	if (!Cur || i != Cur || (NewCol >= 0 && NewCol != Col))
	{
		int OldCtrl = Flags[Col] & GDataGrid::GDG_INTEGER;

		Save();
		Cur = i;

		if (NewCol >= 0)
			Col = NewCol;

		if (Flags[Col] & GDataGrid::GDG_READONLY)
		{
			// Pick a valid column
			int NewCol = -1;
			for (int i=0; i<This->GetColumns(); i++)
			{
				if (Col != i && !(Flags[i] & GDataGrid::GDG_READONLY))
				{
					NewCol = i;
					break;
				}
			}

			if (NewCol < 0)
				return;

			Col = NewCol;
		}

		int NewCtrl = Flags[Col] & GDataGrid::GDG_INTEGER;

		char *CurText = i->GetText(Col);
		GRect *r = i->GetPos(Col);
		if (r)
		{
			GDataGridEdit *Edit = 0;
			GCombo *Combo = 0;
			GRect rc = r;
			
			if (!e || (NewCtrl ^ OldCtrl))
			{
				if (e)
				{
					e->Detach();
					Invalidate();
					DeleteLater = e;
					e = NULL;					
					This->PostEvent(M_DELETE_LATER);
				}

				if (Flags[Col] & GDataGrid::GDG_INTEGER)
				{
                    e = Combo = new GDataGridCombo(this, IDC_EDIT, rc);
					if (e)
					{
						GVariant &e = ColumnArgs[Col];
						if (e.Type == GV_LIST)
						{
							for (GVariant *v = e.Value.Lst->First(); v; v = e.Value.Lst->Next())
							{
								char *s = v->Str();
								Combo->Insert(s);
							}
						}

						if (CurText)
							Combo->Name(CurText);
						else
							Combo->Value(-1);
					}
				}
				else
				{
					rc.x2 -= 1;
					rc.y2 -= 1;

					// Create edit control with the correct info for the cell
                    e = Edit = new GDataGridEdit(this, IDC_EDIT, rc.x1, rc.y1, rc.X(), rc.Y(), CurText);
					if (e)
					{
						e->Sunken(false);
					}
				}

				e->SetPos(rc);
				e->Attach(This);
				Invalidate();
			}
			else
			{
				Invalidate();
				e->SetPos(rc);
				e->Name(CurText);
				Invalidate();
			}

			if (Edit)
				Edit->Select(0);
			e->Focus(true);
		}			
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GDataGrid::GDataGrid(int CtrlId, ItemFactory Func, void *userdata) :
	LList(CtrlId, 0, 0, 1000, 1000)
{
	_ObjName = Res_Custom;

	DrawGridLines(true);
	SetPourLargest(true);

	d = new GDataGridPriv(this);
	SetFactory(Func, userdata);
}

GDataGrid::~GDataGrid()
{
	DeleteObj(d);
}

bool GDataGrid::Remove(LListItem *Obj)
{
	if (Obj == d->Cur)
	{
		DeleteObj(d->e);
		d->Cur = 0;
	}
	return LList::Remove(Obj);
}

void GDataGrid::Empty()
{
	d->NewRecord = 0;
	d->Cur = 0;
	DeleteObj(d->e);
	
	LList::Empty();
}

void GDataGrid::OnItemSelect(GArray<LListItem*> &Items)
{
	if (Items.Length() == 1)
	{
		d->Create();
	}
	else if (d->e)
	{
		d->Cur = 0;
		d->DeleteLater = d->e;
		d->e = NULL;
		PostEvent(M_DELETE_LATER);
	}

	LList::OnItemSelect(Items);
}

void GDataGrid::OnItemClick(LListItem *Item, GMouse &m)
{
	if (m.IsContextMenu())
	{
		GSubMenu s;
		s.AppendItem("Copy", IDC_COPY);
		s.AppendItem("Delete", IDC_DELETE);
		m.ToScreen();
		switch (s.Float(this, m.x, m.y, m.Left()))
		{
			case IDC_COPY:
			{
				List<LListItem> Sel;
				GetSelection(Sel);
				GStringPipe p(256);
				int Cols = GetColumns();
				for (LListItem *i=Sel.First(); i; i=Sel.Next())
				{
					for (int c=0; c<Cols; c++)
					{
						p.Print("%s%s", c ? ", " : "", i->GetText(c));
					}
					p.Print("\n");
				}
				GClipBoard cb(this);
				GAutoString a(p.NewStr());
				cb.Text(a);
				break;
			}
			case IDC_DELETE:
			{
				List<LListItem> Sel;
				GetSelection(Sel);
				d->Deleted.Length(0);
				for (LListItem *i=Sel.First(); i; i=Sel.Next())
				{
					d->Deleted.Add(IndexOf(i));
				}
				SendNotify(GNotifyItem_Delete);
				d->Deleted.Length(0);

				Sel.Delete(d->NewRecord);
				Sel.DeleteObjects();
				break;
			}
		}
	}
	else
	{
		int ClickCol = ColumnAtX(m.x);
		if (ClickCol >= 0 && ClickCol != d->Col)
		{
			d->Create(ClickCol);
		}
	}
}

void GDataGrid::OnCreate()
{
	d->Create(0);
	SetWindow(this);
}

GMessage::Result GDataGrid::OnEvent(GMessage *Msg)
{
	switch (MsgCode(Msg))
	{
		case M_DELETE_LATER:
		{
			DeleteObj(d->DeleteLater);
			break;
		}
	}

	return LList::OnEvent(Msg);
}

int GDataGrid::OnNotify(GViewI *c, int f)
{
	switch (c->GetId())
	{
		case IDC_EDIT:
		{
			d->Dirty = true;
			break;
		}
		case IDC_VSCROLL:
		{
			d->PosDirty = true;
			break;
		}
	}

	return LList::OnNotify(c, f);
}

void GDataGrid::SetColFlag(int Col, GDataGridFlags Flags, GVariant *Arg)
{
	d->Flags[Col] = Flags;
	if (Arg)
		d->ColumnArgs[Col] = *Arg;
}

bool GDataGrid::OnMouseWheel(double Lines)
{
	LList::OnMouseWheel(Lines);
	d->UpdatePos();
	return true;
}

void GDataGrid::OnPaint(GSurface *pDC)
{
	LList::OnPaint(pDC);

	d->UpdatePos();
	if (d->e && !(d->Flags[d->Col] & GDG_INTEGER))
	{
		pDC->Colour(LC_BLACK, 24);
		GRect p = d->e->GetPos();
		p.Size(-CELL_EDGE, -CELL_EDGE);
		pDC->Box(&p);
	}
}

bool GDataGrid::OnLayout(GViewLayoutInfo &Inf)
{
	Inf.Width.Min = 16;
	for (int i=0; i<GetColumns(); i++)
	{
		Inf.Width.Min += ColumnAt(i)->Width();
	}
	Inf.Width.Max = Inf.Width.Min + 20;
	
	Inf.Height.Min = 50;
	Inf.Height.Max = 2000;
	
	return true;
}

bool GDataGrid::CanAddRecord()
{
	return d->NewRecord;
}

void GDataGrid::CanAddRecord(bool b)
{
	if (b)
	{
		if (!d->NewRecord)
			Insert(d->NewRecord = d->NewItem());
	}
	else
	{
		DeleteObj(d->NewRecord);
	}
}

LListItem *GDataGrid::NewItem()
{
	return d->NewItem();
}

void GDataGrid::SetFactory(ItemFactory Func, void *userdata)
{
	d->Factory = Func;
	d->UserData = userdata;
}

void GDataGrid::SetDndFormats(char *SrcFmt, char *AcceptFmt)
{
	d->SrcFmt = SrcFmt;
	d->AcceptFmt = AcceptFmt;
}

int GDataGrid::WillAccept(List<char> &Formats, GdcPt2 Pt, int KeyState)
{
	for (char *f=Formats.First(); f; f=Formats.Next())
	{
		if (d->AcceptFmt && !stricmp(f, d->AcceptFmt))
		{
			return DROPEFFECT_COPY;
		}
	}

	return DROPEFFECT_NONE;
}

GDataGrid::ItemArray *GDataGrid::GetDroppedItems()
{
	return &d->Dropped;
}

GDataGrid::IndexArray *GDataGrid::GetDeletedItems()
{
	return &d->Deleted;
}

int GDataGrid::OnDrop(GArray<GDragData> &Data, GdcPt2 Pt, int KeyState)
{
	if (!d->AcceptFmt)
		return DROPEFFECT_NONE;
	
	for (unsigned n=0; n<Data.Length(); n++)
	{
		GDragData &dd = Data[n];
		if (dd.IsFormat(d->AcceptFmt))
		{
			GVariant *Data = &dd.Data.First();
			if (Data->Type == GV_BINARY)
			{
				LListItem **Item = (LListItem**)Data->Value.Binary.Data;
				int Items = Data->Value.Binary.Length / sizeof(LListItem*);
				d->Dropped.Length(0);
				for (int i=0; i<Items; i++)
				{
					d->Dropped.Add(Item[i]);
				}
				SendNotify(GNotifyItem_ItemsDropped);
				return DROPEFFECT_COPY;
			}
		}
	}

	return DROPEFFECT_NONE;
}

void GDataGrid::OnItemBeginDrag(LListItem *Item, GMouse &m)
{
	Drag(this, DROPEFFECT_COPY);
}

bool GDataGrid::GetFormats(List<char> &Formats)
{
	if (!d->SrcFmt)
		return false;

	Formats.Insert(NewStr(d->SrcFmt));
	return true;
}

bool GDataGrid::GetData(GArray<GDragData> &Data)
{
	for (unsigned i=0; i<Data.Length(); i++)
	{
		if (Data[i].IsFormat(d->SrcFmt))
		{
			List<LListItem> s;
			if (GetSelection(s))
			{
				GArray<LListItem*> a;
				for (LListItem *it=s.First(); it; it=s.Next())
				{
					a.Add(it);
				}
				Data[i].Data.New().SetBinary(sizeof(LListItem*)*a.Length(), &a[0]);
				return true;
			}
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
class GDataGridFactory : public GViewFactory
{
	GView *NewView(const char *Class, GRect *Pos, const char *Text)
	{
		if (Class &&
			stricmp(Class, "GDataGrid") == 0)
		{
			return new GDataGrid(0);
		}

		return 0;
	}
};

static GDataGridFactory _Factory;
