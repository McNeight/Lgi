#include <math.h>
#include <stdio.h>

#include "Lgi.h"
#include "GSkinEngine.h"
#include "GPath.h"
#include "GButton.h"
#include "GCombo.h"
#include "GCheckBox.h"
#include "GRadioGroup.h"
#include "GDisplayString.h"
#include "GCssTools.h"
#include "LStringLayout.h"

#ifdef WIN32
#define BTN_TEXT_OFFSET_Y	-1
#else
#define BTN_TEXT_OFFSET_Y	0
#endif

#define GREY24				Rgb24(44, 44, 44)
#define GREY32(a)			Rgba32(44, 44, 44, a)

#define CHECK_BORDER		2
#define CHECK_RADIUS		3

#define Btn_Value			0x1
#define Btn_Enabled			0x2
#define Btn_Max				((Btn_Value|Btn_Enabled)+1)

#if defined(WIN32) && !defined(LGI_SDL)
// Wine can't handle 32 bit DIB sections... :(
#define OsDefaultCs			System24BitColourSpace
#else
// 32bit is marginally faster to render to.
#define OsDefaultCs			System32BitColourSpace
#endif

#define CUSTOM_COLOURS		0

class GelSkin : public GSkinEngine
{
	GApp *App;
	COLOUR c80;
	COLOUR c160;
	COLOUR c172;
	COLOUR c222;
	COLOUR c232;
	COLOUR c253;
	COLOUR c255;
	GMemDC *CheckBox[Btn_Max];
	GMemDC *RadioBtn[Btn_Max];

	COLOUR LgiLighten(COLOUR c32, int Amount)
	{
		System32BitPixel *p = (System32BitPixel*)&c32;
		p->r += ((255 - p->r) * Amount) >> 8;
		p->g += ((255 - p->g) * Amount) >> 8;
		p->b += ((255 - p->b) * Amount) >> 8;
		return Rgba32(p->r, p->g, p->b, p->a);
	}

	COLOUR LgiDarken(COLOUR c32, int Amount)
	{
		System32BitPixel *p = (System32BitPixel*)&c32;
		p->r = (p->r * Amount) >> 8;
		p->g = (p->g * Amount) >> 8;
		p->b = (p->b * Amount) >> 8;
		return Rgba32(p->r, p->g, p->b, p->a);
	}

	void FillPath(GPath *Path, GSurface *pDC, bool Down, bool Enabled = true)
	{
		if (pDC)
		{
			GRect r(0, 0, pDC->X()-1, pDC->Y()-1);

			#if 0

			COLOUR Top = LgiLighten(Rgb24To32(LC_MED), 62 * 256 / 63); // Rgba32(253, 253, 253, 255);
			COLOUR Mid = LgiLighten(Rgb24To32(LC_MED), 49 * 256 / 63); // Rgba32(239, 239, 239, 255);
			COLOUR Mid2 = LgiLighten(Rgb24To32(LC_MED), 31 * 256 / 63); // Rgba32(222, 222, 222, 255);

			#endif

			COLOUR Top = c253;
			COLOUR Mid = c232;
			COLOUR Mid2 = c222;
			COLOUR Bot = c255;
			if (!Enabled)
			{
				Top = LgiDarken(Top, 230);
				Mid = LgiDarken(Mid, 230);
				Mid2 = LgiDarken(Mid2, 230);
				Bot = LgiDarken(Bot, 230);
			}
			
			// Draw background
			GPath e;
			e.Rectangle(r.x1, r.y1, r.x2+1, r.y2+1);
			
			GPointF c1(r.x1, r.y1);
			GPointF d1(r.x1, r.y2+1);
			if (Down)
			{
				GBlendStop s1[] =
				{
					{0.0, Rgba32(192, 192, 192, 255)},
					{0.1, Top},
					{0.6, Mid},
					{0.601, Mid2},
					{1.0, Bot},
				};					
				GLinearBlendBrush b1(c1, d1, CountOf(s1), s1);
				e.Fill(pDC, b1);
			}
			else
			{
				GBlendStop s1[] =
				{
					{0.0, Top},
					{0.5, Mid},
					{0.501, Mid2},
					{1.0, Bot},
				};					
				GLinearBlendBrush b1(c1, d1, CountOf(s1), s1);
				e.Fill(pDC, b1);
			}

			pDC->Colour(GREY32(255), 32);
			pDC->Line(0, pDC->Y()-1, pDC->X()-1, pDC->Y()-1);
			pDC->Colour(Rgba32(0xc6, 0xc6, 0xc6, 255), 32);
			pDC->Line(0, pDC->Y()-2, pDC->X()-2, pDC->Y()-2);
			pDC->Line(pDC->X()-1, 0, pDC->X()-1, pDC->Y()-2);
		}
	}
	
