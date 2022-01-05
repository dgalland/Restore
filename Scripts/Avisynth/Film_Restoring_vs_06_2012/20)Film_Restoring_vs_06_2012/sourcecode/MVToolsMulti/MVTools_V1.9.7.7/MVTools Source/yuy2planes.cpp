// YUY2 <-> planar
// Copyright(c)2005 A.G.Balakhnin aka Fizick

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License.
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

#include "windows.h" // min
#include "yuy2planes.h"
#include "avisynth.h"

#include <algorithm>

#pragma warning (disable: 4800) // disable bool performance warning

YUY2Planes::YUY2Planes(int const _nWidth, int const _nHeight, int const _PixelType, bool const _isse) : 
            nWidth(_nWidth), nHeight(_nHeight), 
            bYUY2Type(static_cast<bool>((_PixelType & VideoInfo::CS_YUY2) == VideoInfo::CS_YUY2)),
            isse(_isse)
{    
    if (bYUY2Type) {
        // round pitches to be mod 16
        unsigned int PitchAlignment=16;
        srcPitch   = (nWidth   + (PitchAlignment-1)) & (~(PitchAlignment-1));
        srcPitchUV = (nWidth/2 + (PitchAlignment-1)) & (~(PitchAlignment-1)); //v 1.2.1
     
        // round plane sizes to be mod 128
        unsigned int PlaneSizeAlignment=128;
        int YPlaneSize =(srcPitch*nHeight   + (PlaneSizeAlignment-1)) & (~(PlaneSizeAlignment-1));
        int UVPlaneSize=(srcPitchUV*nHeight + (PlaneSizeAlignment-1)) & (~(PlaneSizeAlignment-1));

        pSrcBase  = (unsigned char*) _aligned_malloc(YPlaneSize+2*UVPlaneSize, PlaneSizeAlignment);
        if (pSrcBase) {
            pSrcUBase = pSrcBase  + YPlaneSize;   // pointer past Y plane
            pSrcVBase = pSrcUBase + UVPlaneSize;  // pointer past U plane
        }
        else {
            // no memory 
            throw "Out of memroy YUY2Planes.cpp";
        }
    }
    else {
        // plane is really dummy plane
        srcPitch=0;
        srcPitchUV=0;
        pSrcBase=pSrcUBase=pSrcVBase=0;
    }
}   

YUY2Planes::~YUY2Planes()
{
    if (pSrcBase) _aligned_free(pSrcBase);
}

void YUY2Planes::YUY2ToPlanes(unsigned char const *pSrcYUY2, int const nSrcPitchYUY2)
{
    if (bYUY2Type) {
        // get planes
        unsigned char *pSrc=GetPtr();
        unsigned char *pSrcU=GetPtrU();
        unsigned char *pSrcV=GetPtrV();  
        int srcPitch=GetPitch();
        int srcPitchUV=GetPitchUV();

        // width is minimum of nSrcPitchYUY2/2 and width rounded to next highest mod 8
        unsigned int const CopyAlignment=8;
        unsigned int const awidth = std::min<unsigned int>(static_cast<unsigned int>(nSrcPitchYUY2)>>1, 
                                                           (static_cast<unsigned int>(nWidth) + 
                                                           (CopyAlignment-1)) & ~(CopyAlignment-1));
        if (!(awidth & (CopyAlignment-1)) && isse) {  // Use MMX if width is mod 8
            YUY2Planes::convYUV422to422(pSrcYUY2, pSrc, pSrcU, pSrcV, nSrcPitchYUY2, srcPitch, srcPitchUV, awidth, nHeight);
        }
        else{
            int SrcDelta     = srcPitch     -nWidth;
            int SrcUVDelta   = srcPitchUV   -(nWidth>>1);
            int SrcYUY2Delta = nSrcPitchYUY2-(nWidth<<1);

            for (int h=0; h<nHeight; ++h) {
                for (int w=0; w<nWidth; w+=2) {
                    (*pSrc++)  = (*pSrcYUY2++);
                    (*pSrcU++) = (*pSrcYUY2++);
                    (*pSrc++)  = (*pSrcYUY2++);
                    (*pSrcV++) = (*pSrcYUY2++);
                }
                pSrc     += SrcDelta;
                pSrcU    += SrcUVDelta;
                pSrcV    += SrcUVDelta;
                pSrcYUY2 += SrcYUY2Delta;
            }
        }
    }
}

