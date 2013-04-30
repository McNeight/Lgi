/// \file
/// \author Matthew Allen, fret@memecode.com
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "Lgi.h"

#define LGI_PI				3.141592654
#define LGI_RAD				(360/(2*LGI_PI))

/****************************** Classes *************************************************************************************/
GPalette::GPalette()
{
	Data = 0;
	Size = 0;
}

GPalette::GPalette(GPalette *pPal)
{
	Size = 0;
	Data = 0;
	Set(pPal);
}

GPalette::GPalette(uchar *pPal, int s)
{
	Size = 0;
	Data = 0;
	Set(pPal, s);
}

GPalette::~GPalette()
{
	DeleteArray(Data);
	Size = 0;
}

int GPalette::GetSize()
{
	return Size;
}

GdcRGB *GPalette::operator[](int i)
{
	return (i>=0 AND i<Size) ? Data + i : 0;
}

void GPalette::Set(GPalette *pPal)
{
	DeleteArray(Data);
	Size = 0;
	if (pPal)
	{
		if (pPal->Data)
		{
			Data = new GdcRGB[pPal->Size];
			if (Data)
			{
				memcpy(Data, pPal->Data, sizeof(GdcRGB) * pPal->Size);
			}
		}
		
		Size = pPal->Size;
	}
}

void GPalette::Set(uchar *pPal, int s)
{
	DeleteArray(Data);
	Size = 0;
	Data = new GdcRGB[s];
	if (Data)
	{
		if (pPal)
		{
			for (int i=0; i<s; i++)
			{
				Data[i].R = *pPal++;
				Data[i].G = *pPal++;
				Data[i].B = *pPal++;
			}
		}

		Size = s;
	}
}

bool GPalette::Update()
{
	return true;
}

bool GPalette::SetSize(int s)
{
	GdcRGB *New = new GdcRGB[s];
	if (New)
	{
		memset(New, 0, s * sizeof(GdcRGB));
		if (Data)
		{
			memcpy(New, Data, min(s, Size)*sizeof(GdcRGB));
		}

		DeleteArray(Data);
		Data = New;
		Size = s;
		return true;
	}

	return false;
}

void GPalette::SwapRAndB()
{
	if (Data)
	{
		for (int i=0; i<GetSize(); i++)
		{
			uchar n = (*this)[i]->R;
			(*this)[i]->R = (*this)[i]->B;
			(*this)[i]->B = n;
		}
	}

	Update();
}

uchar *GPalette::MakeLut(int Bits)
{
	uchar *Lut = 0;
	GdcRGB *p = (*this)[0];

	int Size = 1 << Bits;
	switch (Bits)
	{
		case 15:
		{
			Lut = new uchar[Size];
			if (Lut)
			{
				for (int i=0; i<Size; i++)
				{
					int r = (R15(i) << 3) | (R15(i) >> 2);
					int g = (G15(i) << 3) | (G15(i) >> 2);
					int b = (B15(i) << 3) | (B15(i) >> 2);

					Lut[i] = MatchRgb(Rgb24(r, g, b));
				}
			}
			break;
		}
		case 16:
		{
			Lut = new uchar[Size];
			if (Lut)
			{
				for (int i=0; i<Size; i++)
				{
					int r = (R16(i) << 3) | (R16(i) >> 2);
					int g = (G16(i) << 2) | (G16(i) >> 3);
					int b = (B16(i) << 3) | (B16(i) >> 2);

					Lut[i] = MatchRgb(Rgb24(r, g, b));
				}
			}
			break;
		}
	}

	return Lut;
}

int GPalette::MatchRgb(COLOUR Rgb)
{
	if (Data)
	{
		GdcRGB *Entry = (*this)[0];
		int r = (R24(Rgb) & 0xF8) + 4;
		int g = (G24(Rgb) & 0xF8) + 4;
		int b = (B24(Rgb) & 0xF8) + 4;
		ulong *squares = GdcD->GetCharSquares();
		ulong mindist = 200000;
		ulong bestcolor;
		ulong curdist;
		long rdist;
		long gdist;
		long bdist;

		for (int i = 0; i < Size; i++)
		{
			rdist = Entry[i].R - r;
			gdist = Entry[i].G - g;
			bdist = Entry[i].B - b;
			curdist = squares[rdist] + squares[gdist] + squares[bdist];

			if (curdist < mindist)
			{
				mindist = curdist;
				bestcolor = i;
			}
		}

		return bestcolor;
	}

	return 0;
}