	void DrawBtn(GSurface *pDC, GRect &r, GColour *Base, bool Down, bool Enabled, bool Default = false)
	{
		if (!pDC)
			return;

		#if LGI_SDL
		
		pDC->Colour(Base?*Base:GColour(192, 192, 192));
		pDC->Rectangle(&r);
		pDC->Colour(GColour(96, 96, 96));
		if (Down)
		{
			pDC->Line(r.x1, r.y1, r.x2, r.y1);
			pDC->Line(r.x1, r.y1, r.x1, r.y2);
		}
		else
		{
			pDC->Line(r.x1, r.y2, r.x2, r.y2);
			pDC->Line(r.x2, r.y1, r.x2, r.y2);
		}
		
		#else
			
		GRect Client = r;
		{
			// Edge
			GPath e;
			GRectF r(Client);
			// r.y2++;
			e.RoundRect(r, 6);
		
			COLOUR EdgeColour = Default ? Rgba32(40, 40, 40, 255) : Rgba32(114, 114, 114, 255);
			GSolidBrush b(EdgeColour);
			e.Fill(pDC, b);
		}

		{
			// Border
			GPath e;
			GRectF r(Client);
			// r.y2++;
			int Resize = Default ? 2 : 1;
			r.Size(Resize, Resize);
			if (Down)
			{
				r.x1 = r.x1 + 1;
				r.y1 = r.y1 + 1;
			}
			e.RoundRect(r, 6 - Resize);

			// Fill
			COLOUR Top = c253;
			COLOUR Mid = c232;
			COLOUR Mid2 = c222;
			COLOUR Bot = c255;
			if (Base)
			{
				Top = Base->c32();
				Mid = Base->c32();
				Mid2 = Base->c32();
				Bot = Base->c32();
			}
			else
			{
				if (!Enabled)
				{
					Top = LgiDarken(Top, 230);
					Mid = LgiDarken(Mid, 230);
					Mid2 = LgiDarken(Mid2, 230);
					Bot = LgiDarken(Bot, 230);
				}
			}
		
			GPointF c1(r.x1, r.y1);
			GPointF d1(r.x1, r.y2);
			if (Down)
			{
				GBlendStop s1[] =
				{
					{0.0, Rgba32(192, 192, 192, 255)},
					{0.1, Top},
					{0.6, Mid},
					{0.601, Mid2},
					{1.0, Bot},
				};					
				GLinearBlendBrush b1(c1, d1, CountOf(s1), s1);
				e.Fill(pDC, b1);
			}
			else
			{
				GBlendStop s1[] =
				{
					{0.0, Top},
					{0.5, Mid},
					{0.501, Mid2},
					{1.0, Bot},
				};					
				GLinearBlendBrush b1(c1, d1, CountOf(s1), s1);
				e.Fill(pDC, b1);
			}

			double Round = (r.X()-13)/r.X();
			if (Round < 0.6)
				Round = 0.6;
				
			int Sa = Down ? 128 : 50;
			GBlendStop s3[] =
			{
				{Round, GREY32(0)},
				{1.0, GREY32(Sa)},
			};					
			
			// Rounded corners
			GPointF c3(r.x1 + (r.X()/2), r.y1 + (r.Y()/2));
			GPointF d3(r.x1, r.y1);
			GRadialBlendBrush b3(c3, d3, CountOf(s3), s3);
			e.Fill(pDC, b3);
		}
		
		#endif
	}

