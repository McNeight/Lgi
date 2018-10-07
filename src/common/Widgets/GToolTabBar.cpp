/*hdr
**      FILE:           ToolTabBar.cpp
**      AUTHOR:         Matthew Allen
**      DATE:           22/3/2000
**      DESCRIPTION:    Toolbar of tabs
**
**      Copyright (C) 1997-1998 Matthew Allen
**              fret@memecode.com
*/

#include <stdio.h>
#include "Lgi.h"
#include "GToolTabBar.h"
#include "GNotifications.h"
#include "GCss.h"

/////////////////////////////////////////////////////////////////////////
GToolTab::GToolTab() : GToolButton(16, 16)
{
	First = true;
	#ifdef WINDOWS
	SetClassW32("GToolTab");
	#endif
}

GToolTab::~GToolTab()
{
}

void GToolTab::OnPaint(GSurface *pDC)
{
	#if 1
	pDC->Colour(LC_MED, 24);
	#else
	pDC->Colour(GColour(255, 0, 255));
	#endif
	pDC->Rectangle();
}

bool GToolTab::SetPos(GRect &r, bool Repaint)
{
	return GToolButton::SetPos(r, Repaint);
}

/////////////////////////////////////////////////////////////////////////
GToolTabBar::GToolTabBar(int Id)
{
	GRect r(0, 0, 100, 100);
	if (Id >= 0)
		SetId(Id);
	SetPos(r);
	Current = 0;
	FitToArea = false;
	Border = true;
	InOnChangeEvent = false;
}

GToolTabBar::~GToolTabBar()
{
}

int64 GToolTabBar::Value()
{
	int n=0;
	GAutoPtr<GViewIterator> it(IterateViews());
	for (GViewI *i=it->First(); i; i=it->Next(), n++)
	{
		GToolTab *tt = dynamic_cast<GToolTab*>(i);
		if (tt && tt->Value())
			return n;
	}

	return -1;
}

void GToolTabBar::Value(int64 new_value)
{
	int n=0;
	GAutoPtr<GViewIterator> it(IterateViews());
	for (GViewI *i=it->First(); i; i=it->Next(), n++)
	{
		if (n == new_value)
		{
			GToolButton *b = dynamic_cast<GToolButton*>(i);
			if (b)
			{
				OnChange(b);
				break;
			}
		}	
	}
}

void GToolTabBar::OnButtonClick(GToolButton *Btn)
{
	GToolBar::OnButtonClick(Btn);
	
	GToolTab *b = dynamic_cast<GToolTab*>(Btn);
	if (b && b != Current)
		OnChange(Btn);
}

int GToolTabBar::OnNotify(GViewI *c, int f)
{
	if (f == GNotifyValueChanged)
	{
		GToolTab *b = dynamic_cast<GToolTab*>(c);
		if (b && b != Current)
			OnChange(b);
	}
	
	return GLayout::OnNotify(c, f);
}

bool GToolTabBar::IsOk()
{
	int Count[2] = {0, 0};
	GAutoPtr<GViewIterator> it(IterateViews());
	for (GViewI *c = it->First(); c; c = it->Next())
	{
		GToolTab *tt = dynamic_cast<GToolTab*>(c);
		if (tt && tt->GetId() > 0)
		{
			bool Dn = tt->Value() != 0;
			Count[Dn != 0]++;
		}
	}
	LgiAssert(Count[1] == 0 || Count[1] == 1);
	return Count[1] == 0 || Count[1] == 1;
}

void GToolTabBar::OnChange(GToolButton *Btn)
{
	 // Current->Value(...) causes recursive events.
	if (InOnChangeEvent)
		return;
	InOnChangeEvent = true;

	// Detach the old button
	if (Current)
	{
		Invalidate(&Current->TabPos);
		Current->Visible(false);
		Current = 0;
	}

	// Attach new button
	if ((Current = dynamic_cast<GToolTab*>(Btn)))
	{
		Current->Value(true);
		Current->Visible(true);
		Current->SetPos(Client);

		if (Current->First)
		{
			Current->OnSelect();
			Current->First = false;
		}

		Invalidate(&Current->TabPos);
	}

	IsOk();
	InOnChangeEvent = false;
}

void GToolTabBar::_PaintTab(GSurface *pDC, GToolTab *Tab)
{
	GRect t = Tab->TabPos;
	GColour Background(LC_MED, 24);

	if (Tab->Value())
	{
		GCss::ColorDef c;
		if (Tab->GetCss())
			c = Tab->GetCss()->Color();
		if (!c.IsValid())
		{
			c.Type = GCss::ColorRgb;
			c.Rgb32 = Background.c32();
		}
		
		pDC->Colour(c.Rgb32, 32);
	}
	else
	{
		pDC->Colour(Background);
	}
	pDC->Rectangle(&t);
	
	if (!Tab->Value())
	{
		t.Size(2, 2);
		if (IsVertical())
		{
			t.x2++;
		}
		else
		{
			t.y2++;
		}
	}
	else
	{
		pDC->Colour(LC_MED, 24);
		if (IsVertical())
		{
			pDC->Rectangle(t.x1+1, t.y1+1, t.x2, t.y2-1);
		}
		else
		{
			pDC->Rectangle(t.x1+1, t.y1+1, t.x2-1, t.y2);
		}
	}

	// draw lines around tab
	pDC->Colour(LC_LIGHT, 24);
	if (IsVertical())
	{
		pDC->Line(t.x1, t.y1+1, t.x1, t.y2-1);
		pDC->Line(t.x1+1, t.y1, t.x2, t.y1);
		pDC->Colour(LC_LOW, 24);
		pDC->Line(t.x1+1, t.y2, t.x2, t.y2);
	}
	else
	{
		pDC->Line(t.x1+1, t.y1, t.x2-1, t.y1);
		pDC->Line(t.x1, t.y1+1, t.x1, t.y2);
		pDC->Colour(LC_LOW, 24);
		pDC->Line(t.x2, t.y1+1, t.x2, t.y2);
	}

	// draw icon
	int Off = Tab->Value() ? 2 : 1;
	if (GetImageList())
		GetImageList()->Draw(pDC, t.x1 + Off, t.y1 + Off, Tab->Image(), Background);
}

