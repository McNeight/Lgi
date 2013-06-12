#include "Lgi.h"
#include "GMdi.h"
#include <stdio.h>

enum GMdiDrag
{
	DragNone	= 0,
	DragMove	= 0x01,
	DragLeft	= 0x02,
	DragTop		= 0x04,
	DragRight	= 0x08,
	DragBottom	= 0x10,
	DragClose	= 0x20,
	DragSystem	= 0x40
};

class GMdiChildPrivate
{
public:
	GMdiChild *Child;

	GRect Title;
	#if MDI_TAB_STYLE
	GRect Tab;
	int Order;
	#else
	GRect Close;
	GRect System;
	#endif
	
	int Fy;	
	GMdiDrag Drag;	
	int Ox, Oy;
	bool CloseDown;
	bool CloseOnUp;
	
	GMdiChildPrivate(GMdiChild *c)
	{
		CloseOnUp = false;
		Child = c;
		CloseDown = false;
		Fy = SysFont->GetHeight() + 1;
		Title.ZOff(-1, -1);
		
		#if MDI_TAB_STYLE
		Tab.ZOff(-1, -1);
		Order = -1;
		#else
		Close.ZOff(-1, -1);
		System.ZOff(-1, -1);
		#endif
	}

	#if !MDI_TAB_STYLE
	GMdiDrag HitTest(int x, int y)
	{
		GMdiDrag Hit = DragNone;
		
		if (Child->WindowFromPoint(x, y) == Child)
		{
			if (!Child->GetClient().Overlap(x, y))
			{
				if (System.Overlap(x, y))
				{
					Hit = DragSystem;
				}
				else if (Close.Overlap(x, y))
				{
					Hit = DragClose;
				}
				else if (Title.Overlap(x, y))
				{
					Hit = DragMove;
				}
				else
				{
					GRect c = Child->GLayout::GetClient();
					int Corner = 24;
					if (x < c.x1 + Corner)
					{
						(int&)Hit |= DragLeft;
					}
					if (x > c.x2 - Corner)
					{
						(int&)Hit |= DragRight;
					}
					if (y < c.y1 + Corner)
					{
						(int&)Hit |= DragTop;
					}
					if (y > c.y2 - Corner)
					{
						(int&)Hit |= DragBottom;
					}
				}
			}
		}

		return Hit;
	}
	#endif
};

GMdiChild::GMdiChild()
{
	d = new GMdiChildPrivate(this);
	GdcPt2 m(100, d->Fy + 8);
	SetMinimumSize(m);

	#if WIN32NATIVE
	SetStyle(GetStyle() | WS_CLIPSIBLINGS);
	#endif
}

GMdiChild::~GMdiChild()
{
	DeleteObj(d);
}

bool GMdiChild::Attach(GViewI *p)
{
	bool s = GLayout::Attach(p);
	if (s) AttachChildren();
	return s;
}

char *GMdiChild::Name()
{
	return GView::Name();
}

bool GMdiChild::Name(const char *n)
{
	bool s = GView::Name(n);
	Invalidate((GRect*)0, false, true);
	return s;
}

bool GMdiChild::Pour()
{
	GRect c = GetClient();
	GRegion Client(c);
	GRegion Update;

	for (GViewI *w = Children.First(); w; w = Children.Next())
	{
		GRect OldPos = w->GetPos();
		Update.Union(&OldPos);

		if (w->Pour(Client))
		{
			GRect p = w->GetPos();
			
			if (!w->Visible())
			{
				w->Visible(true);
			}

			// w->Invalidate();

			Client.Subtract(&w->GetPos());
			Update.Subtract(&w->GetPos());
		}
		else
		{
			// non-pourable
		}
	}

	return true;
}

#if !MDI_TAB_STYLE
GRect &GMdiChild::GetClient(bool ClientSpace)
{
	static GRect r;
	
	r = GLayout::GetClient(ClientSpace);
	r.Size(3, 3);
	r.y1 += d->Fy + 2;
	r.Size(1, 1);
	
	return r;
}

