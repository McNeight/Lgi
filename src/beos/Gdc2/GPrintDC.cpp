#include "Lgi.h"
#include "GdiLeak.h"

class GPrintDCPrivate
{
public:
	bool PageOpen;
	bool DocOpen;

	GPrintDCPrivate()
	{
		PageOpen = false;
		DocOpen = false;
	}
};

GPrintDC::GPrintDC(void *Handle, const char *PrintJobName, const char *PrinterName)
{
	d = new GPrintDCPrivate;
}

GPrintDC::~GPrintDC()
{
}

int GPrintDC::X()
{
	return 0;
}

int GPrintDC::Y()
{
	return 0;
}

int GPrintDC::GetBits()
{
	return 0;
}

int GPrintDC::DpiX()
{
	return 0; 
}

int GPrintDC::DpiY()
{
	return 0;
}

