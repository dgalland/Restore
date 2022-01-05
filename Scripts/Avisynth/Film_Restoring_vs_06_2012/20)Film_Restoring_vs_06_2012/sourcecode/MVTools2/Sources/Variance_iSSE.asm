;/****************************************************************************
; * $Id$
; Author: Manao
; (c)2007 Fizick: var8x4, luma8x4, var16x8, luma16x8
; See legal notice in Copying.txt for more information

; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
; http://www.gnu.org/copyleft/gpl.html .
; *
; ***************************************************************************/

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

; mm0 & mm1 are the vars
; mm6 & mm7 are times 8 db mean
; edx is the pitch
; esi is the beginning of the block
%macro SUB_VAR32x16x1 0
	movq mm2, [esi]
	movq mm3, [esi + 8]
	movq mm4, [esi + 16]
	movq mm5, [esi + 24]

	psadbw mm2, mm6
	psadbw mm3, mm7

	psadbw mm4, mm6
	psadbw mm5, mm7

	add esi, edx

	paddd mm0, mm2
	paddd mm1, mm3

	paddd mm0, mm4
	paddd mm1, mm5
%endmacro

%macro VAR32x16 0
	pxor mm0, mm0
	pxor mm1, mm1
	SUB_VAR32x16x1
	SUB_VAR32x16x1
	SUB_VAR32x16x1
	SUB_VAR32x16x1
	SUB_VAR32x16x1
	SUB_VAR32x16x1
	SUB_VAR32x16x1
	SUB_VAR32x16x1

	SUB_VAR32x16x1
	SUB_VAR32x16x1
	SUB_VAR32x16x1
	SUB_VAR32x16x1
	SUB_VAR32x16x1
	SUB_VAR32x16x1
	SUB_VAR32x16x1
	SUB_VAR32x16x1

	paddd mm0, mm1
%endmacro


; mm0 & mm1 are the vars
; mm6 & mm7 are times 8 db mean
; edx is the pitch
; esi is the beginning of the block
%macro SUB_VAR16x16x2 0
	movq mm2, [esi]
	movq mm3, [esi + 8]
	movq mm4, [esi + edx]
	movq mm5, [esi + edx + 8]

	psadbw mm2, mm6
	psadbw mm3, mm7

	psadbw mm4, mm6
	psadbw mm5, mm7

	lea esi, [esi + edx * 2]

	paddd mm0, mm2
	paddd mm1, mm3

	paddd mm0, mm4
	paddd mm1, mm5
%endmacro

%macro VAR16x16 0
	pxor mm0, mm0
	pxor mm1, mm1
	SUB_VAR16x16x2
	SUB_VAR16x16x2
	SUB_VAR16x16x2
	SUB_VAR16x16x2
	SUB_VAR16x16x2
	SUB_VAR16x16x2
	SUB_VAR16x16x2
	SUB_VAR16x16x2
	paddd mm0, mm1
%endmacro

%macro VAR16x8 0
	pxor mm0, mm0
	pxor mm1, mm1
	SUB_VAR16x16x2
	SUB_VAR16x16x2
	SUB_VAR16x16x2
	SUB_VAR16x16x2
	paddd mm0, mm1
%endmacro

; mm7 and mm6 must be times 8 db mean
; mm0 will be the var
; mm0..mm7 are used
; esi must point to the beginning of the block, it will be changed
; edx must be the pitch
; ecx must be three times the pitch
%macro VAR8x8 0
    movq mm0, [esi]				; first line
    movq mm1, [esi + edx]		; second line

    movq mm4, [esi + edx * 2]   ; third line
    movq mm5, [esi + ecx]		; fourth line

    psadbw mm0, mm7
    psadbw mm1, mm6

    psadbw mm4, mm7
    psadbw mm5, mm6

    paddw mm0, mm4
    paddw mm1, mm5

    lea eax, [esi + 4 * edx]	;

    movq mm2, [eax]				; fifth line
    movq mm3, [eax + edx]		; sixth line

    movq mm4, [eax + 2 * edx]	; seventh line
    movq mm5, [eax + ecx]		; eighth line

    psadbw mm2, mm7
    psadbw mm3, mm6

    psadbw mm4, mm7
    psadbw mm5, mm6

    paddw mm0, mm2
    paddw mm1, mm3

    paddw mm0, mm4
    paddw mm1, mm5

    paddw mm0, mm1
