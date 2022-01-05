;/****************************************************************************
; * $Id$
; *
; ***************************************************************************/
; assembler (NASM) copy functions
;// 2005 Manao
;// Copyright (c)2006 Fizick - Copy4x8_mmx,Copy8x16_mmx, Copy4x2_mmx, Copy8x4_mmx, Copy16x8_mmx, copy2x4_mmx
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

%macro MOV16x2 0
	movq mm0, [esi]
	movq mm1, [esi + 8]
	movq mm2, [esi + ecx]
	movq mm3, [esi + ecx + 8]

	lea esi, [esi + 2 * ecx]

	movq [edi], mm0
	movq [edi + 8], mm1
	movq [edi + edx], mm2
	movq [edi + edx + 8], mm3

	lea edi, [edi + 2 * edx]
%endmacro

%macro MOV32x1 0
	movq mm0, [esi]
	movq mm1, [esi + 8]
	movq mm2, [esi + 16]
	movq mm3, [esi + 24]
	add esi, ecx
	movq [edi], mm0
	movq [edi + 8], mm1
	movq [edi + 16], mm2
	movq [edi + 24], mm3
	add edi, edx

%endmacro

;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal Copy32x16_mmx
cglobal Copy16x16_mmx
cglobal Copy8x8_mmx
cglobal Copy4x4_mmx
cglobal Copy2x2_mmx
cglobal Copy4x8_mmx
cglobal Copy8x16_mmx
cglobal Copy4x2_mmx
cglobal Copy8x4_mmx
cglobal Copy16x8_mmx
cglobal Copy2x4_mmx
cglobal Copy16x2_mmx
cglobal Copy8x2_mmx
cglobal Copy8x1_mmx

;-----------------------------------------------------------------------------
;
; void Copy16x16_mmx(unsigned char *pDst, int nDstPitch,
;				  const unsigned char *pSrc, int nSrcPitch);
;
;-----------------------------------------------------------------------------

ALIGN 16

Copy16x16_mmx:
		push edi
		push esi

		mov edi, [esp + 12]
		mov esi, [esp + 20]
		mov edx, [esp + 16]
		mov ecx, [esp + 24]

		MOV16x2
		MOV16x2
		MOV16x2
		MOV16x2
		MOV16x2
		MOV16x2
		MOV16x2
		MOV16x2

        pop esi
        pop edi

		ret

;-----------------------------------------------------------------------------
;
; void Copy8x8_mmx(unsigned char *pDst, int nDstPitch,
;				 const unsigned char *pSrc, int nSrcPitch);
;
;-----------------------------------------------------------------------------

ALIGN 16

Copy8x8_mmx:
		push edi
		push esi

		mov edi, [esp + 12]
		mov esi, [esp + 20]
		mov edx, [esp + 16]
		mov ecx, [esp + 24]

        movq mm0, [esi]				; 0 ->
        movq mm1, [esi + ecx]       ; 1 ->
        movq mm2, [esi + 2 * ecx]   ; 2 ->
        movq [edi], mm0             ; 0 <-
        movq mm3, [esi + 4 * ecx]   ; 4 ->
        movq [edi + edx], mm1       ; 1 <-
        movq [edi + 2 * edx], mm2   ; 2 <-
        movq [edi + 4 * edx], mm3   ; 4 <-

        lea esi, [esi + ecx * 2]    ; 2
        lea edi, [edi + edx * 2]    ; 2

        movq mm0, [esi + ecx]       ; 2 + 1 ->
        movq mm1, [esi + 4 * ecx]   ; 2 + 4 ->
        movq [edi + edx], mm0       ; 2 + 1 <-
        movq [edi + 4 * edx], mm1   ; 2 + 4 <-

        lea esi, [esi + ecx]        ; 3
        lea edi, [edi + edx]        ; 3

        movq mm0, [esi + 2 * ecx]   ; 3 + 2 ->
        movq mm1, [esi + 4 * ecx]   ; 3 + 4 ->
        movq [edi + 2 * edx], mm0   ; 3 + 2 <-
        movq [edi + 4 * edx], mm1   ; 3 + 4 <-

        pop esi
        pop edi

        ret

;-----------------------------------------------------------------------------
;
; void Copy8x4_mmx(unsigned char *pDst, int nDstPitch,
;				 const unsigned char *pSrc, int nSrcPitch);
;
;-----------------------------------------------------------------------------

