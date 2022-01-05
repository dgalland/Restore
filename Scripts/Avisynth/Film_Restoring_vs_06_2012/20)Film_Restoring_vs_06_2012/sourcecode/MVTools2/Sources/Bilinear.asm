;/****************************************************************************
; * $Id$
; *
; ***************************************************************************/
; assembler (NASM) interpolation functions (iSSE optimization)
;// Copyright (c)2005 Manao (VerticalBilin, HorizontalBilin, DiagonalBilin, RB2F)
;// Copyright (c)2006 Fizick - Wiener, Bicubic
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

ALIGN 16
mmx_one:
	times 4	dw 1
mask:
;	db 255,0,0,0,0,0,0,0
	db 0,0,0,0,0,0,0,255 ; bug fixed by Fizick
maskw:
	db 0,0,0,255,255,255,255,255
mmx_ones:
	times 8 db 1
mask4:
	times 4 dw 255

;=============================================================================
; Macros
;=============================================================================


;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal VerticalBilin_iSSE
cglobal HorizontalBilin_iSSE
cglobal DiagonalBilin_iSSE
cglobal RB2F_iSSE
cglobal VerticalWiener_iSSE
cglobal HorizontalWiener_iSSE
cglobal VerticalBicubic_iSSE
cglobal HorizontalBicubic_iSSE

;
;void VerticalBilin_iSSE(unsigned char *pDst, const unsigned char *pSrc
;                        int nDstPitch, int nSrcPitch,
;                        int nWidth, int nHeight);
;

ALIGN 16
VerticalBilin_iSSE:

		push esi
		push edi
		push ebx

		mov edi, [esp + 16]		; pDst
		mov esi, [esp + 20]		; pSrc
		mov edx, [esp + 28]		; nSrcPitch
		mov eax, esi
		add eax, edx
		mov ecx, [esp + 36]		; nHeight
		dec ecx

v_loopy:

        xor ebx, ebx

v_loopx:
        movq mm1, [esi+ebx]
        movq mm2, [eax+ebx]
        pavgb mm1, mm2
        movq [edi+ebx], mm1
        add ebx, 8
        cmp ebx, [esp + 32]
        jl v_loopx

        add eax, edx
        add esi, edx
        add edi, [esp + 24]
        dec ecx
        jnz v_loopy

        xor ebx, ebx

v_loopfinal:

        movq mm1, [esi+ebx]
        movq [edi+ebx], mm1
        add ebx, 8
        cmp ebx, [esp + 32]
        jl v_loopfinal

        emms

        pop ebx
        pop edi
        pop esi

		ret

;
;void HorizontalBilin_iSSE(unsigned char *pDst, const unsigned char *pSrc
;                          int nDstPitch, int nSrcPitch,
;                          int nWidth, int nHeight);
;

ALIGN 16
HorizontalBilin_iSSE:

		push esi
		push edi
		push ebx

        mov edi, [esp + 16]		; pDst
        mov esi, [esp + 20]		; pSrc
        mov edx, [esp + 32]		; nWidth
        sub edx, 8
        mov ecx, [esp + 36]		; nHeight
        movq mm7, [mask]

h_loopy:

        xor ebx, ebx

h_loopx:
        movq mm1, [esi+ebx]
        movq mm2, [esi+ebx+1]
        pavgb mm1, mm2
        movq [edi+ebx], mm1
        add ebx, 8
        cmp ebx, edx
        jl h_loopx

        movq mm2, [esi+ebx]
        movq mm1, [esi+ebx]
        movq mm3, mm2
        psrlq mm2, 8
        pand mm3, mm7
        por mm3, mm2
        pavgb mm1, mm3 ; fixed by Fizick
        movq [edi+ebx], mm1

        add esi, [esp + 28]
        add edi, [esp + 24]
        dec ecx
        jnz h_loopy

        emms

        pop ebx
        pop edi
        pop esi

        ret


;
;void DiagonalBilin_iSSE(unsigned char *pDst, const unsigned char *pSrc
;                        int nDstPitch, int nSrcPitch,
;                        int nWidth, int nHeight);
;

