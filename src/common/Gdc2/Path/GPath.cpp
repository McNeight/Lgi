#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "Lgi.h"
#include "GPath.h"
#include "GdiLeak.h"
#include "GDisplayString.h"

#define CIRCLE_STEPS			24
#define BEZIER_STEPS			24

#if 0

	// 8x8 sub pixel anti-aliasing (really accurate/really slow)
	#define SUB_SAMPLE			8
	#define SUB_SHIFT			3
	#define SUB_MASK			7

#elif 1

	// 4x4 sub pixel anti-aliasing (best quality/performance tradeoff)
	#define SUB_SAMPLE			4
	#define SUB_SHIFT			2
	#define SUB_MASK			3

#elif 0

	// 2x2 sub pixel anti-aliasing (fast and nasty)
	#define SUB_SAMPLE			2
	#define SUB_SHIFT			1
	#define SUB_MASK			1

#endif

#define DEBUG_DRAW_VECTORS		0
#define DEBUG_DRAW_SEGMENTS		0
#define DEBUG_ODD_EDGES			0

// #define DEBUG_LOG			Log.Print
#ifndef LGI_SKIN
	// #define DEBUG_LOG		Log.Print
#endif
#ifdef DEBUG_LOG
	GFile Log;
#endif

#ifdef _MSC_VER
#ifdef _DEBUG
	#ifdef DEBUG_LOG
		#define PathAssert(b)			if (!(b)) { Log.Close(); \
											_asm int 3 \
											}
	#else
		#ifdef _WIN64
            #define PathAssert(b)			LgiAssert(b)
        #else
            #define PathAssert(b)			if (!(b)) _asm int 3
        #endif
	#endif
#else
	#define PathAssert(b)
#endif
#else
	#define PathAssert(b)
#endif


#define DEBUG_SCALE				1.0

#define FixedToDbl(i)			( ( (double) ((long*)&i)[0]) / 65536)
#define ToInt(d)				((int)( (d+(0.5/SUB_SAMPLE)+0.00000001) * SUB_SAMPLE))
#define ToDbl(i)				((double)i / SUB_SAMPLE)

double GPointF::Threshold		= 0.00000001;

bool _Disable_ActiveList		= false;
bool _Disable_XSort				= false;
bool _Disable_Alpha				= false;
bool _Disable_Rops				= false;

///////////////////////////////////////////////////////////
class AAEdge
{
public:
	double x[SUB_SAMPLE];
	int Min, Max;
};

class GVector
{
public:
	bool Up;
	int Points;
	GPointF *p;
	GRectF Bounds;
	int Index;

	int y1, y2;
	double *x;
	double Cx;

	int aay1, aay2;
	AAEdge *aax;

	GVector()
	{
		Index = -1;
		x = 0;
		aax = 0;
		aay1 = aay2 = y1 = y2 = -1;
	}

	~GVector()
	{
		DeleteArray(x);
		DeleteArray(aax);
	}

	bool Rasterize(bool aa)
	{
		Bounds = p[0];
		for (int n=1; n<Points; n++)
		{
			double y = p[n].y;
			Bounds.y1 = MIN(Bounds.y1, y);
			Bounds.y2 = MAX(Bounds.y2, y);
		}

		if (aa)
		{
			y1 = (int)floor(Bounds.y1);
			y2 = (int)ceil(Bounds.y2);
			int n = y2 - y1 + 10;

			aax = new AAEdge[n];
			if (aax)
			{
				memset(aax, 0, sizeof(*aax) * n);

				int Inc = (Up) ? -1 : 1;
				GPointF *a = p + (Up ? Points-1 : 0);
				GPointF *b = a + Inc;

				aay1 = ToInt(Bounds.y1);
				aay2 = ToInt(Bounds.y2);

				int Cur = aay1;
				for (int i=0; i<Points-1; i++)
				{
					int End = ToInt(b->y);
					if (End > Cur)
					{
						PathAssert(End >= Cur);

						double k = b->x - a->x;
						double Rx;
						if (k == 0.0)
						{
							// vertical line
							for (int s=Cur; s<=End; s++)
							{
								int Yi = (s>>SUB_SHIFT) - y1;

								PathAssert(Yi < n);
								aax[Yi].x[s&SUB_MASK] = Rx = a->x;
								Bounds.x1 = MIN(Bounds.x1, Rx);
								Bounds.x2 = MAX(Bounds.x2, Rx);
								/*
								PathAssert(	aax[Yi].x[s&SUB_MASK] >= Bounds.x1 - GPointF::Threshold &&
											aax[Yi].x[s&SUB_MASK] <= Bounds.x2 + GPointF::Threshold);
								*/
							}
						}
						else
						{
							 // angled line
							double Lm = (b->y - a->y) / k;
							double Lb = b->y - (Lm * b->x);
							double Ry;
							
							for (int s=Cur; s<=End; s++)
							{
								int Yi = (s>>SUB_SHIFT) - y1;

								PathAssert(Yi < n);

								Ry = ToDbl(s);
								aax[Yi].x[s&SUB_MASK] = Rx = (Ry - Lb) / Lm;
								Bounds.x1 = MIN(Bounds.x1, Rx);
								Bounds.x2 = MAX(Bounds.x2, Rx);
								/*
								PathAssert(	aax[Yi].x[s&SUB_MASK] >= Bounds.x1 - GPointF::Threshold &&
											aax[Yi].x[s&SUB_MASK] <= Bounds.x2 + GPointF::Threshold);
								*/
							}
						}

						Cur = End + 1;
					}
					else
					{
						Bounds.x1 = MIN(Bounds.x1, b->x);
						Bounds.x2 = MAX(Bounds.x2, b->x);
					}

					PathAssert(a >= p);
					a += Inc;
					b += Inc;
				}
			}

			#ifdef DEBUG_LOG

			DEBUG_LOG("<path d=\"");
			for (int i=0; i<Points; i++)
			{
				DEBUG_LOG("%c %g %g ", i ? 'L' : 'M', p[i].x, p[i].y);
				if (i % 8 == 7)
					DEBUG_LOG("\n");
			}

			COLOUR c = LgiRand(0xffffff);
			DEBUG_LOG("\" stroke=\"#%06.6x\" fill=\"none\" stroke-width=\"0.1\" />\n", c);
			DEBUG_LOG("<text x=\"%g\" y=\"%g\" font-family=\"Verdana\" font-size=\"2\" fill=\"#%06.6x\">%i%c</text>\n", p[0].x - 2.5, p[0].y, c, Index, Up?'u':'d');
			
			/*
			DEBUG_LOG("Vector[%i] Up=%i Bounds=%g,%g,%g,%g aay=%i,%i\n", Index, Up, Bounds.x1, Bounds.y1, Bounds.x2, Bounds.y2, aay1, aay2);
			for (int i=0; i<Points; i++)
			{
				DEBUG_LOG("\t[%i]: %g,%g\n", i, p[i].x, p[i].y);
			}
			*/

			#endif

			return aax != 0;
		}
		else
		{
			y1 = (int)floor(Bounds.y1 + 0.5);
			y2 = (int)floor(Bounds.y2 - 0.5);
			int n = y2 - y1 + 1;

			x = new double[n];
			if (x)
			{
				GPointF *Cur = p + (Up ? Points - 1 : 0);
				GPointF *Last = Cur;
				Cur += Up ? -1 : 1;
				bool CSide = 0;
				bool LSide = 1;

				for (int y = y1; y<=y2; y++)
				{
					double Ry = (double)y + 0.5;

					CSide = Cur->y > Ry ? 1 : 0;
					LSide = Last->y > Ry ? 1 : 0;
					while (!CSide ^ LSide)
					{
						Last = Cur;
						Cur += Up ? -1 : 1;
						CSide = Cur->y > Ry ? 1 : 0;
						LSide = Last->y > Ry ? 1 : 0;
					}

					if (Cur->x == Last->x)
					{
						x[y - y1] = Cur->x;
					}
					else
					{
						double m = (Cur->y - Last->y) / (Cur->x - Last->x);
						double b = Cur->y - (m * Cur->x);
						x[y - y1] = (Ry - b) / m;
					}
				}
			}

			return x != 0;
		}
	}
};

