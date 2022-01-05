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

YUY2Planes::YUY2Planes(int _nWidth, int _nHeight)
{
	nWidth = _nWidth;
	nHeight = _nHeight;
	srcPitch = (nWidth + 15) & (~15);
	srcPitchUV = (nWidth/2 + 15) & (~15); //v 1.2.1
	pSrc = (unsigned char*) _aligned_malloc(srcPitch*nHeight,128);   //v 1.2.1
	pSrcU = (unsigned char*) _aligned_malloc(srcPitchUV*nHeight,128);   
	pSrcV = (unsigned char*) _aligned_malloc(srcPitchUV*nHeight,128);

}   

YUY2Planes::~YUY2Planes()
{
	_aligned_free (pSrc);
	_aligned_free (pSrcU);
	_aligned_free (pSrcV);
}

//----------------------------------------------------------------------------------------------
// v1.2.1 asm code borrowed from AviSynth 2.6 CVS
// ConvertPlanar (c) 2005 by Klaus Post

void YUY2Planes::convYUV422to422(const unsigned char *src, unsigned char *py, unsigned char *pu, unsigned char *pv,
                                        int pitch1, int pitch2y, int pitch2uv, int width, int height)
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



void YUY2Planes::conv422toYUV422(const unsigned char *py, const unsigned char *pu, const unsigned char *pv,
                           unsigned char *dst, int pitch1Y, int pitch1UV, int pitch2, int width, int height)
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

void YUY2ToPlanes(const unsigned char *pSrcYUY2, int nSrcPitchYUY2, int nWidth, int _nHeight,
							  const unsigned char * pSrc0, int _srcPitch,
							  const unsigned char * pSrcU0, const unsigned char * pSrcV0, int _srcPitchUV, bool mmx)
{
	unsigned char * pSrc1 = (unsigned char *) pSrc0;
	unsigned char * pSrcU1 = (unsigned char *) pSrcU0;
	unsigned char * pSrcV1 = (unsigned char *) pSrcV0;

	const int awidth = min(nSrcPitchYUY2>>1, (nWidth+7) & -8);
//mmx=false;
	if (!(awidth&7) && mmx) {  // Use MMX

		YUY2Planes::convYUV422to422(pSrcYUY2, pSrc1, pSrcU1, pSrcV1, nSrcPitchYUY2, _srcPitch, _srcPitchUV,  awidth, _nHeight);
	}
	else
	{

	for (int h=0; h<_nHeight; h++)
	{
		for (int w=0; w<nWidth; w+=2)
		{
			int w2 = w+w;
			pSrc1[w] = pSrcYUY2[w2];
			pSrcU1[w>>1] = pSrcYUY2[w2+1];
			pSrc1[w+1] = pSrcYUY2[w2+2];
			pSrcV1[w>>1] = pSrcYUY2[w2+3];
		}
		pSrc1 += _srcPitch;
		pSrcU1 += _srcPitchUV;
		pSrcV1 += _srcPitchUV;
		pSrcYUY2 += nSrcPitchYUY2;
	}
	}
}

void YUY2FromPlanes(unsigned char *pSrcYUY2, int nSrcPitchYUY2, int nWidth, int nHeight,
							  unsigned char * pSrc1, int _srcPitch,
							  unsigned char * pSrcU1, unsigned char * pSrcV1, int _srcPitchUV, bool mmx)
{
	const int awidth = min(_srcPitch, (nWidth+7) & -8);
//mmx=false;
  if (!(awidth&7) && mmx) {  // Use MMX
    YUY2Planes::conv422toYUV422(pSrc1, pSrcU1, pSrcV1, pSrcYUY2, _srcPitch, _srcPitchUV, nSrcPitchYUY2, awidth, nHeight);
  }
  else
  {

  for (int h=0; h<nHeight; h++)
	{
		for (int w=0; w<nWidth; w+=2)
		{
			int w2 = w+w;
			pSrcYUY2[w2] = pSrc1[w];
			pSrcYUY2[w2+1] = pSrcU1[w>>1];
			pSrcYUY2[w2+2] = pSrc1[w+1];
			pSrcYUY2[w2+3] = pSrcV1[w>>1];
		}
		pSrc1 += _srcPitch;
		pSrcU1 += _srcPitchUV;
		pSrcV1 += _srcPitchUV;
		pSrcYUY2 += nSrcPitchYUY2;
	}
  }
}