ALIGN 16
DiagonalBilin_iSSE:

		push esi
		push edi
		push ebx

        mov edi, [esp + 16]
        mov esi, [esp + 20]
        mov edx, [esp + 28]
        mov eax, [esp + 20]
        add eax, edx
        mov edx, [esp + 32]
        sub edx, 8
        mov ecx, [esp + 36]
        dec ecx

        movq mm7, [mmx_ones]
        movq mm6, [mask]

d_loopy:

        xor ebx, ebx

d_loopx:

        movq mm0, [esi+ebx]
        movq mm1, [esi+ebx+1]

        movq mm2, [eax+ebx]
        movq mm3, [eax+ebx+1]

        movq mm4, mm0
        movq mm5, mm2

        pxor mm4, mm1
        pxor mm5, mm3

        pavgb mm0, mm1
        pavgb mm2, mm3

        por mm4, mm5

        movq mm5, mm0

        pxor mm5, mm2
        pand mm4, mm7

        pand mm4, mm5

        pavgb mm0, mm2

        psubb mm0, mm4

        movq [edi+ebx], mm0

        add ebx, 8
        cmp ebx, edx
        jl d_loopx

        movq mm0, [esi+ebx]
        movq mm2, [eax+ebx]

        movq mm1, [esi+ebx]
        movq mm3, [eax+ebx]

        movq mm4, mm0
        movq mm5, mm2

        pand mm4, mm6
        psrlq mm1, 8

        pand mm5, mm6
        psrlq mm3, 8

        por mm1, mm4
        por mm3, mm5

        movq mm4, mm0
        movq mm5, mm2

        pxor mm4, mm1
        pxor mm5, mm3

        pavgb mm0, mm1
        pavgb mm2, mm3

        por mm4, mm5
        movq mm5, mm0

        pxor mm5, mm2

        pand mm4, mm5
        pavgb mm0, mm2

        pand mm4, mm7

        paddb mm0, mm4

        movq [edi+ebx], mm0

        add esi, [esp + 28]
        add edi, [esp + 24]
        add eax, [esp + 28]
        dec ecx
        jnz d_loopy

        xor ebx, ebx

d_loop_final :

        movq mm1, [esi+ebx]
        movq mm2, [esi+ebx+1]
        pavgb mm1, mm2
        movq [edi+ebx], mm1
        add ebx, 8
        cmp ebx, edx
        jl d_loop_final

        movq mm2, [esi+ebx]
        movq mm1, [esi+ebx]
        movq mm3, mm2
        psrlq mm2, 8
        pand mm3, mm6
        por mm3, mm2
        pavgb mm1, mm2
        movq [edi+ebx], mm1

		emms ; Fizick

        pop ebx
        pop edi
        pop esi

        ret

;
;void RB2F_iSSE(unsigned char *pDst, const unsigned char *pSrc
;               int nDstPitch, int nSrcPitch,
;               int nWidth, int nHeight);
;

ALIGN 16
RB2F_iSSE:

		push esi
		push edi
		push ebx
		push ebp

        mov edi, [esp + 20]
        mov esi, [esp + 24]
        mov edx, [esp + 32]
        mov eax, [esp + 24]
        add eax, edx
        shl edx, 1
        mov ebp, edx
        mov edx, [esp + 36]
        sub edx, 4
        mov ecx, [esp + 40]

        movq mm7, [mmx_ones]
        pxor mm6, mm6

r_loopy:

        xor ebx, ebx