%endmacro

; mm7 and mm6 must be times 8 db mean
; mm0 will be the var
; mm0 mm1 mm4 mm5 mm6 mm7 are used
; esi must point to the beginning of the block, it will be changed
; edx must be the pitch
; ecx must be three times the pitch
%macro VAR8x4 0
    movq mm0, [esi]				; first line
    movq mm1, [esi + edx]		; second line

    movq mm4, [esi + edx * 2]   ; third line
    movq mm5, [esi + ecx]		; fourth line

    psadbw mm0, mm7
    psadbw mm1, mm6

    psadbw mm4, mm7
    psadbw mm5, mm6

    paddw mm0, mm4
    paddw mm1, mm5


    paddw mm0, mm1
%endmacro

; mm7 and mm6 must be times 4 dw mean
; mm4 and mm5 must be 0
; mm0 will be the var
; mm0..mm7 are used
; esi must point to the beginning of the block, it will be changed
; edx must be the pitch
%macro VAR4x4 0
	movq mm0, [esi]
	movq mm1, [esi + edx]

	punpcklbw mm0, mm4
	punpcklbw mm1, mm5

	psadbw mm0, mm6
	psadbw mm1, mm7

	lea esi, [esi + 2 * edx]
	paddw mm0, mm1

	movq mm1, [esi]
	movq mm2, [esi + edx]

	punpcklbw mm1, mm4
	punpcklbw mm2, mm5

	psadbw mm1, mm6
	psadbw mm2, mm7

	paddw mm1, mm2
	paddw mm0, mm1
%endmacro


%macro VAR2 0
	movq mm1, [esi]
	psadbw mm1, mm7
	paddw mm0, mm1
	add esi, ebx
%endmacro

;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal Var32x16_iSSE
cglobal Luma32x16_iSSE
cglobal Var16x16_iSSE
cglobal Luma16x16_iSSE
cglobal Var8x8_iSSE
cglobal Luma8x8_iSSE
cglobal Var4x4_iSSE
cglobal Luma4x4_iSSE
cglobal Var8x4_iSSE
cglobal Luma8x4_iSSE
cglobal Var16x8_iSSE
cglobal Luma16x8_iSSE
cglobal Var16x2_iSSE
cglobal Luma16x2_iSSE

;-----------------------------------------------------------------------------
;
; unsigned int Var16x16_iSSE(const unsigned char *pSrc,
;					         int nPitch,
;						     int *pLuma);
;
;-----------------------------------------------------------------------------

ALIGN 16
Var16x16_iSSE:

    push esi
    push edi

    mov esi, [esp + 8 + 4]		; pSrc --> esi
    mov edx, [esp + 8 + 8]		; nPitch --> edx
    mov edi, [esp + 8 + 12]		; pLuma --> edi

    ; computes the mean
    pxor mm7, mm7				; 0 --> mm7
    pxor mm6, mm6				; 0 --> mm6

    VAR16x16

    movd eax, mm0
    mov [edi], eax				; *pLuma <-- sum

    add eax, 128
    shr eax, 8

    movd mm0, eax				; mm0 <-- (sum + 128) / 256 = mean

    pshufw mm1, mm0, 0			; mm1 contains 4 times dw mean

    movq mm7, mm1
    mov esi, [esp + 8 + 4]		; pSrc --> esi
    psllq mm7, 8

    por mm7, mm1				; mm7 contains 8 times db mean
    movq mm6, mm7				; mm6 <-- 8 times db mean

    VAR16x16

    movd eax, mm0

    pop edi
    pop esi

    ret

;-----------------------------------------------------------------------------
;
; unsigned int Luma16x16_iSSE(const unsigned char *pSrc,
;				 		      int nPitch);
;
;-----------------------------------------------------------------------------

ALIGN 16
Luma16x16_iSSE:

    push esi
    push edi

    mov esi, [esp + 8 + 4]		; pSrc --> esi
    mov edx, [esp + 8 + 8]		; nPitch --> ebx

    ; computes the mean
    pxor mm7, mm7				; 0 --> mm7
    pxor mm6, mm6				; 0 --> mm6

    VAR16x16

    movd eax, mm0

    pop edi
    pop esi

    ret


