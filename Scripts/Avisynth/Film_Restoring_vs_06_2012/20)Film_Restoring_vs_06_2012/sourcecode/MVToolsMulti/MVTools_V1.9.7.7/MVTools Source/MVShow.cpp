// Show the motion vectors of a clip
// Author: Manao
// Copyright(c)2006 A.G.Balakhnin aka Fizick - YUY2, overlap

// See legal notice in Copying.txt for more information

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

#include "MVShow.h"
#include <stdio.h>
#include "info.h"
#include "CopyCode.h"

MVShow::MVShow(PClip _child, PClip vectors, int _scale, int _plane, int _tol, bool _showsad,
               int nSCD1, int nSCD2, bool mmx, bool _isse, IScriptEnvironment* env) :
        GenericVideoFilter(_child),
        mvClip(vectors, nSCD1, nSCD2, env, true),
        MVFilter(vectors, "MVShow", env)
{
    isse = _isse;
    nScale = _scale;
    if ( nScale < 1 ) nScale = 1;

    nPlane = _plane;
    if ( nPlane < 0 ) nPlane = 0;
    if ( nPlane >= mvClip.GetLevelCount() - 1 )
        nPlane = mvClip.GetLevelCount() - 1;

    nTolerance = _tol;
    if ( nTolerance < 0 ) nTolerance = 0;

    showSad = _showsad;

    SrcPlanes =  new YUY2Planes(nWidth, nHeight, vi.pixel_type, isse);
    DstPlanes =  new YUY2Planes(nWidth, nHeight, vi.pixel_type, isse);
 }

MVShow::~MVShow()
{
    if (SrcPlanes) delete SrcPlanes;
    if (DstPlanes) delete DstPlanes;
}

PVideoFrame __stdcall MVShow::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame src = child->GetFrame(n, env);
    PVideoFrame dst = env->NewVideoFrame(vi);

    // convert to frames
    unsigned char const *pSrc[3];
    unsigned char *pDst[3];
    int nDstPitches[3], nSrcPitches[3];
    SrcPlanes->ConvertVideoFrameToPlanes(&src, pSrc, nSrcPitches);
    DstPlanes->ConvertVideoFrameToPlanes(&dst, pDst, nDstPitches);

    PVideoFrame mvn = mvClip.GetFrame(n, env);
    mvClip.Update(mvn, env);

    // Copy the frame into the created frame
    if ( !nPlane )
        PlaneCopy(pDst[0], nDstPitches[0], pSrc[0], nSrcPitches[0], nWidth, nHeight, isse);

    if ( mvClip.IsUsable() )
        DrawMVs(pDst[0], nDstPitches[0], pSrc[0], nSrcPitches[0]);

    // copy the chroma planes
    PlaneCopy(pDst[1], nDstPitches[1], pSrc[1], nSrcPitches[1], nWidth >> 1, nHeight /yRatioUV, isse);
    PlaneCopy(pDst[2], nDstPitches[2], pSrc[2], nSrcPitches[2], nWidth >> 1, nHeight /yRatioUV, isse);

    // convert back from planes
    unsigned char *pDstYUY2 = dst->GetWritePtr();
    int nDstPitchYUY2 = dst->GetPitch();
    DstPlanes->YUY2FromPlanes(pDstYUY2, nDstPitchYUY2);

    if ( showSad ) {
        char buf[80];
        double mean = 0;
        for ( int i = 0; i < mvClip.GetBlkCount(); ++i ) {
            int sad = mvClip.GetBlock(0, i).GetSAD();
            //		 DebugPrintf("i= %d, SAD= %d", i, sad);
            mean += sad;
        }
        sprintf_s(buf, 80, "%f", mean / nBlkCount);
        if ( (pixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2 )
            DrawStringYUY2(dst, 0, 0, buf);
        else
            DrawString(dst, 0, 0, buf);
    }
    return dst;
}

void MVShow::DrawPixel(unsigned char *pDst, int nDstPitch, int x, int y, int w, int h)
{
    if (( x >= 0 ) && ( x < w ) && ( y >= 0 ) && ( y < h ))
        pDst[x + y * nDstPitch] = 255;
}

// Draw the vector, scaled with the right scalar factor.
void MVShow::DrawMV(unsigned char *pDst, int nDstPitch, int scale,
			        int x, int y, int sizex, int sizey, int w, int h, VECTOR vector, int pel)
{
    int x0 =  x; // changed in v1.8, now pDdst is the center of first block
    int y0 =  y;

    bool yLonger = false;
    int incrementVal, endVal;
    int shortLen =  scale*vector.y / pel; // added missed scale - Fizick
    int longLen = scale*vector.x / pel;   //
    if ( abs(shortLen) > abs(longLen)) {
        int swap = shortLen;
        shortLen = longLen;
        longLen = swap;
        yLonger = true;
    }
    endVal = longLen;
    if (longLen < 0) {
        incrementVal = -1;
        longLen = -longLen;
    } else incrementVal = 1;
    int decInc;
    if (longLen==0) decInc=0;
    else decInc = (shortLen << 16) / longLen;
    int j=0;
    if (yLonger) {
        for (int i = 0; i != endVal; i += incrementVal) {
            DrawPixel(pDst, nDstPitch, x0 + (j >> 16), y0 + i, w, h);
            j += decInc;
        }
    } else {
        for (int i = 0; i != endVal; i += incrementVal) {
            DrawPixel(pDst, nDstPitch, x0 + i, y0 + (j>> 16), w, h);
            j += decInc;
        }
    }
}

void MVShow::DrawMVs(unsigned char *pDst, int nDstPitch, const unsigned char *pSrc,
                     int nSrcPitch)
{
    const FakePlaneOfBlocks &plane = mvClip.GetPlane(nPlane);
    int effectiveScale = plane.GetEffectiveScale();

    const unsigned char *s = pSrc;

    if ( nPlane ) {
        for ( int j = 0; j < plane.GetHeight(); ++j) {
            for ( int i = 0; i < plane.GetWidth(); ++i) {
                int val = 0;
                for ( int x = i * effectiveScale ; x < effectiveScale * (i + 1); ++x )
                    for ( int y = j * effectiveScale; y < effectiveScale * (j + 1); ++y )
                        val += pSrc[x + y * nSrcPitch];
                val = (val + effectiveScale * effectiveScale / 2) / (effectiveScale * effectiveScale);
                for ( int x = i * effectiveScale ; x < effectiveScale * (i + 1); ++x )
                    for ( int y = j * effectiveScale; y < effectiveScale * (j + 1); ++y )
                        pDst[x + y * nDstPitch] = val;
            }
            s += effectiveScale * nSrcPitch;
        }
    }
    for ( int i = 0; i < plane.GetBlockCount(); ++i )
        if ( plane[i].GetSAD() < nTolerance )
            DrawMV(pDst + plane.GetBlockSizeX() / 2 * effectiveScale + plane.GetBlockSizeY()/ 2 * effectiveScale*nDstPitch, // changed in v1.8, now address is the center of first block
                   nDstPitch, nScale * effectiveScale, plane[i].GetX() * effectiveScale,
                   plane[i].GetY() * effectiveScale, (plane.GetBlockSizeX() - plane.GetOverlapX())* effectiveScale,
                   (plane.GetBlockSizeY() - plane.GetOverlapY())* effectiveScale,
                   plane.GetWidth() * effectiveScale, plane.GetHeight() * effectiveScale,
                   plane[i].GetMV(), plane.GetPel());
}