int VectCompareY(GVector *a, GVector *b, NativeInt Data)
{
	double d = a->Bounds.y1 - b->Bounds.y1;
	return (d < 0) ? -1 : 1;
}

int VectCompareX(GVector *a, GVector *b, NativeInt Data)
{
	NativeInt i = Data >> SUB_SHIFT;
	NativeInt r = Data & SUB_MASK;

	double d = a->aax[i - a->y1].x[r] - b->aax[i - b->y1].x[r];
	
	return (d < 0) ? -1 : 1;
}

///////////////////////////////////////////////////////////
enum GSegType
{
	SegMove,
	SegLine,
	SegQuad,
	SegCube
};

class GSeg
{
public:
	GSegType Type;
	int Points;
	GPointF *Point;

	GSeg(GSegType t)
	{
		Type = t;
		switch (Type)
		{
			case SegMove:
			case SegLine:
				Points = 1; break;
			case SegQuad:
				Points = 2; break;
			case SegCube:
				Points = 3; break;
			default:
				Points = 0; break;
		}
		Point = Points ? new GPointF[Points] : 0;
	}

	~GSeg()
	{
		DeleteArray(Point);
	}

	void Translate(double Dx, double Dy)
	{
		for (int i=0; i<Points; i++)
		{
			Point[i].x += Dx;
			Point[i].y += Dy;
		}
	}

	GPointF *First()
	{
		if (Points > 0 && Point)
		{
			return Point;
		}
		return 0;
	}

	GPointF *Last()
	{
		if (Points > 0 && Point)
		{
			return Point + Points - 1;
		}
		return 0;
	}
};

///////////////////////////////////////////////////////////
void Round(double &n)
{
	int i = (int)floor(n+0.5);
	double d = i - n;
	if (d < 0) d = -d;
	if (d < 0.0000001)
	{
		n = i;
	}
}

void GRectF::Normalize()
{
	Round(x1);
	Round(y1);
	Round(x2);
	Round(y2);

	if (x1 > x2)
	{
		double d = x1;
		x1 = x2;
		x2 = d;
	}

	if (y1 > y2)
	{
		double d = y1;
		y1 = y2;
		y2 = d;
	}
}

void GRectF::Intersect(GRectF &p)
{
	if (Defined)
	{
		x1 = MAX(x1, p.x1);
		y1 = MAX(y1, p.y1);
		x2 = MIN(x2, p.x2);
		y2 = MIN(y2, p.y2);
	}
}

void GRectF::Union(GPointF &p)
{
	if (Defined)
	{
		x1 = MIN(x1, p.x);
		y1 = MIN(y1, p.y);
		x2 = MAX(x2, p.x);
		y2 = MAX(y2, p.y);
	}
	else
	{
		x1 = x2 = p.x;
		y1 = y2 = p.y;
		Defined = true;
	}
}

void GRectF::Union(GRectF &p)
{
	if (Defined)
	{
		p.Normalize();
		x1 = MIN(x1, p.x1);
		y1 = MIN(y1, p.y1);
		x2 = MAX(x2, p.x2);
		y2 = MAX(y2, p.y2);
	}
	else
	{
		*this = p;
	}
}

bool GRectF::Overlap(GPointF &p)
{
	return	(p.x >= x1) &&
			(p.y >= y1) &&
			(p.x <= x2) &&
			(p.y <= y2);
}

bool GRectF::Overlap(GRectF &p)
{
	return	(p.x1 <= x2) &&
			(p.x2 >= x1) &&
			(p.y1 <= y2) &&
			(p.y2 >= y1);
}

void GRectF::Offset(double x, double y)
{
	x1 += x;
	y1 += y;
	x2 += x;
	y2 += y;
}

void GRectF::Size(double dx, double dy)
{
	x1 += dx;
	y1 += dy;
	x2 -= dx;
	y2 -= dy;
}

GRectF &GRectF::operator =(GRect &f)
{
	x1 = f.x1;
	y1 = f.y1;
	x2 = f.x2 + 1;
	y2 = f.y2 + 1;
	Defined = true;
	return *this;
}

GRectF &GRectF::operator =(GRectF &f)
{
	x1 = f.x1;
	y1 = f.y1;
	x2 = f.x2;
	y2 = f.y2;
	Defined = true;
	return *this;
}

GRectF &GRectF::operator =(GPointF &p)
{
	x1 = x2 = p.x;
	y1 = y2 = p.y;
	Defined = true;
	return *this;
}

char *GRectF::Describe()
{
	static char s[64];
	sprintf_s(s, sizeof(s), "%f,%f,%f,%f", x1, y1, x2, y2);
	return s;
}

///////////////////////////////////////////////////////////
void FlattenQuadratic(GPointF *&Out, GPointF &p1, GPointF &p2, GPointF &p3, int numsteps)
{
	double xdelta, xdelta2;
	double ydelta, ydelta2;
	double deltastep, deltastep2;
	double ax, ay;
	double bx, by;
	double i;
	GPointF v1, v2, *o;
	
	if (!Out)
	{
		Out = new GPointF[numsteps];
	}
	if (Out)
	{
		o = Out;

		deltastep = 1.0 / (double) (numsteps - 1);
		deltastep2 = deltastep*deltastep;

		ax = p1.x + -2.0*p2.x + p3.x;
		ay = p1.y + -2.0*p2.y + p3.y;

		bx = -2.0*p1.x + 2.0*p2.x;
		by = -2.0*p1.y + 2.0*p2.y;

		xdelta = ax*deltastep2 + bx*deltastep;
		ydelta = ay*deltastep2 + by*deltastep;

		xdelta2 = 2.0*ax*deltastep2;
		ydelta2 = 2.0*ay*deltastep2;

		v1.x = p1.x;
		v1.y = p1.y;
		o->x = v1.x;
		o->y = v1.y;
		o++;

		for (i=0; i<numsteps - 1; i++)
		{
			v2.x = v1.x + xdelta;
			xdelta += xdelta2;

			v2.y = v1.y + ydelta;
			ydelta += ydelta2;

			o->x = v2.x;
			o->y = v2.y;
			o++;

			v1 = v2;
		}
	}
}