void GToolTabBar::OnPaint(GSurface *pScreen)
{
	GRect r = GetClient();

	#ifdef WIN32
	GMemDC Mem(r.X(), r.Y(), GdcD->GetColourSpace());
	GSurface *pDC = &Mem;
	#else
	GSurface *pDC = pScreen;
	#endif

	int Off = 4 + ((Border) ? 6 : 0);

	if (Border)
	{
		// Draw border
		LgiThinBorder(pDC, r, DefaultRaisedEdge);
		pDC->Colour(LC_MED, 24);
		pDC->Rectangle(&r);
		r.Size(5, 5);
	}
	else
	{
		pDC->Colour(LC_MED, 24);
		if (IsVertical())
		{
			pDC->Rectangle(r.x1, r.y1, r.x1 + GetBx() + Off, r.y2);
		}
		else
		{
			pDC->Rectangle(r.x1, r.y1, r.x2, r.y1 + GetBy() + Off);
		}
	}

	// Draw rest of the box
	if (IsVertical())
	{
		r.x1 = GetBx() + Off;
	}
	else
	{
		r.y1 = GetBy() + Off;
	}
	LgiThinBorder(pDC, r, DefaultRaisedEdge);
	pDC->Colour(LC_MED, 24);
	pDC->Rectangle(&r);

	// Draw tabs
	GViewI *w;
	GArray<GToolTab*> Down;
	for (w = Children.Last(); w; w = Children.Prev())
	{
		GToolTab *Tab = dynamic_cast<GToolTab*>(w);
		if (Tab)
		{
			if (!Tab->Value())
			{
				_PaintTab(pDC, Tab);
			}
			else
			{
				Down.Add(Tab);
			}
		}
	}

	GColour c;
	if (Down.Length() > 1)
		c.Rgb(255, 0, 0);
	else
		c.Rgb(255, 255, 255);
	for (unsigned i=0; i<Down.Length(); i++)
	{
		Down[i]->GetCss(true)->Color(GCss::ColorDef(GCss::ColorRgb, c.c32()));
		_PaintTab(pDC, Down[i]);
	}

	#ifdef WIN32
	pScreen->Blt(0, 0, &Mem);
	#endif
}

void GToolTabBar::OnMouseClick(GMouse &m)
{
	if (m.Down())
	{
		for (GViewI *c = Children.First(); c; c = Children.Next())
		{
			GToolTab *Tab = dynamic_cast<GToolTab*>(c);
			if (Tab)
			{
				if (Tab->TabPos.Overlap(m.x, m.y))
				{
					if (Current != Tab)
					{
						OnButtonClick(Tab);
						break;
					}
				}
			}
		}
	}
}

bool GToolTabBar::Pour(GRegion &r)
{
	bool Status = false;

	Tab.x1 = Tab.y1 = Tab.x2 = Tab.y2 = (Border) ? 6 : 0;
	int x = (IsVertical()) ? Tab.y1 : Tab.x1;
	int y = (IsVertical()) ? Tab.x1 : Tab.y1;
	
	for (GViewI *w = Children.First(); w; w = Children.Next())
	{
		GToolTab *Btn = dynamic_cast<GToolTab*>(w);
		if (Btn)
		{
			if (Btn->GetId() == IDM_SEPARATOR)
			{
				if (IsVertical())
				{
					y += 10;
				}
				else
				{
					x += 10;
				}
			}
			else
			{
				if (Btn->GetId() == IDM_BREAK)
				{
					if (IsVertical())
					{
						x += GetBx() + 4;
						y = Tab.y1;
					}
					else
					{
						x =  Tab.x1;
						y += GetBy() + 4;
					}
				}

				GRect Rgn(x, y, x + GetBx() + 4, y + GetBy() + 4);
				Btn->TabPos = Rgn;
				if (IsVertical())
				{
					y += GetBy() + 1;
				}
				else
				{
					x += GetBx() + 1;
				}
			}

			Btn->Visible(Btn->Value() != 0);
		}
	}

	if (IsVertical())
	{
		Tab.x2 = x + ((Border) ? 12 : 0);
		Tab.y2 = y + 50;
	}
	else
	{
		Tab.x2 = x + 50;
		Tab.y2 = y + ((Border) ? 12 : 0);
	}

	GRect *Best = FindLargest(r);
	if (Best)
	{
		GRect p;
		if (FitToArea)
		{
			p.Set(0, 0, Best->X()-1, Best->Y()-1);
		}
		else
		{
			if (IsVertical())
			{
				p.Set(0, 0, 200, Tab.Y());
			}
			else
			{
				p.Set(0, 0, Tab.X(), 200);
			}
		}

		Client = GetClient();
		if (IsVertical())
		{
			Client.x1 += GetBx() + 4;
		}
		else
		{
			Client.y1 += GetBx() + 4;
		}
		Client.Size(1, 1);

		p.Offset(Best->x1, Best->y1);
		p.Bound(Best);
		SetPos(p);

		Status = true;
	}

	return Status;
}

void GToolTabBar::OnCreate()
{
	AttachChildren();
	
	GToolButton *Btn = dynamic_cast<GToolButton*>(Children.First());
	if (Btn)
	{
		Btn->Value(true);
		OnChange(Btn);
	}
}