	GMemDC *DrawCtrl(GViewI *Ctrl, int Flags, bool Round)
	{
		GMemDC *Mem = new GMemDC;
		if (Mem && Mem->Create(14, 14, OsDefaultCs))
		{
			// blank out background
			GCss::ColorDef Back;
			if (Ctrl->GetCss())
				Back = Ctrl->GetCss()->BackgroundColor();
			if (Back.Type == GCss::ColorRgb)
				Mem->Colour(Back.Rgb32, 32);
			else
				Mem->Colour(LC_MED, 24);
			Mem->Rectangle();
			
			GRectF Box(0, 0, Mem->X(), Mem->Y());
			double Radius = Box.X()/2;
			GPointF Center(Box.X()/2, Box.Y()/2);

			int Grey = R24(LC_MED);
			bool Enabled = Flags & Btn_Enabled;
			int BorderCol = Enabled ? 80 : 160;

			if (Enabled)
			{
				// draw sunken border
				GRectF r = Box;
				GPath p;
				if (Round)
					p.Circle(Center, Radius);
				else
					p.RoundRect(r, CHECK_RADIUS + CHECK_BORDER);
				
				// gradient from 169,169,169 at the top through to 225,225,225
				int Dark = MAX(0, Grey - 40);
				int Light = MIN(255, Grey + 40);				
				
				GPointF a(0, 0), b(0, 15);
				GBlendStop s[] =
				{
					{0, c172},
					{1, c253}
				};
				GLinearBlendBrush c(a, b, 2, s);
				p.Fill(Mem, c);
			}
			
			if (Enabled)
			{
				// draw button center
				GRectF r = Box;
				r.Size(CHECK_BORDER+1, CHECK_BORDER+1);
				GPath p;
				if (Round)
					p.Circle(Center, r.X()/2);
				else
					p.RoundRect(r, CHECK_RADIUS-1);

				if (Enabled)
				{
					GPointF a(0, r.y1), b(0, r.y2);
					GBlendStop s[] =
					{
						{1.0/15.0, c255},
						{1.0/14.0, c253},
						{1, c232}
					};
					GLinearBlendBrush c(a, b, CountOf(s), s);
					p.Fill(Mem, c);
				}
				else
				{
					GSolidBrush c(Rgb24To32(LC_MED));
					p.Fill(Mem, c);
				}
			}
			else
			{
				// draw button hightlight, a white outline shifted down 1 pixel
				GRectF r = Box;
				r.Size(CHECK_BORDER, CHECK_BORDER);
				r.Offset(0, 1);
				GPointF Cntr = Center;
				Cntr.y = Cntr.y + 1;
				
				GPath p;
				if (Round)
					p.Circle(Cntr, r.X()/2);
				else
					p.RoundRect(r, CHECK_RADIUS);
				r.Size(1, 1);
				if (Round)
					p.Circle(Cntr, r.X()/2);
				else
					p.RoundRect(r, CHECK_RADIUS-1);
				
				GSolidBrush c(Rgb24To32(LC_LIGHT));
				p.Fill(Mem, c);
			}

			{
				// draw button outline
				GRectF r = Box;
				r.Size(CHECK_BORDER, CHECK_BORDER);
				GPath p;
				if (Round)
					p.Circle(Center, r.X()/2);
				else
					p.RoundRect(r, CHECK_RADIUS);
				r.Size(1, 1);
				if (Round)
					p.Circle(Center, r.X()/2);
				else
					p.RoundRect(r, CHECK_RADIUS-1);
				
				GSolidBrush c(Enabled ? c80 : c160);
				p.Fill(Mem, c);
			}
			
			
			if (Flags & Btn_Value)
			{
				// draw the check mark
				GRectF r = Box;
				r.Size(CHECK_BORDER+2, CHECK_BORDER+2);

				double Cx = r.x1 + (r.X() / 2);
				double Cy = r.y1 + (r.Y() / 2);
				double A = r.X() / 6;
				double B = (r.X() / 2) - A;

				GPath p;
				if (Round)
				{
					p.Circle(Center, r.X()/3);
				}
				else
				{
					p.MoveTo(r.x1, r.y1);
					p.LineTo(r.x1 + A, r.y1);
					p.LineTo(Cx, r.y1 + B);
					p.LineTo(r.x2 - A, r.y1);
					p.LineTo(r.x2, r.y1);
					p.LineTo(r.x2, r.y1 + A);
					p.LineTo(r.x2 - B, Cy);
					p.LineTo(r.x2, r.y2 - A);
					p.LineTo(r.x2, r.y2);
					p.LineTo(r.x2 - A, r.y2);
					p.LineTo(Cx, r.y2 - B);
					p.LineTo(r.x1 + A, r.y2);
					p.LineTo(r.x1, r.y2);
					p.LineTo(r.x1, r.y2 - A);
					p.LineTo(r.x1 + B, Cy);
					p.LineTo(r.x1, r.y1 + A);
					p.LineTo(r.x1, r.y1);
				}
				
				GSolidBrush c(Enabled ? c80 : c160);
				p.Fill(Mem, c);
			}
		}
		
		return Mem;
	}

