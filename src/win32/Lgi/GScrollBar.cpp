#include "Lgi.h"
#include "GScrollBar.h"

//////////////////////////////////////////////////////////////////////////////////////
class GScrollBarPrivate
{
public:
	int			Type;		// vertical, horzontal or control
	SCROLLINFO	Info;
	int			Shift;

	int64		Min;
	int64		Max;
	int64		Page;
	int64		Value;

	GScrollBarPrivate()
	{
		Type = SB_VERT;
		ZeroObj(Info);
		Info.cbSize = sizeof(Info);
		Info.fMask = SIF_ALL; // | SIF_DISABLENOSCROLL;
		Info.nMax = -1;
		Shift = 0;
		Min = Max = Page = Value = 0;
	}
};

GScrollBar::GScrollBar()
	: ResObject(Res_ScrollBar)
{
	d = new GScrollBarPrivate;
}

GScrollBar::GScrollBar(int id, int x, int y, int cx, int cy, const char *name)
	: ResObject(Res_ScrollBar)
{
	d = new GScrollBarPrivate;
	d->Type = SB_CTL;
	d->Info.fMask |= SIF_DISABLENOSCROLL;

	SetId(id);
	GRect r(x, y, x+cx, y+cy);
	SetPos(r);
	if (name) Name(name);

	SetStyle(GetStyle() | WS_CHILD | WS_TABSTOP | WS_VISIBLE);
	if (SubClass = GWin32Class::Create(GetClass()))
	{
		SubClass->SubClass("SCROLLBAR");
	}
}

GScrollBar::~GScrollBar()
{
	if (!Handle())
	{
		SetParentFlag(false);
	}
	DeleteObj(d);
}

int GScrollBar::GetScrollSize()
{
	return GetSystemMetrics(SM_CXVSCROLL);
}

bool GScrollBar::SetPos(GRect &p, bool Repaint)
{
	if (d->Type == SB_CTL)
	{
		SetVertical(p.Y() > p.X());
	}

	return GView::SetPos(p, Repaint);
}

bool GScrollBar::Vertical()
{
	return d->Type == SB_VERT;
}

void GScrollBar::SetVertical(bool v)
{
	if (d->Type != SB_CTL)
	{
		if (v)
		{
			d->Type = SB_VERT;
		}
		else
		{
			d->Type = SB_HORZ;
		}
	}
	else
	{
		if (v)
		{
			SetStyle(GetStyle() | SBS_VERT);
		}
		else
		{
			SetStyle(GetStyle() & ~SBS_VERT);
		}
		if (_View)
		{
			SetWindowPos(_View, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSIZE);
		}
	}
}

void GScrollBar::SetParentFlag(bool Set)
{
	int Flag = (Vertical()) ? WS_VSCROLL : WS_HSCROLL;

	if (d->Type != SB_CTL && !Handle() && GetParent())
	{
		if (GetParent()->IsAttached())
		{
			DWORD Flags = GetWindowLong(GetParent()->Handle(), GWL_STYLE);

			if (Set)
			{
				Flags |= Flag;
			}
			else
			{
				Flags &= ~Flag;

				// clear limits.
				d->Info.nMin = 0;
				d->Info.nMax = -1;
				Update();
			}

			// check that we're in thread...
			if (GetParent()->InThread())
			{
				// you can't call 'SetWindowLong' from a thread other than
				// the one that created the window.
				SetWindowLong(GetParent()->Handle(), GWL_STYLE, Flags);
				SetWindowPos(	GetParent()->Handle(),
								0, 0, 0, 0, 0,
								SWP_NOMOVE | SWP_NOZORDER | SWP_NOSIZE | SWP_FRAMECHANGED);

				if (Set)
				{
					Update();
				}
			}
			else
			{
				// out of thread... post a message to the window
				// so it can do it itself
				GetParent()->PostEvent(M_SET_WND_STYLE, 0, Flags);
			}
		}
	}
}

GViewI *GScrollBar::GetMyView()
{
	if (Handle())
	{
		return this;
	}
	else if (GetParent() &&
			 GetParent()->Handle())
	{
		return GetParent();
	}

	return 0;
}

void GScrollBar::Update()
{
	if (Handle())
	{
		if (InThread())
		{
			SetScrollInfo(Handle(), SB_CTL, &d->Info, true);

			bool En = Enabled();
			if (En && d->Info.nMax < d->Info.nMin)
				En = false;

			EnableScrollBar(Handle(), SB_CTL, (En) ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH);
		}
		else
		{
            PostEvent(M_SCROLL_BAR_CHANGED, 0, (GMessage::Param)this);
		}
	}
	else if (GetParent() &&
			 GetParent()->Handle())
	{
		if (GetParent()->InThread())
		{
			d->Info.fMask = SIF_ALL;
			SetScrollInfo(GetParent()->Handle(), d->Type, &d->Info, true);
		}
		else
		{
            GetParent()->PostEvent(M_SCROLL_BAR_CHANGED, 0, (GMessage::Param)this);
		}
	}
}