void GPalette::CreateGreyScale()
{
	SetSize(256);
	GdcRGB *p = (*this)[0];

	for (int i=0; i<256; i++)
	{
		p->R = i;
		p->G = i;
		p->B = i;
		p->Flags = 0;
		p++;
	}
}

void GPalette::CreateCube()
{
	SetSize(256);
	GdcRGB *p = (*this)[0];

	for (int r=0; r<6; r++)
	{
		for (int g=0; g<6; g++)
		{
			for (int b=0; b<6; b++)
			{
				p->R = r * 51;
				p->G = g * 51;
				p->B = b * 51;
				p->Flags = 0;
				p++;
			}
		}
	}

	for (int i=216; i<256; i++)
	{
		(*this)[i]->R = 0;
		(*this)[i]->G = 0;
		(*this)[i]->B = 0;
	}
}


void TrimWhite(char *s)
{
	char *White = " \r\n\t";
	char *c = s;
	while (*c AND strchr(White, *c)) c++;
	if (c != s)
	{
		strcpy(s, c);
	}

	c = s + strlen(s) - 1;
	while (c > s AND strchr(White, *c))
	{
		*c = 0;
		c--;
	}
}

bool GPalette::Load(GFile &F)
{
	bool Status = false;
	char Buf[256];
	
	F.ReadStr(Buf, sizeof(Buf));
	TrimWhite(Buf);
	if (strcmp(Buf, "JASC-PAL") == 0)
	{
		// is JASC palette
		// skip hex length
		F.ReadStr(Buf, sizeof(Buf));

		// read decimal length
		F.ReadStr(Buf, sizeof(Buf));
		SetSize(atoi(Buf));
		for (int i=0; i<GetSize() AND !F.Eof(); i++)
		{
			F.ReadStr(Buf, sizeof(Buf));
			GdcRGB *p = (*this)[i];
			if (p)
			{
				p->R = atoi(strtok(Buf, " "));
				p->G = atoi(strtok(NULL, " "));
				p->B = atoi(strtok(NULL, " "));

			}

			Status = true;
		}
	}
	else
	{
		// check for microsoft format
	}

	return Status;
}

bool GPalette::Save(GFile &F, int Format)
{
	bool Status = false;

	switch (Format)
	{
		case GDCPAL_JASC:
		{
			char Buf[256];

			sprintf(Buf, "JASC-PAL\r\n%04.4X\r\n%i\r\n", GetSize(), GetSize());
			F.Write(Buf, strlen(Buf));

			for (int i=0; i<GetSize(); i++)
			{
				GdcRGB *p = (*this)[i];
				if (p)
				{
					sprintf(Buf, "%i %i %i\r\n", p->R, p->G, p->B);
					F.Write(Buf, strlen(Buf));
				}
			}

			Status = true;
			break;
		}
	}

	return Status;
}

bool GPalette::operator ==(GPalette &p)
{
	if (GetSize() == p.GetSize())
	{
		GdcRGB *a = (*this)[0];
		GdcRGB *b = p[0];

		for (int i=0; i<GetSize(); i++)
		{
			if (	a->R != b->R ||
				a->G != b->G ||
				a->B != b->B)
			{
				return false;
			}

			a++;
			b++;
		}

		return true;
	}

	return false;
}