	void DrawText(GSkinState *State, int x, int y, GRect &rcFill, bool Enabled, GViewI *Ctrl)
	{
		GCss::ColorDef CssFore, CssBack;
		GColour Fore(LC_TEXT, 24), Back(LC_MED, 24), Light, Low;
		if (Ctrl->GetCss())
		{
			CssFore = Ctrl->GetCss()->Color();
			if (CssFore.Type == GCss::ColorRgb)
				Fore.Set(CssFore.Rgb32, 32);
			CssBack = Ctrl->GetCss()->BackgroundColor();
			if (CssBack.Type == GCss::ColorRgb)
				Back.Set(CssBack.Rgb32, 32);
			else
				Back.Empty();
		}
		if (!Enabled)
		{
			Light.Set(LC_LIGHT, 24);
			Low.Set(LC_LOW, 24);
		}

		GRegion Rgn;
		Rgn = rcFill;
		GArray<GDisplayString*> *Text = State->AllText();
		GSurface *pDC = State->pScreen;
		if (Text && Text->Length() > 0 && rcFill.X() > 3)
		{
			for (unsigned i=0; i<Text->Length(); i++)
			{
				LLayoutString *t = dynamic_cast<LLayoutString*>((*Text)[i]);
				if (!t)
					break;
				GRect c;
				c.ZOff(t->X() - 1, t->Y() - 1);
				c.Offset(x + (t->Fx >> GDisplayString::FShift), y + t->y);
				Rgn.Subtract(&c);
				
				GFont *f = t->GetFont();
				if (Enabled)
				{
					f->Colour(Fore, Back);
					if (Ctrl->Focus())
					{
						pDC->Colour(LC_MIDGREY, 24);
						pDC->Box(&c);
						c.Size(1, 1);
						pDC->Colour(Back);
						pDC->Rectangle(&c);
						c.Size(-1, -1);
						
						f->Transparent(true);
						t->Draw(pDC, c.x1, c.y1, &c);
					}
					else
					{
						f->Transparent(!Back.IsValid());
						t->Draw(pDC, c.x1, c.y1, &c);
					}
				}
				else
				{
					f->Transparent(!Back.IsValid());
					f->Colour(Light, Back);
					t->Draw(pDC, c.x1 + 1, c.y1 + 1, &c);

					f->Transparent(true);
					f->Colour(Low, Back);
					t->Draw(pDC, c.x1, c.y1, &c);
				}
			}
		}
		
		if (Back.IsValid())
		{
			pDC->Colour(Back);
			for (GRect *rc = Rgn.First(); rc; rc = Rgn.Next())
				pDC->Rectangle(rc);
		}
	}

public:
	GelSkin(GApp *a)
	{
		// printf("Skin @ %i bits\n", GdcD->GetBits());
		App = a;
		ZeroObj(CheckBox);
		ZeroObj(RadioBtn);

		COLOUR Med = Rgb24To32(LC_MED);
		c80 = LgiDarken(Med, 80 * 256 / 192);
		c160 = LgiDarken(Med, 160 * 256 / 192);
		c172 = LgiDarken(Med, 172 * 256 / 192);
		c222 = Med;
		c232 = LgiLighten(Med, 33 * 256 / 63);
		c253 = LgiLighten(Med, 60 * 256 / 63);
		c255 = Rgba32(255, 255, 255, 255);
	}
	
