#include <stdlib.h>
#include <stdio.h>

#include "Lgi.h"
#include "GSkinEngine.h"
#include "GCheckBox.h"
#include "GDisplayString.h"
#include "LStringLayout.h"
#include "LgiRes.h"

static int PadX1Px = 20;
static int PadX2Px = 6;
#ifdef MAC
static int PadYPx = 8;
static int TextYOffset = 6;
#else
static int PadYPx = 0;
static int TextYOffset = 0;
#endif
static int MinYSize = 16;

class GCheckBoxPrivate : public LMutex, public LStringLayout
{
	GCheckBox *Ctrl;
	GFontCache Cache;
	
public:
	int64 Val;
	bool Over;
	bool Three;
	GRect ValuePos;

	GCheckBoxPrivate(GCheckBox *ctrl) :
		LMutex("GCheckBoxPrivate"),
		LStringLayout(&Cache)
	{
		Ctrl = ctrl;
		Val = 0;
		Over = false;
		Three = false;
		Wrap = true;
		AmpersandToUnderline = true;
		ValuePos.ZOff(-1, -1);
	}

	bool PreLayout(int32 &Min, int32 &Max)
	{
		if (Lock(_FL))
		{
			DoPreLayout(Min, Max);
			Unlock();
		}
		else return false;
		return true;
	}

