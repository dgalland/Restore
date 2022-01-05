;/****************************************************************************
; * $Id$
; *
; ***************************************************************************/
; assembler (NASM) block overlap window functions
;// Copyright (c)2006 Fizick
;// This program is free software; you can redistribute it and/or modify
;// it under the terms of the GNU General Public License as published by
;// the Free Software Foundation; either version 2 of the License, or
;// (at your option) any later version.
;//
;// This program is distributed in the hope that it will be useful,
;// but WITHOUT ANY WARRANTY; without even the implied warranty of
;// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;// GNU General Public License for more details.
;//
;// You should have received a copy of the GNU General Public License
;// along with this program; if not, write to the Free Software
;// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
;// http://www.gnu.org/copyleft/gpl.html .

BITS 32

%macro cglobal 1
	%ifdef PREFIX
		global _%1
		%define %1 _%1
	%else
		global %1
	%endif
%endmacro

;=============================================================================
; Read only data
;=============================================================================

SECTION .rodata data align=16

;=============================================================================
; Macros
;=============================================================================

%macro OVERSLAPS2 1 ; height
ALIGN 16
Overlaps2x%1_mmx:		; might be mmxext, but no SSE used, added by TSchniede
		push edi
		push esi
		push ebx
;load parameter
		mov edi, [esp + 16] ; pDst
		mov edx, [esp + 20] ; nDstPitch
		add edx, edx ; dst pitch in bytes (*2) byte->words
		mov esi, [esp + 24] ; pSrc
		mov ecx, [esp + 28] ; nSrcPitch
		mov ebx, [esp + 32] ; pWin
		mov eax, [esp + 36] ; nWinPitch
		add eax, eax ; win pitch in bytes (*2) byte->words

	; prepare constants
		pcmpeqd mm6, mm6; = FFFFFFFF		; Fizick
		psrlw mm6, 15	; = 1 in all 4 words (16 bit)
		psllw mm6, 4	; *16 (sqrt(256)...
		pxor mm7,mm7
%assign i 0
%rep %1
	; overlap
%if i > 0
		add edi, edx
		add esi, ecx
		add ebx, eax
%endif
		movd mm0, [esi]		; 2 (4) bytes pSrc (2 values + 2 rubbish)
		punpcklbw mm0, mm7	; convert bytes to words (mm7=0)
		movd mm1, [ebx]		; 4 (8) bytes pWin (2 values)

		punpcklwd mm0, mm6	; interleave with 16 (16*16 = 256)
		punpcklwd mm1, mm6

		pmaddwd mm0, mm1	; (pSrc*pWin)+(16*16), wraparound impossible (256 too small and pSrc is extended...)
		psrld mm0, 6		; >>6

		packssdw mm0, mm1	; dwords to words (the mm1 is a "any register" placeholder)
		movd mm1, [edi]		; old pDst
		paddusw mm0, mm1	; add to pDst
        movd [edi], mm0		; write low 2 values
%assign i i+1
%endrep
		pop ebx
		pop esi
		pop edi

		ret
%endmacro

%macro OVERS4 0
		movd mm0, [esi]		; 4 bytes pSrc
		punpcklbw mm0, mm7	; convert bytes to words (mm7=0)
		movq mm1, [ebx]		; 8 bytes pWin (4 values)
		movq mm2, mm0
		pmullw mm0, mm1		; pSrc*pWin
		pmulhw mm2, mm1		; pSrc*pWin high
		movq mm1, mm0
		punpcklwd mm0, mm2
		punpckhwd mm1, mm2
		paddd mm0, mm6	; + 256 (=mm6)
		paddd mm1, mm6	; + 256 (=mm6)
		psrld mm0, 6		; >>6
		psrld mm1, 6		; >>6
		packssdw mm0, mm1	; dwords to words
		movq mm1, [edi]		; old pDst
		paddusw mm0, mm1	; add to pDst
        movq [edi], mm0		; write low 4 values
		add edi, edx
		add esi, ecx
		add ebx, eax
%endmacro

%macro OVERS8 0
		movd mm0, [esi]		; 4 bytes pSrc
		punpcklbw mm0, mm7	; convert bytes to words (mm7=0)
		movq mm1, [ebx]		; 8 bytes pWin (4 values)
		movq mm2, mm0
		pmullw mm0, mm1		; pSrc*pWin
		pmulhw mm2, mm1		; pSrc*pWin high
		movq mm1, mm0
		punpcklwd mm0, mm2
		punpckhwd mm1, mm2
		paddd mm0, mm6	; + 256 (=mm6)
		paddd mm1, mm6	; + 256 (=mm6)
		psrld mm0, 6		; >>6
		psrld mm1, 6		; >>6
		packssdw mm0, mm1	; dwords to words
		movq mm1, [edi]		; old pDst
		paddusw mm0, mm1	; add to pDst
        movq [edi], mm0		; write low 4 values

		movd mm0, [esi+4]		; 4 bytes pSrc
		punpcklbw mm0, mm7	; convert bytes to words (mm7=0)
		movq mm1, [ebx+8]		; 8 bytes pWin (4 values)
		movq mm2, mm0
		pmullw mm0, mm1		; pSrc*pWin
		pmulhw mm2, mm1		; pSrc*pWin high
		movq mm1, mm0
		punpcklwd mm0, mm2
		punpckhwd mm1, mm2
		paddd mm0, mm6	; + 256 (=mm6)
		paddd mm1, mm6	; + 256 (=mm6)
		psrld mm0, 6		; >>6
		psrld mm1, 6		; >>6
		packssdw mm0, mm1	; dwords to words
		movq mm1, [edi+8]	; old pDst
		paddusw mm0, mm1	; add to pDst
        movq [edi+8], mm0	; write low 4 values
		add edi, edx
		add esi, ecx
		add ebx, eax
%endmacro

%macro OVERS16 0
		movd mm0, [esi]		; 4 bytes pSrc
		punpcklbw mm0, mm7	; convert bytes to words (mm7=0)
		movq mm1, [ebx]		; 8 bytes pWin (4 values)
		movq mm2, mm0
		pmullw mm0, mm1		; pSrc*pWin
		pmulhw mm2, mm1		; pSrc*pWin high
		movq mm1, mm0
		punpcklwd mm0, mm2
		punpckhwd mm1, mm2
		paddd mm0, mm6	; + 256 (=mm6)
		paddd mm1, mm6	; + 256 (=mm6)
		psrld mm0, 6		; >>6
		psrld mm1, 6		; >>6
		packssdw mm0, mm1	; dwords to words
		movq mm1, [edi]		; old pDst
		paddusw mm0, mm1	; add to pDst
        movq [edi], mm0		; write low 4 values

		movd mm0, [esi+4]		; 4 bytes pSrc
		punpcklbw mm0, mm7	; convert bytes to words (mm7=0)
		movq mm1, [ebx+8]		; 8 bytes pWin (4 values)
		movq mm2, mm0
		pmullw mm0, mm1		; pSrc*pWin
		pmulhw mm2, mm1		; pSrc*pWin high
		movq mm1, mm0
		punpcklwd mm0, mm2
		punpckhwd mm1, mm2
		paddd mm0, mm6	; + 256 (=mm6)
		paddd mm1, mm6	; + 256 (=mm6)
		psrld mm0, 6		; >>6
		psrld mm1, 6		; >>6
		packssdw mm0, mm1	; dwords to words
		movq mm1, [edi+8]		; old pDst
		paddusw mm0, mm1	; add to pDst
        movq [edi+8], mm0		; write low 4 bytes

		movd mm0, [esi+8]		; 4 bytes pSrc
		punpcklbw mm0, mm7	; convert bytes to words (mm7=0)
		movq mm1, [ebx+16]		; 8 bytes pWin (4 values)
		movq mm2, mm0
		pmullw mm0, mm1		; pSrc*pWin
		pmulhw mm2, mm1		; pSrc*pWin high
		movq mm1, mm0
		punpcklwd mm0, mm2
		punpckhwd mm1, mm2
		paddd mm0, mm6	; + 256 (=mm6)
		paddd mm1, mm6	; + 256 (=mm6)
		psrld mm0, 6		; >>6
		psrld mm1, 6		; >>6
		packssdw mm0, mm1	; dwords to words
		movq mm1, [edi+16]		; old pDst
		paddusw mm0, mm1	; add to pDst
        movq [edi+16], mm0		; write low 4 values

		movd mm0, [esi+12]		; 4 bytes pSrc
		punpcklbw mm0, mm7	; convert bytes to words (mm7=0)
		movq mm1, [ebx+24]		; 8 bytes pWin (4 values)
		movq mm2, mm0
		pmullw mm0, mm1		; pSrc*pWin
		pmulhw mm2, mm1		; pSrc*pWin high
		movq mm1, mm0
		punpcklwd mm0, mm2
		punpckhwd mm1, mm2
		paddd mm0, mm6	; + 256 (=mm6)
		paddd mm1, mm6	; + 256 (=mm6)
		psrld mm0, 6		; >>6
		psrld mm1, 6		; >>6
		packssdw mm0, mm1	; dwords to words
		movq mm1, [edi+24]		; old pDst
		paddusw mm0, mm1	; add to pDst
        movq [edi+24], mm0		; write low 4 values
		add edi, edx
		add esi, ecx
		add ebx, eax
%endmacro

%macro OVERS32 0
		movd mm0, [esi]		; 4 bytes pSrc
		punpcklbw mm0, mm7	; convert bytes to words (mm7=0)
		movq mm1, [ebx]		; 8 bytes pWin (4 values)
		movq mm2, mm0
		pmullw mm0, mm1		; pSrc*pWin
		pmulhw mm2, mm1		; pSrc*pWin high
		movq mm1, mm0
		punpcklwd mm0, mm2
		punpckhwd mm1, mm2
		paddd mm0, mm6	; + 256 (=mm6)
		paddd mm1, mm6	; + 256 (=mm6)
		psrld mm0, 6		; >>6
		psrld mm1, 6		; >>6
		packssdw mm0, mm1	; dwords to words
		movq mm1, [edi]		; old pDst
		paddusw mm0, mm1	; add to pDst
        movq [edi], mm0		; write low 4 values

		movd mm0, [esi+4]		; 4 bytes pSrc
		punpcklbw mm0, mm7	; convert bytes to words (mm7=0)
		movq mm1, [ebx+8]		; 8 bytes pWin (4 values)
		movq mm2, mm0
		pmullw mm0, mm1		; pSrc*pWin
		pmulhw mm2, mm1		; pSrc*pWin high
		movq mm1, mm0
		punpcklwd mm0, mm2
		punpckhwd mm1, mm2
		paddd mm0, mm6	; + 256 (=mm6)
		paddd mm1, mm6	; + 256 (=mm6)
		psrld mm0, 6		; >>6
		psrld mm1, 6		; >>6
		packssdw mm0, mm1	; dwords to words
		movq mm1, [edi+8]		; old pDst
		paddusw mm0, mm1	; add to pDst
        movq [edi+8], mm0		; write low 4 bytes

		movd mm0, [esi+8]		; 4 bytes pSrc
		punpcklbw mm0, mm7	; convert bytes to words (mm7=0)
		movq mm1, [ebx+16]		; 8 bytes pWin (4 values)
		movq mm2, mm0
		pmullw mm0, mm1		; pSrc*pWin
		pmulhw mm2, mm1		; pSrc*pWin high
		movq mm1, mm0
		punpcklwd mm0, mm2
		punpckhwd mm1, mm2
		paddd mm0, mm6	; + 256 (=mm6)
		paddd mm1, mm6	; + 256 (=mm6)
		psrld mm0, 6		; >>6
		psrld mm1, 6		; >>6
		packssdw mm0, mm1	; dwords to words
		movq mm1, [edi+16]		; old pDst
		paddusw mm0, mm1	; add to pDst
        movq [edi+16], mm0		; write low 4 values

		movd mm0, [esi+12]		; 4 bytes pSrc
		punpcklbw mm0, mm7	; convert bytes to words (mm7=0)
		movq mm1, [ebx+24]		; 8 bytes pWin (4 values)
		movq mm2, mm0
		pmullw mm0, mm1		; pSrc*pWin
		pmulhw mm2, mm1		; pSrc*pWin high
		movq mm1, mm0
		punpcklwd mm0, mm2
		punpckhwd mm1, mm2
		paddd mm0, mm6	; + 256 (=mm6)
		paddd mm1, mm6	; + 256 (=mm6)
		psrld mm0, 6		; >>6
		psrld mm1, 6		; >>6
		packssdw mm0, mm1	; dwords to words
		movq mm1, [edi+24]		; old pDst
		paddusw mm0, mm1	; add to pDst
        movq [edi+24], mm0		; write low 4 values

		movd mm0, [esi+16]		; 4 bytes pSrc
		punpcklbw mm0, mm7	; convert bytes to words (mm7=0)
		movq mm1, [ebx+32]		; 8 bytes pWin (4 values)
		movq mm2, mm0
		pmullw mm0, mm1		; pSrc*pWin
		pmulhw mm2, mm1		; pSrc*pWin high
		movq mm1, mm0
		punpcklwd mm0, mm2
		punpckhwd mm1, mm2
		paddd mm0, mm6	; + 256 (=mm6)
		paddd mm1, mm6	; + 256 (=mm6)
		psrld mm0, 6		; >>6
		psrld mm1, 6		; >>6
		packssdw mm0, mm1	; dwords to words
		movq mm1, [edi+32]		; old pDst
		paddusw mm0, mm1	; add to pDst
        movq [edi+32], mm0		; write low 4 values

		movd mm0, [esi+20]		; 4 bytes pSrc
		punpcklbw mm0, mm7	; convert bytes to words (mm7=0)
		movq mm1, [ebx+40]		; 8 bytes pWin (4 values)
		movq mm2, mm0
		pmullw mm0, mm1		; pSrc*pWin
		pmulhw mm2, mm1		; pSrc*pWin high
		movq mm1, mm0
		punpcklwd mm0, mm2
		punpckhwd mm1, mm2
		paddd mm0, mm6	; + 256 (=mm6)
		paddd mm1, mm6	; + 256 (=mm6)
		psrld mm0, 6		; >>6
		psrld mm1, 6		; >>6
		packssdw mm0, mm1	; dwords to words
		movq mm1, [edi+40]		; old pDst
		paddusw mm0, mm1	; add to pDst
        movq [edi+40], mm0		; write low 4 bytes

		movd mm0, [esi+24]		; 4 bytes pSrc
		punpcklbw mm0, mm7	; convert bytes to words (mm7=0)
		movq mm1, [ebx+48]		; 8 bytes pWin (4 values)
		movq mm2, mm0
		pmullw mm0, mm1		; pSrc*pWin
		pmulhw mm2, mm1		; pSrc*pWin high
		movq mm1, mm0
		punpcklwd mm0, mm2
		punpckhwd mm1, mm2
		paddd mm0, mm6	; + 256 (=mm6)
		paddd mm1, mm6	; + 256 (=mm6)
		psrld mm0, 6		; >>6
		psrld mm1, 6		; >>6
		packssdw mm0, mm1	; dwords to words
		movq mm1, [edi+48]		; old pDst
		paddusw mm0, mm1	; add to pDst
        movq [edi+48], mm0		; write low 4 values

		movd mm0, [esi+28]		; 4 bytes pSrc
		punpcklbw mm0, mm7	; convert bytes to words (mm7=0)
		movq mm1, [ebx+56]		; 8 bytes pWin (4 values)
		movq mm2, mm0
		pmullw mm0, mm1		; pSrc*pWin
		pmulhw mm2, mm1		; pSrc*pWin high
		movq mm1, mm0
		punpcklwd mm0, mm2
		punpckhwd mm1, mm2
		paddd mm0, mm6	; + 256 (=mm6)
		paddd mm1, mm6	; + 256 (=mm6)
		psrld mm0, 6		; >>6
		psrld mm1, 6		; >>6
		packssdw mm0, mm1	; dwords to words
		movq mm1, [edi+56]		; old pDst
		paddusw mm0, mm1	; add to pDst
        movq [edi+56], mm0		; write low 4 values
		add edi, edx
		add esi, ecx
		add ebx, eax
%endmacro



;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal Overlaps32x16_mmx
cglobal Overlaps4x4_mmx
cglobal Overlaps4x2_mmx
cglobal Overlaps4x8_mmx
cglobal Overlaps8x8_mmx
cglobal Overlaps8x4_mmx
cglobal Overlaps8x16_mmx
cglobal Overlaps16x16_mmx
cglobal Overlaps16x8_mmx
cglobal Overlaps16x2_mmx
cglobal Overlaps8x2_mmx
cglobal Overlaps8x1_mmx
cglobal LimitChanges_mmx

cglobal Overlaps2x4_mmx
cglobal Overlaps2x2_mmx

OVERSLAPS2 4
OVERSLAPS2 2

;-------------------------------------------------------------------
;
;void Overlaps4x4_mmx(unsigned short *pDst, int nDstPitch,
;				const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch)
;{
;	for (int j=0; j<4; j++)
;	{
;		for (int i=0; i<4; i++)
;			pDst[i] = pDst[i] + ((pSrc[i]*pWin[i]+256)>>6);
;		pDst += nDstPitch;
;		pSrc += nSrcPitch;
;		pWin += nWinPitch;
;	}
;}

ALIGN 16
Overlaps4x4_mmx:
		push edi
		push esi
		push ebx

		mov edi, [esp + 16] ; pDst
		mov edx, [esp + 20] ; nDstPitch
		add edx, edx ; dst pitch in bytes
		mov esi, [esp + 24] ; pSrc
		mov ecx, [esp + 28] ; nSrcPitch
		mov ebx, [esp + 32] ; pWin
		mov eax, [esp + 36] ; nWinPitch
		add eax, eax ; win pitch in bytes

	; prepare constants
		pcmpeqd mm7, mm7; = FFFFFFFF
		pxor mm6, mm6	; =0
		psubd mm6, mm7	; =1
		pslld mm6, 8	; *256 = 256
		pxor mm7, mm7	; =0

		OVERS4
		OVERS4
		OVERS4
		OVERS4

		pop ebx
		pop esi
		pop edi

		ret

;-------------------------------------------------------------------
;
;void Overlaps4x2_mmx(unsigned short *pDst, int nDstPitch,
;				const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch)
;{
;	for (int j=0; j<2; j++)
;	{
;		for (int i=0; i<4; i++)
;			pDst[i] = pDst[i] + ((pSrc[i]*pWin[i]+256)>>6);
;		pDst += nDstPitch;
;		pSrc += nSrcPitch;
;		pWin += nWinPitch;
;	}
;}

ALIGN 16
Overlaps4x2_mmx:
		push edi
		push esi
		push ebx

		mov edi, [esp + 16] ; pDst
		mov edx, [esp + 20] ; nDstPitch
		add edx, edx ; dst pitch in bytes
		mov esi, [esp + 24] ; pSrc
		mov ecx, [esp + 28] ; nSrcPitch
		mov ebx, [esp + 32] ; pWin
		mov eax, [esp + 36] ; nWinPitch
		add eax, eax ; win pitch in bytes

	; prepare constants
		pcmpeqd mm7, mm7; = FFFFFFFF
		pxor mm6, mm6	; =0
		psubd mm6, mm7	; =1
		pslld mm6, 8	; *256 = 256
		pxor mm7, mm7	; =0

		OVERS4
		OVERS4

		pop ebx
		pop esi
		pop edi

		ret


; void Overlaps16x16_mmx(unsigned short *pDst, int nDstPitch, const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch)
;{
;	// pWin from 0 to 256
;	for (int j=0; j<16; j++)
;	{
;		for (int i=0; i<16; i++)
;			pDst[i] = pDst[i] + ((pSrc[i]*pWin[i]+256)>>6);
;	}
;		pDst += nDstPitch;
;		pSrc += nSrcPitch;
;		pWin += nWinPitch;
;	}
;}