	~GelSkin()
	{
		int i;
		for (i=0; i<CountOf(CheckBox); i++)
			DeleteObj(CheckBox[i]);
		for (i=0; i<CountOf(RadioBtn); i++)
			DeleteObj(RadioBtn[i]);
	}

	uint32 GetFeatures()
	{
		return
				#if CUSTOM_COLOURS
				GSKIN_COLOUR |
				#endif
				GSKIN_BUTTON |
				GSKIN_COMBO |
				GSKIN_LISTCOL |
				GSKIN_CHECKBOX |
				GSKIN_RADIO;
	}

	#if CUSTOM_COLOURS
	uint32 GetColour(int i)
	{
		switch (i)
		{
			// LC_BLACK
			case 0: return Rgb24(0, 0, 0);
			// LC_DKGREY
			case 1: return Rgb24(88, 88, 88);
			// LC_MIDGREY
			case 2: return Rgb24(180, 180, 180);
			// LC_LTGREY
			case 3:	return Rgb24(222, 222, 222);
			// LC_WHITE
			case 4: return Rgb24(255, 255, 255);

			// LC_SHADOW
			case 5: return Rgb24(64, 64, 64);
			// LC_LOW
			case 6: return Rgb24(128, 128, 128);
			#ifdef LINUX
			// LC_MED
			case 7: return Rgb24(230, 230, 230);
			#else
			// LC_MED
			case 7: return Rgb24(216, 216, 216);
			#endif
			// LC_HIGH
			case 8: return Rgb24(0xf6, 0xf6, 0xf6);
			// LC_LIGHT
			case 9: return Rgb24(0xff, 0xff, 0xff);
			
			// LC_DIALOG
			case 10: return Rgb24(216, 216, 216);
			// LC_WORKSPACE
			case 11: return Rgb24(0xff, 0xff, 0xff);
			// LC_TEXT
			case 12: return Rgb24(0, 0, 0);
			// LC_SELECTION
			case 13: return Rgb24(0x4a, 0x59, 0xa5);
			// LC_SEL_TEXT
			case 14: return Rgb24(0xff, 0xff, 0xff);

			// LC_ACTIVE_TITLE
			case 15: return Rgb24(0, 0, 0x80);
			// LC_ACTIVE_TITLE_TEXT
			case 16: return Rgb24(0xff, 0xff, 0xff);
			// LC_INACTIVE_TITLE
			case 17: return Rgb24(0x80, 0x80, 0x80);
			// LC_INACTIVE_TITLE_TEXT
			case 18: return Rgb24(0x40, 0x40, 0x40);

			// LC_MENU_BACKGROUN
			case 19: return Rgb24(222, 222, 222);
			// LC_MENU_TEXT
			case 20: return Rgb24(0, 0, 0);
		}
		
		return 0;
	}
	#endif
	
