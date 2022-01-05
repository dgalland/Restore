#ifndef __YUY2PLANES_H__
#define __YUY2PLANES_H__

#include <malloc.h>

class YUY2Planes
{
	unsigned char *pSrc;
	unsigned char *pSrcU;
	unsigned char *pSrcV;
	int nWidth;
	int nHeight;
	int srcPitch;
	int srcPitchUV;

   
public :

	YUY2Planes(int _nWidth, int _nHeight);
   ~YUY2Planes();

   inline int GetPitch() const { return srcPitch; }
   inline int GetPitchUV() const { return srcPitchUV; }
   inline unsigned char *GetPtr() const { return pSrc; }
   inline unsigned char *GetPtrU() const { return pSrcU; }
   inline unsigned char *GetPtrV() const { return pSrcV; }

static	void convYUV422to422(const unsigned char *src, unsigned char *py, unsigned char *pu, unsigned char *pv,
                                        int pitch1, int pitch2y, int pitch2uv, int width, int height);
static	void conv422toYUV422(const unsigned char *py, const unsigned char *pu, const unsigned char *pv,
                           unsigned char *dst, int pitch1Y, int pitch1UV, int pitch2, int width, int height);
};

void YUY2ToPlanes(const unsigned char *pSrcYUY2, int nSrcPitchYUY2, int nWidth, int nHeight,
							  const unsigned char * pSrc1, int _srcPitch,
							  const unsigned char * pSrcU1, const unsigned char * pSrcV1, int _srcPitchUV, bool mmx);

void YUY2FromPlanes(unsigned char *pSrcYUY2, int nSrcPitchYUY2, int nWidth, int nHeight,
							  unsigned char * pSrc1, int _srcPitch,
							  unsigned char * pSrcU1, unsigned char * pSrcV1, int _srcPitchUV, bool mmx);


#endif