void GScrollBar::SetParent(GViewI *p)
{
	GView::SetParent(p);
	SetParentFlag(true);
}

int64 GScrollBar::Value()
{
	LgiAssert(d->Min < 1000000);

	int64 Max = d->Max - d->Page + 1;
	if (Max < d->Min)
		Max = d->Min;
	return limit(d->Value, d->Min, Max);
}

void GScrollBar::Value(int64 p)
{
	LgiAssert(d->Min < 1000000);

	int64 Val = limit(p, d->Min, d->Max);
	if (d->Value != Val)
	{
		d->Value = Val;
		d->Info.nPos = Val >> d->Shift;
		Update();
		
		if (GetParent())
		{
			GetParent()->Invalidate();
		}
	}
}

void GScrollBar::Limits(int64 &Low, int64 &High)
{
	Low = d->Min;
	High = d->Max;
}

void GScrollBar::SetLimits(int64 Low, int64 High)
{
	int64 h = High;
	d->Shift = 0;
	while (h > 0 && ((h >> d->Shift) >> 31) != 0)
	{
		d->Shift++;
	}
 
	LgiAssert(Low < 1000000);
 
	d->Min = Low;
	d->Max = High;
	d->Info.nMin = Low >> d->Shift;
	d->Info.nMax = High >> d->Shift;

	Update();
}

int64 GScrollBar::Page()
{
	return d->Page;
}

void GScrollBar::SetPage(int64 p)
{
	d->Page = p;
	d->Info.nPage = p >> d->Shift;
	Update();
}

bool GScrollBar::Invalidate(GRect *r, bool Repaint, bool NonClient)
{
	if (GetParent())
	{
		SetScrollInfo(GetParent()->Handle(), d->Type, &d->Info, true);
		return true;
	}
	return false;
}

GMessage::Result GScrollBar::OnEvent(GMessage *Msg)
{
	int Status = SubClass ? SubClass->CallParent(Handle(), Msg->m, Msg->a, Msg->b) : 0;

	if ((Msg->m == WM_HSCROLL && !Vertical())
		||
		(Msg->m == WM_VSCROLL && Vertical()))
	{
		SCROLLINFO Si;
		ZeroObj(Si);
		Si.cbSize = sizeof(Si);
		Si.fMask = SIF_ALL;

		GViewI *Hnd = GetMyView();
		if (Hnd &&
			GetScrollInfo(Hnd->Handle(), d->Type, &Si))
		{
			int ScrollCode = (int) LOWORD(Msg->a);
			// int PageMax = d->Info.nMax - d->Info.nPage + 1;
			int64 PageMax = d->Max - d->Page + 1;
			int64 Val = Si.nPos << d->Shift;
			int64 NewVal = Val;

			switch (ScrollCode)
			{
				case SB_BOTTOM:
				case SB_LINEDOWN:
				{
					NewVal = limit(Val+1, d->Min, PageMax);
					break;
				}
				case SB_TOP:
				case SB_LINEUP:
				{
					NewVal = limit(Val-1, d->Min, PageMax);
					break;
				}
				case SB_PAGEDOWN:
				{
					NewVal = limit(Val+d->Page, d->Min, PageMax);
					break;
				}
				case SB_PAGEUP:
				{
					NewVal = limit(Val-d->Page, d->Min, PageMax);
					break;
				}
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
				{
					NewVal = limit((int64)Si.nTrackPos<<d->Shift, d->Min, PageMax);
					break;
				}
			}

			if (NewVal != Val)
			{
				d->Value = NewVal;
				d->Info.nPos = d->Value >> d->Shift;
				Update();
				OnChange(d->Value);

				if (GetParent())
				{
					SendNotify(Si.nPos);
				}
			}
		}
	}

	switch (MsgCode(Msg))
	{
		case WM_CREATE:
		{
			Update();
			break;
		}
		case M_SCROLL_BAR_CHANGED:
		{
            if (MsgB(Msg) == (GMessage::Param)this)
			{
				Update();
			}
			break;
		}
		/*
		case WM_MOUSEMOVE:
		{
			int CurX = (Msg->b&0xFFFF), CurY = (Msg->b>>16);
			LgiCursor Cursor = LCUR_Normal;
			for (GViewI *c = this; Cursor == LCUR_Normal && c->GetParent(); c = c->GetParent())
			{
				GRect CPos = c->GetPos();
				Cursor = c->GetCursor(CurX, CurY);
				LgiTrace("Scrollbar MouseMOve %s = %i, pos=%i, %i\n", c->GetClass(), Cursor, CurX, CurY);
				CurX += CPos.x1;
				CurY += CPos.y1;
			}
			LgiToWindowsCursor(Cursor);
			break;
		}
		*/
	}

	return Status;
}