ALIGN 16

Copy8x4_mmx:
		push edi
		push esi

		mov edi, [esp + 12]
		mov esi, [esp + 20]
		mov edx, [esp + 16]
		mov ecx, [esp + 24]

        movq mm0, [esi]				; 0 ->
        movq mm1, [esi + ecx]       ; 1 ->
        movq mm2, [esi + 2 * ecx]   ; 2 ->
        movq [edi], mm0             ; 0 <-
;        movq mm3, [esi + 4 * ecx]   ; 4 ->
        movq [edi + edx], mm1       ; 1 <-
        movq [edi + 2 * edx], mm2   ; 2 <-
;        movq [edi + 4 * edx], mm3   ; 4 <-

        lea esi, [esi + ecx * 2]    ; 2
        lea edi, [edi + edx * 2]    ; 2

        movq mm0, [esi + ecx]       ; 2 + 1 ->
 ;       movq mm1, [esi + 4 * ecx]   ; 2 + 4 ->
        movq [edi + edx], mm0       ; 2 + 1 <-
 ;       movq [edi + 4 * edx], mm1   ; 2 + 4 <-

 ;       lea esi, [esi + ecx]        ; 3
 ;       lea edi, [edi + edx]        ; 3

;        movq mm0, [esi + 2 * ecx]   ; 3 + 2 ->
 ;       movq mm1, [esi + 4 * ecx]   ; 3 + 4 ->
;        movq [edi + 2 * edx], mm0   ; 3 + 2 <-
;        movq [edi + 4 * edx], mm1   ; 3 + 4 <-

        pop esi
        pop edi

        ret

;-----------------------------------------------------------------------------
;
; void Copy4x4_mmx(unsigned char *pDst, int nDstPitch,
;				 const unsigned char *pSrc, int nSrcPitch);
;
;-----------------------------------------------------------------------------

ALIGN 16

Copy4x4_mmx:

		push edi
		push esi

		mov edi, [esp + 12]
		mov esi, [esp + 20]
		mov edx, [esp + 16]
		mov ecx, [esp + 24]

		mov eax, [esi]
		mov [edi], eax
		mov eax, [esi + ecx]
		mov [edi + edx], eax
		lea esi, [esi + 2 * ecx]
		lea edi, [edi + 2 * edx]
		mov eax, [esi + ecx]
		mov [edi + edx], eax
		movsd

		pop esi
		pop edi

		ret

;-----------------------------------------------------------------------------
;
; void Copy4x2_mmx(unsigned char *pDst, int nDstPitch,
;				 const unsigned char *pSrc, int nSrcPitch);
;
;-----------------------------------------------------------------------------

ALIGN 16

Copy4x2_mmx:

		push edi
		push esi

		mov edi, [esp + 12]
		mov esi, [esp + 20]
		mov edx, [esp + 16]
		mov ecx, [esp + 24]

		mov eax, [esi]
		mov [edi], eax
		mov eax, [esi + ecx]
		mov [edi + edx], eax

		pop esi
		pop edi

		ret

;-----------------------------------------------------------------------------
;
; void Copy2x2_mmx(unsigned char *pDst, int nDstPitch,
;				 const unsigned char *pSrc, int nSrcPitch);
;
;-----------------------------------------------------------------------------

ALIGN 16

Copy2x2_mmx:

		push edi
		push esi

		mov edi, [esp + 12]
		mov esi, [esp + 20]

		mov edx, [esp + 16]
		mov ecx, [esp + 24]

		movsw
		lea esi, [esi + ecx - 2]
		lea edi, [edi + edx - 2]
		movsw

		pop esi
		pop edi

		ret

;-----------------------------------------------------------------------------
;
; void Copy2x4_mmx(unsigned char *pDst, int nDstPitch,
;				 const unsigned char *pSrc, int nSrcPitch);
;
;-----------------------------------------------------------------------------

ALIGN 16

Copy2x4_mmx:

		push edi
		push esi

		mov edi, [esp + 12] ; dst
		mov esi, [esp + 20] ; src

		mov edx, [esp + 16] ; dstpitch
		mov ecx, [esp + 24] ; srcpitch

		movsw
		lea esi, [esi + ecx - 2]
		lea edi, [edi + edx - 2]
		movsw

		lea esi, [esi + ecx - 2]
		lea edi, [edi + edx - 2]
		movsw

		lea esi, [esi + ecx - 2]
		lea edi, [edi + edx - 2]
		movsw

		pop esi
		pop edi

		ret


