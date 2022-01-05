%INCLUDE "common.asm"

SECTION .rodata
ALIGN 16
   MT_WORD_128    : TIMES 8 DW 0x0080
   MT_WORD_256    : TIMES 8 DW 0x0100
   MT_BYTE_ZEROING: TIMES 8 DW 0x00FF

;=============================================================================
; Code
;=============================================================================

SECTION .text

   mt_mangle Merge, merge4_mmx
   mt_mangle Merge, merge8_mmx
   mt_mangle Merge, merge8_isse
   mt_mangle Merge, merge8_3dnow
   mt_mangle Merge, merge8_sse2
   mt_mangle Merge, merge8_asse2
   mt_mangle Merge, merge_luma_4208_mmx
   mt_mangle Merge, merge_luma_4208_isse
   mt_mangle Merge, merge_luma_4208_3dnow
   mt_mangle Merge, merge_luma_4208_sse2

%MACRO MT_LUMA_LOAD_420 2
   SET_XMMX             %1
%IFIDNI LOAD, ALOAD   
   ULOAD                R2, [ebx + 2 * eax + 2 * %2 + 1 + 0 * RWIDTH]
   ULOAD                R6, [ebx + 2 * eax + 2 * %2 + 1 + 1 * RWIDTH]
   ULOAD                R1, [ebp + 2 * eax + 2 * %2 + 1 + 0 * RWIDTH]
   ULOAD                R0, [ebp + 2 * eax + 2 * %2 + 1 + 1 * RWIDTH]
   pavgb                R2, [ebx + 2 * eax + 2 * %2 + 0 * RWIDTH]
   pavgb                R6, [ebx + 2 * eax + 2 * %2 + 1 * RWIDTH]
   pavgb                R1, [ebp + 2 * eax + 2 * %2 + 0 * RWIDTH]
   pavgb                R0, [ebp + 2 * eax + 2 * %2 + 1 * RWIDTH]
   pavgb                R2, R1
   pavgb                R6, R0
%ELSE
   ULOAD                R2, [ebx + 2 * eax + 2 * %2 + 1 + 0 * RWIDTH]
   ULOAD                R1, [ebx + 2 * eax + 2 * %2 + 0 * RWIDTH]
   ULOAD                R6, [ebp + 2 * eax + 2 * %2 + 1 + 0 * RWIDTH]
   ULOAD                R0, [ebp + 2 * eax + 2 * %2 + 0 * RWIDTH]
   pavgb                R2, R1
   pavgb                R6, R0
   ULOAD                R1, [ebx + 2 * eax + 2 * %2 + 1 + 1 * RWIDTH]
   ULOAD                R0, [ebx + 2 * eax + 2 * %2 + 1 * RWIDTH]
   pavgb                R2, R6
   pavgb                R1, R0
   ULOAD                R6, [ebp + 2 * eax + 2 * %2 + 1 + 1 * RWIDTH]
   ULOAD                R0, [ebp + 2 * eax + 2 * %2 + 1 * RWIDTH]
   pavgb                R6, R0
   pavgb                R6, R1
%ENDIF   
   pand                 R2, [MT_BYTE_ZEROING]
   pand                 R6, [MT_BYTE_ZEROING]
%ENDMACRO    

%MACRO MT_LOAD          2
   SET_XMMX             %1
   HLOAD                R2, [ebx + eax + RHWIDTH * 0 + %2]
   HLOAD                R6, [ebx + eax + RHWIDTH * 1 + %2]
%ENDMACRO

%MACRO MT_LUMA_END_LOAD_420 1
%ENDMACRO

%MACRO MT_END_LOAD      1
   SET_XMMX             %1
   punpcklbw            R2, R7
   punpcklbw            R6, R7
%ENDMACRO
   