void FlattenCubic(GPointF *&Out, GPointF &p1, GPointF &p2, GPointF &p3, GPointF &p4, int numsteps)
{
	double xdelta, xdelta2, xdelta3;
	double ydelta, ydelta2, ydelta3;
	double deltastep, deltastep2, deltastep3;
	double ax, ay;
	double bx, by;
	double cx, cy;
	double i;
	GPointF v1, v2, *o;
	
	if (!Out)
	{
		Out = new GPointF[numsteps];
	}
	if (Out)
	{
		o = Out;

		deltastep = 1.0 / (double) (numsteps - 1);
		deltastep2 = deltastep*deltastep;
		deltastep3 = deltastep2*deltastep;

		ax = -p1.x + 3.0*p2.x - 3.0*p3.x + p4.x;
		ay = -p1.y + 3.0*p2.y - 3.0*p3.y + p4.y;

		bx = 3.0*p1.x - 6.0*p2.x + 3.0*p3.x;
		by = 3.0*p1.y - 6.0*p2.y + 3.0*p3.y;

		cx = -3.0*p1.x + 3.0*p2.x;
		cy = -3.0*p1.y + 3.0*p2.y;

		xdelta = ax*deltastep3 + bx*deltastep2 + cx*deltastep;
		ydelta = ay*deltastep3 + by*deltastep2 + cy*deltastep;

		xdelta2 = 6.0*ax*deltastep3 + 2.0*bx*deltastep2;
		ydelta2 = 6.0*ay*deltastep3 + 2.0*by*deltastep2;

		xdelta3 = 6.0*ax*deltastep3;
		ydelta3 = 6.0*ay*deltastep3;

		v1.x = p1.x;
		v1.y = p1.y;
		o->x = v1.x;
		o->y = v1.y;
		o++;

		for (i=0; i<numsteps - 1; i++)
		{
			v2.x = v1.x + xdelta;
			xdelta += xdelta2;
			xdelta2 += xdelta3;

			v2.y = v1.y + ydelta;
			ydelta += ydelta2;
			ydelta2 += ydelta3;

			// v2.z = v1.z + zdelta;
			// zdelta += zdelta2;
			// zdelta2 += zdelta3;

			// DrawLine3D(v1, v2, colour);
			o->x = v2.x;
			o->y = v2.y;
			o++;

			v1 = v2;
		}
	}
}

///////////////////////////////////////////////////////////
GPath::GPath(bool aa)
{
	Aa = aa;
	Points = 0;
	Point = 0;
	FillRule = FILLRULE_ODDEVEN;
	Bounds.x1 = Bounds.y1 = Bounds.x2 = Bounds.y2 = 0.0;

	#ifdef DEBUG_LOG
	Log.Open("c:\\temp\\gpath-log.txt", O_WRITE);
	Log.SetPos(Log.GetSize());
	#endif
}

GPath::~GPath()
{
	#ifdef DEBUG_LOG
	Log.Close();
	#endif

	Unflatten();
	Segs.DeleteObjects();
}

void GPath::GetBounds(GRectF *b)
{
	if (b)
	{
		if (!IsFlattened())
		{
			Flatten();
		}

		*b = Bounds;
	}
}

void GPath::Transform(Matrix &m)
{
	Mat = m;
}

void GPath::MoveTo(double x, double y)
{
	Close();
	
	GPointF p(x, y);
	MoveTo(p);
}

void GPath::MoveTo(GPointF &pt)
{
	GSeg *s = new GSeg(SegMove);
	if (s)
	{
		s->Point[0] = pt;
		Segs.Insert(s);
	}
}

void GPath::LineTo(double x, double y)
{
	GPointF p(x, y);
	LineTo(p);
}

void GPath::LineTo(GPointF &pt)
{
	GSeg *s = new GSeg(SegLine);
	if (s)
	{
		s->Point[0] = pt;
		Segs.Insert(s);
	}
}

void GPath::QuadBezierTo(double cx, double cy, double px, double py)
{
	GPointF c(cx, cy);
	GPointF p(px, py);
	QuadBezierTo(c, p);
}

void GPath::QuadBezierTo(GPointF &c, GPointF &p)
{
	GSeg *s = new GSeg(SegQuad);
	if (s)
	{
		s->Point[0] = c;
		s->Point[1] = p;
		Segs.Insert(s);
	}
}

void GPath::CubicBezierTo(double c1x, double c1y, double c2x, double c2y, double px, double py)
{
	GPointF c1(c1x, c1y);
	GPointF c2(c2x, c2y);
	GPointF p(px, py);
	CubicBezierTo(c1, c2, p);
}

void GPath::CubicBezierTo(GPointF &c1, GPointF &c2, GPointF &p)
{
	GSeg *s = new GSeg(SegCube);
	if (s)
	{
		s->Point[0] = c1;
		s->Point[1] = c2;
		s->Point[2] = p;
		Segs.Insert(s);
	}
}

void GPath::Rectangle(double x1, double y1, double x2, double y2)
{
	MoveTo(x1, y1);
	LineTo(x2, y1);
	LineTo(x2, y2);
	LineTo(x1, y2);
	LineTo(x1, y1); // is this last one necessary?
}

void GPath::Rectangle(GPointF &tl, GPointF &rb)
{
	Rectangle(tl.x, tl.y, rb.x, rb.y);
}

void GPath::Rectangle(GRectF &r)
{
	MoveTo(r.x1, r.y1);
	LineTo(r.x2, r.y1);
	LineTo(r.x2, r.y2);
	LineTo(r.x1, r.y2);
	LineTo(r.x1, r.y1);
}

void GPath::RoundRect(GRectF &b, double r)
{
	double k = 0.5522847498 * r;
	
	MoveTo(b.x1 + r, b.y1);

	GPointF c(b.x2 - r, b.y1 + r);
	LineTo(c.x, b.y1);
	CubicBezierTo(	c.x + k, c.y - r,
					c.x + r, c.y - k,
					c.x + r, c.y);

	c.Set(b.x2 - r, b.y2 - r);
	LineTo(b.x2, c.y);

	CubicBezierTo(	c.x + r, c.y + k,
					c.x + k, c.y + r,
					c.x, c.y + r);

	c.Set(b.x1 + r, b.y2 - r);
	LineTo(c.x, b.y2);

	CubicBezierTo(	c.x - k, c.y + r,
					c.x - r, c.y + k,
					c.x - r, c.y);

	c.Set(b.x1 + r, b.y1 + r);
	LineTo(b.x1, c.y);

	CubicBezierTo(	c.x - r, c.y - k,
					c.x - k, c.y - r,
					c.x, c.y - r);
}

void GPath::Circle(double cx, double cy, double radius)
{
	GPointF c(cx, cy);
	Circle(c, radius);
}