	void OnPaint_GButton(GButton *Ctrl, GSkinState *State)
	{
		GMemDC Mem;
		if (Mem.Create(Ctrl->X(), Ctrl->Y(), OsDefaultCs))
		{
			// Font
			if (Ctrl->GetFont() == SysFont)
			{
				Ctrl->SetFont(SysBold);
			}

			// Background
			GCssTools Tools(Ctrl->GetCss(), Ctrl->GetFont());
			GColour DefaultBack;
			GColour &Fore = Tools.GetFore(), &Back = Tools.GetBack(&DefaultBack);
			GColour NoPaint(LC_MED, 24);			
			if (Ctrl->GetCss())
			{
				GCss::ColorDef np = Ctrl->GetCss()->NoPaintColor();
				if (np.Type == GCss::ColorRgb)
					NoPaint.Set(np.Rgb32, 32);
				else
					NoPaint.Empty();
			}			
			if (NoPaint.IsValid())
				Mem.Colour(NoPaint);
			else
				Mem.Colour(0, 32);
			Mem.Rectangle();
			
			DrawBtn(&Mem,
					Ctrl->GetClient(),
					Back.IsValid() ? &Back : NULL,
					Ctrl->Value(),
					Ctrl->Enabled(),
					Ctrl->Default());
			
			GSurface *Out = &Mem;
			
			GArray<GDisplayString*> *Txt = State->AllText();

			int ContentX = 0;
			int SpacingPx = 4;
			if (State->Image)
				ContentX += State->Image->X();			
			int MaxTxt = 0;
			if (Txt)
			{
				for (unsigned i=0; i<Txt->Length(); i++)
				{
					MaxTxt = MAX(MaxTxt, (*Txt)[i]->X());
				}
				ContentX += MaxTxt;
			}
			if (State->Image && Txt && Txt->Length() > 0)
				ContentX += SpacingPx;			

			int CurX = (Ctrl->X() - ContentX) >> 1;
			int Off = Ctrl->Value() ? 1 : 0;
			if (State->Image)
			{
				int CurY = (Ctrl->Y() - State->Image->Y()) >> 1;
				int Op = Out->Op(GDC_ALPHA);
				Out->Blt(CurX+Off, CurY+Off, State->Image);
				Out->Op(Op);
				CurX += State->Image->X() + SpacingPx;
			}
			if (Txt && Txt->Length() > 0)
			{
				GDisplayString *First = (*Txt)[0];
				int sx = MaxTxt, sy = Txt->Length() * First->Y();
				int ty = (Ctrl->Y()-sy) >> 1;

				GFont *f = First->GetFont();
				f->Transparent(true);
				
				for (unsigned i=0; i<Txt->Length(); i++)
				{
					GDisplayString *Text = (*Txt)[i];
					if (Ctrl->Enabled())
					{
						f->Colour(Fore, Back);
						Text->Draw(Out, CurX+Off, ty+Off+BTN_TEXT_OFFSET_Y);
					}
					else
					{
						f->Colour(GColour(LC_LIGHT, 24), Back);
						Text->Draw(Out, CurX+Off+1, ty+Off+1+BTN_TEXT_OFFSET_Y);

						f->Colour(GColour(LC_LOW, 24), Back);
						Text->Draw(Out, CurX+Off, ty+Off+BTN_TEXT_OFFSET_Y);
					}
					ty += Text->Y();
				}
				
				if (Ctrl->Focus())
				{
					GRect b(CurX-2, ty, CurX + sx + 1, ty + sy - 2);
					b.Offset(Off, Off);
					Out->Colour(Rgb24(180, 180, 180), 24);
					Out->Box(&b);
				}
			}
			
			int Op = State->pScreen->Op(GDC_ALPHA);
			State->pScreen->Blt(0, 0, &Mem);	
			State->pScreen->Op(Op);
		}
		else
		{
			State->pScreen->Colour(Rgb24(255, 0, 255), 24);
			State->pScreen->Rectangle();
		}
	}

	void OnPaint_ListColumn(ProcColumnPaint Callback, void *UserData, GSkinState *State)
	{
		// Setup memory context
		GRect r = State->Rect;
		GMemDC Mem(r.X(), r.Y(), OsDefaultCs);
		if (Mem[0])
		{
			r.Offset(-r.x1, -r.y1);		
			
			GPath e;
			e.Rectangle(r.x1, r.y1, r.x2, r.y2);
			static bool LastEnabled = true;			
			FillPath(&e, &Mem, State ? State->Value : false, State ? LastEnabled = State->Enabled : LastEnabled);
			if (State && State->Value)
			{
				Mem.Colour(Rgb24(0xc0, 0xc0, 0xc0), 24);
				Mem.Line(r.x1, r.y1, r.x1, r.y2-1);
				Mem.Colour(Rgb24(0xe0, 0xe0, 0xe0), 24);
				Mem.Line(r.x1+1, r.y1+1, r.x1+1, r.y2-2);
			}
			r.Size(2, 2);
			if (Callback)
			{
				Mem.Op(GDC_ALPHA);
				Callback(UserData, &Mem, r, false);
			}
			State->pScreen->Blt(State->Rect.x1, State->Rect.y1, &Mem);
		}
	}

