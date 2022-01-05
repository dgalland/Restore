// Author: Manao
// Copyright(c)2006 A.G.Balakhnin aka Fizick - YUY2

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .

#include "Padding.h"
#include "CopyCode.h"

void PadCorner(unsigned char *p, unsigned char v, int hPad, int vPad, int refPitch)
{
    int refPitch1=refPitch-hPad;
    for ( int i = 0; i < vPad; ++i ) {
        memset(p, v, hPad); // faster than loop
    /*
        for ( int j = 0; j < hPad; ++j ) {
            p[j] = v;
        }
    */
        p += refPitch;
    }
}

Padding::Padding(PClip _child, int hPad, int vPad, IScriptEnvironment* env) :
         GenericVideoFilter(_child)
{
    if ( !vi.IsYV12() && !vi.IsYUY2() )
        env->ThrowError("Padding : clip must be in the YV12 or YUY2 color format");

    horizontalPadding = hPad;
    verticalPadding = vPad;
    width = vi.width;
    height = vi.height;

    isse = env->GetCPUFlags() >= CPUF_INTEGER_SSE;

    SrcPlanes =  new YUY2Planes(vi.width, vi.height, vi.pixel_type, isse);

    // pad height and width
    vi.width += horizontalPadding * 2;
    vi.height += verticalPadding * 2;
    DstPlanes =  new YUY2Planes(vi.width, vi.height, vi.pixel_type, isse);
}

Padding::~Padding()
{
    if (SrcPlanes) delete SrcPlanes;
    if (DstPlanes) delete DstPlanes;
}

PVideoFrame __stdcall Padding::GetFrame(int n, IScriptEnvironment *env)
{
    PVideoFrame src = child->GetFrame(n, env);
    PVideoFrame dst = env->NewVideoFrame(vi);

    // convert to frames
    unsigned char const *pSrc[3];
    unsigned char *pDst[3];
    int nDstPitches[3], nSrcPitches[3];
    SrcPlanes->ConvertVideoFrameToPlanes(&src, pSrc, nSrcPitches);
    DstPlanes->ConvertVideoFrameToPlanes(&dst, pDst, nDstPitches);

    int yRatioUV = (vi.IsYV12()) ? 2 : 1;

    PlaneCopy(pDst[0] + horizontalPadding + verticalPadding * nDstPitches[0], nDstPitches[0],
              pSrc[0], nSrcPitches[0], width, height, isse);
    PadReferenceFrame(pDst[0], nDstPitches[0], horizontalPadding, verticalPadding, width, height);


    PlaneCopy(pDst[1] + horizontalPadding/2 + verticalPadding/yRatioUV * nDstPitches[1], 
              nDstPitches[1],	pSrc[1], nSrcPitches[1], width/2, height/yRatioUV, isse);
    PadReferenceFrame(pDst[1], nDstPitches[1], horizontalPadding/2, verticalPadding/yRatioUV, width/2, height/yRatioUV);


    PlaneCopy(pDst[2] + horizontalPadding/2 + verticalPadding/yRatioUV * nDstPitches[2], 
              nDstPitches[2],	pSrc[2], nSrcPitches[2], width/2, height/yRatioUV, isse);
    PadReferenceFrame(pDst[2], nDstPitches[2], horizontalPadding/2, verticalPadding/yRatioUV, width/2, height/yRatioUV);

    // convert back from planes
    unsigned char *pDstYUY2 = dst->GetWritePtr();
    int nDstPitchYUY2 = dst->GetPitch();
    DstPlanes->YUY2FromPlanes(pDstYUY2, nDstPitchYUY2);

    return dst;
}

void Padding::PadReferenceFrame(unsigned char *refFrame, int refPitch, int hPad, int vPad, int width, int height)
{
    unsigned char value;
    unsigned char *pfoff = refFrame + vPad * refPitch + hPad;
    unsigned char *p;

    // Up-Left
    PadCorner(refFrame, pfoff[0], hPad, vPad, refPitch);
    // Up-Right
    PadCorner(refFrame + hPad + width, pfoff[width - 1], hPad, vPad, refPitch);
    // Down-Left
    PadCorner(refFrame + (vPad + height) * refPitch, pfoff[(height - 1) * refPitch], hPad, vPad, refPitch);
    // Down-Right
    PadCorner(refFrame + hPad + width + (vPad + height) * refPitch, pfoff[(height - 1) * refPitch + width - 1], 
              hPad, vPad, refPitch);

    // Up
    unsigned char* pBase=refFrame+hPad;
    for ( int i = 0; i < width; ++i ) {
        value = pfoff[i];
        p = pBase + i;
        for ( int j = 0; j < vPad; ++j ) {
            *p = value;
            p += refPitch;
        }
    }

    // Left
    pBase=refFrame+vPad*refPitch;
    for ( int i = 0; i < height; ++i ) {
        int offset=i*refPitch;
        value = pfoff[offset];
        p = pBase + offset;
        memset(p, value, hPad);
        /*
        for ( int j = 0; j < hPad; ++j )
            p[j] = value; 
        */
    }

    // Right
    pBase=refFrame + vPad*refPitch + width + hPad;
    unsigned char* pfoffbase=pfoff+width - 1;
    for ( int i = 0; i < height; ++i ) {
        int offset=i*refPitch;
        value = *(pfoffbase+offset);
        p = pBase +  offset;
        memset(p, value, hPad);
        /*
        for ( int j = 0; j < hPad; j++ )
            p[j] = value;
        */
    }

    // Down
    pBase=refFrame + hPad + (height + vPad) * refPitch;
    pfoffbase=pfoff+(height - 1) * refPitch;
    for ( int i = 0; i < width; ++i ) {
        value = *(pfoffbase+i);
        p = pBase + i;
        for ( int j = 0; j < vPad; ++j ) {
            *p = value;
            p += refPitch;
        }
    }
}