void GPath::Circle(GPointF &c, double r)
{
	/*
	double theta;

	for (int i = 0; i < CIRCLE_STEPS + 1; i++)
	{
		theta = (i * LGI_PI * 2.0 / CIRCLE_STEPS);
		GSeg *s = new GSeg(i ? SegLine : SegMove);
		if (s)
		{
			s->Point[0].x = c.x + (radius * cos(theta));
			s->Point[0].y = c.y - (radius * sin(theta));
			Segs.Insert(s);
		}
	}
	*/
	
	double k = 0.5522847498 * r;
	MoveTo(c.x, c.y - r);
	CubicBezierTo(	c.x + k, c.y - r,
					c.x + r, c.y - k,
					c.x + r, c.y);
	CubicBezierTo(	c.x + r, c.y + k,
					c.x + k, c.y + r,
					c.x, c.y + r);
	CubicBezierTo(	c.x - k, c.y + r,
					c.x - r, c.y + k,
					c.x - r, c.y);
	CubicBezierTo(	c.x - r, c.y - k,
					c.x - k, c.y - r,
					c.x, c.y - r);
}

void GPath::Ellipse(double cx, double cy, double x, double y)
{
	GPointF c(cx, cy);
	Ellipse(c, x, y);
}

void GPath::Ellipse(GPointF &c, double x, double y)
{
	double dx = x * 0.552;
	double dy = y * 0.552;

	MoveTo(c.x + x, c.y);
	CubicBezierTo(	c.x + x, c.y - dy,
					c.x + dx, c.y - y,
					c.x, c.y - y);
	CubicBezierTo(	c.x - dx, c.y - y,
					c.x - x, c.y - dy,
					c.x - x, c.y);
	CubicBezierTo(	c.x - x, c.y + dy,
					c.x - dx, c.y + y,
					c.x, c.y + y);
	CubicBezierTo(	c.x + dx, c.y + y,
					c.x + x, c.y + dy,
					c.x + x, c.y);
}

#ifdef WIN32
GPointF &PointConv(POINTFX pt, double x, double y, double Scale)
{
	static GPointF f;
	f.x = x + (FixedToDbl(pt.x) * Scale);
	f.y = y - (FixedToDbl(pt.y) * Scale);
	return f;
}										
#endif

void GPath::Append(GPath &p)
{
	Unflatten();
	for (GSeg *s = p.Segs.First(); s; s = p.Segs.Next())
	{
		Segs.Insert(s);
	}
	p.Segs.Empty();
}

bool GPath::Text(	GFont *Font,
					double x,
					double y,
					char *Utf8,
					int Bytes)
{
	bool Error = false;

	if (Font && Utf8)
	{
		char16 *Utf16 = Utf8ToWide(Utf8, Bytes);
		if (Utf16)
		{
			GPath Temp;
			GDisplayString Sp(Font, " ");

			#ifdef WIN32
			double _x = 0, _y = 0;
			HDC hDC = CreateCompatibleDC(0);
			if (hDC)
			{
				HFONT Old = (HFONT)SelectObject(hDC, Font->Handle());
				GLYPHMETRICS Metrics;
				MAT2 Mat;

				ZeroObj(Mat);
				Mat.eM11.value = 1;
				Mat.eM22.value = 1;

				double Scale = DEBUG_SCALE; // Font->Height();
				for (char16 *In = Utf16; *In; In++)
				{
					if (*In == ' ')
					{
						_x += Sp.X() * Scale;
						continue;
					}

					int Format = GGO_NATIVE; // | GGO_UNHINTED;
					DWORD BufSize = GetGlyphOutline(hDC,
													*In,
													Format,
													&Metrics,
													0, 0, &Mat);
					if (BufSize != -1)
					{
						uchar *Buf = new uchar[BufSize];
						if (Buf)
						{
							if (GetGlyphOutline(hDC,
												*In,
												Format,
												&Metrics,
												BufSize, Buf, &Mat) > 0)
							{
								GPointF Cur;

								TTPOLYGONHEADER *tt = (TTPOLYGONHEADER*)Buf;
								while ((void*)tt < (void*)((char*)Buf + BufSize))
								{
									if (tt->dwType == TT_POLYGON_TYPE)
									{
										Cur = PointConv(tt->pfxStart, _x, _y, Scale);
										GPointF Start = Cur;
										Temp.MoveTo(Cur);

										#ifdef DEBUG_LOG
										DEBUG_LOG("ttf.move %f,%f\n", Cur.x, Cur.y);
										#endif

										TTPOLYCURVE *c = (TTPOLYCURVE*) (tt + 1);
										while ((uint8*)c < ((uint8*)tt) + tt->cb)
										{
											switch (c->wType)
											{
												case TT_PRIM_LINE:
												{
													for (int i=0; i<c->cpfx; i++)
													{
														Cur = PointConv(c->apfx[i], _x, _y, Scale);
														Temp.LineTo(Cur);

														#ifdef DEBUG_LOG
														DEBUG_LOG("ttf.line %.2f,%.2f\n", Cur.x, Cur.y);
														#endif
													}
													break;
												}
												case TT_PRIM_QSPLINE: // quadratic
												{
													for (int i = 0; i < c->cpfx;)
													{
														GPointF c1 = PointConv(c->apfx[i], _x, _y, Scale);
														GPointF e;
														i++;

														if (i == (c->cpfx - 1))
														{
															e = PointConv(c->apfx[i], _x, _y, Scale);
															i++;
														}     
														else
														{
															GPointF b = PointConv(c->apfx[i], _x, _y, Scale);

															e.x = (c1.x + b.x) / 2;
															e.y = (c1.y + b.y) / 2;
														}

														Temp.QuadBezierTo(c1, e);
														#ifdef DEBUG_LOG
														DEBUG_LOG("ttf.quad %.2f,%.2f %.2f,%.2f\n", c1.x, c1.y, e.x, e.y);
														#endif
														Cur = e;
													}
													break;
												}
												default:
												case TT_PRIM_CSPLINE: // cubic
												{
													PathAssert(0);
													break;
												}
											}

											int Len =	sizeof(c->cpfx) + 
														sizeof(c->wType) +
														(sizeof(c->apfx) * c->cpfx);
											c = (TTPOLYCURVE*)&c->apfx[c->cpfx];
										}

										#ifdef DEBUG_LOG
										if (Cur != Start)
										{
											Temp.LineTo(Start);
											DEBUG_LOG("Warning: Not closed!! -> ttf.line %.2f,%.2f\n", Start.x, Start.y);
										}
										#endif

										tt = (TTPOLYGONHEADER*)c;
									}
									else break;
								}

								_x += Metrics.gmCellIncX * Scale;
							}
							else Error = true;

							DeleteArray(Buf);
						}
					}
				}

				SelectObject(hDC, Old);
				DeleteDC(hDC);
			}
			else Error = true;
			#endif

			DeleteArray(Utf16);
			
			#ifdef DEBUG_LOG
			DEBUG_LOG("\n");
			#endif

			GRectF b;
			Temp.GetBounds(&b);

			double Dx = x - b.x1;
			double Dy = y - b.y1 - Font->Ascent();
			for (GSeg *s=Temp.Segs.First(); s; s = Temp.Segs.Next())
			{
				s->Translate(Dx, Dy);
			}

			Append(Temp);
		}
		else Error = true;
	}
	else Error = true;

	return !Error;
}

int GPath::Segments()
{
	return (int)Segs.Length();
}

void GPath::DeleteSeg(int i)
{
	Segs.DeleteAt(i);
}

bool GPath::IsClosed()
{
	GSeg *f = Segs.First();
	GSeg *l = Segs.Last();
	if (f && l)
	{
		GPointF *Start = f->First();
		GPointF *End = l->Last();
		if (Start && End)
		{
			return *Start == *End;
		}
	}

	return false;
}