;-----------------------------------------------------------------------------
;
; unsigned int Var8x8_iSSE(const unsigned char *pSrc,
;						   int nPitch,
;						   int *pLuma);
;
;-----------------------------------------------------------------------------

ALIGN 16
Var8x8_iSSE:

    push esi
    push edi

    mov esi, [esp + 8 + 4]		; pSrc --> esi
    mov edx, [esp + 8 + 8]		; nPitch --> edx
    mov edi, [esp + 8 + 12]		; pLuma --> edi

    mov ecx, edx
    shl ecx, 1
    add ecx, edx				; 3 * nPitch --> ecx

    ; computes the mean
    pxor mm7, mm7				; 0 --> mm7
    pxor mm6, mm6				; 0 --> mm6

    VAR8x8

    movd eax, mm0
    mov [edi], eax				; *pLuma <-- sum

    add eax, 32
    shr eax, 6

    movd mm0, eax				; mm0 <-- (sum + 32) / 64 = mean

    pshufw mm1, mm0, 0			; mm1 contains 4 times dw mean

    movq mm7, mm1
    mov esi, [esp + 8 + 4]		; pSrc --> esi
    psllq mm7, 8

    por mm7, mm1				; mm7 contains 8 times db mean
    movq mm6, mm7				; mm6 <-- 8 times db mean

    VAR8x8

    movd eax, mm0

    pop edi
    pop esi

    ret

;-----------------------------------------------------------------------------
;
; unsigned int Luma8x8_iSSE(const unsigned char *pSrc,
;				 		    int nPitch);
;
;-----------------------------------------------------------------------------

ALIGN 16
Luma8x8_iSSE:

    push esi
    push edi

    mov esi, [esp + 8 + 4]		; pSrc --> esi
    mov edx, [esp + 8 + 8]		; nPitch --> ebx

    mov ecx, edx
    shl ecx, 1
    add ecx, edx				; 3 * nPitch --> ecx

    ; computes the mean
    pxor mm7, mm7				; 0 --> mm7
    pxor mm6, mm6				; 0 --> mm6

    VAR8x8

    movd eax, mm0

    pop edi
    pop esi

    ret

;-----------------------------------------------------------------------------
;
; unsigned int Var8x4_iSSE(const unsigned char *pSrc,
;						   int nPitch,
;						   int *pLuma);
;
;-----------------------------------------------------------------------------

ALIGN 16
Var8x4_iSSE:

    push esi
    push edi

    mov esi, [esp + 8 + 4]		; pSrc --> esi
    mov edx, [esp + 8 + 8]		; nPitch --> edx
    mov edi, [esp + 8 + 12]		; pLuma --> edi

    mov ecx, edx
    shl ecx, 1
    add ecx, edx				; 3 * nPitch --> ecx

    ; computes the mean
    pxor mm7, mm7				; 0 --> mm7
    pxor mm6, mm6				; 0 --> mm6

    VAR8x4

    movd eax, mm0
    mov [edi], eax				; *pLuma <-- sum

    add eax, 16
    shr eax, 5

    movd mm0, eax				; mm0 <-- (sum + 16) / 32 = mean

    pshufw mm1, mm0, 0			; mm1 contains 4 times dw mean

    movq mm7, mm1
    mov esi, [esp + 8 + 4]		; pSrc --> esi
    psllq mm7, 8

    por mm7, mm1				; mm7 contains 8 times db mean
    movq mm6, mm7				; mm6 <-- 8 times db mean

    VAR8x4

    movd eax, mm0

    pop edi
    pop esi

    ret

;-----------------------------------------------------------------------------
;
; unsigned int Luma8x4_iSSE(const unsigned char *pSrc,
;				 		    int nPitch);
;
;-----------------------------------------------------------------------------