ALIGN 16
Overlaps16x16_mmx:
		push edi
		push esi
		push ebx

		mov edi, [esp + 16] ; pDst
		mov edx, [esp + 20] ; nDstPitch
		add edx, edx ; dst pitch in bytes
		mov esi, [esp + 24] ; pSrc
		mov ecx, [esp + 28] ; nSrcPitch
		mov ebx, [esp + 32] ; pWin
		mov eax, [esp + 36] ; nWinPitch
		add eax, eax ; win pitch in bytes

	; prepare constants
		pcmpeqd mm7, mm7; = FFFFFFFF
		pxor mm6, mm6	; =0
		psubd mm6, mm7	; =1
		pslld mm6, 8	; *256 = 256
		pxor mm7, mm7	; =0

		OVERS16
		OVERS16
		OVERS16
		OVERS16
		OVERS16
		OVERS16
		OVERS16
		OVERS16
		OVERS16
		OVERS16
		OVERS16
		OVERS16
		OVERS16
		OVERS16
		OVERS16
		OVERS16

		pop ebx
		pop esi
		pop edi

		ret


; void Overlaps16x8_mmx(unsigned short *pDst, int nDstPitch, const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch)
;{
;	// pWin from 0 to 256
;	for (int j=0; j<8; j++)
;	{
;		for (int i=0; i<16; i++)
;			pDst[i] = pDst[i] + ((pSrc[i]*pWin[i]+256)>>6);
;	}
;		pDst += nDstPitch;
;		pSrc += nSrcPitch;
;		pWin += nWinPitch;
;	}
;}