r_loopx:
        movq mm0, [esi+ebx*2]
        movq mm2, [eax+ebx*2]

        movq mm1, [esi+ebx*2+1]
        movq mm3, [eax+ebx*2+1]

        movq mm4, mm0
        movq mm5, mm2

        pxor mm4, mm1
        pxor mm5, mm3

        pavgb mm0, mm1
        pavgb mm2, mm3

        por mm4, mm5

        movq mm5, mm0

        pxor mm5, mm2
        pand mm4, mm7

        pand mm4, mm5

        pavgb mm0, mm2

        psubb mm0, mm4

        pand mm0, [mask4]

        packuswb mm0, mm6

        movd [edi+ebx], mm0

        add ebx, 4
        cmp ebx, edx
        jl r_loopx

        movq mm0, [esi+ebx*2]
        movq mm2, [eax+ebx*2]

        movq mm1, [esi+ebx*2]
        movq mm3, [eax+ebx*2]

        movq mm4, mm0
        movq mm5, mm2

        psrlq mm1, 8
        psrlq mm3, 8

        pxor mm4, mm1
        pxor mm5, mm3

        pavgb mm0, mm1
        pavgb mm2, mm3

        por mm4, mm5

        movq mm5, mm0

        pxor mm5, mm2
        pand mm4, mm7

        pand mm4, mm5

        pavgb mm0, mm2

        psubb mm0, mm4

        pand mm0, [mask4]

        packuswb mm0, mm6

        movd [edi+ebx], mm0

        add esi, ebp
        add edi, [esp + 28]
        add eax, ebp
        dec ecx
        jnz r_loopy

        emms

        pop ebp
        pop ebx
        pop edi
        pop esi

        ret

;void VerticalWiener_iSSE(unsigned char *pDst, const unsigned char *pSrc
;                        int nDstPitch, int nSrcPitch,
;                        int nWidth, int nHeight); added in v1.3 by Fizick
;

ALIGN 16
VerticalWiener_iSSE:

		push esi
		push edi
		push ebx

		mov edi, [esp + 16]		; pDst
		mov esi, [esp + 20]		; pSrc
		mov edx, [esp + 28]		; nSrcPitch
		mov eax, esi
		add eax, edx
		mov ecx, [esp + 36]		; nHeight
		dec ecx ; skip last line

		dec ecx ; skip 3 more last lines
		dec ecx
		dec ecx

	; line j=0
        xor ebx, ebx
vw_loopx0:
        movq mm0, [esi+ebx]
        movq mm1, [eax+ebx]
        pavgb mm0, mm1
        movq [edi+ebx], mm0
        add ebx, 8
        cmp ebx, [esp + 32] ; nWidth
        jl vw_loopx0

        add eax, edx
        add esi, edx
        add edi, [esp + 24] ; nDstPitch
		dec ecx


	; line j=1
        xor ebx, ebx
vw_loopx1:
        movq mm0, [esi+ebx]
        movq mm1, [eax+ebx]
        pavgb mm0, mm1
        movq [edi+ebx], mm0
        add ebx, 8
        cmp ebx, [esp + 32] ; nWidth
        jl vw_loopx1

        sub eax, edx
        sub esi, edx ; offset to start
        add edi, [esp + 24] ; nDstPitch
		dec ecx

	; Middle lines

	; prepare constants
		pcmpeqw mm7, mm7 ; = FFFFFFFF
		pxor mm6, mm6 ; =0
		psubw mm6, mm7 ; =1
		psllw mm6, 4 ; *16 = 16
		pxor mm7, mm7 ; =0

vw_loopy:
        xor ebx, ebx
vw_loopx:
		mov eax, esi
        movd mm0, [esi+ebx]
		add eax, edx
        movd mm1, [eax+ebx]
		add eax, edx
        movd mm2, [eax+ebx]
		add eax, edx
        movd mm3, [eax+ebx]
		add eax, edx
        movd mm4, [eax+ebx]
		add eax, edx
        movd mm5, [eax+ebx]

		punpcklbw mm0, mm7 ; convert bytes to words
		punpcklbw mm1, mm7
		punpcklbw mm2, mm7
		punpcklbw mm3, mm7
		punpcklbw mm4, mm7
		punpcklbw mm5, mm7

		paddw mm2, mm3
		psllw mm2, 2 ; *4
		movq mm3, mm2
		psllw mm3, 2 ; *4
		paddw mm2, mm3

		paddw mm0, mm5
		paddw mm0, mm2

		paddw mm0, mm6 ; +16

		paddw mm1, mm4 ; negative
		movq mm4, mm1
		psllw mm4, 2 ; *4
		paddw mm1, mm4

		psubusw mm0, mm1
		psrlw mm0, 5

		packuswb mm0, mm0 ; words to bytes

        movd [edi+ebx], mm0 ; low 4 bytes

        add ebx, 4
        cmp ebx, [esp + 32] ; nWidth
        jl vw_loopx

        add esi, edx
        add edi, [esp + 24] ; nDstPitch
        dec ecx
        jnz vw_loopy

	; Rest lines
		add esi, edx ; restore offset
		add esi, edx
		mov eax, esi
		add eax, edx

	; line j=h-4
        xor ebx, ebx