%MACRO MT_SUB_MERGE     4

   SET_XMMX             %1
   
   ALOAD                R3, [MT_WORD_256]
   
   %3                   %1, %2
   
   HLOAD                R1, [edi + eax + RHWIDTH * 0 + %2]
   HLOAD                R5, [edi + eax + RHWIDTH * 1 + %2]
   HLOAD                R0, [esi + eax + RHWIDTH * 0 + %2]
   HLOAD                R4, [esi + eax + RHWIDTH * 1 + %2]

   %4                   %1
      
   punpcklbw            R1, R7
   punpcklbw            R5, R7
   psubw                R3, R2
   punpcklbw            R0, R7
   punpcklbw            R4, R7
   pmullw               R1, R3
   ALOAD                R3, [MT_WORD_256]
   pmullw               R4, R6
   pmullw               R0, R2
   psubw                R3, R6
   paddw                R4, [MT_WORD_128]
   pmullw               R5, R3
   paddw                R0, [MT_WORD_128]
   paddw                R4, R5
   paddw                R0, R1
   psrlw                R4, 8
   psrlw                R0, 8

   packuswb             R0, R4
   
   STORE                [edi + eax + %2], R0
%ENDMACRO

%MACRO MT_MERGE         2
   MT_SUB_MERGE         %1, %2, MT_LOAD, MT_END_LOAD
%ENDMACRO

%MACRO MT_LUMA_MERGE_420 2
   MT_SUB_MERGE         %1, %2, MT_LUMA_LOAD_420, MT_LUMA_END_LOAD_420
%ENDMACRO   

%MACRO MT_MERGE_ROLLED  1
   SET_XMMX             %1
   MT_MERGE             DOWNGRADED, 0
%ENDMACRO

%MACRO MT_LUMA_MERGE_420_ROLLED 1
   SET_XMMX             %1
   MT_LUMA_MERGE_420    DOWNGRADED, 0
%ENDMACRO

%MACRO MT_MERGE_UNROLLED 1
   SET_XMMX             %1
   PREFETCH_RW          edi + eax + 64
   PREFETCH_R           esi + eax + 64
   PREFETCH_R           ebx + eax + 64
   
%ASSIGN offset 0
%REP 64 / RWIDTH
   MT_MERGE             %1, offset
%ASSIGN offset offset + RWIDTH
%ENDREP
%ENDMACRO

%MACRO MT_LUMA_MERGE_420_UNROLLED 1
   SET_XMMX             %1
   PREFETCH_RW          edi + eax + 64
   PREFETCH_R           esi + eax + 64
   
%ASSIGN offset 0
%REP 64 / RWIDTH
   MT_LUMA_MERGE_420    %1, offset
%ASSIGN offset offset + RWIDTH
%ENDREP
%ENDMACRO

%MACRO MT_MERGE_ENDLINE 0
   add                  edi, [esp + .nDstPitch]
   add                  esi, [esp + .nSrc1Pitch]
   add                  ebx, [esp + .nSrc2Pitch]
%ENDMACRO

%MACRO MT_LUMA_MERGE_420_ENDLINE 0
   add                  edi, [esp + .nDstPitch]
   add                  esi, [esp + .nSrc1Pitch]
   add                  ebp, [esp + .nSrc2Pitch]
   mov                  ebx, ebp
   add                  ebp, [esp + .nSrc2Pitch]
%ENDMACRO

%MACRO MT_MERGE_COMMON  2
%2 %+ 8_ %+ %1:
   SET_XMMX             %1
   STACK                esi, edi, ebx
   .pDst                PARAMETER(1)
   .nDstPitch           PARAMETER(2)
   .pSrc1               PARAMETER(3)
   .nSrc1Pitch          PARAMETER(4)
   .pSrc2               PARAMETER(5)
   .nSrc2Pitch          PARAMETER(6)
   .nWidth              PARAMETER(7)
   .nHeight             PARAMETER(8)

   pxor                 mm7, mm7
%IFIDNI REGISTERS, xmm
   pxor                 xmm7, xmm7