void GMdiChild::OnPaint(GSurface *pDC)
{
	GRect p = GLayout::GetClient();

	// Border
	LgiWideBorder(pDC, p, RAISED);
	pDC->Colour(LC_MED, 24);
	pDC->Box(&p);
	p.Size(1, 1);

	// Title bar
	char *n = Name();
	if (!n) n = "";
	d->Title.ZOff(p.X()-1, d->Fy + 1);
	d->Title.Offset(p.x1, p.y1);
	d->System = d->Title;
	d->System.x2 = d->System.x1 + d->System.Y() - 1;

	// Title text
	bool Top = false;
	if (GetParent())
	{
		GViewIterator *it = GetParent()->IterateViews();
		Top = it->Last() == (GViewI*)this;
		DeleteObj(it);
	}
	
	SysFont->Colour(Top ? LC_ACTIVE_TITLE_TEXT : LC_INACTIVE_TITLE_TEXT, Top ? LC_ACTIVE_TITLE : LC_INACTIVE_TITLE);
	SysFont->Transparent(false);
	GDisplayString ds(SysFont, n);
	ds.Draw(pDC, d->System.x2 + 1, d->Title.y1 + 1, &d->Title);
	
	// System button
	GRect r = d->System;
	r.Size(2, 1);
	pDC->Colour(LC_BLACK, 24);
	pDC->Box(&r);
	r.Size(1, 1);
	pDC->Colour(LC_WHITE, 24);
	pDC->Rectangle(&r);
	pDC->Colour(LC_BLACK, 24);
	for (int k=r.y1 + 1; k < r.y2 - 1; k += 2)
	{
		pDC->Line(r.x1 + 1, k, r.x2 - 1, k);
	}

	// Close button
	d->Close = d->Title;
	d->Close.x1 = d->Close.x2 - d->Close.Y() + 1;
	d->Close.Size(2, 2);
	r = d->Close;
	LgiWideBorder(pDC, r, d->CloseDown ? SUNKEN : RAISED);
	pDC->Colour(LC_MED, 24);
	pDC->Rectangle(&r);
	int Cx = d->Close.x1 + (d->Close.X() >> 1) - 1 + d->CloseDown;
	int Cy = d->Close.y1 + (d->Close.Y() >> 1) - 1 + d->CloseDown;
	int s = 3;
	pDC->Colour(Rgb24(108,108,108), 24);
	pDC->Line(Cx-s, Cy-s+1, Cx+s-1, Cy+s);
	pDC->Line(Cx-s+1, Cy-s, Cx+s, Cy+s-1);
	pDC->Line(Cx-s, Cy+s-1, Cx+s-1, Cy-s);
	pDC->Line(Cx-s+1, Cy+s, Cx+s, Cy-s+1);
	pDC->Colour(LC_BLACK, 24);
	pDC->Line(Cx-s, Cy-s, Cx+s, Cy+s);
	pDC->Line(Cx-s, Cy+s, Cx+s, Cy-s);

	// Client
	GRect Client = GetClient();
	Client.Size(-1, -1);
	pDC->Colour(LC_MED, 24);
	pDC->Box(&Client);
	
	if (!Children.First())
	{
		Client.Size(1, 1);
		pDC->Colour(LC_WORKSPACE, 24);
		pDC->Rectangle(&Client);
	}
	
	Pour();
}