vw_loopxh_4:
        movq mm0, [esi+ebx]
        movq mm1, [eax+ebx]
        pavgb mm0, mm1
        movq [edi+ebx], mm0
        add ebx, 8
        cmp ebx, [esp + 32] ; nWidth
        jl vw_loopxh_4

        add eax, edx
        add esi, edx
        add edi, [esp + 24] ; nDstPitch
		dec ecx

	; line j=h-3
        xor ebx, ebx
vw_loopxh_3:
        movq mm0, [esi+ebx]
        movq mm1, [eax+ebx]
        pavgb mm0, mm1
        movq [edi+ebx], mm0
        add ebx, 8
        cmp ebx, [esp + 32] ; nWidth
        jl vw_loopxh_3

        add eax, edx
        add esi, edx
        add edi, [esp + 24] ; nDstPitch
		dec ecx

	; line j=h-2
        xor ebx, ebx
vw_loopxh_2:
        movq mm0, [esi+ebx]
        movq mm1, [eax+ebx]
        pavgb mm0, mm1
        movq [edi+ebx], mm0
        add ebx, 8
        cmp ebx, [esp + 32] ; nWidth
        jl vw_loopxh_2

        add eax, edx
        add esi, edx
        add edi, [esp + 24] ; nDstPitch
		dec ecx

	; last line
        xor ebx, ebx
vw_loopfinal:

        movq mm0, [esi+ebx]
        movq [edi+ebx], mm0
        add ebx, 8
        cmp ebx, [esp + 32]
        jl vw_loopfinal

        emms

        pop ebx
        pop edi
        pop esi

		ret

;
;void HorizontalWiener_iSSE(unsigned char *pDst, const unsigned char *pSrc
;                          int nDstPitch, int nSrcPitch,
;                          int nWidth, int nHeight);
;

ALIGN 16
HorizontalWiener_iSSE:

		push esi
		push edi
		push ebx

        mov edi, [esp + 16]		; pDst
        mov esi, [esp + 20]		; pSrc
        mov edx, [esp + 32]		; nWidth
        sub edx, 4
        mov ecx, [esp + 36]		; nHeight

	; prepare constants
		pcmpeqw mm7, mm7 ; = FFFFFFFF
		pxor mm6, mm6 ; =0
		psubw mm6, mm7 ; =1
		psllw mm6, 4 ; *16 = 16
		pxor mm7, mm7 ; =0

hw_loopy:

        xor ebx, ebx

	; first 4 averaged (really may be 2, but 4 is more convinient)
        movd mm1, [esi+ebx]
        movd mm2, [esi+ebx+1]
        pavgb mm1, mm2
        movd [edi+ebx], mm1
		add ebx, 4

hw_loopx:
	; all middle
        movd mm0, [esi+ebx-2]
        movd mm1, [esi+ebx-1]
        movd mm2, [esi+ebx]
        movd mm3, [esi+ebx+1]
        movd mm4, [esi+ebx+2]
        movd mm5, [esi+ebx+3]

