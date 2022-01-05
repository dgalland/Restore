%INCLUDE "common.asm"

SECTION .rodata
ALIGN 16
   MT_BYTE_80     : TIMES 16 DB 0x80

;=============================================================================
; Code
;=============================================================================

SECTION .text

   mt_mangle Binarize, lower8_mmx
   mt_mangle Binarize, upper8_mmx
   mt_mangle Binarize, lower8_isse
   mt_mangle Binarize, upper8_isse
   mt_mangle Binarize, lower8_3dnow
   mt_mangle Binarize, upper8_3dnow
   mt_mangle Binarize, lower8_sse2
   mt_mangle Binarize, upper8_sse2
   mt_mangle Binarize, lower8_asse2
   mt_mangle Binarize, upper8_asse2
   
%MACRO MT_LOWER_ROLLED  0
   movq                 mm0, [edi + eax]
   paddb                mm0, mm6
   pcmpgtb              mm0, mm7
   movq                 [edi + eax], mm0
%ENDMACRO

%MACRO MT_UPPER_ROLLED  0
   movq                 mm0, [edi + eax]
   psubusb              mm0, mm7
   pcmpeqb              mm0, mm6
   movq                 [edi + eax], mm0
%ENDMACRO

%MACRO MT_LOWER_UNROLLED 1

   SET_XMMX             %1
   
   PREFETCH_RW          edi + eax + 320

%ASSIGN offset 0
%REP 64 / (RWIDTH * 4)
   LOAD                 R0, [edi + eax + offset + RWIDTH * 0]
   LOAD                 R1, [edi + eax + offset + RWIDTH * 1]
   LOAD                 R2, [edi + eax + offset + RWIDTH * 2]
   LOAD                 R3, [edi + eax + offset + RWIDTH * 3]
   paddb                R0, R6
   paddb                R1, R6
   paddb                R2, R6
   paddb                R3, R6
   pcmpgtb              R0, R7
   pcmpgtb              R1, R7
   pcmpgtb              R2, R7
   pcmpgtb              R3, R7
   STORE                [edi + eax + offset + RWIDTH * 0], R0
   STORE                [edi + eax + offset + RWIDTH * 1], R1
   STORE                [edi + eax + offset + RWIDTH * 2], R2
   STORE                [edi + eax + offset + RWIDTH * 3], R3
%ASSIGN offset offset + 4 * RWIDTH
%ENDREP   

%ENDMACRO

%MACRO MT_UPPER_UNROLLED 1
   SET_XMMX             %1
   
   PREFETCH_RW          edi + eax + 320

%ASSIGN offset 0
%REP 64 / (RWIDTH * 4)
   LOAD                 R0, [edi + eax + offset + RWIDTH * 0]
   LOAD                 R1, [edi + eax + offset + RWIDTH * 1]
   LOAD                 R2, [edi + eax + offset + RWIDTH * 2]
   LOAD                 R3, [edi + eax + offset + RWIDTH * 3]
   psubusb              R0, R7
   psubusb              R1, R7
   psubusb              R2, R7
   psubusb              R3, R7
   pcmpeqb              R0, R6
   pcmpeqb              R1, R6
   pcmpeqb              R2, R6
   pcmpeqb              R3, R6
   STORE                [edi + eax + offset + RWIDTH * 0], R0
   STORE                [edi + eax + offset + RWIDTH * 1], R1
   STORE                [edi + eax + offset + RWIDTH * 2], R2
   STORE                [edi + eax + offset + RWIDTH * 3], R3
%ASSIGN offset offset + 4 * RWIDTH
%ENDREP   

%ENDMACRO

%MACRO MT_BINARIZE_ENDLINE 0
   add                  edi, [esp + .nDstPitch]
%ENDMACRO

%MACRO MT_BINARIZE_COMMON 3
%2 %+ 8_ %+ %1 :
   SET_XMMX             %1
   STACK                edi, ebp
   .pDst                PARAMETER(1)
   .nDstPitch           PARAMETER(2)
   .nThreshold          PARAMETER(3)
   .nWidth              PARAMETER(4)
   .nHeight             PARAMETER(5)
   
   mov                  eax, [esp + .nThreshold]
%IFIDNI %2, lower
   add                  eax, 128
   movq                 mm6, [MT_BYTE_80]   
%ELSE
   pxor                 mm6, mm6
%ENDIF
   movd                 mm7, eax
   punpcklbw            mm7, mm7
   punpcklwd            mm7, mm7
   punpckldq            mm7, mm7
   
   
%IFIDNI REGISTERS, xmm
   movq2dq              xmm7, mm7
   movq2dq              xmm6, mm6
   punpcklbw            xmm7, xmm7
   punpcklbw            xmm6, xmm6
%ENDIF   
   
   mov                  edi, [esp + .pDst]

   mov                  ebp, [esp + .nDstPitch]
   sub                  ebp, [esp + .nWidth]
   
   MT_UNROLL_INPLACE_WIDTH    {MT_ %+ %3 %+ _UNROLLED %1}, MT_ %+ %3 %+ _ROLLED, MT_BINARIZE_ENDLINE
   
   RETURN
%ENDMACRO

MT_BINARIZE_COMMON   mmx, lower, LOWER
MT_BINARIZE_COMMON   isse, lower, LOWER
MT_BINARIZE_COMMON   3dnow, lower, LOWER
MT_BINARIZE_COMMON   sse2, lower, LOWER
MT_BINARIZE_COMMON   asse2, lower, LOWER

MT_BINARIZE_COMMON   mmx, upper, UPPER
MT_BINARIZE_COMMON   isse, upper, UPPER
MT_BINARIZE_COMMON   3dnow, upper, UPPER
MT_BINARIZE_COMMON   sse2, upper, UPPER
MT_BINARIZE_COMMON   asse2, upper, UPPER
   




