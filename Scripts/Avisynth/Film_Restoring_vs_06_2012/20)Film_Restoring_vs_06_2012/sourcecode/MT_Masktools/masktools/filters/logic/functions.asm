%INCLUDE "common.asm"

SECTION .rodata

;=============================================================================
; Code
;=============================================================================

SECTION .text

   mt_mangle Logic, and8_mmx
   mt_mangle Logic, and8_isse
   mt_mangle Logic, and8_3dnow
   mt_mangle Logic, and8_sse2
   mt_mangle Logic, and8_asse2

   mt_mangle Logic, or8_mmx
   mt_mangle Logic, or8_isse
   mt_mangle Logic, or8_3dnow
   mt_mangle Logic, or8_sse2
   mt_mangle Logic, or8_asse2

   mt_mangle Logic, andn8_mmx
   mt_mangle Logic, andn8_isse
   mt_mangle Logic, andn8_3dnow
   mt_mangle Logic, andn8_sse2
   mt_mangle Logic, andn8_asse2

   mt_mangle Logic, xor8_mmx
   mt_mangle Logic, xor8_isse
   mt_mangle Logic, xor8_3dnow
   mt_mangle Logic, xor8_sse2
   mt_mangle Logic, xor8_asse2

   mt_mangle Logic, max8_isse
   mt_mangle Logic, max8_3dnow
   mt_mangle Logic, max8_sse2
   mt_mangle Logic, max8_asse2
   
   mt_mangle Logic, min8_isse
   mt_mangle Logic, min8_3dnow
   mt_mangle Logic, min8_sse2
   mt_mangle Logic, min8_asse2
   
%MACRO MT_LOGIC_ROLLED 1
   movq              mm0, [edi + eax]
   %1                mm0, [esi + eax]
   movq              [edi + eax], mm0
%ENDMACRO

%MACRO MT_LOGIC_UNROLLED 2

   SET_XMMX          %1

   PREFETCH_RW       edi + eax + 384
   PREFETCH_R        esi + eax + 384

%ASSIGN offset 0
%REP 64 / (4 * RWIDTH)
   LOAD              R0, [edi + eax + offset + RWIDTH * 0]
   LOAD              R1, [edi + eax + offset + RWIDTH * 1]
   LOAD              R2, [edi + eax + offset + RWIDTH * 2]
   LOAD              R3, [edi + eax + offset + RWIDTH * 3]
   
%IFNIDNI LOAD, ALOAD
   LOAD              R4, [esi + eax + offset + RWIDTH * 0]
   LOAD              R5, [esi + eax + offset + RWIDTH * 1]
   LOAD              R6, [esi + eax + offset + RWIDTH * 2]
   LOAD              R7, [esi + eax + offset + RWIDTH * 3]
   %2                R0, R4
   %2                R1, R5
   %2                R2, R6
   %2                R3, R7
%ELSE
   %2                R0, [esi + eax + offset + RWIDTH * 0]
   %2                R1, [esi + eax + offset + RWIDTH * 1]
   %2                R2, [esi + eax + offset + RWIDTH * 2]
   %2                R3, [esi + eax + offset + RWIDTH * 3]
%ENDIF
   
   STORE             [edi + eax + offset + RWIDTH * 0], R0
   STORE             [edi + eax + offset + RWIDTH * 1], R1
   STORE             [edi + eax + offset + RWIDTH * 2], R2
   STORE             [edi + eax + offset + RWIDTH * 3], R3
   
%ASSIGN offset offset + RWIDTH * 4
%ENDREP

%ENDMACRO

%MACRO MT_LOGIC_ENDLINE 0
   add                  edi, [esp + .nDstPitch]
   add                  esi, [esp + .nSrcPitch]
%ENDMACRO

%MACRO MT_LOGIC_COMMON  2
   STACK                esi, edi
   .pDst                PARAMETER(1)
   .nDstPitch           PARAMETER(2)
   .pSrc                PARAMETER(3)
   .nSrcPitch           PARAMETER(4)
   .nWidth              PARAMETER(5)
   .nHeight             PARAMETER(6)

   mov                  edi, [esp + .pDst]
   mov                  esi, [esp + .pSrc]
   
   MT_UNROLL_INPLACE_WIDTH  {MT_LOGIC_UNROLLED %1, %2}, {MT_LOGIC_ROLLED %2}, MT_LOGIC_ENDLINE
   
   RETURN
%ENDMACRO

%MACRO MT_LOGIC 2-*
%DEFINE name %1
%DEFINE instruction %2
%ROTATE 2
%REP %0 - 2
name %+ 8_ %+ %1:
   MT_LOGIC_COMMON %1, instruction
%ROTATE 1   
%ENDREP
%ENDMACRO

MT_LOGIC and , pand  , mmx, isse, 3dnow, sse2, asse2
MT_LOGIC andn, pandn , mmx, isse, 3dnow, sse2, asse2
MT_LOGIC or  , por   , mmx, isse, 3dnow, sse2, asse2
MT_LOGIC xor , pxor  , mmx, isse, 3dnow, sse2, asse2
MT_LOGIC min , pminub,      isse, 3dnow, sse2, asse2
MT_LOGIC max , pmaxub,      isse, 3dnow, sse2, asse2