void GPath::Close()
{
	if (!IsClosed())
	{
		// Find last moveto
		GSeg *f = Segs.Last();
		while (f && f->Type != SegMove)
		{
			f = Segs.Prev();
		}

		// If there is a moveto...
		if (f)
		{
			// Then close that shape
			LineTo(f->Point[0]);
		}
	}
}

bool GPath::IsEmpty()
{
	return Segs.Length() == 0;
}

void GPath::Empty()
{
	Unflatten();
	Segs.DeleteObjects();
	Bounds.x1 = Bounds.y1 = Bounds.x2 = Bounds.y2 = 0.0;
}

bool GPath::IsFlattened()
{
	return Outline.Length() > 0;
}

bool GPath::Flatten()
{
	bool Status = false;

	Unflatten();

	for (GSeg *s=Segs.First(); s; s=Segs.Next())
	{
		switch (s->Type)
		{
			default:
			case SegMove:
			{
				Points += 2; // start AND end
				break;
			}
			case SegLine:
			{
				Points++;
				break;
			}
			case SegQuad:
			case SegCube:
			{
				Points += BEZIER_STEPS - 1;
				break;
			}
		}
	}

	Point = new GPointF[Points];
	if (Point)
	{
		int n = 0;

		GPointF *c = Point;
		GPointF *OutlineStart = Point;
		for (GSeg *s=Segs.First(); s; s=Segs.Next(), n++)
		{
			switch (s->Type)
			{
				default:
				case SegMove:
				{
					// Close the shape back to the start of the object
					if (n && c[-1] != *OutlineStart)
					{
						*c++ = *OutlineStart;
					}

					// Get the index into the array of point
					int k = (int)(c - Point) - 1;
					if (k >= 0)
					{
						#ifdef DEBUG_LOG
						DEBUG_LOG("Outline at %i\n", k);
						#endif

						// Store the index pointing to the start of the shape
						Outline.Add(k);
					}

					// Keep a pointer to the start of the shape (for closing it later)
					OutlineStart = c;

					// Store the point at the start of the shape
					*c++ = s->Point[0];

					#ifdef DEBUG_LOG
					DEBUG_LOG("Move[%i]: %g,%g\n", n, s->Point[0].x, s->Point[0].y);
					#endif
					break;
				}
				case SegLine:
				{
					*c++ = s->Point[0];

					#ifdef DEBUG_LOG
					DEBUG_LOG("Line[%i]: %g,%g\n", n, s->Point[0].x, s->Point[0].y);
					#endif
					break;
				}
				case SegQuad:
				{
					c--;
					FlattenQuadratic(c, *c, s->Point[0], s->Point[1], BEZIER_STEPS);
					c += BEZIER_STEPS;

					#ifdef DEBUG_LOG
					DEBUG_LOG("Quad[%i]: %g,%g %g,%g\n", n,
								s->Point[0].x, s->Point[0].y,
								s->Point[1].x, s->Point[1].y);
					#endif
					break;
				}
				case SegCube:
				{
					c--;
					FlattenCubic(c, *c, s->Point[0], s->Point[1], s->Point[2], BEZIER_STEPS);
					c += BEZIER_STEPS;

					#ifdef DEBUG_LOG
					DEBUG_LOG("Quad[%i]: %g,%g %g,%g %g,%g\n", n,
								s->Point[0].x, s->Point[0].y,
								s->Point[1].x, s->Point[1].y,
								s->Point[2].x, s->Point[2].y);
					#endif
					break;
				}
			}
		}

		// Close the last shape
		if (c[-1] != *OutlineStart)
		{
			*c++ = *OutlineStart;
		}

		// Calculate the number of points in the array
		Points = (int) (c - Point);

		// Set the end of the points to be a full shape
		Outline.Add(Points);

		Status = true;
	}

	#ifdef DEBUG_LOG
	DEBUG_LOG("\n");
	#endif

	// Create vector list
	bool Up = false;
	int Start = -1;
	int VecIndex = 0;
	int NextOutline = 0;
	
	for (int i=1; i<Points; i++)
	{
		// Collect points
		GPointF *Prev = Point + i - 1;
		GPointF *Cur = Point + i;
		GPointF *Next = i < Points - 1 && i < Outline[NextOutline] ? Point + i + 1 : 0;

		// Get absolute change in y
		double dy = Cur->y - Prev->y;
		dy = dy < 0 ? -dy : dy;

		// Start the next segment if no start defined and there is some change in y
		if (Start < 0 && dy > GPointF::Threshold)
		{
			// Store the index of the start for this vector
			Start = i - 1;

			// Calculate which direction we are travelling in
			Up = Cur->y < Prev->y;
		}

		if (Start >= 0)
		{
			bool DirChange = false;
			if (Next)
			{
				// Work out if we have crossed an apex and are travelling in
				// the other direction:
				DirChange = Up ? Next->y >= Cur->y : Next->y <= Cur->y;
			}

			// Do we need to start another vector?
			if (DirChange || Outline[NextOutline] == i || !Next)
			{
				// Create a new vector
				GVector *v = new GVector;
				if (v)
				{
					// Store it
					Vecs.Insert(v);

					// Bump it's index
					v->Index = VecIndex++;

					// Store whether it's up or down
					v->Up = Up;

					// Calculate how many points it has
					v->Points = i - Start + 1;

					// Check it's valid
					PathAssert(v->Points > 1);

					// Store the pointer to the start of the vector
					v->p = Point + Start;

					// Rasterize it into a bunch of cor-ordinates for each sub-pixel scanline
					v->Rasterize(Aa);

					// Work out the bounds
					Bounds.Union(v->Bounds);
				}

				// Set that we are starting a new vector
				Start = -1;

				// Seek to the next outline start index
				if (Outline[NextOutline] == i)
				{
					i = Outline[NextOutline++] + 1;
				}
				else if (Outline[NextOutline] == i + 1)
				{
					// i = Outline[NextOutline++] + 1;
				}
			}
		}
		else
		{
			if (i >= Outline[NextOutline])
			{
				i = Outline[NextOutline++] + 1;
			}
		}
	}

	// Sort the vectors on their top edge
	Vecs.Sort(VectCompareY);

	#ifdef DEBUG_LOG
	DEBUG_LOG("\n");
	#endif
	
	Bounds.Normalize();

	return Status;
}

void GPath::Unflatten()
{
	Points = 0;
	Outline.Length(0);
	DeleteArray(Point);
	Vecs.DeleteObjects();
}

