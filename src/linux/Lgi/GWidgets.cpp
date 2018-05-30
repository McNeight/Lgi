/*hdr
**      FILE:           GuiDlg.cpp
**      AUTHOR:         Matthew Allen
**      DATE:           8/9/1998
**      DESCRIPTION:    Dialog components
**
**      Copyright (C) 1998 Matthew Allen
**              fret@memecode.com
*/

#include <stdlib.h>
#include <stdio.h>

#include "Lgi.h"
#include "GSlider.h"
#include "GBitmap.h"
#include "GTableLayout.h"
#include "GDisplayString.h"

using namespace Gtk;
#include "LgiWidget.h"

///////////////////////////////////////////////////////////////////////////////////////////
#define GreyBackground()

struct GDialogPriv
{
	int ModalStatus;
	bool IsModal;
	bool Resizable;
	
	GDialogPriv()
	{
		IsModal = true;
		Resizable = true;
		ModalStatus = 0;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////
GDialog::GDialog()
	: ResObject(Res_Dialog)
	#ifdef __GTK_H__
	, GWindow(gtk_dialog_new())
	#endif
{
	d = new GDialogPriv();
	Name("Dialog");
	_View = GTK_WIDGET(Wnd);
	_SetDynamic(false);
}

GDialog::~GDialog()
{
	DeleteObj(d);
}

bool GDialog::IsModal()
{
	return d->IsModal;
}

void GDialog::Quit(bool DontDelete)
{
	if (d->IsModal)
		EndModal(0);
	else
		GView::Quit(DontDelete);
}

void GDialog::OnPosChange()
{
    if (Children.Length() == 1)
    {
        List<GViewI>::I it = Children.begin();
        GTableLayout *t = dynamic_cast<GTableLayout*>((GViewI*)it);
        if (t)
        {
            GRect r = GetClient();
            r.Size(GTableLayout::CellSpacing, GTableLayout::CellSpacing);
            t->SetPos(r);
        }
    }
}

bool GDialog::LoadFromResource(int Resource, char *TagList)
{
	GAutoString n;
	GRect p;

	bool Status = GLgiRes::LoadFromResource(Resource, this, &p, &n, TagList);
	if (Status)
	{
		Name(n);
		SetPos(p);
	}
	return Status;
}

bool GDialog::OnRequestClose(bool OsClose)
{
	if (d->IsModal)
	{
		EndModal(0);
		return false;
	}

	return true;
}

#define MWM_DECOR_ALL						(1L << 0)
#define MWM_HINTS_INPUT_MODE				(1L << 2)
#define MWM_INPUT_FULL_APPLICATION_MODAL	3L
#define XA_ATOM								((xcb_atom_t) 4)

class MotifWmHints
{
public:
	ulong Flags, Functions, Decorations;
	long InputMode;
	ulong Status;
	
	MotifWmHints()
	{
		Flags = Functions = Status = 0;
		Decorations = MWM_DECOR_ALL;
		InputMode = 0;
	}
};

bool GDialog::IsResizeable()
{
    return d->Resizable;
}

void GDialog::IsResizeable(bool r)
{
	d->Resizable = r;
}

gboolean GDialog::OnGtkEvent(GtkWidget *widget, GdkEvent *event)
{
	if (!event)
	{
		printf("%s:%i - No event %i\n", _FL);
		return FALSE;
	}

	switch (event->type)
	{
		case GDK_DELETE:
		{
			Quit();
			OnGtkDelete();
			return false;
		}
		case GDK_CONFIGURE:
		{
			GdkEventConfigure *c = (GdkEventConfigure*)event;
			Pos.Set(c->x, c->y, c->x+c->width-1, c->y+c->height-1);
			OnPosChange();
			return false;
			break;
		}
		case GDK_FOCUS_CHANGE:
		{
			GWindow::OnGtkEvent(widget, event);
			break;
		}
		case GDK_CLIENT_EVENT:
		{
			GMessage m(event);
			OnEvent(&m);
			break;
		}
		default:
		{
			printf("%s:%i - Unknown event %i\n", _FL, event->type);
			break;
		}
	}
	
	return true;
}

static
gboolean
GtkDialogDestroy(GtkWidget *widget, GDialog *This)
{
	This->_View = NULL;
	return 0;
}

static
void
GtkDialogRealize(GtkWidget *widget, GDialog *This)
{
	This->OnGtkRealize();
}
               
bool GDialog::SetupDialog(bool Modal)
{
	if (GBase::Name())
		gtk_window_set_title(GTK_WINDOW(Wnd), GBase::Name());

	gtk_dialog_set_has_separator(GTK_DIALOG(Wnd), false);
	if (IsResizeable())
	{
	    gtk_window_set_default_size(Wnd, Pos.X(), Pos.Y());
	}
	else
	{
	    gtk_widget_set_size_request(GTK_WIDGET(Wnd), Pos.X(), Pos.Y());
	    gtk_window_set_resizable(Wnd, FALSE);
	}
	
	GtkWidget *content_area = 	
	#if GtkVer(2, 14)
		gtk_dialog_get_content_area(GTK_DIALOG(Wnd));
	#else
		GTK_DIALOG(Wnd)->vbox;
	#endif
	if (content_area)
	{
		GtkContainer *container = GTK_CONTAINER(content_area);
		GtkHButtonBox *btns = NULL;
		
		Gtk::GList *list = gtk_container_get_children(container);
		for (Gtk::GList *i = list; i != NULL; i = i->next)
		{
			const gchar *type = G_OBJECT_TYPE_NAME(i->data);
			GtkWidget *w = GTK_WIDGET(i->data);
			if (!btns && GTK_IS_HBUTTON_BOX(i->data))
			{
				btns = GTK_HBUTTON_BOX(i->data);
			}
		}		
		g_list_free(list);
    
		// Add our own root control to contain LGI widgets
		if (_Root = lgi_widget_new(this, Pos.X(), Pos.Y(), true))
		{
			gtk_container_add(container, _Root);
			gtk_widget_show(_Root);
			if (btns)
			{
				// Hide the btns container, as Lgi won't use it.
				gtk_widget_hide(GTK_WIDGET(btns));
			}
		}
	}
	
	GView *gv = this;
	g_signal_connect(	G_OBJECT(Wnd),
						"delete_event",
						G_CALLBACK(GtkViewCallback),
						gv);
	g_signal_connect(	G_OBJECT(Wnd),
						"configure-event",
						G_CALLBACK(GtkViewCallback),
						gv);
	g_signal_connect(	G_OBJECT(Wnd),
						"destroy",
						G_CALLBACK(GtkDialogDestroy),
						this);
	g_signal_connect(	G_OBJECT(Wnd),
						"client-event",
						G_CALLBACK(GtkViewCallback),
						gv);
	g_signal_connect(	G_OBJECT(Wnd),
						"focus-in-event",
						G_CALLBACK(GtkViewCallback),
						gv);
	g_signal_connect(	G_OBJECT(Wnd),
						"focus-out-event",
						G_CALLBACK(GtkViewCallback),
						gv);
	g_signal_connect(	G_OBJECT(Wnd),
						"realize",
						G_CALLBACK(GtkDialogRealize),
						gv);

	gtk_widget_realize(GTK_WIDGET(Wnd));
	OnCreate();
	AttachChildren();

	if (!_Default)
	{
		_Default = FindControl(IDOK);
	}

	gtk_widget_show(GTK_WIDGET(Wnd));
	GView::Visible(true);

	return true;
}

int GDialog::DoModal(OsView OverrideParent)
{
	d->ModalStatus = -1;

	if (GetParent())
		gtk_window_set_transient_for(GTK_WINDOW(Wnd), GetParent()->WindowHandle());

	d->IsModal = true;
	SetupDialog(true);
	
	gtk_main();
	
	return d->ModalStatus;
}

void _Dump(GViewI *v, int Depth = 0)
{
	for (int i=0; i<Depth*4; i++)
		printf(" ");

	GViewIterator *it = v->IterateViews();
	if (it)
	{
		for (GViewI *c=it->First(); c; c=it->Next())
			_Dump(c, Depth+1);
		DeleteObj(it);
	}					
}

void GDialog::EndModal(int Code)
{
	if (d->IsModal)
	{
		d->IsModal = false;
		d->ModalStatus = Code;
		// gtk_dialog_response(GTK_DIALOG(Wnd), Code);
		gtk_main_quit();
	}
	else
	{
		// LgiAssert(0);
	}
}

int GDialog::DoModeless()
{
	d->IsModal = false;
	SetupDialog(false);
	return 0;
}

void GDialog::EndModeless(int Code)
{
	Quit(Code);
}

extern GButton *FindDefault(GView *w);

GMessage::Param GDialog::OnEvent(GMessage *Msg)
{
	switch (MsgCode(Msg))
	{
	}

	return GView::OnEvent(Msg);
}


///////////////////////////////////////////////////////////////////////////////////////////
GControl::GControl(OsView view) : GView(view)
{
	Pos.ZOff(10, 10);
}

GControl::~GControl()
{
}

GMessage::Param GControl::OnEvent(GMessage *Msg)
{
	switch (MsgCode(Msg))
	{
	}
	return 0;
}

GdcPt2 GControl::SizeOfStr(const char *Str)
{
	int y = SysFont->GetHeight();
	GdcPt2 Pt(0, 0);

	if (Str)
	{
		const char *e = 0;
		for (const char *s = Str; s && *s; s = e?e+1:0)
		{
			e = strchr(s, '\n');
			int Len = e ? (int)e-(int)s : strlen(s);

			GDisplayString ds(SysFont, s, Len);
			Pt.x = MAX(Pt.x, ds.X());
			Pt.y += y;
		}
	}

	return Pt;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Text
/*
GText::GText(int id, int x, int y, int cx, int cy, char *name) :
	GControl( new DefaultOsView(this) ),
	ResObject(Res_StaticText)
{
	GdcPt2 Size = SizeOfStr(name);
	if (cx < 0) cx = Size.x;
	if (cy < 0) cy = Size.y;

	Name(name);
	GRect r(x, y, x+cx, y+cy);
	SetPos(r);
	SetId(id);

	GreyBackground();
}

GText::~GText()
{
}

int GText::Value()
{
	int Status = 0;
	char *n = Name();
	if (n)
	{
		Status = atoi(n);
	}
	return Status;
}

void GText::Value(int i)
{
	char n[32];
	sprintf(n, "%i", i);
	Name(n);
}

void GText::OnPaint(GSurface *pDC)
{
	pDC->Colour(LC_MED, 24);
	pDC->Rectangle();

	int Ly = SysFont->Y("A");
	int y = 0;
	char *e = 0;
	for (char *s = Name(); s && *s; s = e?e+1:0)
	{
		e = strchr(s, '\n');
		
		int l = e ? (int)e - (int)s : strlen(s);

		SysFont->Transparent(true);
		
		if (Enabled())
		{
			SysFont->Colour(LC_TEXT, LC_MED);
			SysFont->Text(pDC, 0, y, s, l);
		}
		else
		{
			SysFont->Colour(LC_LIGHT, LC_MED);
			SysFont->Text(pDC, 1, y+1, s, l);
			
			SysFont->Colour(LC_LOW, LC_MED);
			SysFont->Text(pDC, 0, y, s, l);
		}
		
		y += Ly;
	}
}

int GText::OnEvent(GMessage *Msg)
{
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Radio group
int GRadioGroup::NextId = 20000;

GRadioGroup::GRadioGroup(int id, int x, int y, int cx, int cy, char *name, int Init)
	: ResObject(Res_Group)
{
	Name(name);
	GRect r(x, y, x+cx, y+cy);
	SetPos(r);
	SetId(id);
	Val = Init;

	GreyBackground();
}

GRadioGroup::~GRadioGroup()
{
}

void GRadioGroup::OnCreate()
{
	AttachChildren();
	Value(Val);
}

int GRadioGroup::Value()
{
	int i=0;
	for (	GView *w = Children.First();
			w;
			w = Children.Next())
	{
		GRadioButton *But = dynamic_cast<GRadioButton*>(w);
		if (But)
		{
			if (But->Value())
			{
				Val = i;
				break;
			}
			i++;
		}
	}

	return Val;
}

void GRadioGroup::Value(int Which)
{
	Val = Which;

	int i=0;
	for (	GView *w = Children.First();
			w;
			w = Children.Next())
	{
		GRadioButton *But = dynamic_cast<GRadioButton*>(w);
		if (But)
		{
			if (i == Which)
			{
				But->Value(true);
				break;
			}
			i++;
		}
	}
}

int GRadioGroup::OnNotify(GViewI *Ctrl, int Flags)
{
	GView *n = GetNotify() ? GetNotify() : GetParent();
	if (n)
	{
		if (dynamic_cast<GRadioButton*>(Ctrl))
		{
			return n->OnNotify(this, Flags);
		}
		else
		{
			return n->OnNotify(Ctrl, Flags);
		}
	}
	return 0;
}

int GRadioGroup::OnEvent(GMessage *Msg)
{
	return 0;
}

void GRadioGroup::OnPaint(GSurface *pDC)
{
	pDC->Colour(LC_MED, 24);
	pDC->Rectangle();

	int y = SysFont->Y("A");
	GRect b(0, y/2, X()-1, Y()-1);
	LgiWideBorder(pDC, b, CHISEL);

	char *n = Name();
	if (n)
	{
		int x;
		SysFont->Size(&x, &y, n);
		GRect t;
		t.ZOff(x, y);
		t.Offset(6, 0);
		SysFont->Colour(LC_TEXT, LC_MED);
		SysFont->Transparent(false);
		SysFont->Text(pDC, t.x1, t.y1, n, -1, &t);
	}
}

GRadioButton *GRadioGroup::Append(int x, int y, char *name)
{
	GRadioButton *But = new GRadioButton(NextId++, x, y, -1, -1, name, Children.GetItems() == 0);
	if (But)
	{
		Children.Insert(But);
	}

	return But;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Radio button
GRadioButton::GRadioButton(int id, int x, int y, int cx, int cy, char *name, bool first) :
	GControl( new DefaultOsView(this) ),
	ResObject(Res_RadioBox)
{
	GdcPt2 Size = SizeOfStr(name);
	if (cx < 0) cx = Size.x + 26;
	if (cy < 0) cy = Size.y + 6;

	Name(name);
	GRect r(x, y, x+cx, y+cy);
	SetPos(r);
	SetId(id);
	Val = false;
	Over = false;
	Handle()->isTabStop(true);

	GreyBackground();
}

GRadioButton::~GRadioButton()
{
}

int GRadioButton::Value()
{
	return Val;
}

void GRadioButton::Value(int i)
{
	if (Val != i)
	{
		if (i)
		{
			// remove the value from the currenly selected radio value
			GView *p = GetParent();
			if (p)
			{
				Iterator<GViewI> l(&p->Children);
				for (GViewI *c=l.First(); c; c=l.Next())
				{
					if (c != this)
					{
						GRadioButton *b = dynamic_cast<GRadioButton*>(c);
						if (b && b->Val)
						{
							b->Val = false;
							b->Invalidate();
						}
					}
				}
			}
		}

		Val = i;
		Invalidate();

		if (i)
		{
			GView *n = GetNotify() ? GetNotify() : GetParent();
			if (n)
			{
				n->OnNotify(this, 0);
			}
		}
	}
}

int GRadioButton::OnEvent(GMessage *Msg)
{
	return 0;
}

void GRadioButton::OnMouseClick(GMouse &m)
{
	if (Enabled())
	{
		Capture(m.Down());
		Over = m.Down();
	
		GRect r(0, 0, X()-1, Y()-1);
		if (!m.Down() &&
			r.Overlap(m.x, m.y))
		{
			Value(true);
		}
		else
		{
			Invalidate();
		}
	}
}

void GRadioButton::OnMouseEnter(GMouse &m)
{
	if (Enabled() && IsCapturing())
	{
		Over = true;
		Invalidate();
	}
}

void GRadioButton::OnMouseExit(GMouse &m)
{
	if (Enabled() && IsCapturing())
	{
		Over = false;
		Invalidate();
	}
}

void GRadioButton::OnKey(GKey &k)
{
}

void GRadioButton::OnFocus(bool f)
{
	Invalidate();
}

void GRadioButton::OnPaint(GSurface *pDC)
{
	GRect r(0, 0, X()-1, Y()-1);
	GRect c(0, 0, 12, 12);
	pDC->Colour(LC_MED, 24);
	pDC->Rectangle();

	bool e = Enabled();
	char *n = Name();
	if (n)
	{
		GRect t = r;
		t.x1 = c.x2 + 1;
		
		int Off = e ? 0 : 1;
		SysFont->Colour(e ? LC_TEXT : LC_LIGHT, LC_MED);
		SysFont->Transparent(false);
		SysFont->Text(pDC, t.x1 + 5 + Off, t.y1 + Off, n, -1, &t);

		if (!e)
		{
			SysFont->Transparent(true);
			SysFont->Colour(LC_LOW, LC_MED);
			SysFont->Text(pDC, t.x1 + 5, t.y1, n, -1, &t);
		}
	}

	// Draw border
	pDC->Colour(LC_LOW, 24);
	pDC->Line(c.x1+1, c.y1+9, c.x1+1, c.y1+10);
	pDC->Line(c.x1, c.y1+4, c.x1, c.y1+8);
	pDC->Line(c.x1+1, c.y1+2, c.x1+1, c.y1+3);
	pDC->Line(c.x1+2, c.y1+1, c.x1+3, c.y1+1);
	pDC->Line(c.x1+4, c.y1, c.x1+8, c.y1);
	pDC->Line(c.x1+9, c.y1+1, c.x1+10, c.y1+1);

	pDC->Colour(LC_SHADOW, 24);
	pDC->Set(c.x1+2, c.y1+9);
	pDC->Line(c.x1+1, c.y1+4, c.x1+1, c.y1+8);
	pDC->Line(c.x1+2, c.y1+2, c.x1+2, c.y1+3);
	pDC->Set(c.x1+3, c.y1+2);
	pDC->Line(c.x1+4, c.y1+1, c.x1+8, c.y1+1);
	pDC->Set(c.x1+9, c.y1+2);

	pDC->Colour(LC_LIGHT, 24);
	pDC->Line(c.x1+11, c.y1+2, c.x1+11, c.y1+3);
	pDC->Line(c.x1+12, c.y1+4, c.x1+12, c.y1+8);
	pDC->Line(c.x1+11, c.y1+9, c.x1+11, c.y1+10);
	pDC->Line(c.x1+9, c.y1+11, c.x1+10, c.y1+11);
	pDC->Line(c.x1+4, c.y1+12, c.x1+8, c.y1+12);
	pDC->Line(c.x1+2, c.y1+11, c.x1+3, c.y1+11);

	/// Draw center
	pDC->Colour(Over || !e ? LC_MED : LC_WORKSPACE, 24);
	pDC->Rectangle(c.x1+2, c.y1+4, c.x1+10, c.y1+8);
	pDC->Box(c.x1+3, c.y1+3, c.x1+9, c.y1+9);
	pDC->Box(c.x1+4, c.y1+2, c.x1+8, c.y1+10);

	// Draw value
	if (Val)
	{
		pDC->Colour(e ? LC_TEXT : LC_LOW, 24);
		pDC->Rectangle(c.x1+4, c.y1+5, c.x1+8, c.y1+7);
		pDC->Rectangle(c.x1+5, c.y1+4, c.x1+7, c.y1+8);
	}
}
*/

//////////////////////////////////////////////////////////////////////////////////
// Slider control
GSlider::GSlider(int id, int x, int y, int cx, int cy, const char *name, bool vert) :
	ResObject(Res_Slider)
{
	SetId(id);
	GRect r(x, y, x+cx, y+cy);
	SetPos(r);
	Name(name);
	Vertical = vert;
	Min = Max = 0;
	Val = 0;
	SetTabStop(true);
}

GSlider::~GSlider()
{
}

void GSlider::Value(int64 i)
{
	if (i > Max) i = Max;
	if (i < Min) i = Min;
	
	if (i != Val)
	{
		Val = i;

		GViewI *n = GetNotify() ? GetNotify() : GetParent();
		if (n)
		{
			n->OnNotify(this, Val);
		}
		
		Invalidate();
	}
}

int64 GSlider::Value()
{
	return Val;
}

void GSlider::GetLimits(int64 &min, int64 &max)
{
	min = Min;
	max = Max;
}

void GSlider::SetLimits(int64 min, int64 max)
{
	Min = min;
	Max = max;
}

GMessage::Param GSlider::OnEvent(GMessage *Msg)
{
	return 0;
}

void GSlider::OnPaint(GSurface *pDC)
{
	pDC->Colour(LC_MED, 24);
	pDC->Rectangle();
	
	GRect r = GetClient();
	int y = r.Y() >> 1;
	r.y1 = y - 2;
	r.y2 = r.y1 + 3;
	r.x1 += 3;
	r.x2 -= 3;
	LgiWideBorder(pDC, r, DefaultSunkenEdge);
	
	if (Min <= Max)
	{
		int x = Val * r.X() / (Max-Min);
		Thumb.ZOff(5, 9);
		Thumb.Offset(r.x1 + x - 3, y - 5);
		GRect b = Thumb;
		LgiWideBorder(pDC, b, DefaultRaisedEdge);
		pDC->Rectangle(&b);		
	}
}

void GSlider::OnMouseClick(GMouse &m)
{
	Capture(m.Down());
	if (Thumb.Overlap(m.x, m.y))
	{
		Tx = m.x - Thumb.x1;
		Ty = m.y - Thumb.y1;
	}
}

void GSlider::OnMouseMove(GMouse &m)
{
	if (IsCapturing())
	{
		int Rx = X() - 6;
		if (Rx > 0 && Max >= Min)
		{
			int x = m.x - Tx;
			int v = x * (Max-Min) / Rx;
			Value(v);
		}
	}
}

class GSlider_Factory : public GViewFactory
{
	GView *NewView(const char *Class, GRect *Pos, const char *Text)
	{
		if (stricmp(Class, "GSlider") == 0)
		{
			return new GSlider(-1, 0, 0, 100, 20, 0, 0);
		}

		return 0;
	}

} GSliderFactory;