	bool Layout(int Px)
	{		
		if (Lock(_FL))
		{
			DoLayout(Px, MinYSize);
			Unlock();
		}
		else return false;
		return true;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////
// Check box
GCheckBox::GCheckBox(int id, int x, int y, int cx, int cy, const char *name, int InitState) :
	ResObject(Res_CheckBox)
{
	d = new GCheckBoxPrivate(this);
	Name(name);
	GdcPt2 Max = d->GetMax();
	if (cx < 0) cx = Max.x + PadX1Px + PadX2Px;
	if (cy < 0) cy = MAX(Max.y, MinYSize) + PadYPx;

	d->Val = InitState;
	GRect r(x, y, x+cx, y+cy);
	SetPos(r);
	SetId(id);
	SetTabStop(true);
}

GCheckBox::~GCheckBox()
{
	DeleteObj(d);
}

void GCheckBox::OnAttach()
{
	LgiResources::StyleElement(this);
	OnStyleChange();
	GView::OnAttach();
}

void GCheckBox::OnStyleChange()
{
	if (d->Lock(_FL))
	{
		d->Empty();
		d->Add(GView::Name(), GetCss());
		d->DoLayout(X());
		d->Unlock();
		Invalidate();
	}
}

int GCheckBox::OnNotify(GViewI *Ctrl, int Flags)
{
	if (Ctrl == (GViewI*)this && Flags == GNotify_Activate)
	{
		Value(!Value());
	}

	return 0;
}

GMessage::Result GCheckBox::OnEvent(GMessage *m)
{
	return GView::OnEvent(m);
}

bool GCheckBox::ThreeState()
{
	return d->Three;
}

void GCheckBox::ThreeState(bool t)
{
	d->Three = t;
}

int64 GCheckBox::Value()
{
	return d->Val;
}

void GCheckBox::Value(int64 i)
{
	if (d->Val != i)
	{
		d->Val = i;
		Invalidate(&d->ValuePos);

		SendNotify((int)d->Val);
	}
}

bool GCheckBox::Name(const char *n)
{
	bool Status = false;
	if (d->Lock(_FL))
	{
		Status = GView::Name(n);
		
		d->Empty();
		d->Add(n, GetCss());
		d->SetBaseFont(GetFont());
		d->DoLayout(X());
		
		d->Unlock();
	}

	return Status;
}

bool GCheckBox::NameW(const char16 *n)
{
	bool Status = false;
	if (d->Lock(_FL))
	{
		Status = GView::NameW(n);
		d->Unlock();
	}
	d->Layout(X());
	return Status;
}

void GCheckBox::SetFont(GFont *Fnt, bool OwnIt)
{
	LgiAssert(Fnt && Fnt->Handle());

	if (d->Lock(_FL))
	{
		GView::SetFont(Fnt, OwnIt);
		d->Unlock();
	}
	d->Layout(X());
	Invalidate();
}

void GCheckBox::OnMouseClick(GMouse &m)
{
	if (Enabled())
	{
		int Click = IsCapturing();

		Capture(d->Over = m.Down());
		if (m.Down())
		{
			Focus(true);
		}
		
		GRect r(0, 0, X()-1, Y()-1);
		if (!m.Down() &&
			r.Overlap(m.x, m.y) &&
			Click)
		{
			if (d->Three)
			{
				switch (d->Val)
				{
					case 0:
						Value(2); break;
					case 2:
						Value(1); break;
					default:
						Value(0); break;
				}
			}
			else
			{
				Value(!d->Val);
			}
		}
		else
		{
			Invalidate(&d->ValuePos);
		}
	}
}

void GCheckBox::OnMouseEnter(GMouse &m)
{
	if (IsCapturing())
	{
		d->Over = true;
		Invalidate(&d->ValuePos);
	}
}

void GCheckBox::OnMouseExit(GMouse &m)
{
	if (IsCapturing())
	{
		d->Over = false;
		Invalidate(&d->ValuePos);
	}
}

bool GCheckBox::OnKey(GKey &k)
{
	switch (k.vkey)
	{
		case ' ':
		{
			if (!k.Down())
			{
				Value(!Value());
			}

			return true;
		}
	}
	
	return false;
}

void GCheckBox::OnFocus(bool f)
{
	Invalidate();
}

void GCheckBox::OnPosChange()
{
	d->Layout(X());
}

bool GCheckBox::OnLayout(GViewLayoutInfo &Inf)
{
	if (!Inf.Width.Min)
	{
		d->PreLayout(Inf.Width.Min,
					 Inf.Width.Max);
		Inf.Width.Min += PadX1Px + PadX2Px;
		Inf.Width.Max += PadX1Px + PadX2Px;
	}
	else
	{
		d->Layout(Inf.Width.Max);
		Inf.Height.Min = d->GetMin().y + PadYPx;
		Inf.Height.Max = d->GetMax().y + PadYPx;
	}
	return true;
}

void GCheckBox::OnPaint(GSurface *pDC)
{
	#if 0
	pDC->Colour(GColour(255, 0, 255));
	pDC->Rectangle();
	#endif
	
	if (GApp::SkinEngine &&
		TestFlag(GApp::SkinEngine->GetFeatures(), GSKIN_CHECKBOX))
	{
		GSkinState State;
		State.pScreen = pDC;
		State.MouseOver = d->Over;
		State.aText = d->GetStrs();
		d->ValuePos.Set(0, 0, 15, 15);
		GApp::SkinEngine->OnPaint_GCheckBox(this, &State);
	}
	else
	{
		bool en = Enabled();
		GRect r = GetClient();
		
		#if defined MAC && !defined COCOA
		d->ValuePos.Set(0, 0, PadX1Px, MinYSize);
		#else
		d->ValuePos.Set(0, 0, 12, 12);
		#endif
		
		if (d->ValuePos.y2 < r.y2)
		{
			pDC->Colour(LC_MED, 24);
			pDC->Rectangle(0, d->ValuePos.y2+1, d->ValuePos.x2, r.y2);
		}

		GRect t = r;
		t.x1 = d->ValuePos.x2 + 1;
		// GColour cFore = StyleColour(GCss::PropColor, LC_TEXT);
		GColour cBack = StyleColour(GCss::PropBackgroundColor, GColour(LC_MED, 24));

		if (d->Lock(_FL))
		{
			int Ty = MAX(0, r.Y() - d->GetBounds().Y()) >> 1;
			GdcPt2 pt(t.x1, t.y1 + MIN(Ty, TextYOffset));
					
			d->Paint(pDC, pt, cBack, t, en, false);
			d->Unlock();
		}

		#if defined MAC && !defined COCOA && !defined(LGI_SDL)

			if (!cBack.IsTransparent())
			{
				pDC->Colour(cBack);
				pDC->Rectangle(d->ValuePos.x1, d->ValuePos.y1, d->ValuePos.x2, Y()-1);
			}
		
			GRect c = GetClient();
			#if 0
			pDC->Colour(Rgb24(255, 0, 0), 24);
			pDC->Box(&c);
			pDC->Line(c.x1, c.y1, c.x2, c.y2);
			pDC->Line(c.x2, c.y1, c.x1, c.y2);
			#endif

			for (GViewI *v = this; v && !v->Handle(); v = v->GetParent())
			{
				GRect p = v->GetPos();
				c.Offset(p.x1, p.y1);
			}
			
			HIRect Bounds = c;
			HIThemeButtonDrawInfo Info;
			HIRect LabelRect;
	
			Info.version = 0;
			Info.state = d->Val ? kThemeStatePressed : (Enabled() ? kThemeStateActive : kThemeStateInactive);
			Info.kind = kThemeCheckBox;
			Info.value = d->Val ? kThemeButtonOn : kThemeButtonOff;
			Info.adornment = Focus() ? kThemeAdornmentFocus : kThemeAdornmentNone;
	
			OSStatus e = HIThemeDrawButton(	  &Bounds,
											  &Info,
											  pDC->Handle(),
											  kHIThemeOrientationNormal,
											  &LabelRect);
	
			if (e) printf("%s:%i - HIThemeDrawButton failed %li\n", _FL, e);
		
		
		#else
	
			LgiWideBorder(pDC, d->ValuePos, DefaultSunkenEdge);
			pDC->Colour(d->Over || !en ? LC_MED : LC_WORKSPACE, 24);
			pDC->Rectangle(&d->ValuePos);
			pDC->Colour(en ? LC_TEXT : LC_LOW, 24);

			if (d->Three && d->Val == 2)
			{
				for (int y=d->ValuePos.y1; y<=d->ValuePos.y2; y++)
				{
					for (int x=d->ValuePos.x1; x<=d->ValuePos.x2; x++)
					{
						if ( (x&1) ^ (y&1) )
						{
							pDC->Set(x, y);
						}
					}
				}
			}
			else if (d->Val)
			{
				pDC->Line(d->ValuePos.x1+1, d->ValuePos.y1+1, d->ValuePos.x2-1, d->ValuePos.y2-1);
				pDC->Line(d->ValuePos.x1+1, d->ValuePos.y1+2, d->ValuePos.x2-2, d->ValuePos.y2-1);
				pDC->Line(d->ValuePos.x1+2, d->ValuePos.y1+1, d->ValuePos.x2-1, d->ValuePos.y2-2);

				pDC->Line(d->ValuePos.x1+1, d->ValuePos.y2-1, d->ValuePos.x2-1, d->ValuePos.y1+1);
				pDC->Line(d->ValuePos.x1+1, d->ValuePos.y2-2, d->ValuePos.x2-2, d->ValuePos.y1+1);
				pDC->Line(d->ValuePos.x1+2, d->ValuePos.y2-1, d->ValuePos.x2-1, d->ValuePos.y1+2);
			}
		
		#endif
	}
}