void GPath::Fill(GSurface *pDC, GBrush &c)
{
	if (!GdcD || !pDC || !(*pDC)[0])
	{
		PathAssert(0);
		return;
	}

	if (!Point)
	{
		Flatten();
	}

	if (Point)
	{
		#ifdef DEBUG_LOG
		DEBUG_LOG("\n");
		int *ko = &Outline[0];
		for (int k=0; k<Points; k++)
		{
			DEBUG_LOG("[%i] %g,%g\n", k, Point[k].x, Point[k].y);
			if (k == *ko)
			{
				DEBUG_LOG("\n");
				ko++;
			}
		}

		DEBUG_LOG("Bounds=%g,%g,%g,%g\n", Bounds.x1, Bounds.y1, Bounds.x2, Bounds.y2);
		#endif

		// Loop scanlines
		List<GVector> In;
		for (GVector *v=Vecs.First(); v; v=Vecs.Next())
		{
			In.Insert(v);
		}

		List<GVector> Active;
		size_t VectorCount = In.Length();
		double *x = new double[VectorCount];
		GVector **xv = new GVector*[VectorCount];
		int x1 = (int)floor(Bounds.x1);
		int x2 = (int)ceil(Bounds.x2);
		int Width = x2 - x1 + 1;
		uchar *Alpha = new uchar[Width];

		#if 0
			#define CHECK_XV(idx)		PathAssert((idx) >= 0 && (idx) < VectorCount)
			#define CHECK_ALPHA(idx)	PathAssert((idx) >= 0 && (idx) < Width)
		#else
			#define CHECK_XV(idx)
			#define CHECK_ALPHA(idx)
		#endif

		GRectF Doc = Bounds;
		int Ox = 0, Oy = 0;
		
		pDC->GetOrigin(Ox, Oy);
		double OffsetX = Mat[2][0] - Ox;
		double OffsetY = Mat[2][1] - Oy;
		
		Doc.Offset(OffsetX, OffsetY);
		GRectF Page(0, 0, pDC->X(), pDC->Y());
		GRect DcClip = pDC->ClipRgn();
		if (DcClip.Valid())
			Page = DcClip;
		GRectF Clip = Doc;
		Clip.Intersect(Page);
		
		// Convert the doc and clip back to path coords.
		Doc.Offset(-OffsetX, -OffsetY);
		Clip.Offset(-OffsetX, -OffsetY);

		GBrush::GRopArgs a;
		a.pDC = pDC;
		a.Pixels = 0;
		a.Alpha = 0;
		a.Cs = pDC->GetColourSpace();
		a.BytesPerPixel = pDC->GetBits() >> 3;
		a.x = (int)(Clip.x1 - Mat[2][0]);
		int RopLength = (int)ceil(Clip.x2) - (int)floor(Clip.x1);
		a.Len = RopLength;
		a.EndOfMem = (*pDC)[pDC->Y()-1] + (pDC->X() * pDC->GetBits() / 8);

		if (x && Alpha && c.Start(a))
		{
			if (Aa)
			{
				#define ADD_EVENT		0x01
				#define REMOVE_EVENT	0x02

				int y1 = ToInt(Doc.y1);
				int y2 = ToInt(Doc.y2);

				int aax1 = x1 << SUB_SHIFT;

				int Height = (y2>>SUB_SHIFT) - (y1>>SUB_SHIFT) + 1;
				uchar *Events = new uchar[Height];
				if (Events)
				{
					// Setup events
					memset(Events, 0, Height);
					for (GVector *New=In.First(); New; New=In.Next())
					{
						int Aay1 = (New->aay1 - y1) >> SUB_SHIFT;
						int Aay2 = (New->aay2 - y1) >> SUB_SHIFT;

						PathAssert(Aay1 >= 0 && Aay1 < Height);
						PathAssert(Aay2 >= 0 && Aay2 < Height);

						Events[Aay1] |= ADD_EVENT;
						Events[Aay2] |= REMOVE_EVENT;
					}

					// For each scan line
					for (int y=y1; y<=y2; )
					{
						// Anti-aliased edges
						memset(Alpha, 0, Width);

						// Loop subpixel
						for (int s = y&SUB_MASK; s<SUB_SAMPLE && y<=y2; s++, y++)
						{
							if (!_Disable_ActiveList)
							{
								if (Events[(y-y1)>>SUB_SHIFT] & ADD_EVENT)
								{
									// Add new active vecs
									for (GVector *New=In.First(); New;)
									{
										if (New->aay1 == y)
										{
											Active.Insert(New);
											In.Delete(New);
											New = In.Current();
										}
										else
										{
											New = In.Next();
										}
									}
								}
								
								if (Events[(y-y1)>>SUB_SHIFT] & REMOVE_EVENT)
								{
									// Remove old active vecs
									for (GVector *Old=Active.First(); Old; )
									{
										PathAssert(Old->aay2 >= y);
										
										if (Old->aay2 == y)
										{
											Active.Delete(Old);
											Old = Active.Current();
										}
										else
										{
											Old = Active.Next();
										}
									}
								}
							}

							int Xs = 0;
							if (!_Disable_XSort)
							{
								// Sort X values from active vectors
								if (FillRule == FILLRULE_NONZERO)
								{
									GVector *a=Active.First();
									if (a)
									{
										// bool Up = a->Up;
										int n = 0;
										for (; a; a=Active.Next())
										{
											a->Cx = a->aax[(y>>SUB_SHIFT) - a->y1].x[s];

											#if defined(_DEBUG) && DEBUG_NEGITIVE_X
											if (a->Cx < 1.0)
											{
												PathAssert(0);
											}
											#endif

											CHECK_XV(n-1);
											if (n == 0 || a->Cx >= xv[n-1]->Cx)
											{
												// Insert at end
												CHECK_XV(n);
												xv[n++] = a;
											}
											else
											{
												// int Oldn = n;

												// Insert in the middle or start
												for (int i=0; i<n; i++)
												{
													CHECK_XV(i);
													if (a->Cx < xv[i]->Cx)
													{
														for (int k = n - 1; k >= i; k--)
														{
															CHECK_XV(k+1);
															CHECK_XV(k);
															xv[k+1] = xv[k];
														}

														CHECK_XV(i);
														xv[i] = a;
														n++;
														break;
													}
												}
											}
										}

										int Depth = 0;
										for (int i=0; i<n; i++)
										{
											if (Depth == 0)
											{
												CHECK_XV(i);
												x[Xs++] = xv[i]->Cx;
											}

											Depth += xv[i]->Up ? -1 : 1;

											if (Depth == 0)
											{
												x[Xs++] = xv[i]->Cx;
											}
										}
									}
								}
								else
								{
									for (GVector *a=Active.First(); a; a=Active.Next())
									{
										double Cx = a->aax[(y>>SUB_SHIFT) - a->y1].x[s];

										PathAssert(Cx >= Bounds.x1 - GPointF::Threshold && Cx <= Bounds.x2 + GPointF::Threshold);

										if (Xs == 0 || (Xs > 0 && Cx >= x[Xs-1]))
										{
											// Insert at end
											x[Xs++] = Cx;
										}
										else
										{
											// int OldXs = Xs;

											// Insert in the middle or start
											for (int i=0; i<Xs; i++)
											{
												if (Cx < x[i])
												{
													for (int n = Xs - 1; n >= i; n--)
													{
														x[n+1] = x[n];
													}

													x[i] = Cx;
													Xs++;
													break;
												}
											}
										}
									}
								}
							}

							#if defined(_DEBUG) && DEBUG_ODD_EDGES
							if (Xs % 2 == 1)
							{
								PathAssert(0);
							}
							#endif

							#ifdef DEBUG_LOG

							if (Xs % 2 == 1)
							{
								DEBUG_LOG("ERROR: Odd number of edges (%i) on this line:\n", Xs);
							}

							DEBUG_LOG("Y=%i Xs=%i ", y, Xs);
							for (int n=0; n<Xs; n++)
							{
								DEBUG_LOG("%g, ", x[n]);
							}
							DEBUG_LOG("\n");
							#endif

							if (!_Disable_Alpha)
							{
								// Add to our alpha run
								for (int i=0; i<Xs-1; i+=2)
								{
									int Sx = ToInt(x[i]) - aax1;
									int Ex = ToInt(x[i+1]) - aax1;

									if ((Sx>>SUB_SHIFT) == (Ex>>SUB_SHIFT))
									{
										CHECK_ALPHA(Sx>>SUB_SHIFT);
										Alpha[Sx>>SUB_SHIFT] += Ex-Sx;
									}
									else
									{
										int Before = SUB_SAMPLE - (Sx & SUB_MASK);
										if (Before)
										{
											CHECK_ALPHA(Sx>>SUB_SHIFT);
											Alpha[Sx>>SUB_SHIFT] += Before;
											Sx += Before;
										}

										for (int k=Sx>>SUB_SHIFT; k<(Ex>>SUB_SHIFT); k++)
										{
											CHECK_ALPHA(k);
											Alpha[k] += SUB_SAMPLE;
										}

										int After = Ex & SUB_MASK;
										if (After)
										{
											CHECK_ALPHA(Ex>>SUB_SHIFT);
											Alpha[Ex>>SUB_SHIFT] += After;
										}
									}
								}
							}
						}

						if (!_Disable_Rops)
						{
							// Draw pixels..
							int DocX = (int)(Clip.x1 - Doc.x1);
							a.y = ((y + SUB_SHIFT - 1) >> SUB_SHIFT) + (int)Mat[2][1] - 1;
							if (a.y >= floor(Clip.y1) && a.y <= ceil(Clip.y2))
							{
								a.Pixels = (*pDC)[a.y-Oy];
								if (a.Pixels)
								{
									int AddX = DocX + (int)Doc.x1;
									a.y -= (int)Mat[2][1];
									a.Len = RopLength;
									a.Pixels += a.BytesPerPixel * (AddX - Ox);
									// memset(a.Pixels, 0xff, 12);
									a.Alpha = Alpha + DocX;
									c.Rop(a);
								}
							}
						}
					}
				}

				DeleteArray(Events);
				DeleteArray(Alpha);
			}
			else // Not anti-aliased
			{
				int y1 = (int)floor(Bounds.y1);
				int y2 = (int)ceil(Bounds.y2);

				// For each scan line
				for (int y=y1; y<=y2; y++)
				{
					memset(Alpha, 0, Width);

					// Add new active vecs
					for (GVector *New=In.First(); New; New=In.First())
					{
						if (New->y1 == y)
						{
							Active.Insert(New);
							In.Delete(New);
						}
						else break;
					}

					// Sort X values from active vectors
					int Xs = 0;
					for (GVector *v=Active.First(); v; v=Active.Next())
					{
						if (y < v->y1 || y > v->y2)
						{
							PathAssert(0);
						}

						double Cx = v->x[y - v->y1];
						if (Xs == 0 || Cx > x[Xs-1])
						{
							// Insert at end
							x[Xs++] = Cx;
						}
						else
						{
							// Insert in the middle or start
							for (int i=0; i<Xs; i++)
							{
								if (Cx < x[i])
								{
									memmove(x+(i+1), x+i, sizeof(*x) * (Xs - i));
									x[i] = Cx;
									Xs++;
									break;
								}
							}
						}
					}

					#if defined(_DEBUG) && DEBUG_ODD_EDGES
					if (Xs % 2 == 1)
					{
						PathAssert(0);
					}
					#endif

					// Draw pixels..
					for (int i=0; i<Xs-1; i+=2)
					{
						int c1 = (int)(x[i] + 0.5 - x1);
						int c2 = (int)(x[i+1] + 0.5 - x1);
						memset(Alpha + c1, SUB_SAMPLE*SUB_SAMPLE, c2 - c1 + 1);
					}
					if (Xs > 1)
					{
						a.y = y;
						a.Pixels = (*pDC)[a.y];
						if (a.Pixels)
						{
							a.Pixels += a.BytesPerPixel * a.x;
							a.Alpha = Alpha + a.x - x1;
							c.Rop(a);
						}
					}

					// Remove old active vecs
					for (GVector *Old=Active.First(); Old; )
					{
						if (Old->y2 == y)
						{
							Active.Delete(Old);
							Old = Active.Current();
						}
						else
						{
							Old = Active.Next();
						}
					}
				}
			}
		}

		// Clean up
		DeleteArray(x);
		DeleteArray(xv);

		#if DEBUG_DRAW_VECTORS
		for (GVector *h = Vecs.First(); h; h = Vecs.Next())
		{
			pDC->Colour(h->Up ? Rgb24(255, 0, 0) : Rgb24(0, 0, 255), 24);
			for (int i=0; i<h->Points-1; i++)
			{
				pDC->Line(	h->p[i].x, h->p[i].y,
							h->p[i+1].x, h->p[i+1].y);
			}
		}
		#endif

		#if DEBUG_DRAW_SEGMENTS
		// Debug
		GPointF Cur;
		int n = 0;
		for (GSeg *s=Segs.First(); s; s=Segs.Next(), n++)
		{
			switch (s->Type)
			{
				case SegMove:
				{
					Cur = s->Point[0];
					break;
				}
				case SegLine:
				{
					pDC->Colour(Rgb24(0, 0, 255), 24);
					pDC->Line(Cur.x, Cur.y, s->Point[0].x, s->Point[0].y);
					Cur = s->Point[0];
					break;
				}
				case SegQuad:
				{
					pDC->Colour(Rgb24(255, 0, 0), 24);
					pDC->Line(Cur.x, Cur.y, s->Point[0].x, s->Point[0].y);
					pDC->Circle(Cur.x, Cur.y, 3);
					pDC->Line(s->Point[0].x, s->Point[0].y, s->Point[1].x, s->Point[1].y);
					Cur = s->Point[1];
					pDC->Circle(Cur.x, Cur.y, 3);
					pDC->Circle(s->Point[0].x, s->Point[0].y, 2);
					break;
				}
				case SegCube:
				{
					pDC->Colour(Rgb24(255, 0, 0), 24);
					pDC->Line(Cur.x, Cur.y, s->Point[0].x, s->Point[0].y);
					pDC->Line(s->Point[0].x, s->Point[0].y, s->Point[1].x, s->Point[1].y);
					pDC->Line(s->Point[1].x, s->Point[1].y, s->Point[2].x, s->Point[2].y);

					pDC->Circle(Cur.x, Cur.y, 3);
					Cur = s->Point[2];
					pDC->Circle(Cur.x, Cur.y, 3);
					pDC->Circle(s->Point[0].x, s->Point[0].y, 2);
					pDC->Circle(s->Point[1].x, s->Point[1].y, 2);
					break;
				}
			}

			if (n)
			{
				char s[256];
				sprintf_s(s, sizeof(s), "%i", n);

				/*
				Debug.Fore(Rgb24(0x80, 0x80, 0x80));
				Debug.Transparent(true);
				Debug.Text(pDC, Cur.x + 5, Cur.y - 4, s);
				*/
			}
		}
		#endif
	}
}