ALIGN 16
Overlaps16x8_mmx:
		push edi
		push esi
		push ebx

		mov edi, [esp + 16] ; pDst
		mov edx, [esp + 20] ; nDstPitch
		add edx, edx ; dst pitch in bytes
		mov esi, [esp + 24] ; pSrc
		mov ecx, [esp + 28] ; nSrcPitch
		mov ebx, [esp + 32] ; pWin
		mov eax, [esp + 36] ; nWinPitch
		add eax, eax ; win pitch in bytes

	; prepare constants
		pcmpeqd mm7, mm7; = FFFFFFFF
		pxor mm6, mm6	; =0
		psubd mm6, mm7	; =1
		pslld mm6, 8	; *256 = 256
		pxor mm7, mm7	; =0

		OVERS16
		OVERS16
		OVERS16
		OVERS16
		OVERS16
		OVERS16
		OVERS16
		OVERS16

		pop ebx
		pop esi
		pop edi

		ret



;
;void Overlaps8x16_mmx(unsigned short *pDst, int nDstPitch, const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch)
;{
;	for (int j=0; j<16; j++)
;	{
;		for (int i=0; i<8; i++)
;			pDst[i] = pDst[i] + ((pSrc[i]*pWin[i]+256)>>6);
;		pDst += nDstPitch;
;		pSrc += nSrcPitch;
;		pWin += nWinPitch;
;	}
;}

