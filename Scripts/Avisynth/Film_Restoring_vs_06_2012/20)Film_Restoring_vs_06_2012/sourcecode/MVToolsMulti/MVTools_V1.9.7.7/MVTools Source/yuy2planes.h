#ifndef __YUY2PLANES_H__
#define __YUY2PLANES_H__

#include <malloc.h>

class PVideoFrame;

class YUY2Planes
{
    unsigned char *pSrcBase;
    unsigned char *pSrcUBase;
    unsigned char *pSrcVBase;
    int nWidth;
    int nHeight;
    int srcPitch;
    int srcPitchUV;
    bool bYUY2Type; // true if pixel type is YUY2
    bool isse;

public :

    YUY2Planes(int const _nWidth, int const _nHeight, int const _PixelType, bool const _isse);
    ~YUY2Planes();

    inline int GetWidth() const { return nWidth; }
    inline int GetHeight() const { return nHeight; }
    inline int GetPitch() const { return srcPitch; }
    inline int GetPitchUV() const { return srcPitchUV; }
    inline unsigned char *GetPtr() const { return pSrcBase; }
    inline unsigned char *GetPtrU() const { return pSrcUBase; }
    inline unsigned char *GetPtrV() const { return pSrcVBase; }

    // convert YUY2 from to planes
    void YUY2ToPlanes(unsigned char const *pSrcYUY2, int const nSrcPitchYUY2);
    void YUY2FromPlanes(unsigned char *pSrcYUY2, int const nSrcPitchYUY2);
    // convert video frame to planes and return pointers and pitches
    void ConvertVideoFrameToPlanes(PVideoFrame *pVF, unsigned char const *VFPtrs[3], int *VFPitches);
    void ConvertVideoFrameToPlanes(PVideoFrame *pVF, unsigned char *VFPtrs[3], int *VFPitches);


private:
    // mmx conversion returns
    static void convYUV422to422(unsigned char const *src, unsigned char *py, unsigned char *pu, unsigned char *pv,
                                int const pitch1, int const pitch2y, int const pitch2uv, int const width, int const height);
    static void conv422toYUV422(unsigned char const *py, unsigned char const *pu, unsigned char const *pv,
                                unsigned char *dst, int const pitch1Y, int const pitch1UV, int const pitch2, 
                                int const width, int const height);
};


#endif