void YUY2Planes::YUY2FromPlanes(unsigned char *pSrcYUY2, int const nSrcPitchYUY2)
{
    if (bYUY2Type) {
        // get planes
        unsigned char *pSrc=GetPtr();
        unsigned char *pSrcU=GetPtrU();
        unsigned char *pSrcV=GetPtrV();  
        int srcPitch=GetPitch();
        int srcPitchUV=GetPitchUV();

        // width is minimum of nSrcPitchYUY2/2 and width rounded to next highest mod 8
        // width is minimum of nSrcPitchYUY2/2 and width rounded to next highest mod 8
        unsigned int const CopyAlignment=8;
        unsigned int const awidth = std::min<unsigned int>(static_cast<unsigned int>(nSrcPitchYUY2)>>1, 
                                                           (static_cast<unsigned int>(nWidth) + 
                                                           (CopyAlignment-1)) & ~(CopyAlignment-1));
        if (!(awidth & (CopyAlignment-1)) && isse) {  // Use MMX if width is mod 8
            YUY2Planes::conv422toYUV422(pSrc, pSrcU, pSrcV, pSrcYUY2, srcPitch, srcPitchUV, nSrcPitchYUY2, awidth, nHeight);
        }
        else {
            int SrcDelta     = srcPitch     -nWidth;
            int SrcUVDelta   = srcPitchUV   -(nWidth>>1);
            int SrcYUY2Delta = nSrcPitchYUY2-(nWidth<<1);

            for (int h=0; h<nHeight; ++h) {
                for (int w=0; w<nWidth; w+=2) {
                    (*pSrcYUY2++) = (*pSrc++);
                    (*pSrcYUY2++) = (*pSrcU++);
                    (*pSrcYUY2++) = (*pSrc++);
                    (*pSrcYUY2++) = (*pSrcV++);
                }
                pSrc     += SrcDelta;
                pSrcU    += SrcUVDelta;
                pSrcV    += SrcUVDelta;
                pSrcYUY2 += SrcYUY2Delta;
            }
        }
    }
}

// this converts read only frames to YUY2 if needed; and returns pointers to planes and pitches
void YUY2Planes::ConvertVideoFrameToPlanes(PVideoFrame *pVF, unsigned char const *VFPtrs[3], int *VFPitches)
{
     if (bYUY2Type) {
        // fill the planes
        YUY2ToPlanes((*pVF)->GetReadPtr(), (*pVF)->GetPitch());
        // return pointers and pitches
        VFPtrs[0] = GetPtr();
        VFPtrs[1] = GetPtrU();
        VFPtrs[2] = GetPtrV();
        VFPitches[0] = GetPitch();
        VFPitches[1] = GetPitchUV();
        VFPitches[2] = GetPitchUV();
     }
     else {
        // return pointers and pitches
        VFPtrs[0] = (*pVF)->GetReadPtr();
        VFPtrs[1] = (*pVF)->GetReadPtr(PLANAR_U);
        VFPtrs[2] = (*pVF)->GetReadPtr(PLANAR_V);
        VFPitches[0] = (*pVF)->GetPitch();
        VFPitches[1] = (*pVF)->GetPitch(PLANAR_U);
        VFPitches[2] = (*pVF)->GetPitch(PLANAR_V);
     }
}

// this converts write only to YUY2 if needed; and returns pointers to planes and pitches
void YUY2Planes::ConvertVideoFrameToPlanes(PVideoFrame *pVF, unsigned char *VFPtrs[3], int *VFPitches)
{
     if ((bYUY2Type) ) {
        // fill the planes
        YUY2ToPlanes((*pVF)->GetReadPtr(), (*pVF)->GetPitch());
        // return pointers and pitches
        VFPtrs[0] = GetPtr();
        VFPtrs[1] = GetPtrU();
        VFPtrs[2] = GetPtrV();
        VFPitches[0] = GetPitch();
        VFPitches[1] = GetPitchUV();
        VFPitches[2] = GetPitchUV();
     }
     else {
        // return pointers and pitches
        VFPtrs[0] = (*pVF)->GetWritePtr();
        VFPtrs[1] = (*pVF)->GetWritePtr(PLANAR_U);
        VFPtrs[2] = (*pVF)->GetWritePtr(PLANAR_V);
        VFPitches[0] = (*pVF)->GetPitch();
        VFPitches[1] = (*pVF)->GetPitch(PLANAR_U);
        VFPitches[2] = (*pVF)->GetPitch(PLANAR_V);
     }
}