ALIGN 16
Overlaps8x16_mmx:
		push edi
		push esi
		push ebx

		mov edi, [esp + 16] ; pDst
		mov edx, [esp + 20] ; nDstPitch
		add edx, edx ; dst pitch in bytes
		mov esi, [esp + 24] ; pSrc
		mov ecx, [esp + 28] ; nSrcPitch
		mov ebx, [esp + 32] ; pWin
		mov eax, [esp + 36] ; nWinPitch
		add eax, eax ; win pitch in bytes

	; prepare constants
		pcmpeqd mm7, mm7; = FFFFFFFF
		pxor mm6, mm6	; =0
		psubd mm6, mm7	; =1
		pslld mm6, 8	; *256 = 256
		pxor mm7, mm7	; =0

		OVERS8
		OVERS8
		OVERS8
		OVERS8
		OVERS8
		OVERS8
		OVERS8
		OVERS8
		OVERS8
		OVERS8
		OVERS8
		OVERS8
		OVERS8
		OVERS8
		OVERS8
		OVERS8

		pop ebx
		pop esi
		pop edi

		ret


;
;void Overlaps8x8_mmx(unsigned short *pDst, int nDstPitch, const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch)
;{
;	for (int j=0; j<8; j++)
;	{
;		for (int i=0; i<8; i++)
;			pDst[i] = pDst[i] + ((pSrc[i]*pWin[i]+256)>>6);
;		pDst += nDstPitch;
;		pSrc += nSrcPitch;
;		pWin += nWinPitch;
;	}
;}