; a little optimized ? version for memory read - disabled - not faster
;        movq mm0, [esi+ebx-4] ;read 8 bytes
;        psrlq mm0, 16; // right shift by 2 bytes to get [esi+ebx-2]
;        movq mm1,  mm0; copy
;        psrlq mm2, 8; // right shift by 1 byte more
;        movq mm3,  mm0;
;        psrlq mm3, 16; // right shift by 2 bytes more
;        movq mm4,  [esi+ebx-1];
;        psrlq mm4, 24; // right shift by 3 bytes to get [esi+ebx+2]
;        movq mm5,  mm4;
;        psrlq mm5, 32; // right shift by 4 bytes
; note: we will use low doubleword only, so high doubleword is not important

		punpcklbw mm0, mm7 ; convert bytes to words
		punpcklbw mm1, mm7
		punpcklbw mm2, mm7
		punpcklbw mm3, mm7
		punpcklbw mm4, mm7
		punpcklbw mm5, mm7

		paddw mm2, mm3
		psllw mm2, 2 ; *4
		movq mm3, mm2
		psllw mm3, 2 ; *4
		paddw mm2, mm3

		paddw mm0, mm5
		paddw mm0, mm2

		paddw mm0, mm6 ; +16

		paddw mm1, mm4 ; negative
		movq mm4, mm1
		psllw mm4, 2 ; *4
		paddw mm1, mm4

		psubusw mm0, mm1
		psrlw mm0, 5

		packuswb mm0, mm0 ; words to bytes

        movd [edi+ebx], mm0 ; low 4 bytes
        add ebx, 4
        cmp ebx, edx
        jl hw_loopx

	; last 4 (3 averaged, very last copied)

        movd mm2, [esi+ebx]
        movd mm1, [esi+ebx]
        movq mm3, mm2
        psrld mm2, 8
		movq mm5, [maskw]
        pand mm3, mm5
        por mm3, mm2
        pavgb mm1, mm3 ; fixed
        movd [edi+ebx], mm1

        add esi, [esp + 28]
        add edi, [esp + 24]
        dec ecx
        jnz hw_loopy

        emms

        pop ebx
        pop edi
        pop esi

        ret

;void VerticalBicubic_iSSE(unsigned char *pDst, const unsigned char *pSrc
;                        int nDstPitch, int nSrcPitch,
;                        int nWidth, int nHeight); added in v1.3 by Fizick
;

ALIGN 16
VerticalBicubic_iSSE:

		push esi
		push edi
		push ebx

		mov edi, [esp + 16]		; pDst
		mov esi, [esp + 20]		; pSrc
		mov edx, [esp + 28]		; nSrcPitch
		mov eax, esi
		add eax, edx
		mov ecx, [esp + 36]		; nHeight
		dec ecx ; skip last line

		dec ecx ; skip 2 more last lines
		dec ecx

	; line j=0
        xor ebx, ebx
vb_loopx0:
        movq mm0, [esi+ebx]
        movq mm1, [eax+ebx]
        pavgb mm0, mm1
        movq [edi+ebx], mm0
        add ebx, 8
        cmp ebx, [esp + 32] ; nWidth
        jl vb_loopx0

        add edi, [esp + 24] ; nDstPitch
		dec ecx

	; Middle lines

	; prepare constants
		pcmpeqw mm7, mm7 ; = FFFFFFFF
		pxor mm6, mm6 ; =0
		psubw mm6, mm7 ; =1
		psllw mm6, 3 ; *8 = 8
		pxor mm7, mm7 ; =0

vb_loopy:
        xor ebx, ebx
vb_loopx:
		mov eax, esi
        movd mm0, [esi+ebx]
		add eax, edx
        movd mm1, [eax+ebx]
		add eax, edx
        movd mm2, [eax+ebx]
		add eax, edx
        movd mm3, [eax+ebx]

		punpcklbw mm0, mm7 ; convert bytes to words
		punpcklbw mm1, mm7
		punpcklbw mm2, mm7
		punpcklbw mm3, mm7

		paddw mm1, mm2
		movq mm2, mm1
		psllw mm1, 3 ; *8
		paddw mm1, mm2

		paddw mm0, mm3

		paddw mm1, mm6 ; +8

		psubusw mm1, mm0
		psrlw mm1, 4	; /16

		packuswb mm1, mm1 ; words to bytes

        movd [edi+ebx], mm1 ; low 4 bytes

        add ebx, 4
        cmp ebx, [esp + 32] ; nWidth
        jl vb_loopx

        add esi, edx
        add edi, [esp + 24] ; nDstPitch
        dec ecx
        jnz vb_loopy

	; Rest lines
		add esi, edx ; restore offset
		mov eax, esi
		add eax, edx

	; line j=h-3
        xor ebx, ebx