%ENDIF   
   mov                  edi, [esp + .pDst]
   mov                  esi, [esp + .pSrc1]
   mov                  ebx, [esp + .pSrc2]
   
   MT_UNROLL_INPLACE_WIDTH  {MT_MERGE_UNROLLED %1}, {MT_MERGE_ROLLED %1}, MT_MERGE_ENDLINE
      
   RETURN
%ENDMACRO

%MACRO MT_LUMA_MERGE_420_COMMON  2
%2 %+ 8_ %+ %1:
   SET_XMMX             %1
   STACK                esi, edi, ebx, ebp
   .pDst                PARAMETER(1)
   .nDstPitch           PARAMETER(2)
   .pSrc1               PARAMETER(3)
   .nSrc1Pitch          PARAMETER(4)
   .pSrc2               PARAMETER(5)
   .nSrc2Pitch          PARAMETER(6)
   .nWidth              PARAMETER(7)
   .nHeight             PARAMETER(8)

   pxor                 mm7, mm7
%IFIDNI REGISTERS, xmm
   pxor                 xmm7, xmm7
%ENDIF   
   
   mov                  edi, [esp + .pDst]
   mov                  esi, [esp + .pSrc1]
   mov                  ebx, [esp + .pSrc2]
   mov                  ebp, ebx
   add                  ebp, [esp + .nSrc2Pitch]
   
   MT_UNROLL_INPLACE_WIDTH  {MT_LUMA_MERGE_420_UNROLLED %1}, {MT_LUMA_MERGE_420_ROLLED %1}, MT_LUMA_MERGE_420_ENDLINE
      
   RETURN
%ENDMACRO

MT_MERGE_COMMON mmx, merge
MT_MERGE_COMMON isse, merge
MT_MERGE_COMMON 3dnow, merge
MT_MERGE_COMMON sse2, merge
MT_MERGE_COMMON asse2, merge

MT_LUMA_MERGE_420_COMMON mmx, merge_luma_420
MT_LUMA_MERGE_420_COMMON isse, merge_luma_420
MT_LUMA_MERGE_420_COMMON 3dnow, merge_luma_420
MT_LUMA_MERGE_420_COMMON sse2, merge_luma_420

merge4_mmx:
   STACK                esi, edi, ebx, ebp
   .pDst                PARAMETER(1)
   .nDstPitch           PARAMETER(2)
   .pSrc1               PARAMETER(3)
   .nSrc1Pitch          PARAMETER(4)
   .pSrc2               PARAMETER(5)
   .nSrc2Pitch          PARAMETER(6)
   .nWidth              PARAMETER(7)
   .nHeight             PARAMETER(8)
   
   mov                  edi, [esp + .pDst]
   mov                  esi, [esp + .pSrc1]
   mov                  edx, [esp + .pSrc2]
   mov                  ecx, [esp + .nHeight]
   
   pxor                 mm7, mm7
   
.loopy

   mov                  eax, [esp + .nWidth]
   
.loopx

   sub                  eax, 4
   
   movd                 mm2, [edx + eax]
   movd                 mm0, [esi + eax]
   movd                 mm1, [edi + eax]
   movq                 mm3, [MT_WORD_256]
   movq                 mm4, [MT_WORD_128]
   
   punpcklbw            mm2, mm7
   punpcklbw            mm0, mm7
   punpcklbw            mm1, mm7
   
   psubw                mm3, mm2
   pmullw               mm0, mm2
   pmullw               mm1, mm3
   paddw                mm0, mm1
   paddw                mm0, mm4
   psrlw                mm0, 8
   
   packuswb             mm0, mm0
   
   movd                 [edi + eax], mm0
   
   test                 eax, eax
   jnz                  .loopx
   
   add                  edi, [esp + .nDstPitch]
   add                  esi, [esp + .nSrc1Pitch]
   add                  edx, [esp + .nSrc2Pitch]
   dec                  ecx
   jnz                  .loopy
   
   RETURN