ALIGN 16
Overlaps8x8_mmx:
		push edi
		push esi
		push ebx

		mov edi, [esp + 16] ; pDst
		mov edx, [esp + 20] ; nDstPitch
		add edx, edx ; dst pitch in bytes
		mov esi, [esp + 24] ; pSrc
		mov ecx, [esp + 28] ; nSrcPitch
		mov ebx, [esp + 32] ; pWin
		mov eax, [esp + 36] ; nWinPitch
		add eax, eax ; win pitch in bytes

	; prepare constants
		pcmpeqd mm7, mm7; = FFFFFFFF
		pxor mm6, mm6	; =0
		psubd mm6, mm7	; =1
		pslld mm6, 8	; *256 = 256
		pxor mm7, mm7	; =0

		OVERS8
		OVERS8
		OVERS8
		OVERS8
		OVERS8
		OVERS8
		OVERS8
		OVERS8

		pop ebx
		pop esi
		pop edi

		ret


;
;void Overlaps8x4_mmx(unsigned short *pDst, int nDstPitch, const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch)
;{
;	for (int j=0; j<4; j++)
;	{
;		for (int i=0; i<8; i++)
;			pDst[i] = pDst[i] + ((pSrc[i]*pWin[i]+256)>>6);
;		pDst += nDstPitch;
;		pSrc += nSrcPitch;
;		pWin += nWinPitch;
;	}
;}