ALIGN 16
Luma8x4_iSSE:

    push esi
    push edi

    mov esi, [esp + 8 + 4]		; pSrc --> esi
    mov edx, [esp + 8 + 8]		; nPitch --> ebx

    mov ecx, edx
    shl ecx, 1
    add ecx, edx				; 3 * nPitch --> ecx

    ; computes the mean
    pxor mm7, mm7				; 0 --> mm7
    pxor mm6, mm6				; 0 --> mm6

    VAR8x4

    movd eax, mm0

    pop edi
    pop esi

    ret

;-----------------------------------------------------------------------------
;
; unsigned int Var4x4_iSSE(const unsigned char *pSrc,
;						   int nPitch,
;						   int *pLuma);
;
;-----------------------------------------------------------------------------

ALIGN 16
Var4x4_iSSE:

	push esi
	push edi

    mov esi, [esp + 8 + 4]		; pSrc --> esi
    mov edx, [esp + 8 + 8]		; nPitch --> edx
    mov edi, [esp + 8 + 12]		; pLuma --> edi

    ; computes the mean
    pxor mm7, mm7				; 0 --> mm7
    pxor mm6, mm6				; 0 --> mm6
    pxor mm5, mm5
    pxor mm4, mm4

    VAR4x4

    movd eax, mm0
    mov [edi], eax				; *pLuma <-- sum

    add eax, 8
    shr eax, 4

    movd mm0, eax				; mm0 <-- (sum + 8) / 16 = mean

    pshufw mm7, mm0, 0			; mm7 contains 4 times dw mean

    mov esi, [esp + 8 + 4]		; pSrc --> esi

    movq mm6, mm7				; mm6 <-- 8 times db mean

    VAR4x4

    movd eax, mm0

	pop edi
	pop esi

	ret

;-----------------------------------------------------------------------------
;
; unsigned int Luma4x4_iSSE(const unsigned char *pSrc,
;				 		    int nPitch);
;
;-----------------------------------------------------------------------------

ALIGN 16
Luma4x4_iSSE:

	push esi
	push edi

    mov esi, [esp + 8 + 4]		; pSrc --> esi
    mov edx, [esp + 8 + 8]		; nPitch --> edx
    mov edi, [esp + 8 + 12]		; pLuma --> edi

    ; computes the mean
    pxor mm7, mm7				; 0 --> mm7
    pxor mm6, mm6				; 0 --> mm6
    pxor mm5, mm5
    pxor mm4, mm4

    VAR4x4

    movd eax, mm0

	pop edi
	pop esi

	ret


;-----------------------------------------------------------------------------
;
; unsigned int Var16x8_iSSE(const unsigned char *pSrc,
;					         int nPitch,
;						     int *pLuma);
;
;-----------------------------------------------------------------------------

ALIGN 16
Var16x8_iSSE:

    push esi
    push edi

    mov esi, [esp + 8 + 4]		; pSrc --> esi
    mov edx, [esp + 8 + 8]		; nPitch --> edx
    mov edi, [esp + 8 + 12]		; pLuma --> edi

    ; computes the mean
    pxor mm7, mm7				; 0 --> mm7
    pxor mm6, mm6				; 0 --> mm6

    VAR16x8

    movd eax, mm0
    mov [edi], eax				; *pLuma <-- sum

    add eax, 64
    shr eax, 7

    movd mm0, eax				; mm0 <-- (sum + 64) / 128 = mean

    pshufw mm1, mm0, 0			; mm1 contains 4 times dw mean

    movq mm7, mm1
    mov esi, [esp + 8 + 4]		; pSrc --> esi
    psllq mm7, 8

    por mm7, mm1				; mm7 contains 8 times db mean
    movq mm6, mm7				; mm6 <-- 8 times db mean

    VAR16x8

    movd eax, mm0

    pop edi
    pop esi

    ret

;-----------------------------------------------------------------------------
;
; unsigned int Luma16x8_iSSE(const unsigned char *pSrc,
;				 		      int nPitch);
;
;-----------------------------------------------------------------------------

ALIGN 16
Luma16x8_iSSE:

    push esi
    push edi

    mov esi, [esp + 8 + 4]		; pSrc --> esi
    mov edx, [esp + 8 + 8]		; nPitch --> ebx

    ; computes the mean
    pxor mm7, mm7				; 0 --> mm7
    pxor mm6, mm6				; 0 --> mm6

    VAR16x8

    movd eax, mm0

    pop edi
    pop esi

    ret


