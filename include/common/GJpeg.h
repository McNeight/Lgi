#if !defined(_GJPEG_H_) && HAS_LIBJPEG
#define _GJPEG_H_

class GdcJpeg : public GFilter
{
public:
    enum SubSampleMode
    {
        Sample_1x1_1x1_1x1,
        Sample_2x2_1x1_1x1,
        Sample_2x1_1x1_1x1,
        Sample_1x2_1x1_1x1,
    };

private:
	friend class GJpegOptions;
	#if LIBJPEG_SHARED
	class LibJpeg *d;
	#endif

	GFilter::IoStatus _Write(GStream *Out, GSurface *pDC, int Quality, SubSampleMode SubSample, GdcPt2 Dpi);

public:
	GdcJpeg();
	
    const char *GetComponentName() { return "libjpeg"; }
	Format GetFormat() { return FmtJpeg; }
	int GetCapabilites() { return FILTER_CAP_READ | FILTER_CAP_WRITE; }
	IoStatus ReadImage(GSurface *pDC, GStream *In);
	IoStatus WriteImage(GStream *Out, GSurface *pDC);

	bool GetVariant(const char *n, GVariant &v, char *a)
	{
		if (!_stricmp(n, LGI_FILTER_TYPE))
		{
			v = "Jpeg";
		}
		else if (!_stricmp(n, LGI_FILTER_EXTENSIONS))
		{
			v = "JPG,JPEG";
		}
		else return false;

		return true;
	}
};

#endif