ALIGN 16
Overlaps8x4_mmx:
		push edi
		push esi
		push ebx

		mov edi, [esp + 16] ; pDst
		mov edx, [esp + 20] ; nDstPitch
		add edx, edx ; dst pitch in bytes
		mov esi, [esp + 24] ; pSrc
		mov ecx, [esp + 28] ; nSrcPitch
		mov ebx, [esp + 32] ; pWin
		mov eax, [esp + 36] ; nWinPitch
		add eax, eax ; win pitch in bytes

	; prepare constants
		pcmpeqd mm7, mm7; = FFFFFFFF
		pxor mm6, mm6	; =0
		psubd mm6, mm7	; =1
		pslld mm6, 8	; *256 = 256
		pxor mm7, mm7	; =0

		OVERS8
		OVERS8
		OVERS8
		OVERS8

		pop ebx
		pop esi
		pop edi

		ret


;
;void Overlaps4x8_mmx(unsigned short *pDst, int nDstPitch, const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch)
;{
;	for (int j=0; j<8; j++)
;	{
;		for (int i=0; i<4; i++)
;			pDst[i] = pDst[i] + ((pSrc[i]*pWin[i]+256)>>6);
;		pDst += nDstPitch;
;		pSrc += nSrcPitch;
;		pWin += nWinPitch;
;	}
;}