vb_loopxh_3:
        movq mm0, [esi+ebx]
        movq mm1, [eax+ebx]
        pavgb mm0, mm1
        movq [edi+ebx], mm0
        add ebx, 8
        cmp ebx, [esp + 32] ; nWidth
        jl vb_loopxh_3

        add eax, edx
        add esi, edx
        add edi, [esp + 24] ; nDstPitch
		dec ecx

	; line j=h-2
        xor ebx, ebx
vb_loopxh_2:
        movq mm0, [esi+ebx]
        movq mm1, [eax+ebx]
        pavgb mm0, mm1
        movq [edi+ebx], mm0
        add ebx, 8
        cmp ebx, [esp + 32] ; nWidth
        jl vb_loopxh_2

        add eax, edx
        add esi, edx
        add edi, [esp + 24] ; nDstPitch
		dec ecx

	; last line
        xor ebx, ebx
vb_loopfinal:

        movq mm0, [esi+ebx]
        movq [edi+ebx], mm0
        add ebx, 8
        cmp ebx, [esp + 32]
        jl vb_loopfinal

        emms

        pop ebx
        pop edi
        pop esi

		ret

;
;void HorizontalBicubic_iSSE(unsigned char *pDst, const unsigned char *pSrc
;                          int nDstPitch, int nSrcPitch,
;                          int nWidth, int nHeight);
;

ALIGN 16
HorizontalBicubic_iSSE:

		push esi
		push edi
		push ebx

        mov edi, [esp + 16]		; pDst
        mov esi, [esp + 20]		; pSrc
        mov edx, [esp + 32]		; nWidth
        sub edx, 4
        mov ecx, [esp + 36]		; nHeight

	; prepare constants
		pcmpeqw mm7, mm7 ; = FFFFFFFF
		pxor mm6, mm6 ; =0
		psubw mm6, mm7 ; =1
		psllw mm6, 3 ; *8 = 8
		pxor mm7, mm7 ; =0

hb_loopy:

        xor ebx, ebx

	; first 4 averaged (really may be 1, but 4 is more convinient)
        movd mm1, [esi+ebx]
        movd mm2, [esi+ebx+1]
        pavgb mm1, mm2
        movd [edi+ebx], mm1
		add ebx, 4

hb_loopx:
	; all middle
        movd mm0, [esi+ebx-1]
        movd mm1, [esi+ebx]
        movd mm2, [esi+ebx+1]
        movd mm3, [esi+ebx+2]

		punpcklbw mm0, mm7 ; convert bytes to words
		punpcklbw mm1, mm7
		punpcklbw mm2, mm7
		punpcklbw mm3, mm7

		paddw mm1, mm2
		movq mm2, mm1 ; copy
		psllw mm1, 3 ; *8
		paddw mm1, mm2 ; 8+1=9

		paddw mm0, mm3

		paddw mm1, mm6 ; +8

		psubusw mm1, mm0
		psrlw mm1, 4 ; /16

		packuswb mm1, mm1 ; words to bytes

        movd [edi+ebx], mm1 ; low 4 bytes
        add ebx, 4
        cmp ebx, edx
        jl hb_loopx

	; last 4 (3 averaged, very last copied)

        movd mm2, [esi+ebx]
        movd mm1, [esi+ebx]
        movq mm3, mm2
        psrld mm2, 8
		movq mm5, [maskw]
        pand mm3, mm5
        por mm3, mm2
        pavgb mm1, mm3 ; fixed
        movd [edi+ebx], mm1

        add esi, [esp + 28]
        add edi, [esp + 24]
        dec ecx
        jnz hb_loopy

        emms

        pop ebx
        pop edi
        pop esi

        ret