void GPath::Stroke(GSurface *pDC, GBrush &Brush, double Width)
{
	if (!Point)
	{
		Flatten();
	}

	if (Point)
	{
		// pDC->Colour(c, 24);

		int Ox, Oy;
		pDC->GetOrigin(Ox, Oy);
		pDC->SetOrigin((int)-Mat[2][0], (int)-Mat[2][1]);

		int i;
		GPointF *p = Point;
		for (i=0; i<Points-1; i++)
		{
			pDC->Line(  (int)p[i].x,
			            (int)p[i].y,
			            (int)p[i+1].x,
			            (int)p[i+1].y);
		}
		
		pDC->Line((int)p[0].x,
		          (int)p[0].y,
		          (int)p[i].x,
		          (int)p[i].y);

		pDC->SetOrigin(Ox, Oy);
	}
}

///////////////////////////////////////////////////////////////////
void GBrush::MakeAlphaLut()
{
	for (int i=0; i<65; i++)
	{
		AlphaLut[i] = (i * 255) / (SUB_SAMPLE * SUB_SAMPLE);
	}
}

///////////////////////////////////////////////////////////////////
// Compositing
void GSolidBrush::Rop(GRopArgs &Args)
{
	switch (Args.Cs)
	{
		default:
			LgiAssert(!"Impl me");
			break;

		#define SolidCase(cs, bits) \
			case Cs##cs: SolidRop##bits((G##cs*)Args.Pixels, Args.Len, Args.Alpha, c32); break;

		SolidCase(Rgb16, 16);
		SolidCase(Bgr16, 16);

		SolidCase(Rgb24, 24);
		SolidCase(Bgr24, 24);
		SolidCase(Xrgb32, 24);
		SolidCase(Xbgr32, 24);
		SolidCase(Rgbx32, 24);
		SolidCase(Bgrx32, 24);
			
		SolidCase(Rgba32, 32);
		SolidCase(Argb32, 32);
		SolidCase(Bgra32, 32);
		SolidCase(Abgr32, 32);
	}
}