void GMdiChild::OnMouseClick(GMouse &m)
{
	if (m.Left())
	{
		Raise();

		if (m.Down())
		{
			GRect c = GLayout::GetClient();
			d->Drag = DragNone;
			d->Ox = d->Oy = -1;
			
			d->Drag = d->HitTest(m.x, m.y);
			if (d->Drag)
			{
				Capture(true);
			}

			if (d->Drag == DragSystem)
			{
				if (m.Double())
				{
					d->CloseOnUp = true;
				}
				else
				{
					/*
					GdcPt2 p(d->System.x1, d->System.y2+1);
					PointToScreen(p);
					GSubMenu *Sub = new GSubMenu;
					if (Sub)
					{
						Sub->AppendItem("Close", 1, true);
						Close = Sub->Float(this, p.x, p.y, true) == 1;
						DeleteObj(Sub);
					}
					*/
				}
			}
			else if (d->Drag == DragClose)
			{
				d->CloseDown = true;
				Invalidate(&d->Close);
			}
			else if (d->Drag == DragMove)
			{
				d->Ox = m.x - c.x1;
				d->Oy = m.y - c.y1;
			}
			else
			{
				if (d->Drag & DragLeft)
				{
					d->Ox = m.x - c.x1;
				}
				if (d->Drag & DragRight)
				{
					d->Ox = m.x - c.x2;
				}
				if (d->Drag & DragTop)
				{
					d->Oy = m.y - c.y1;
				}
				if (d->Drag & DragBottom)
				{
					d->Oy = m.y - c.y2;
				}
			}
		}
		else
		{
			if (IsCapturing())
			{
				Capture(false);
			}

			if (d->CloseOnUp)
			{
				d->Drag = DragNone;
				if (OnRequestClose(false))
				{
					Quit();
					return;
				}
			}
			else if (d->Drag == DragClose)
			{
				if (d->Close.Overlap(m.x, m.y))
				{
					d->Drag = DragNone;
					d->CloseDown = false;
					Invalidate(&d->Close);
					
					if (OnRequestClose(false))
					{
						Quit();
						return;
					}
				}
				else printf("%s:%i - Outside close btn.\n", _FL);
			}
			else
			{
				d->Drag = DragNone;
			}
		}
	}
}

LgiCursor GMdiChild::GetCursor(int x, int y)
{
	GMdiDrag Hit = d->HitTest(x, y);
	if (Hit & DragLeft)
	{
		if (Hit & DragTop)
		{
			return LCUR_SizeFDiag;
		}
		else if (Hit & DragBottom)
		{
			return LCUR_SizeBDiag;
		}
		else
		{
			return LCUR_SizeHor;
		}
	}
	else if (Hit & DragRight)
	{
		if (Hit & DragTop)
		{
			return LCUR_SizeBDiag;
		}
		else if (Hit & DragBottom)
		{
			return LCUR_SizeFDiag;
		}
		else
		{
			return LCUR_SizeHor;
		}
	}
	else if ((Hit & DragTop) || (Hit & DragBottom))
	{
		return LCUR_SizeVer;
	}
	
	return LCUR_Normal;
}

void GMdiChild::OnMouseMove(GMouse &m)
{
	if (IsCapturing())
	{
		GRect p = GetPos();
		GdcPt2 Min = GetMinimumSize();

		if (d->Drag == DragClose)
		{
			bool Over = d->Close.Overlap(m.x, m.y);
			if (Over ^ d->CloseDown)
			{
				d->CloseDown = Over;
				Invalidate(&d->Close);
			}
		}
		else
		{
			// GetMouse(m, true);
			GdcPt2 n(m.x, m.y);
			n.x += GetPos().x1;
			n.y += GetPos().y1;
			// GetParent()->PointToView(n);
				
			if (d->Drag == DragMove)
			{
				p.Offset(n.x - d->Ox - p.x1, n.y - d->Oy - p.y1);
				if (p.x1 < 0) p.Offset(-p.x1, 0);
				if (p.y1 < 0) p.Offset(0, -p.y1);
			}
			else
			{
				if (d->Drag & DragLeft)
				{
					p.x1 = min(p.x2 - Min.x, n.x - d->Ox);
					if (p.x1 < 0) p.x1 = 0;
				}
				if (d->Drag & DragRight)
				{
					p.x2 = max(p.x1 + Min.x, n.x - d->Ox);
				}
				if (d->Drag & DragTop)
				{
					p.y1 = min(p.y2 - Min.y, n.y - d->Oy);
					if (p.y1 < 0) p.y1 = 0;
				}
				if (d->Drag & DragBottom)
				{
					p.y2 = max(p.y1 + Min.y, n.y - d->Oy);
				}
			}
		}

		SetPos(p);
	}
}
#endif