ALIGN 16
Overlaps4x8_mmx:
		push edi
		push esi
		push ebx

		mov edi, [esp + 16] ; pDst
		mov edx, [esp + 20] ; nDstPitch
		add edx, edx ; dst pitch in bytes
		mov esi, [esp + 24] ; pSrc
		mov ecx, [esp + 28] ; nSrcPitch
		mov ebx, [esp + 32] ; pWin
		mov eax, [esp + 36] ; nWinPitch
		add eax, eax ; win pitch in bytes

	; prepare constants
		pcmpeqd mm7, mm7; = FFFFFFFF
		pxor mm6, mm6	; =0
		psubd mm6, mm7	; =1
		pslld mm6, 8	; *256 = 256
		pxor mm7, mm7	; =0

		OVERS4
		OVERS4
		OVERS4
		OVERS4
		OVERS4
		OVERS4
		OVERS4
		OVERS4

		pop ebx
		pop esi
		pop edi

		ret

;void LimitChanges(unsigned char *pDst, int nDstPitch, const unsigned char *pSrc, int nSrcPitch, int nWidth, int nHeight, int nLimit)
;{
;	for (int h=0; h<nHeight; h++)
;	{
;		for (int i=0; i<nWidth; i++)
;			pDst[i] = min( max (pDst[i], (pSrc[i]-nLimit)), (pSrc[i]+nLimit));;
;		pDst += nDstPitch;
;		pSrc += nSrcPitch;
;	}
;}

ALIGN 16
LimitChanges_mmx:

		push esi
		push edi
		push ebx

        mov edi, [esp + 16]		; pDst
        mov esi, [esp + 24]		; pSrc
        mov edx, [esp + 32]		; nWidth
        mov ecx, [esp + 36]		; nHeight
        movd mm5, [esp + 40]    ; nLimit
        punpckldq mm5, mm5
        packssdw mm5, mm5
        packuswb mm5, mm5 ; correction limit  - 8 bytes
        pxor mm4, mm4 ; 0

h_loopy:

        xor ebx, ebx

h_loopx:
        movq mm0, [esi+ebx] ; src bytes
        movq mm1, [edi+ebx] ; dest bytes
		movq	mm2, mm5		;/* copy limit */
		paddusb	mm2, mm0	;/* max possible (mm0 is original) */
		movq		mm3, mm1	;/* (mm1 is changed) */
		psubusb	mm3, mm0	;/* changed - orig,   saturated to 0 */
		psubusb	mm3, mm5	;/* did it change too much, Y where nonzero */
		pcmpeqb	mm3, mm4	;/* now ff where new value should be used, else 00 (mm4=0)*/
		pand	mm1, mm3	;	/* use new value for these pixels */
		pandn	mm3, mm2	; /* use max value for these pixels */
		por	mm1, mm3		;/* combine them, get result with limited  positive correction */
		movq	mm3, mm5		;/* copy limit */
		movq	mm2, mm0		;/* copy orig */
		psubusb	mm2, mm5	;/* min possible */
		movq		mm3, mm0	;/* copy orig */
		psubusb	mm3, mm1	;/* orig - changed, saturated to 0 */
		psubusb	mm3, mm5	;/* did it change too much, Y where nonzero */
		pcmpeqb	mm3, mm4	;/* now ff where new value should be used, else 00 */
		pand	mm1, mm3		;/* use new value for these pixels */
		pandn	mm3, mm2		;/* use min value for these pixels */
		por	mm1, mm3		;/* combine them, get result with limited  negative correction */

        movq [edi+ebx], mm1
        add ebx, 8
        cmp ebx, edx
        jl h_loopx

 ; do not process rightmost rest bytes

        add esi, [esp + 28] ; src pitch
        add edi, [esp + 20] ; dst pitch
        dec ecx
        jnz h_loopy

        emms

        pop ebx
        pop edi
        pop esi

        ret

; void Overlaps16x2_mmx(unsigned short *pDst, int nDstPitch, const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch)
;{
;	// pWin from 0 to 256
;	for (int j=0; j<2; j++)
;	{
;		for (int i=0; i<16; i++)
;			pDst[i] = pDst[i] + ((pSrc[i]*pWin[i]+256)>>6);
;	}
;		pDst += nDstPitch;
;		pSrc += nSrcPitch;
;		pWin += nWinPitch;
;	}
;}

