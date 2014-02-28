/**
	\file
	\author Matthew Allen
	\date 24/2/2014
	\brief 64 bit primitives (RGBA 16 bit)
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "Gdc2.h"

#define BytePtr	((uint8*&)Ptr)
#undef NonPreMulOver64
#define NonPreMulOver64(c)	d->c = ((s->c * sa) + ((d->c * 0xffff) / 0xffff * o)) / 0xffff

template<typename Pixel, GColourSpace ColourSpace>
class App64 : public GApplicator
{
	union
	{
		uint8 *u8;
		Pixel *p;
	};
	
	int ConstAlpha;
	GPalette *PalAlpha;

public:
	App64()
	{
		p = NULL;
		ConstAlpha = 0xffff;
		PalAlpha = NULL;
	}

	int GetVar(int Var)
	{
		switch (Var)
		{
			case GAPP_ALPHA_A:
			{
				return ConstAlpha;
				break;
			}
			default:
			{
				LgiAssert(!"impl me.");
				break;
			}
		}
		return 0;
	}
	
	int SetVar(int Var, NativeInt Value)
	{
		switch (Var)
		{
			case GAPP_ALPHA_A:
			{
				ConstAlpha = Value;
				break;
			}
			case GAPP_ALPHA_PAL:
			{
				PalAlpha = (GPalette*)Value;
				break;
			}
			default:
			{
				LgiAssert(!"impl me.");
				break;
			}
		}
		return 0;
	}

	bool SetSurface(GBmpMem *d, GPalette *pal = NULL, GBmpMem *a = NULL)
	{
		if (d && d->Cs == ColourSpace)
		{
			Dest = d;
			Pal = pal;
			p = (Pixel*) d->Base;
			Alpha = 0;
			return true;
		}
		return false;
	}

	void SetPtr(int x, int y)
	{
		p = (Pixel*) (Dest->Base + (y * Dest->Line) + (x * sizeof(Pixel)));
	}
	
	void IncX()
	{
		p++;
	}
	
	void IncY()
	{
		u8 += Dest->Line;
	}
	
	void IncPtr(int X, int Y)
	{
		p += X;
		u8 += Y * Dest->Line;
	}
	
	void Set()
	{
		p->r = ((uint16)p24.r << 8) | p24.r;
		p->g = ((uint16)p24.g << 8) | p24.g;
		p->b = ((uint16)p24.b << 8) | p24.b;
	}
	
	COLOUR Get()
	{
		return Rgb24(p->r >> 8, p->g >> 8, p->b >> 8);
	}
	
	void VLine(int height)
	{
		Pixel cp;
		cp.r = ((uint16)p24.r << 8) | p24.r;
		cp.g = ((uint16)p24.g << 8) | p24.g;
		cp.b = ((uint16)p24.b << 8) | p24.b;
		
		while (height-- > 0)
		{
			*p = cp;
			u8 += Dest->Line;
		}
	}
	
	void Rectangle(int x, int y)
	{
		Pixel cp;
		cp.r = ((uint16)p24.r << 8) | p24.r;
		cp.g = ((uint16)p24.g << 8) | p24.g;
		cp.b = ((uint16)p24.b << 8) | p24.b;
		
		while (y-- > 0)
		{
			Pixel *i = p, *e = i + x;
			while (i < e)
			{
				*i++ = cp;
			}
			u8 += Dest->Line;
		}
	}
	
	template<typename T>
	bool CopyBltNoAlphaUpScale(GBmpMem *Src)
	{
		for (int y=0; y<Src->y; y++)
		{
			Pixel *d = p;
			T *s = (T*) (Src->Base + (y * Src->Line));
			T *e = s + Src->x;

			while (s < e)
			{
				d->r = G8bitTo16Bit(s->r);
				d->g = G8bitTo16Bit(s->g);
				d->b = G8bitTo16Bit(s->b);
				d->a = 0xffff;
				s++;
				d++;
			}

			u8 += Dest->Line;
		}
		
		return true;
	}
	
	template<typename T>
	bool CopyBltWithAlphaUpScale(GBmpMem *Src)
	{
		for (int y=0; y<Src->y; y++)
		{
			Pixel *d = p;
			T *s = (T*) (Src->Base + (y * Src->Line));
			T *e = s + Src->x;

			while (s < e)
			{
				d->r = G8bitTo16Bit(s->r);
				d->g = G8bitTo16Bit(s->g);
				d->b = G8bitTo16Bit(s->b);
				d->a = G8bitTo16Bit(s->a);
				s++;
				d++;
			}

			u8 += Dest->Line;
		}
		
		return true;
	}
	
	template<typename T>
	bool CopyBltNoAlpha(GBmpMem *Src)
	{
		for (int y=0; y<Src->y; y++)
		{
			Pixel *d = p;
			T *s = (T*) (Src->Base + (y * Src->Line));
			T *e = s + Src->x;

			while (s < e)
			{
				d->r = s->r;
				d->g = s->g;
				d->b = s->b;
				d->a = 0xffff;
				s++;
				d++;
			}

			u8 += Dest->Line;
		}
		
		return true;
	}
	
	template<typename T>
	bool CopyBltWithAlpha(GBmpMem *Src)
	{
		for (int y=0; y<Src->y; y++)
		{
			Pixel *d = p;
			T *s = (T*) (Src->Base + (y * Src->Line));
			T *e = s + Src->x;

			while (s < e)
			{
				d->r = s->r;
				d->g = s->g;
				d->b = s->b;
				d->a = s->a;
				s++;
				d++;
			}

			u8 += Dest->Line;
		}
		
		return true;
	}
	
	template<typename T>
	bool AlphaBlt(GBmpMem *Src, GBmpMem *SrcAlpha)
	{
		uchar *DivLut = Div255Lut;

		for (int y=0; y<Src->y; y++)
		{
			Pixel *d = p;
			T *s = (T*) (Src->Base + (y * Src->Line));
			T *e = s + Src->x;
			uint16 *a = (uint16*) (Src->Base + (y * SrcAlpha->Line));

			while (s < e)
			{
				uint16 sa = *a++;
				if (sa == 255)
				{
					d->r = s->r;
					d->g = s->g;
					d->b = s->b;
				}
				else if (sa > 0)
				{
					uint16 o = 0xffff - sa;
					d->r = ((s->r * sa) + (d->r * o)) / 0xffff;
					d->g = ((s->g * sa) + (d->g * o)) / 0xffff;
					d->b = ((s->b * sa) + (d->b * o)) / 0xffff;
				}
				
				s++;
				d++;
			}

			u8 += Dest->Line;
		}
		
		return true;
	}
	
	bool Blt(GBmpMem *Src, GPalette *SPal, GBmpMem *SrcAlpha = NULL)
	{
		if (!Src)
			return false;
		
		if (!SrcAlpha)
		{
			if (Dest->Cs == Src->Cs)
			{
				uchar *s = Src->Base;
				for (int y=0; y<Src->y; y++)
				{
					MemCpy(p, s, Src->x * 3);
					s += Src->Line;
					u8 += Dest->Line;
				}
			}
			else
			{
				switch (Src->Cs)
				{
					#define CopyCaseNoAlpha(name) \
						case Cs##name: return CopyBltNoAlphaUpScale<G##name>(Src);
					#define CopyCaseWithAlpha(name) \
						case Cs##name: return CopyBltWithAlphaUpScale<G##name>(Src);

					CopyCaseNoAlpha(Rgb24);
					CopyCaseNoAlpha(Bgr24);
					CopyCaseNoAlpha(Xrgb32);
					CopyCaseNoAlpha(Xbgr32);
					CopyCaseNoAlpha(Rgbx32);
					CopyCaseNoAlpha(Bgrx32);

					CopyCaseWithAlpha(Argb32);
					CopyCaseWithAlpha(Abgr32);
					CopyCaseWithAlpha(Rgba32);
					CopyCaseWithAlpha(Bgra32);

					case CsRgb48:
						return CopyBltNoAlpha<GRgb48>(Src);
					case CsBgr48:
						return CopyBltNoAlpha<GBgr48>(Src);

					case CsRgba64:
						return CopyBltWithAlpha<GRgba64>(Src);
					case CsBgra64:
						return CopyBltWithAlpha<GBgra64>(Src);
					case CsArgb64:
						return CopyBltWithAlpha<GArgb64>(Src);
					case CsAbgr64:
						return CopyBltWithAlpha<GAbgr64>(Src);

					default:
						LgiAssert(!"Impl me.");
						break;
				}
			}
		}
		else
		{
			switch (Src->Cs)
			{
				#define AlphaCase(name) \
					case Cs##name: return AlphaBlt<G##name>(Src, SrcAlpha);

				AlphaCase(Rgb24);
				AlphaCase(Bgr24);
				AlphaCase(Xrgb32);
				AlphaCase(Xbgr32);
				AlphaCase(Rgbx32);
				AlphaCase(Bgrx32);

				AlphaCase(Argb32);
				AlphaCase(Abgr32);
				AlphaCase(Rgba32);
				AlphaCase(Bgra32);

				default:
					LgiAssert(!"Impl me.");
					break;
			}
		}
		
		return false;
	}
};

class G64BitFactory : public GApplicatorFactory
{
public:
	GApplicator *Create(GColourSpace Cs, int Op)
	{
		switch (Cs)
		{
			#define Case64(name) \
				case Cs##name: \
					return new App64<G##name, Cs##name>();
			
			Case64(Rgba64);
			Case64(Bgra64);
			Case64(Argb64);
			Case64(Abgr64);
			default:
				break;
		}
		return 0;
	}
} App48Factory;