	void OnPaint_GCombo(GCombo *Ctrl, GSkinState *State)
	{
		GMemDC Mem;
		if (Mem.Create(Ctrl->X(), Ctrl->Y(), OsDefaultCs))
		{
			// Font
			if (Ctrl->GetFont() == SysFont)
				Ctrl->SetFont(SysBold);

			// Back
			GColour TextDefault(GREY24, 24);
			GCssTools Tools(Ctrl->GetCss(), Ctrl->GetFont());
			GColour &Fore = Tools.GetFore(&TextDefault), &Back = Tools.GetBack();
			if (Back.IsValid())
			{
				Mem.Colour(Back);
				Mem.Rectangle();
			}

			DrawBtn(&Mem, Ctrl->GetClient(), NULL, false, State->Enabled);
			
			int n = 22;
			GColour DkGrey(LC_DKGREY, 24);
			
			if (Ctrl->X() > 32)
			{
				GDisplayString *Text = State->FirstText();
				if (Text)
				{
					int sx = Text->X(), sy = Text->Y();
					int tx = GCombo::Pad.x1;
					int ty = (Ctrl->Y()-sy) >> 1;
						
					int Off = 0;
					GRect c = Ctrl->GetClient();
					c.x1 += 8;
					c.x2 -= n + 3;
					
					int Cx = Ctrl->X();
					int PadX = GCombo::Pad.x1 + GCombo::Pad.x2;
					if (Text->X() > PadX)
					{
						// Make the text fit
						Text->TruncateWithDots(Cx - PadX);
					}

					GFont *f = Text->GetFont();
					f->Transparent(true);
					if (Ctrl->Enabled())
					{
						f->Colour(Fore, Back);
						Text->Draw(&Mem, tx+Off, ty+Off+BTN_TEXT_OFFSET_Y, &c);
					}
					else
					{
						f->Colour(LC_LIGHT, LC_MED);
						Text->Draw(&Mem, tx+Off+1, ty+Off+1+BTN_TEXT_OFFSET_Y, &c);

						f->Colour(LC_LOW, LC_MED);
						Text->Draw(&Mem, tx+Off, ty+Off+BTN_TEXT_OFFSET_Y, &c);
					}
					
					if (Ctrl->Focus() && c.X() > 4)
					{
						GRect b(tx-2, ty, tx + sx + 1, ty + sy - 2);
						b.Offset(Off, Off);
						c.Size(-2, 0);
						b.Bound(&c);

						Mem.Colour(Rgb24(180, 180, 180), 24);
						Mem.Box(&b);
					}
				}
				
				// Draw separator
				Mem.Colour(Rgba32(180, 180, 180, 255), 32);
				Mem.Line(Mem.X()-n, 1, Mem.X()-n, Mem.Y()-2);
			}
			
			Mem.Colour(State->Enabled ? Fore : DkGrey);
			int Bx = Mem.X() < 26 ? Mem.X()/2 : Mem.X()-13, By = (Mem.Y() + 4) >> 1;
			for (int i=0; i<5; i++)
			{				
				Mem.Line(Bx-i, By-i, Bx+i, By-i);
			}
			
			State->pScreen->Blt(0, 0, &Mem);
		}
		else
		{
			State->pScreen->Colour(Rgb24(255, 0, 255), 24);
			State->pScreen->Rectangle();
		}
	}