//----------------------------------------------------------------------------------------------
// v1.2.1 asm code borrowed from AviSynth 2.6 CVS
// ConvertPlanar (c) 2005 by Klaus Post

void YUY2Planes::convYUV422to422(unsigned char const *src, unsigned char *py, unsigned char *pu, unsigned char *pv,
                                 int const pitch1, int const pitch2y, int const pitch2uv, int const width, int const height)
{
    int widthdiv2 = width>>1;
    __int64 Ymask = 0x00FF00FF00FF00FFi64;
    __asm
    {
    push ebx
    mov edi,[src]
    mov ebx,[py]
    mov edx,[pu]
    mov esi,[pv]
    mov ecx,widthdiv2
    movq mm5,Ymask
    yloop:
    xor eax,eax
    align 16
    xloop:
    movq mm0,[edi+eax*4]   ; VYUYVYUY - 1
    movq mm1,[edi+eax*4+8] ; VYUYVYUY - 2
    movq mm2,mm0           ; VYUYVYUY - 1
    movq mm3,mm1           ; VYUYVYUY - 2
    pand mm0,mm5           ; 0Y0Y0Y0Y - 1
    psrlw mm2,8        ; 0V0U0V0U - 1
    pand mm1,mm5           ; 0Y0Y0Y0Y - 2
    psrlw mm3,8            ; 0V0U0V0U - 2
    packuswb mm0,mm1       ; YYYYYYYY
    packuswb mm2,mm3       ; VUVUVUVU
    movq [ebx+eax*2],mm0   ; store y
    movq mm4,mm2           ; VUVUVUVU
    pand mm2,mm5           ; 0U0U0U0U
    psrlw mm4,8            ; 0V0V0V0V
    packuswb mm2,mm2       ; xxxxUUUU
    packuswb mm4,mm4       ; xxxxVVVV
    movd [edx+eax],mm2     ; store u
    add eax,4
    cmp eax,ecx
    movd [esi+eax-4],mm4   ; store v
    jl xloop
    add edi,pitch1
    add ebx,pitch2y
    add edx,pitch2uv
    add esi,pitch2uv
    dec height
    jnz yloop
    emms
    pop ebx
    }
}

void YUY2Planes::conv422toYUV422(unsigned char const *py, unsigned char const *pu, unsigned char const *pv,
                                 unsigned char *dst, int const pitch1Y, int const pitch1UV, int const pitch2, 
                                 int const width, int const height)
{
    int widthdiv2 = width >> 1;
    __asm
    {
    push ebx
    mov ebx,[py]
    mov edx,[pu]
    mov esi,[pv]
    mov edi,[dst]
    mov ecx,widthdiv2
    yloop:
    xor eax,eax
    align 16
    xloop:
    movd mm1,[edx+eax]     ;0000UUUU
    movd mm2,[esi+eax]     ;0000VVVV
    movq mm0,[ebx+eax*2]   ;YYYYYYYY
    punpcklbw mm1,mm2      ;VUVUVUVU
    movq mm3,mm0           ;YYYYYYYY
    punpcklbw mm0,mm1      ;VYUYVYUY
    add eax,4
    punpckhbw mm3,mm1      ;VYUYVYUY
    movq [edi+eax*4-16],mm0 ;store ; bug fixed in v1.2.2
    cmp eax,ecx
    movq [edi+eax*4-8],mm3   ;store  ; bug fixed in v1.2.2
    jl xloop
    add ebx,pitch1Y
    add edx,pitch1UV
    add esi,pitch1UV
    add edi,pitch2
    dec height
    jnz yloop
    emms
    pop ebx
    }
}

//----------------------------------------------------------------------------------------------