#if defined __GTK_H__
using namespace Gtk;
#endif

void GMdiChild::Raise()
{
	GViewI *p = GetParent();
	if (p)
	{
		#if !MDI_TAB_STYLE
			#if defined __GTK_H__
			GtkWidget *wid = Handle();
			GdkWindow *wnd = wid ? GDK_WINDOW(wid->window) : NULL;
			if (wnd)
				gdk_window_raise(wnd);
			else
				LgiAssert(0);
			#elif WIN32NATIVE
			SetWindowPos(Handle(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			#else
			#error "Impl me."
			#endif
		#endif

		p->DelView(this);
		p->AddView(this);
		p->OnChildrenChanged(this, true);
	}
}

void GMdiChild::Lower()
{
	GViewI *p = GetParent();
	if (p)
	{
		#if !MDI_TAB_STYLE
			#if defined __GTK_H__
			GtkWidget *wid = Handle();
			GdkWindow *wnd = wid ? GDK_WINDOW(wid->window) : NULL;
			if (wnd)
				gdk_window_lower(wnd);
			else
				LgiAssert(0);
			#elif WIN32NATIVE
			SetWindowPos(Handle(), HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			#else
			#error "Impl me."
			#endif
		#endif

		p->DelView(this);
		p->AddView(this, 0);
		p->OnChildrenChanged(this, true);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
class GMdiParentPrivate
{
public:
	GRect Tabs;
	GRect Content;
};

GMdiParent::GMdiParent()
{
	d = new GMdiParentPrivate;
	SetPourLargest(true);
}

GMdiParent::~GMdiParent()
{
	if (GetWindow())
	{
		GetWindow()->UnregisterHook(this);
	}
	
	DeleteObj(d);
}

void GMdiParent::OnPaint(GSurface *pDC)
{
	#if MDI_TAB_STYLE
	// Draw tabs...
	GFont *Fnt = GetFont();
	
	GColour Back(LC_LOW, 24), Shadow(LC_SHADOW, 24);
	List<GViewI>::I it = Children.Start();
	int Idx = 0, Cx = 5;
	pDC->Colour(Back);
	pDC->Rectangle(d->Tabs.x1, d->Tabs.y1, d->Tabs.x1 + Cx - 1, d->Tabs.y2 - 1);
	pDC->Colour(Shadow);
	pDC->Line(d->Tabs.x1, d->Tabs.y2, d->Tabs.x2, d->Tabs.y2);
	
	for (GViewI *v = *it; v; v = *++it, Idx++)
	{
		GMdiChild *c = dynamic_cast<GMdiChild*>(v);
		if (!c) continue;
		
		GDisplayString ds(GetFont(), c->Name());
		c->d->Tab.ZOff(ds.X()-1, ds.Y()-1);
		c->d->Tab.Offset(d->Tabs.x1 + Cx, d->Tabs.y1);
		
		bool Active = Idx == 0;
		c->Visible(Active);
		GColour Bk(Active ? LC_WORKSPACE : LC_LIGHT, 24);
		GColour Edge(LC_BLACK, 24);
		GColour Txt(LC_TEXT, 24);
		
		GRect r = c->d->Tab;
		pDC->Colour(Edge);
		pDC->Box(&r);
		r.Size(1, 1);
		Fnt->Fore(Txt);
		Fnt->Back(Bk);
		Fnt->Transparent(false);
		ds.Draw(pDC, r.x1, r.y1, &r);
		
		Cx += ds.X();
	}

	pDC->Colour(Back);
	pDC->Rectangle(d->Tabs.x1 + Cx, d->Tabs.y1, d->Tabs.x2, d->Tabs.y2 - 1);
	
	if (Children.Length() == 0)
	{
		pDC->Colour(LC_MED, 24);
		pDC->Rectangle(&d->Content);
	}
	#else
	pDC->Colour(LC_LOW, 24);
	pDC->Rectangle();
	#endif
}

bool GMdiParent::Attach(GViewI *p)
{
	bool s = GLayout::Attach(p);
	if (s)
	{
		AttachChildren();
		GetWindow()->RegisterHook(this, GKeyAndMouseEvents);
	}
	return s;
}

GMdiChild *GMdiParent::IsChild(GView *View)
{
	for (GViewI *v=View; v; v=v->GetParent())
	{
		if (v->GetParent() == this)
		{
			GMdiChild *c = dynamic_cast<GMdiChild*>(v);
			if (c)
			{
				return c;
			}
			LgiAssert(0);
			break;
		}
	}
	
	return 0;
}

bool GMdiParent::OnViewMouse(GView *View, GMouse &m)
{
	if (m.Down())
	{
		GMdiChild *c = IsChild(View);
		if (c)
		{
			c->Raise();
		}
	}
	
	return true;
}

bool GMdiParent::OnViewKey(GView *View, GKey &Key)
{
	if (Key.Down() AND Key.Ctrl() AND Key.c16 == '\t')
	{
		bool Child = IsChild(View);
		if (Child)
		{
			GView *v;

			if (Key.Shift())
			{
				v = dynamic_cast<GView*>(Children.First());
			}
			else
			{
				v = dynamic_cast<GView*>(Children.Last());
			}

			GMdiChild *c = dynamic_cast<GMdiChild*>(v);
			if (c)
			{
				if (Key.Shift())
				{
					c->Raise();
				}
				else
				{
					c->Lower();
				}

				return true;
			}
		}
	}

	return false;
}

#if MDI_TAB_STYLE
void GMdiParent::OnPosChange()
{
	GFont *Fnt = GetFont();
	GRect Client = GetClient();
	d->Tabs = Client;
	d->Tabs.y2 = d->Tabs.y1 + Fnt->GetHeight() + 2;
	d->Content = Client;
	d->Content.y1 = d->Tabs.y2 + 1;

	GMdiChild *c = dynamic_cast<GMdiChild*>(Children.First());
	if (c)
	{	
		c->SetPos(d->Content);
		c->Pour();
	}
}
#endif

GRect GMdiParent::NewPos()
{
	GRect Status(0, 0, X()*0.75, Y()*0.75);

	int Block = 5;
	for (int y=0; y<Y()>>Block; y++)
	{
		for (int x=0; x<X()>>Block; x++)
		{
			bool Has = false;
			for (GViewI *c = Children.First(); c; c = Children.Next())
			{
				GRect p = c->GetPos();
				if (p.x1 >> Block == x AND
					p.y1 >> Block == y)
				{
					Has = true;
					break;
				}
			}
			
			if (!Has)
			{
				Status.Offset(x << Block, y << Block);
				return Status;
			}
		}
	}
	
	return Status;
}

void GMdiParent::OnChildrenChanged(GViewI *Wnd, bool Attaching)
{
	#if MDI_TAB_STYLE
	if (Wnd && Attaching)
	{
		OnPosChange();
	}
	#else
	for (GViewI *v=Children.First(); v; )
	{
		GMdiChild *c = dynamic_cast<GMdiChild*>(v);
		v = Children.Next();
		if (c)
		{
			c->Invalidate(&c->d->Title);
			
			if (!v)
			{
				GViewI *n = c->Children.First();
				if (n)
				{
					n->Focus(true);
				}
			}
		}
	}
	#endif
}
