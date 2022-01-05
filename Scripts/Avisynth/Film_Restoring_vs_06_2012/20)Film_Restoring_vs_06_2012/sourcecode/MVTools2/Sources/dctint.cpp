// DCT calculation with integer dct asm
// Copyright(c)2006 A.G.Balakhnin aka Fizick

// Code mostly borrowed from DCTFilter plugin by Tom Barry
// Copyright (c) 2002 Tom Barry.  All rights reserved.
//      trbarry@trbarry.com

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

#include "dctint.h"
//#include "stdio.h"
#include "malloc.h"

#define DCTSHIFT 2 // 2 is 4x decreasing,
// only DC component need in shift 4 (i.e. 16x) for real images

DCTINT::DCTINT(int _sizex, int _sizey, int _dctmode) 

{
	// only 8x8 implemented !
		sizex = _sizex;
		sizey = _sizey;
		dctmode = _dctmode;
		int size2d = sizey*sizex;

//		int cursize = 1;
//		dctshift = 0;
//		while (cursize < size2d) 
//		{
//			dctshift++;
//			cursize = (cursize<<1);
//		}
		

		pWorkArea = (short * const) _aligned_malloc(8*8*2, 128);


}



DCTINT::~DCTINT()
{
	_aligned_free(pWorkArea);
}