;-----------------------------------------------------------------------------
;
; void Copy4x8_mmx(unsigned char *pDst, int nDstPitch,
;				 const unsigned char *pSrc, int nSrcPitch); 4 bytes, 8 lines - added by Fizick
;
;-----------------------------------------------------------------------------

ALIGN 16

Copy4x8_mmx:

		push edi
		push esi

		mov edi, [esp + 12] ; pDst
		mov esi, [esp + 20] ; pSrc
		mov edx, [esp + 16] ; nDstPitch
		mov ecx, [esp + 24] ; nSrcPitch

		mov eax, [esi] ; get 1
		mov [edi], eax ; put 1
		mov eax, [esi + ecx] ; get 2
		mov [edi + edx], eax ; put 2
		lea esi, [esi + 2 * ecx] ; increment address by 2 lines
		lea edi, [edi + 2 * edx] ; increment address by 2 lines
		mov eax, [esi] ; get 3
		mov [edi], eax ; put 3
		mov eax, [esi + ecx] ; get 4
		mov [edi + edx], eax ; put 4
		lea esi, [esi + 2 * ecx] ; increment address by 2 lines
		lea edi, [edi + 2 * edx] ; increment address by 2 lines
		mov eax, [esi] ; get 5
		mov [edi], eax ; put 5
		mov eax, [esi + ecx] ; get 6
		mov [edi + edx], eax ; put 6
		lea esi, [esi + 2 * ecx] ; increment address by 2 lines
		lea edi, [edi + 2 * edx] ; increment address by 2 lines
		mov eax, [esi] ; get 7
		mov [edi], eax ; put 7
		mov eax, [esi + ecx] ; get 8
		mov [edi + edx], eax ; put 8

		pop esi
		pop edi

		ret

;-----------------------------------------------------------------------------
;
; void Copy8x16_mmx(unsigned char *pDst, int nDstPitch,
;				 const unsigned char *pSrc, int nSrcPitch); 8 bytes, 16 lines - added by Fizick
;
;-----------------------------------------------------------------------------

ALIGN 16

Copy8x16_mmx:
		push edi
		push esi

		mov edi, [esp + 12]
		mov esi, [esp + 20]
		mov edx, [esp + 16]
		mov ecx, [esp + 24]

        movq mm0, [esi]				; 0 ->
        movq mm1, [esi + ecx]       ; 1 ->
        movq mm2, [esi + 2 * ecx]   ; 2 ->
        movq [edi], mm0             ; 0 <-
        movq mm3, [esi + 4 * ecx]   ; 4 ->
        movq [edi + edx], mm1       ; 1 <-
        movq [edi + 2 * edx], mm2   ; 2 <-
        movq [edi + 4 * edx], mm3   ; 4 <-

        lea esi, [esi + ecx * 2]    ; 2
        lea edi, [edi + edx * 2]    ; 2

        movq mm0, [esi + ecx]       ; 2 + 1 ->
        movq mm1, [esi + 4 * ecx]   ; 2 + 4 ->
        movq [edi + edx], mm0       ; 2 + 1 <-
        movq [edi + 4 * edx], mm1   ; 2 + 4 <-

        lea esi, [esi + ecx]        ; 3
        lea edi, [edi + edx]        ; 3

        movq mm0, [esi + 2 * ecx]   ; 3 + 2 ->
        movq mm1, [esi + 4 * ecx]   ; 3 + 4 ->
        movq [edi + 2 * edx], mm0   ; 3 + 2 <-
        movq [edi + 4 * edx], mm1   ; 3 + 4 <-