bool GPalette::operator !=(GPalette &p)
{
	return !((*this) == p);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GBmpMem::GBmpMem()
{
	Base = 0;
	Flags = 0;
}

GBmpMem::~GBmpMem()
{
	if (Base AND (Flags & GDC_OWN_MEMORY))
	{
		delete [] Base;
	}
}



////////////////////////////////////////////////////////////////////////////////////////////
class GdcDevicePrivate
{
public:
	GdcDevice *Device;

	// Current mode info
	int ScrX;
	int ScrY;
	int ScrBits;

	// Palette
	double GammaCorrection;
	uchar GammaTable[256];
	GPalette *pSysPal;
	GGlobalColour *GlobalColour;

	// Data
	ulong *CharSquareData;
	uchar *Div255;
	// Options
	int OptVal[GDC_MAX_OPTION];

	// Alpha
	#if defined WIN32
	GLibrary MsImg;
	#endif

	GdcDevicePrivate(GdcDevice *d)
	#ifdef WIN32
	 : MsImg("msimg32")
	#endif
	{
		Device = d;
		GlobalColour = 0;
    	ZeroObj(OptVal);
    	ScrX = ScrY = 0;
    	ScrBits = 0;

		// Palette information
		GammaCorrection = 1.0;

		#if WIN32NATIVE
		// Get mode stuff
		HWND hDesktop = GetDesktopWindow();
		HDC hScreenDC = GetDC(hDesktop);

		ScrX = GetSystemMetrics(SM_CXSCREEN);
		ScrY = GetSystemMetrics(SM_CYSCREEN);
		ScrBits = GetDeviceCaps(hScreenDC, BITSPIXEL);

		int Colours = 1 << ScrBits;
		pSysPal = (ScrBits <= 8) ? new GPalette(0, Colours) : 0;
		if (pSysPal)
		{
			GdcRGB *p = (*pSysPal)[0];
			if (p)
			{
				PALETTEENTRY pal[256];
				GetSystemPaletteEntries(hScreenDC, 0, Colours, pal);
				for (int i=0; i<Colours; i++)
				{
					p[i].R = pal[i].peRed;
					p[i].G = pal[i].peGreen;
					p[i].B = pal[i].peBlue;
				}
			}
		}

		ReleaseDC(hDesktop, hScreenDC);

		// Options
		// OptVal[GDC_REDUCE_TYPE] = REDUCE_ERROR_DIFFUSION;
		// OptVal[GDC_REDUCE_TYPE] = REDUCE_HALFTONE;
		OptVal[GDC_REDUCE_TYPE] = REDUCE_NEAREST;
		OptVal[GDC_HALFTONE_BASE_INDEX] = 0;
		
		#elif defined __GTK_H__
		
		// Get mode stuff
		Gtk::GdkDisplay *Dsp = Gtk::gdk_display_get_default();
		Gtk::gint Screens = Gtk::gdk_display_get_n_screens(Dsp);
		for (Gtk::gint i=0; i<Screens; i++)
		{
            Gtk::GdkScreen *Scr = Gtk::gdk_display_get_screen(Dsp, i);
            if (Scr)
            {
		        ScrX = Gtk::gdk_screen_get_width(Scr);
		        ScrY = Gtk::gdk_screen_get_height(Scr);
		        Gtk::GdkVisual *Vis = Gtk::gdk_screen_get_system_visual(Scr);
		        if (Vis)
		            ScrBits = Vis->depth;
		    }
        }
		
		printf("Screen: %i x %i @ %i bpp\n", ScrX, ScrY, ScrBits);

		int Colours = 1 << ScrBits;
		pSysPal = (ScrBits <= 8) ? new GPalette(0, Colours) : 0;
		if (pSysPal)
		{
		}

		// Work out the size of a 24bit pixel
		char *Data =(char*) malloc(4);
		
		#if 0
		XImage *Img = XCreateImage(XDisplay(), DefaultVisual(XDisplay(), 0), 24, ZPixmap, 0, Data, 1, 1, 32, 4);
		if (Img)
		{
			Pixel24::Size = Img->bits_per_pixel >> 3;
			XDestroyImage(Img);
		}
		#else
		printf("%s:%i - FIXME, get the size of 24bit pixel here.\n", _FL);
		#endif
		
		// printf("Pixel24Size=%i\n", Pixel24Size);
		OptVal[GDC_PROMOTE_ON_LOAD] = ScrBits;
		#endif

		// Calcuate lookups
		CharSquareData = new ulong[255+255+1];
		if (CharSquareData)
		{
			for (int i = -255; i <= 255; i++)
			{
				CharSquareData[i+255] = i*i;
			}
		}

		// Divide by 255 lookup, real handy for alpha blending 8 bit components
		int Size = (255 * 255) * 2;
		Div255 = new uchar[Size];
		if (Div255)
		{
			for (int i=0; i<Size; i++)
			{
				int n = (i + 128) / 255;
				Div255[i] = min(n, 255);
			}
		}

	}

	~GdcDevicePrivate()
	{
		DeleteObj(GlobalColour);
		DeleteArray(CharSquareData);
		DeleteArray(Div255);
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////
int GColourSpaceToBits(GColourSpace ColourSpace)
{
	uint32 c = ColourSpace;
	int bits = 0;
	while (c)
	{
		if (c & 0xf0)
		{
			int n = c & 0xf;
			bits += n ? n : 16;
		}
		c >>= 8;
	}
	return bits;
}

GColourSpace GBitsToColourSpace(int Bits)
{
	switch (Bits)
	{
		case 8:
			return CsIndex8;
		case 15:
			return CsRgb15;
		case 16:
			return CsRgb16;
		case 24:
			return CsBgr24;
		case 32:
			return CsBgra32;
		default:
			LgiAssert(!"Unknown colour space.");
			break;
	}
	
	return CsNone;
}

//////////////////////////////////////////////////////////////
GdcDevice *GdcDevice::pInstance = 0;
GdcDevice::GdcDevice()
{
	d = new GdcDevicePrivate(this);
	pInstance = this;
}

GdcDevice::~GdcDevice()
{
	// delete stuff
	pInstance = NULL;
	DeleteObj(d);
}

int GdcDevice::GetOption(int Opt)
{
	if (Opt >= 0 AND Opt < GDC_MAX_OPTION)
	{
		return d->OptVal[Opt];
	}

	LgiAssert(0);
	return 0;
}

int GdcDevice::SetOption(int Opt, int Value)
{
	int Prev = d->OptVal[Opt];
	if (Opt >= 0 AND Opt < GDC_MAX_OPTION)
	{
		d->OptVal[Opt] = Value;
	}
	else
	{
		LgiAssert(0);
	}
	return Prev;
}

ulong *GdcDevice::GetCharSquares()
{
	return (d->CharSquareData) ? d->CharSquareData + 255 : 0;
}

uchar *GdcDevice::GetDiv255()
{
	return d->Div255;
}

GGlobalColour *GdcDevice::GetGlobalColour()
{
	return d->GlobalColour;
}

int GdcDevice::GetBits()
{
	return d->ScrBits;
}

int GdcDevice::X()
{
	return d->ScrX;
}

int GdcDevice::Y()
{
	return d->ScrY;
}

void GdcDevice::SetGamma(double Gamma)
{
	d->GammaCorrection = Gamma;
	for (int i=0; i<256; i++)
	{
		d->GammaTable[i] = (uchar) (pow(((double)i)/256, Gamma) * 256);
	}
}

double GdcDevice::GetGamma()
{
	return d->GammaCorrection;
}

void GdcDevice::SetSystemPalette(int Start, int Size, GPalette *pPal)
{
	/*
	if (pPal)
	{
		uchar Pal[768];
		uchar *Temp = Pal;
		uchar *System = Palette + (Start * 3);
		GdcRGB *P = (*pPal)[Start];
		
		for (int i=0; i<Size; i++, P++)
		{
			*Temp++ = GammaTable[*System++ = P->R] >> PalShift;
			*Temp++ = GammaTable[*System++ = P->G] >> PalShift;
			*Temp++ = GammaTable[*System++ = P->B] >> PalShift;
		}
		
		SetPaletteBlockDirect(Pal, Start, Size * 3);
	}
	*/
}

GPalette *GdcDevice::GetSystemPalette()
{
	return d->pSysPal;
}

void GdcDevice::SetColourPaletteType(int Type)
{
	bool SetOpt = true;

	/*
	switch (Type)
	{
		case PALTYPE_ALLOC:
		{
			SetPalIndex(0, 0, 0, 0);
			SetPalIndex(255, 255, 255, 255);
			break;
		}
		case PALTYPE_RGB_CUBE:
		{
			uchar Pal[648];
			uchar *p = Pal;

			for (int r=0; r<6; r++)
			{
				for (int g=0; g<6; g++)
				{
					for (int b=0; b<6; b++)
					{
						*p++ = r * 51;
						*p++ = g * 51;
						*p++ = b * 51;
					}
				}
			}

			SetPalBlock(0, 216, Pal);
			SetPalIndex(255, 0xFF, 0xFF, 0xFF);
			break;
		}
		case PALTYPE_HSL:
		{
			for (int h = 0; h<16; h++)
			{
			}
			break;
		}
		default:
		{
			SetOpt = false;
		}
	}

	if (SetOpt)
	{
		SetOption(GDC_PALETTE_TYPE, Type);
	}
	*/
}

COLOUR GdcDevice::GetColour(COLOUR Rgb24, GSurface *pDC)
{
	int Bits = (pDC) ? pDC->GetBits() : GetBits();
	COLOUR C;

	switch (Bits)
	{
		case 8:
		{
			switch (GetOption(GDC_PALETTE_TYPE))
			{
				case PALTYPE_ALLOC:
				{
					static uchar Current = 1;

					Rgb24 &= 0xFFFFFF;
					if (Rgb24 == 0xFFFFFF)
					{
						C = 0xFF;
					}
					else if (Rgb24 == 0)
					{
						C = 0;
					}
					else
					{
						(*d->pSysPal)[Current]->Set(R24(Rgb24), G24(Rgb24), B24(Rgb24));
						C = Current++;
						if (Current == 255) Current = 1;
					}
					break;
				}
				case PALTYPE_RGB_CUBE:
				{
					uchar r = (R24(Rgb24) + 25) / 51;
					uchar g = (G24(Rgb24) + 25) / 51;
					uchar b = (B24(Rgb24) + 25) / 51;
					
					C = (r*36) + (g*6) + b;
					break;
				}
				case PALTYPE_HSL:
				{
					C = 0;
					break;
				}
			}
			break;
		}
		case 16:
		{
			C = Rgb24To16(Rgb24);
			break;
		}
		case 24:
		{
			C = Rgb24;
			break;
		}
		case 32:
		{
			C = Rgb24To32(Rgb24);
			break;
		}
	}

	return C;
}

/*
GSprite::GSprite()
{
	Sx = Sy = Bits = 0;
	PosX = PosY = 0;
	HotX = HotY = 0;
	Visible = false;
	pScreen = pBack = pMask = pColour = pTemp = 0;
}

GSprite::~GSprite()
{
	Delete();
}

void GSprite::SetXY(int x, int y)
{
	PosX = x;
	PosY = y;
}

bool GSprite::Create(GSurface *pScr, int x, int y, int SrcBitDepth, uchar *Colour, uchar *Mask)
{
	bool Status = false;

	pScreen = pScr;
	if (pScreen AND Colour AND Mask)
	{
		Delete();

		if (SetSize(x, y, pScreen->GetBits()))
		{
			COLOUR cBlack = 0;
			COLOUR cWhite = 0xFFFFFFFF;

			switch (SrcBitDepth)
			{
				case 1:
				{
					int Scan = (Sx + 7) / 8;

					for (int y=0; y<Sy; y++)
					{
						uchar *c = Colour + (y * Scan);
						uchar *m = Mask + (y * Scan);

						for (int x=0; x<Sx; x++)
						{
							int Div = x >> 3;
							int Mod = x & 0x7;

							pColour->Colour((c[Div] & (0x80 >> Mod)) ? cWhite : cBlack);
							pColour->Set(x, y);

							pMask->Colour((m[Div] & (0x80 >> Mod)) ? cWhite : cBlack);
							pMask->Set(x, y);
						}
					}

					Status = true;
					break;
				}
			}
		}

		if (!Status)
		{
			Delete();
		}
	}

	return Status;
}

bool GSprite::Create(GSurface *pScr, GSprite *pSpr)
{
	bool Status = false;

	pScreen = pSpr->pScreen;
	if (pScreen)
	{
		Delete();

		if (SetSize(pSpr->X(), pSpr->Y(), pSpr->GetBits()))
		{
			HotX = pSpr->GetHotX();
			HotY = pSpr->GetHotY();

			pBack->Blt(0, 0, pSpr->pBack, NULL);
			pMask->Blt(0, 0, pSpr->pMask, NULL);
			pColour->Blt(0, 0, pSpr->pColour, NULL);

			Status = true;
		}

		if (!Status)
		{
			Delete();
		}
	}

	return Status;
}

bool GSprite::CreateKey(GSurface *pScr, int x, int y, int Bits, uchar *Colour, int Key)
{
	bool Status = false;

	pScreen = pScr;
	if (pScreen AND Colour)
	{
		Delete();

		if (SetSize(x, y, Bits))
		{
			COLOUR cBlack = 0;
			COLOUR cWhite = 0xFFFFFFFF;

			switch (Bits)
			{
				case 8:
				{
					for (int y=0; y<Sy; y++)
					{
						uchar *c = Colour + (y * Sx);

						for (int x=0; x<Sx; x++, c++)
						{
							pColour->Colour(*c);
							pColour->Set(x, y);

							pMask->Colour((*c != Key) ? cWhite : cBlack);
							pMask->Set(x, y);
						}
					}

					Status = true;
					break;
				}
				case 16:
				case 15:
				{
					for (int y=0; y<Sy; y++)
					{
						ushort *c = ((ushort*) Colour) + (y * Sx);

						for (int x=0; x<Sx; x++, c++)
						{
							pColour->Colour(*c);
							pColour->Set(x, y);

							pMask->Colour((*c != Key) ? cWhite : cBlack);
							pMask->Set(x, y);
						}
					}

					Status = true;
					break;
				}
				case 24:
				{
					for (int y=0; y<Sy; y++)
					{
						uchar *c = Colour + (y * Sx * 3);

						for (int x=0; x<Sx; x++, c+=3)
						{
							COLOUR p = Rgb24(c[2], c[1], c[0]);
							pColour->Colour(p);
							pColour->Set(x, y);

							pMask->Colour((p != Key) ? cWhite : cBlack);
							pMask->Set(x, y);
						}
					}

					Status = true;
					break;
				}
				case 32:
				{
					for (int y=0; y<Sy; y++)
					{
						ulong *c = ((ulong*) Colour) + (y * Sx);

						for (int x=0; x<Sx; x++, c++)
						{
							pColour->Colour(*c);
							pColour->Set(x, y);

							pMask->Colour((*c != Key) ? cWhite : cBlack);
							pMask->Set(x, y);
						}
					}

					Status = true;
					break;
				}
			}
		}

		if (!Status)
		{
			Delete();
		}
	}

	return Status;
}

bool GSprite::SetSize(int x, int y, int BitSize, GSurface *pScr)
{
	bool Status = false;
	if (pScr)
	{
		pScreen = pScr;
	}

	if (pScreen)
	{
		Sx = x;
		Sy = y;
		Bits = BitSize;
		HotX = 0;
		HotY = 0;
		DrawMode = 0;

		pBack = new GMemDC;
		pMask = new GMemDC;
		pColour = new GMemDC;
		pTemp = new GMemDC;

		if (pBack AND pMask AND pColour AND pTemp)
		{
			if (	pBack->Create(x, y, Bits) AND
				pMask->Create(x, y, Bits) AND
				pColour->Create(x, y, Bits) AND
				pTemp->Create(x<<1, y<<1, Bits))
			{
				if (pScreen->GetBits() == 8)
				{
					pTemp->Palette(new GPalette(pScreen->Palette()));
				}

				Status = true;
			}
		}
	}

	return Status;
};

void GSprite::Delete()
{
	SetVisible(false);
	DeleteObj(pBack);
	DeleteObj(pMask);
	DeleteObj(pColour);
	DeleteObj(pTemp);
}

void GSprite::SetHotPoint(int x, int y)
{
	HotX = x;
	HotY = y;
}

void GSprite::SetVisible(bool v)
{
	if (pScreen)
	{
		if (Visible)
		{
			if (!v)
			{
				// Hide
				int Op = pScreen->Op(GDC_SET);
				pScreen->Blt(PosX-HotX, PosY-HotY, pBack, NULL);
				pScreen->Op(Op);
				Visible = false;
			}
		}
		else
		{
			if (v)
			{
				// Show
				GRect s, n;

				s.ZOff(Sx-1, Sy-1);
				n = s;
				s.Offset(PosX-HotX, PosY-HotY);
				pBack->Blt(0, 0, pScreen, &s);
				
				pTemp->Op(GDC_SET);
				pTemp->Blt(0, 0, pBack);

				if (DrawMode == 3)
				{
					int Op = pTemp->Op(GDC_ALPHA);
					pTemp->Blt(0, 0, pColour, NULL);
					pTemp->Op(Op);
				}
				else
				{
					int OldOp = pTemp->Op(GDC_AND);
					pTemp->Blt(0, 0, pMask, NULL);
					if (DrawMode == 1)
					{
						pTemp->Op(GDC_OR);
					}
					else
					{
						pTemp->Op(GDC_XOR);
					}
					pTemp->Blt(0, 0, pColour, NULL);
					pTemp->Op(OldOp);
				}

				int OldOp = pScreen->Op(GDC_SET);
				pScreen->Blt(PosX-HotX, PosY-HotY, pTemp, &n);
				pScreen->Op(OldOp);
				Visible = true;
			}
		}
	}
}

void GSprite::Draw(int x, int y, COLOUR Back, int Mode, GSurface *pDC)
{
	if (pScreen)
	{
		if (!pDC)
		{
			pDC = pScreen;
		}

		if (Mode == 3)
		{
			pDC->Blt(x-HotX, y-HotY, pColour, NULL);
		}
		else if (Mode == 2)
		{
			int Op = pScreen->Op(GDC_AND);
			pDC->Blt(x-HotX, y-HotY, pMask, NULL);
			pDC->Op(GDC_OR);
			pDC->Blt(x-HotX, y-HotY, pColour, NULL);
			pDC->Op(Op);
		}
		else
		{
			pBack->Colour(Back);
			pBack->Rectangle(0, 0, pBack->X(), pBack->Y());
			
			if (Mode == 0)
			{
				pBack->Op(GDC_AND);
				pBack->Blt(0, 0, pColour, NULL);
				pBack->Op(GDC_XOR);
				pBack->Blt(0, 0, pMask, NULL);
			}
			else if (Mode == 1)
			{
				pBack->Op(GDC_AND);
				pBack->Blt(0, 0, pMask, NULL);
				pBack->Op(GDC_OR);
				pBack->Blt(0, 0, pColour, NULL);
			}

			pDC->Blt(x-HotX, y-HotY, pBack, NULL);
		}
	}
}

void GSprite::Move(int x, int y, bool WaitRetrace)
{
	if (	pScreen AND
		(x != PosX ||
		y != PosY))
	{
		if (Visible)
		{
			GRect New, Old;
			
			New.ZOff(Sx-1, Sy-1);
			Old = New;
			New.Offset(x - HotX, y - HotY);
			Old.Offset(PosX - HotX, PosY - HotY);
			
			if (New.Overlap(&Old))
			{
				GRect Both;
				
				Both.Union(&Old, &New);
				
				if (pTemp)
				{
					// Get working area of screen
					pTemp->Blt(0, 0, pScreen, &Both);

					// Restore the back of the sprite
					pTemp->Blt(Old.x1-Both.x1, Old.y1-Both.y1, pBack, NULL);

					// Update any dependent graphics
					DrawOnMove(pTemp, x, y, HotX-Both.x1, HotX-Both.y1);

					// Get the new background
					GRect Bk = New;
					Bk.Offset(-Both.x1, -Both.y1);
					pBack->Blt(0, 0, pTemp, &Bk);
		
					// Paint the new sprite
					int NewX = New.x1 - Both.x1;
					int NewY = New.y1 - Both.y1;

					if (DrawMode == 3)
					{
						int Op = pTemp->Op(GDC_ALPHA);
						pTemp->Blt(NewX, NewY, pColour, NULL);
						pTemp->Op(Op);
					}
					else
					{
						int OldOp = pTemp->Op(GDC_AND);
						pTemp->Blt(NewX, NewY, pMask, NULL);

						if (DrawMode == 1)
						{
							pTemp->Op(GDC_OR);
						}
						else
						{
							pTemp->Op(GDC_XOR);
						}

						pTemp->Blt(NewX, NewY, pColour, NULL);
						pTemp->Op(OldOp);
					}
		
					// Update all under sprite info on screen
					DrawOnMove(pScreen, x, y, 0, 0);

					// Put it all back on the screen
					GRect b = Both;
					b.Offset(-b.x1, -b.y1);

					int OldOp = pScreen->Op(GDC_SET);
					pScreen->Blt(Both.x1, Both.y1, pTemp, &b);
					pScreen->Op(OldOp);

					PosX = x;
					PosY = y;
				}
			}
			else
			{
				SetVisible(false);

				DrawOnMove(pScreen, x, y, 0, 0);
				PosX = x;
				PosY = y;

				SetVisible(true);
			}
		}
		else
		{
			PosX = x;
			PosY = y;
		}
	}
}
*/

//////////////////////////////////////////////////////////////////////////////////////////////
static int _Factories;
static GApplicatorFactory *_Factory[16];

GApp8 Factory8;
GApp15 Factory15;
GApp16 Factory16;
GApp24 Factory24;
GApp32 Factory32;
GAlphaFactory FactoryAlpha;

GApplicatorFactory::GApplicatorFactory()
{
	LgiAssert(_Factories >= 0 AND _Factories < CountOf(_Factory));
	if (_Factories < CountOf(_Factory) - 1)
	{
		_Factory[_Factories++] = this;
	}
}

GApplicatorFactory::~GApplicatorFactory()
{
	LgiAssert(_Factories >= 0 AND _Factories < CountOf(_Factory));
	for (int i=0; i<_Factories; i++)
	{
		if (_Factory[i] == this)
		{
			_Factory[i] = _Factory[_Factories-1];
			_Factories--;
			break;
		}
	}
}

GApplicator *GApplicatorFactory::NewApp(GColourSpace Cs, int Op)
{
	LgiAssert(_Factories >= 0 && _Factories < CountOf(_Factory));
	for (int i=0; i<_Factories; i++)
	{
		GApplicator *a = _Factory[i]->Create(Cs, Op);
		if (a) return a;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
class GlobalColourEntry
{
public:
	COLOUR c24;
	bool Fixed;
	bool Used;

	GlobalColourEntry()
	{
		c24 = 0;
		Fixed = false;
		Used = false;
	}
};

class GGlobalColourPrivate
{
public:
	GlobalColourEntry c[256];
	GPalette *Global;
	List<GSurface> Cache;
	int FirstUnused;
	
	int FreeColours()
	{
		int f = 0;
		
		for (int i=0; i<256; i++)
		{
			if (!c[i].Used)
			{
				f++;
			}
		}

		return f;
	}

	GGlobalColourPrivate()
	{
	}

	~GGlobalColourPrivate()
	{
		Cache.DeleteObjects();
	}
};

GGlobalColour::GGlobalColour()
{
	d = new GGlobalColourPrivate;
}

GGlobalColour::~GGlobalColour()
{
	DeleteObj(d);
}

COLOUR GGlobalColour::AddColour(COLOUR c24)
{
	return c24;
}

bool GGlobalColour::AddBitmap(GSurface *pDC)
{
	return false;
}

bool GGlobalColour::AddBitmap(GImageList *il)
{
	return false;
}

bool GGlobalColour::MakeGlobalPalette()
{
	return 0;
}

GPalette *GGlobalColour::GetPalette()
{
	return 0;
}

COLOUR GGlobalColour::GetColour(COLOUR c24)
{
	return c24;
}

bool GGlobalColour::RemapBitmap(GSurface *pDC)
{
	return false;
}

////////////////////////////////////////////////////////////////////////
GSurface *GInlineBmp::Create()
{
	GSurface *pDC = new GMemDC;
	if (pDC->Create(X, Y, 32))
	{
		int Line = X * Bits / 8;
		for (int y=0; y<Y; y++)
		{
			void *addr = ((uchar*)Data) + (y * Line);
			switch (Bits)
			{
				case 16:
				{
					uint32 *out = (uint32*)(*pDC)[y];
					uint16 *in = (uint16*)addr;
					uint16 *end = in + X;
					while (in < end)
					{
						*out = Rgb16To32(*in);						
						in++;
						out++;
					}
					break;
				}
				case 24:
				{
					System32BitPixel *out = (System32BitPixel*)(*pDC)[y];
					System32BitPixel *end = out + X;
					System24BitPixel *in = (System24BitPixel*)addr;
					while (out < end)
					{
						out->r = in->r;
						out->g = in->g;
						out->b = in->b;
						out->a = 255;
						out++;
						in++;
					}
					break;
				}
				case 32:
				{
					for (int y=0; y<Y; y++)
						memcpy((*pDC)[y], addr, Line);
					break;
				}
				default:
				{
					LgiAssert(!"Not a valid bit depth.");
					break;
				}
			}
		}
	}

	return pDC;
}