	void OnPaint_GCheckBox(GCheckBox *Ctrl, GSkinState *State)
	{
		int Flags = (Ctrl->Value() ? Btn_Value : 0) |
					(Ctrl->Enabled() ? Btn_Enabled : 0);
		
		// Create the bitmaps in cache if not already there
		GCssTools Tools(Ctrl->GetCss(), Ctrl->GetFont());
		GColour &Back = Tools.GetBack();

		GMemDC *Temp = 0;
		GMemDC *&Mem = Back.IsValid() ? Temp : CheckBox[Flags];
		if (!Mem)
		{
			Mem = DrawCtrl(Ctrl, Flags, false);
		}

		// Output to screen
		if (Mem)
		{
			// Draw icon
			int FontY = Ctrl->GetFont()->GetHeight();

			GRect Box(0, 0, Mem->X()-1, Mem->Y()-1);
			if (FontY > Mem->Y())
				Box.Offset(0, FontY - Mem->Y());

			State->pScreen->Blt(Box.x1, Box.y1, Mem);

			GRect Box1(Box.x1, 0, Box.x2, Box.y1 - 1);
			GRect Box2(Box.x1, Box.y2 + 1, Box.x2, Ctrl->Y()-1);
			if (Back.IsValid())
			{
				State->pScreen->Colour(Back);
				if (Box.y1 > 0)
					State->pScreen->Rectangle(&Box1);
				if (Box.y2 < Ctrl->Y() - 1)
					State->pScreen->Rectangle(&Box2);
			}
			else
			{
				State->pScreen->Colour(LC_MED, 24);
				if (Box.y1 > 0)
					State->pScreen->Rectangle(&Box1);
				if (Box.y2 < Ctrl->Y() - 1)
					State->pScreen->Rectangle(&Box2);
			}	

			// Draw text
			GRect t(Mem->X(), 0, Ctrl->X()-1, Ctrl->Y()-1);
			if (t.Valid())
			{
				DrawText(State,
						Mem->X() + 4, 0,
						t,
						Flags & Btn_Enabled,
						Ctrl);
			}
		}
		else
		{
			State->pScreen->Colour(Rgb24(255, 0, 255), 24);
			State->pScreen->Rectangle();
		}

		DeleteObj(Temp);
	}

	void OnPaint_GRadioButton(GRadioButton *Ctrl, GSkinState *State)
	{
		int Flags = (Ctrl->Value() ? Btn_Value : 0) |
					(Ctrl->Enabled() ? Btn_Enabled : 0);
		
		// Create the bitmaps in cache if not already there
		GMemDC *&Mem = RadioBtn[Flags];
		if (!Mem)
		{
			Mem = DrawCtrl(Ctrl, Flags, true);
		}

		// Output to screen
		if (Mem)
		{
			// Draw icon
			GRect ico;
			GCssTools Tools(Ctrl->GetCss(), Ctrl->GetFont());
			GColour &Fore = Tools.GetFore(), &Back = Tools.GetBack();

			ico.ZOff(Mem->X()-1, Mem->Y()-1);
		    if (ico.Y() < Ctrl->Y())
		        ico.Offset(0, (Ctrl->Y() - ico.Y()) >> 1);
			State->pScreen->Blt(ico.x1, ico.y1, Mem);
			if (Back.IsValid())
			{
				State->pScreen->Colour(Back);
				if (ico.y1 > 0)
					State->pScreen->Rectangle(0, 0, ico.x2, ico.y1-1);
				if (ico.y2 < Ctrl->Y())
					State->pScreen->Rectangle(0, ico.y2+1, ico.x2, Ctrl->Y()-1);
			}

			// Draw text
			GRect t(Mem->X(), 0, Ctrl->X()-1, Ctrl->Y()-1);
			if (t.Valid())
			{
				int y = 0;
				if (State->TextObjects())
				{
					GDisplayString *ds = State->FirstText();
					if (ds && t.Y() > ds->Y())
					{
						y = (t.Y() - ds->Y()) >> 1;
					}					
				}
				
				DrawText(State,
						Mem->X() + 4, y,
						t,
						Flags & Btn_Enabled,
						Ctrl);
			}
		}
		else
		{
			State->pScreen->Colour(Rgb24(255, 0, 255), 24);
			State->pScreen->Rectangle();
		}
	}

	GFont *GetDefaultFont(char *Class)
	{
		if (Class && stricmp(Class, Res_Button) == 0)
		{
			return SysBold;
		}

		return SysFont;
	}
};

GSkinEngine *
CreateSkinEngine(class GApp *App)
{
	return new GelSkin(App);
}