; next 8 lines

        lea esi, [esi + ecx *4]        ; 7
        lea edi, [edi + edx *4]        ; 7

        lea esi, [esi + ecx]        ; 8
        lea edi, [edi + edx]        ; 8

        movq mm0, [esi]   ; 8 ->
        movq mm1, [esi + ecx]   ; 8 + 1 ->
        movq [edi], mm0   ; 8 <-
        movq [edi +  edx], mm1   ; 8 + 1 <-

        movq mm0, [esi + 2*ecx]   ; 8 + 2 ->
        movq mm1, [esi + 4*ecx]   ; 8 + 4 ->
        movq [edi +  2*edx], mm0   ; 8 + 2 <- ; fixed bug in v.1.3
        movq [edi +  4*edx], mm1   ; 8 + 4 <-

        lea esi, [esi + ecx * 2]    ; 10
        lea edi, [edi + edx * 2]    ; 10

        movq mm0, [esi + ecx]       ; 10 + 1 ->
        movq mm1, [esi + 4 * ecx]   ; 10 + 4 ->
        movq [edi + edx], mm0       ; 10 + 1 <-
        movq [edi + 4 * edx], mm1   ; 10 + 4 <-

        lea esi, [esi + ecx]        ; 11
        lea edi, [edi + edx]        ; 11

        movq mm0, [esi + 2 * ecx]   ; 11 + 2 ->
        movq mm1, [esi + 4 * ecx]   ; 11 + 4 ->
        movq [edi + 2 * edx], mm0   ; 11 + 2 <-
        movq [edi + 4 * edx], mm1   ; 11 + 4 <-

        pop esi
        pop edi

        ret

;-----------------------------------------------------------------------------
;
; void Copy16x8_mmx(unsigned char *pDst, int nDstPitch,
;				  const unsigned char *pSrc, int nSrcPitch);
;
;-----------------------------------------------------------------------------

ALIGN 16

Copy16x8_mmx:
		push edi
		push esi

		mov edi, [esp + 12]
		mov esi, [esp + 20]
		mov edx, [esp + 16]
		mov ecx, [esp + 24]

		MOV16x2
		MOV16x2
		MOV16x2
		MOV16x2

        pop esi
        pop edi

		ret

;-----------------------------------------------------------------------------
;
; void Copy16x2_mmx(unsigned char *pDst, int nDstPitch,
;				  const unsigned char *pSrc, int nSrcPitch);
;
;-----------------------------------------------------------------------------

ALIGN 16

Copy16x2_mmx:
		push edi
		push esi

		mov edi, [esp + 12]
		mov esi, [esp + 20]
		mov edx, [esp + 16]
		mov ecx, [esp + 24]

		MOV16x2

        pop esi
        pop edi

		ret


;-----------------------------------------------------------------------------
;
; void Copy8x2_mmx(unsigned char *pDst, int nDstPitch,
;				 const unsigned char *pSrc, int nSrcPitch);
;
;-----------------------------------------------------------------------------

ALIGN 16

Copy8x2_mmx:
		push edi
		push esi

		mov edi, [esp + 12]
		mov esi, [esp + 20]
		mov edx, [esp + 16]
		mov ecx, [esp + 24]

        movq mm0, [esi]				; 0 ->
        movq mm1, [esi + ecx]       ; 1 ->
        movq [edi], mm0             ; 0 <-
        movq [edi + edx], mm1       ; 1 <-

        pop esi
        pop edi

        ret

;-----------------------------------------------------------------------------
;
; void Copy8x1_mmx(unsigned char *pDst, int nDstPitch,
;				 const unsigned char *pSrc, int nSrcPitch);
;
;-----------------------------------------------------------------------------

ALIGN 16

Copy8x1_mmx:
		push edi
		push esi

		mov edi, [esp + 12]
		mov esi, [esp + 20]
		mov edx, [esp + 16]
		mov ecx, [esp + 24]

        movq mm0, [esi]				; 0 ->
        movq [edi], mm0             ; 0 <-

        pop esi
        pop edi

        ret

;-----------------------------------------------------------------------------
;
; void Copy32x16_mmx(unsigned char *pDst, int nDstPitch,
;				  const unsigned char *pSrc, int nSrcPitch);
;
;-----------------------------------------------------------------------------

ALIGN 16

Copy32x16_mmx:
		push edi
		push esi

		mov edi, [esp + 12]
		mov esi, [esp + 20]
		mov edx, [esp + 16]
		mov ecx, [esp + 24]

		MOV32x1
		MOV32x1
		MOV32x1
		MOV32x1
		MOV32x1
		MOV32x1
		MOV32x1
		MOV32x1

		MOV32x1
		MOV32x1
		MOV32x1
		MOV32x1
		MOV32x1
		MOV32x1
		MOV32x1
		MOV32x1

        pop esi
        pop edi

		ret