;-----------------------------------------------------------------------------
;
; unsigned int Var16x2_iSSE(const unsigned char *pSrc,
;					         int nPitch,
;						     int *pLuma);
;
;-----------------------------------------------------------------------------

ALIGN 16
Var16x2_iSSE:

    push esi
    push edi

    mov esi, [esp + 8 + 4]		; pSrc --> esi
    mov edx, [esp + 8 + 8]		; nPitch --> edx
    mov edi, [esp + 8 + 12]		; pLuma --> edi

    ; computes the mean
    pxor mm7, mm7				; 0 --> mm7
    pxor mm6, mm6				; 0 --> mm6

	pxor mm0, mm0
	pxor mm1, mm1
	SUB_VAR16x16x2
	paddd mm0, mm1

    movd eax, mm0
    mov [edi], eax				; *pLuma <-- sum

    add eax, 16
    shr eax, 5

    movd mm0, eax				; mm0 <-- (sum + 16) / 32 = mean

    pshufw mm1, mm0, 0			; mm1 contains 4 times dw mean

    movq mm7, mm1
    mov esi, [esp + 8 + 4]		; pSrc --> esi
    psllq mm7, 8

    por mm7, mm1				; mm7 contains 8 times db mean
    movq mm6, mm7				; mm6 <-- 8 times db mean

	pxor mm0, mm0
	pxor mm1, mm1
	SUB_VAR16x16x2
	paddd mm0, mm1


    movd eax, mm0

    pop edi
    pop esi

    ret

;-----------------------------------------------------------------------------
;
; unsigned int Luma16x2_iSSE(const unsigned char *pSrc,
;				 		      int nPitch);
;
;-----------------------------------------------------------------------------

ALIGN 16
Luma16x2_iSSE:

    push esi
    push edi

    mov esi, [esp + 8 + 4]		; pSrc --> esi
    mov edx, [esp + 8 + 8]		; nPitch --> edx

    ; computes the mean
    pxor mm7, mm7				; 0 --> mm7
    pxor mm6, mm6				; 0 --> mm6

	pxor mm0, mm0
	pxor mm1, mm1
	SUB_VAR16x16x2
	paddd mm0, mm1

    movd eax, mm0

    pop edi
    pop esi

    ret

;-----------------------------------------------------------------------------
;
; unsigned int Var32x16_iSSE(const unsigned char *pSrc,
;					         int nPitch,
;						     int *pLuma);
;
;-----------------------------------------------------------------------------

ALIGN 16
Var32x16_iSSE:

    push esi
    push edi

    mov esi, [esp + 8 + 4]		; pSrc --> esi
    mov edx, [esp + 8 + 8]		; nPitch --> edx
    mov edi, [esp + 8 + 12]		; pLuma --> edi

    ; computes the mean
    pxor mm7, mm7				; 0 --> mm7
    pxor mm6, mm6				; 0 --> mm6

    VAR32x16

    movd eax, mm0
    mov [edi], eax				; *pLuma <-- sum

    add eax, 256
    shr eax, 9

    movd mm0, eax				; mm0 <-- (sum + 256) / 512 = mean

    pshufw mm1, mm0, 0			; mm1 contains 4 times dw mean

    movq mm7, mm1
    mov esi, [esp + 8 + 4]		; pSrc --> esi
    psllq mm7, 8

    por mm7, mm1				; mm7 contains 8 times db mean
    movq mm6, mm7				; mm6 <-- 8 times db mean

    VAR32x16

    movd eax, mm0

    pop edi
    pop esi

    ret

;-----------------------------------------------------------------------------
;
; unsigned int Luma16x16_iSSE(const unsigned char *pSrc,
;				 		      int nPitch);
;
;-----------------------------------------------------------------------------

ALIGN 16
Luma32x16_iSSE:

    push esi
    push edi

    mov esi, [esp + 8 + 4]		; pSrc --> esi
    mov edx, [esp + 8 + 8]		; nPitch --> ebx

    ; computes the mean
    pxor mm7, mm7				; 0 --> mm7
    pxor mm6, mm6				; 0 --> mm6

    VAR32x16

    movd eax, mm0

    pop edi
    pop esi

    ret