void DCTINT::DCTBytes2D(const unsigned char *srcp, int src_pit, unsigned char *dstp, int dst_pit)
{
	if ( (sizex != 8) || (sizey != 8) ) // may be sometime we will implement 4x4 and 16x16 dct
		return;

	int rowsize = sizex;
	int FldHeight = sizey;
	int dctshift0ext = 4 - DCTSHIFT;// for DC component


const BYTE* pSrc = srcp;
const BYTE* pSrcW;
BYTE* pDest = dstp;
BYTE* pDestW;

const unsigned __int64 i128 = 0x8080808080808080;

int y;
int ct = (rowsize/8);
int ctW;
//short* pFactors = &factors[0][0];
short* pWorkAreaW = pWorkArea;			// so inline asm can access

	for (y=0; y <= FldHeight-8; y+=8)	
	{
		pSrcW = pSrc;
		pDestW = pDest;
		ctW = ct;
		__asm
		{
		// Loop general reg usage
		//
		// ecx - inner loop counter
		// ebx - src pitch
		// edx - dest pitch
		// edi - dest
		// esi - src pixels

// SPLIT Qword

		align 16
LoopQ:	
		mov		esi, pSrcW   
		lea		eax, [esi+8]
		mov		pSrcW, eax					// save for next time
		mov		ebx, src_pit 
		mov		eax, pWorkAreaW

		// expand bytes to words in work area
		pxor	mm7, mm7
		
		movq	mm0, qword ptr[esi]			// # 0
		movq	mm2, qword ptr[esi+ebx]		// # 1

		movq	mm1, mm0
		punpcklbw mm0, mm7					// low bytes to words
		movq	qword ptr[eax], mm0			// low words to work area
		punpckhbw mm1, mm7					// high bytes to words
		movq	qword ptr[eax+8], mm1		// high words to work area
		
		movq	mm3, mm2
		punpcklbw mm2, mm7					// low bytes to words
		movq	qword ptr[eax+1*16], mm2	// low words to work area
		punpckhbw mm3, mm7					// high bytes to words
		movq	qword ptr[eax+1*16+8], mm3	// high words to work area
		
		lea		esi, [esi+2*ebx]

		movq	mm0, qword ptr[esi]			// #2
		movq	mm2, qword ptr[esi+ebx]		// #3

		movq	mm1, mm0
		punpcklbw mm0, mm7					// low bytes to words
		movq	qword ptr[eax+2*16], mm0	// low words to work area
		punpckhbw mm1, mm7					// high bytes to words
		movq	qword ptr[eax+2*16+8], mm1	// high words to work area
		
		movq	mm3, mm2
		punpcklbw mm2, mm7					// low bytes to words
		movq	qword ptr[eax+3*16], mm2	// low words to work area
		punpckhbw mm3, mm7					// high bytes to words
		movq	qword ptr[eax+3*16+8], mm3	// high words to work area
		
		lea		esi, [esi+2*ebx]

		movq	mm0, qword ptr[esi]			// #4
		movq	mm2, qword ptr[esi+ebx]		// #5

		movq	mm1, mm0
		punpcklbw mm0, mm7					// low bytes to words
		movq	qword ptr[eax+4*16], mm0	// low words to work area
		punpckhbw mm1, mm7					// high bytes to words
		movq	qword ptr[eax+4*16+8], mm1	// high words to work area
		
		movq	mm3, mm2
		punpcklbw mm2, mm7					// low bytes to words
		movq	qword ptr[eax+5*16], mm2	// low words to work area
		punpckhbw mm3, mm7					// high bytes to words
		movq	qword ptr[eax+5*16+8], mm3	// high words to work area
		
		lea		esi, [esi+2*ebx]

		movq	mm0, qword ptr[esi]			// #6
		movq	mm2, qword ptr[esi+ebx]		// #7

		movq	mm1, mm0
		punpcklbw mm0, mm7					// low bytes to words
		movq	qword ptr[eax+6*16], mm0	// low words to work area
		punpckhbw mm1, mm7					// high bytes to words
		movq	qword ptr[eax+6*16+8], mm1	// high words to work area
		
		movq	mm3, mm2
		punpcklbw mm2, mm7					// low bytes to words
		movq	qword ptr[eax+7*16], mm2	// low words to work area
		punpckhbw mm3, mm7					// high bytes to words
		movq	qword ptr[eax+7*16+8], mm3	// high words to work area	
		}


		
		fdct_mmx(pWorkAreaW);					// go do forward DCT

		_asm 
		{
		// decrease dc component
			mov	eax, pWorkAreaW;
			mov ebx, [eax];
			mov ecx, dctshift0ext;
			sar bx, cl;
			mov [eax], ebx;
		}
		// decrease all components

		// lets adjust some of the DCT components
//		DO_ADJUST(pWorkAreaW);

		_asm
		{

		movq mm7, i128 // prepare constant: bytes sign bits

		mov		edi, pDestW					// got em, now store em
		lea		eax, [edi+8]
		mov		pDestW, eax					// save for next time
		mov		edx, dst_pit 

		mov		eax, pWorkAreaW
		// collapse words to bytes in dest

		movq	mm0, qword ptr[eax]			// low qwords
		movq	mm1, qword ptr[eax+1*16]
		movq	mm2, qword ptr[eax+8]			// high qwords
		movq	mm3, qword ptr[eax+1*16+8]
		// decrease by bits shift from (-2047, +2047) to 
		psraw	mm0, DCTSHIFT  
		psraw	mm1, DCTSHIFT
		psraw	mm2, DCTSHIFT
		psraw	mm3, DCTSHIFT
		// to bytes with signed saturation
		packsswb mm0, mm2		
		packsswb mm1, mm3
		// convert to unsigned (0, 255) by adding 128
		pxor mm0, mm7     
		pxor mm1, mm7
		// store to dest
		movq	qword ptr[edi], mm0	
		movq	qword ptr[edi+edx], mm1

		//NEXT
		movq	mm0, qword ptr[eax+2*16]			// low qwords
		movq	mm1, qword ptr[eax+3*16]
		movq	mm2, qword ptr[eax+2*16+8]			// high qwords
		movq	mm3, qword ptr[eax+3*16+8]
		// decrease by bits shift from (-2047, +2047) to 
		psraw	mm0, DCTSHIFT  
		psraw	mm1, DCTSHIFT
		psraw	mm2, DCTSHIFT
		psraw	mm3, DCTSHIFT
		// to bytes with signed saturation
		packsswb mm0, mm2		
		packsswb mm1, mm3
		// convert to unsigned (0, 255) by adding 128
		pxor mm0, mm7     
		pxor mm1, mm7
		// store to dest
		lea		edi, [edi+2*edx]
		movq	qword ptr[edi], mm0	
		movq	qword ptr[edi+edx], mm1

		//NEXT
		movq	mm0, qword ptr[eax+4*16]			// low qwords
		movq	mm1, qword ptr[eax+5*16]
		movq	mm2, qword ptr[eax+4*16+8]			// high qwords
		movq	mm3, qword ptr[eax+5*16+8]
		// decrease by bits shift from (-2047, +2047) to 
		psraw	mm0, DCTSHIFT  
		psraw	mm1, DCTSHIFT
		psraw	mm2, DCTSHIFT
		psraw	mm3, DCTSHIFT
		// to bytes with signed saturation
		packsswb mm0, mm2		
		packsswb mm1, mm3
		// convert to unsigned (0, 255) by adding 128
		pxor mm0, mm7     
		pxor mm1, mm7
		// store to dest
		lea		edi, [edi+2*edx]
		movq	qword ptr[edi], mm0	
		movq	qword ptr[edi+edx], mm1

		//NEXT
		movq	mm0, qword ptr[eax+6*16]			// low qwords
		movq	mm1, qword ptr[eax+7*16]
		movq	mm2, qword ptr[eax+6*16+8]			// high qwords
		movq	mm3, qword ptr[eax+7*16+8]
		// decrease by bits shift from (-2047, +2047) to 
		psraw	mm0, DCTSHIFT  
		psraw	mm1, DCTSHIFT
		psraw	mm2, DCTSHIFT
		psraw	mm3, DCTSHIFT
		// to bytes with signed saturation
		packsswb mm0, mm2		
		packsswb mm1, mm3
		// convert to unsigned (0, 255) by adding 128
		pxor mm0, mm7     
		pxor mm1, mm7
		// store to dest
		lea		edi, [edi+2*edx]
		movq	qword ptr[edi], mm0	
		movq	qword ptr[edi+edx], mm1

	
		dec		ctW
		jnz		LoopQ

		}

	// adjust for next line
	pSrc  += 8*src_pit;
	pDest += 8*dst_pit;
	}
	_asm emms;
}