////////////////////////////////////////////////////////////////////////////
int StopCompare(GBlendStop *a, GBlendStop *b, NativeInt Data)
{
	return a->Pos > b->Pos ? 1 : -1;
}

bool GBlendBrush::Start(GRopArgs &Args)
{
	List<GBlendStop> s;
	for (int n=0; n<Stops; n++)
	{
		s.Insert(Stop+n);
	}
	s.Sort(StopCompare);

	GBlendStop *Prev = s.First();
	GBlendStop *Next = s.Next();
	if (Prev && Next)
	{
		for (int i=0; i<CountOf(Lut); i++)
		{
			double p = (double)i / CountOf(Lut);

			if (p > Next->Pos)
			{
				GBlendStop *n = s.Next();
				if (n)
				{
					Prev = Next;
					Next = n;
				}
			}

			if (p < Prev->Pos)
			{
				Lut[i] = Prev->c32;
			}
			else if (p < Next->Pos)
			{
				double d = Next->Pos - Prev->Pos;
				uchar a = (int) (((p - Prev->Pos) / d) * 255);
				uchar oma = 255 - a;
				
				Lut[i]=	Rgba32
						(
							(R32(Prev->c32) * oma / 255) + (R32(Next->c32) * a / 255),
							(G32(Prev->c32) * oma / 255) + (G32(Next->c32) * a / 255),
							(B32(Prev->c32) * oma / 255) + (B32(Next->c32) * a / 255),
							(A32(Prev->c32) * oma / 255) + (A32(Next->c32) * a / 255)
						);
			}
			else if (p >= Next->Pos)
			{
				Lut[i] = Next->c32;
			}
		}

		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////
bool GLinearBlendBrush::Start(GRopArgs &Args)
{
	GBlendBrush::Start(Args);

	double dx = p[1].x - p[0].x;
	double dy = p[1].y - p[0].y;
	// bool Vert = (dx < 0 ? -dx : dx) < 0.0000001;

	double d = sqrt( (dx*dx) + (dy*dy) );
	double a = atan2(dy, dx);

	GPointF Origin(0, 0);
	Origin.Translate(-p[0].x, -p[0].y);
	Origin.Rotate(-a);

	GPointF x(1, 0);
	x.Translate(-p[0].x, -p[0].y);
	x.Rotate(-a);

	GPointF y(0, 1);
	y.Translate(-p[0].x, -p[0].y);
	y.Rotate(-a);

	Base = Origin.x / d;
	IncX = (x.x - Origin.x) / d;
	IncY = (y.x - Origin.x) / d;

	return true;
}

void GLinearBlendBrush::Rop(GRopArgs &Args)
{
	PathAssert(GdcD);

	switch (Args.Cs)
	{
		default:
			LgiAssert(!"Impl me");
			break;

		#define LinearCase(cs, bits) \
			case Cs##cs: Linear##bits((G##cs*)Args.Pixels, Args); break;

		LinearCase(Rgb16, 16);
		LinearCase(Bgr16, 16);

		LinearCase(Rgb24, 24);
		LinearCase(Bgr24, 24);
		LinearCase(Xrgb32, 24);
		LinearCase(Xbgr32, 24);
		LinearCase(Rgbx32, 24);
		LinearCase(Bgrx32, 24);
			
		LinearCase(Rgba32, 32);
		LinearCase(Argb32, 32);
		LinearCase(Bgra32, 32);
		LinearCase(Abgr32, 32);
	}
}

////////////////////////////////////////////////////////////////////////////
// The squaring of the root:
//		http://www.azillionmonkeys.com/qed/sqroot.html

////////////////////////////////////////////////////////////////////////////
void GRadialBlendBrush::Rop(GRopArgs &Args)
{
	switch (Args.Cs)
	{
		default:
			LgiAssert(!"Impl me");
			break;

		#define RadialCase(cs, bits) \
			case Cs##cs: Radial##bits((G##cs*)Args.Pixels, Args); break;

		RadialCase(Rgb16, 16);
		RadialCase(Bgr16, 16);

		RadialCase(Rgb24, 24);
		RadialCase(Bgr24, 24);
		RadialCase(Xrgb32, 24);
		RadialCase(Xbgr32, 24);
		RadialCase(Rgbx32, 24);
		RadialCase(Bgrx32, 24);
			
		RadialCase(Rgba32, 32);
		RadialCase(Argb32, 32);
		RadialCase(Bgra32, 32);
		RadialCase(Abgr32, 32);

	}
}

//////////////////////////////////////////////
void GEraseBrush::Rop(GRopArgs &Args)
{
	uchar *DivLut = Div255Lut;
	uchar *r = Args.Alpha;

	switch (Args.Cs)
	{
		default:
			LgiAssert(!"Impl me");
			break;
		case System24BitColourSpace:
		{
			break;
		}
		case System32BitColourSpace:
		{
			System32BitPixel *d = (System32BitPixel*) Args.Pixels;
			System32BitPixel *end = d + Args.Len;

			while (d < end)
			{
				d->a = DivLut[d->a * (255 - AlphaLut[*r++])];
				d++;
			}
			break;
		}
	}
}