ALIGN 16
Overlaps16x2_mmx:
		push edi
		push esi
		push ebx

		mov edi, [esp + 16] ; pDst
		mov edx, [esp + 20] ; nDstPitch
		add edx, edx ; dst pitch in bytes
		mov esi, [esp + 24] ; pSrc
		mov ecx, [esp + 28] ; nSrcPitch
		mov ebx, [esp + 32] ; pWin
		mov eax, [esp + 36] ; nWinPitch
		add eax, eax ; win pitch in bytes

	; prepare constants
		pcmpeqd mm7, mm7; = FFFFFFFF
		pxor mm6, mm6	; =0
		psubd mm6, mm7	; =1
		pslld mm6, 8	; *256 = 256
		pxor mm7, mm7	; =0

		OVERS16
		OVERS16

		pop ebx
		pop esi
		pop edi

		ret

;void Overlaps8x2_mmx(unsigned short *pDst, int nDstPitch, const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch)
;{
;	for (int j=0; j<2; j++)
;	{
;		for (int i=0; i<8; i++)
;			pDst[i] = pDst[i] + ((pSrc[i]*pWin[i]+256)>>6);
;		pDst += nDstPitch;
;		pSrc += nSrcPitch;
;		pWin += nWinPitch;
;	}
;}

ALIGN 16
Overlaps8x2_mmx:
		push edi
		push esi
		push ebx

		mov edi, [esp + 16] ; pDst
		mov edx, [esp + 20] ; nDstPitch
		add edx, edx ; dst pitch in bytes
		mov esi, [esp + 24] ; pSrc
		mov ecx, [esp + 28] ; nSrcPitch
		mov ebx, [esp + 32] ; pWin
		mov eax, [esp + 36] ; nWinPitch
		add eax, eax ; win pitch in bytes

	; prepare constants
		pcmpeqd mm7, mm7; = FFFFFFFF
		pxor mm6, mm6	; =0
		psubd mm6, mm7	; =1
		pslld mm6, 8	; *256 = 256
		pxor mm7, mm7	; =0

		OVERS8
		OVERS8

		pop ebx
		pop esi
		pop edi

		ret

;void Overlaps8x1_mmx(unsigned short *pDst, int nDstPitch, const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch)
;{
;	for (int j=0; j<1; j++)
;	{
;		for (int i=0; i<8; i++)
;			pDst[i] = pDst[i] + ((pSrc[i]*pWin[i]+256)>>6);
;		pDst += nDstPitch;
;		pSrc += nSrcPitch;
;		pWin += nWinPitch;
;	}
;}

ALIGN 16
Overlaps8x1_mmx:
		push edi
		push esi
		push ebx

		mov edi, [esp + 16] ; pDst
		mov edx, [esp + 20] ; nDstPitch
		add edx, edx ; dst pitch in bytes
		mov esi, [esp + 24] ; pSrc
		mov ecx, [esp + 28] ; nSrcPitch
		mov ebx, [esp + 32] ; pWin
		mov eax, [esp + 36] ; nWinPitch
		add eax, eax ; win pitch in bytes

	; prepare constants
		pcmpeqd mm7, mm7; = FFFFFFFF
		pxor mm6, mm6	; =0
		psubd mm6, mm7	; =1
		pslld mm6, 8	; *256 = 256
		pxor mm7, mm7	; =0

		OVERS8

		pop ebx
		pop esi
		pop edi

		ret


; void Overlaps32x16_mmx(unsigned short *pDst, int nDstPitch, const unsigned char *pSrc, int nSrcPitch, short *pWin, int nWinPitch)
;{
;	// pWin from 0 to 256
;	for (int j=0; j<16; j++)
;	{
;		for (int i=0; i<32; i++)
;			pDst[i] = pDst[i] + ((pSrc[i]*pWin[i]+256)>>6);
;	}
;		pDst += nDstPitch;
;		pSrc += nSrcPitch;
;		pWin += nWinPitch;
;	}
;}

ALIGN 16
Overlaps32x16_mmx:
		push edi
		push esi
		push ebx

		mov edi, [esp + 16] ; pDst
		mov edx, [esp + 20] ; nDstPitch
		add edx, edx ; dst pitch in bytes
		mov esi, [esp + 24] ; pSrc
		mov ecx, [esp + 28] ; nSrcPitch
		mov ebx, [esp + 32] ; pWin
		mov eax, [esp + 36] ; nWinPitch
		add eax, eax ; win pitch in bytes

	; prepare constants
		pcmpeqd mm7, mm7; = FFFFFFFF
		pxor mm6, mm6	; =0
		psubd mm6, mm7	; =1
		pslld mm6, 8	; *256 = 256
		pxor mm7, mm7	; =0

		OVERS32
		OVERS32
		OVERS32
		OVERS32
		OVERS32
		OVERS32
		OVERS32
		OVERS32

		OVERS32
		OVERS32
		OVERS32
		OVERS32
		OVERS32
		OVERS32
		OVERS32
		OVERS32

		pop ebx
		pop esi
		pop edi

		ret


