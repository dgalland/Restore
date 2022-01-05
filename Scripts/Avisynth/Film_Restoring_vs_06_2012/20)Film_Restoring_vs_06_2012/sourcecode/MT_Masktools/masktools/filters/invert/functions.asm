%INCLUDE "common.asm"

SECTION .rodata
ALIGN 16
   MT_BYTE_FF     : TIMES 16 DB 0xFF


;=============================================================================
; Code
;=============================================================================

SECTION .text

   mt_mangle Invert, invert8_mmx
   mt_mangle Invert, invert8_isse
   mt_mangle Invert, invert8_3dnow
   mt_mangle Invert, invert8_sse2
   mt_mangle Invert, invert8_asse2
   
%MACRO MT_INVERT_ROLLED 0
   movq                 mm0, [edi + eax]
   pxor                 mm0, mm7
   movq                 [edi + eax], mm0
%ENDMACRO

%MACRO MT_INVERT_UNROLLED 1
   SET_XMMX             %1
   
   ;MT_PREFETCHT0        edi + eax + 640
   
%ASSIGN offset 0   
%REP 64 / (RWIDTH * 4)

   LOAD                 R0, [edi + eax + offset + 0 * RWIDTH]
   LOAD                 R1, [edi + eax + offset + 1 * RWIDTH]
   LOAD                 R2, [edi + eax + offset + 2 * RWIDTH]
   LOAD                 R3, [edi + eax + offset + 3 * RWIDTH]
   pxor                 R0, R7
   pxor                 R1, R7
   pxor                 R2, R7
   pxor                 R3, R7
   STORE                [edi + eax + offset + 0 * RWIDTH], R0
   STORE                [edi + eax + offset + 1 * RWIDTH], R1
   STORE                [edi + eax + offset + 2 * RWIDTH], R2
   STORE                [edi + eax + offset + 3 * RWIDTH], R3
   
%ASSIGN offset offset + 4 * RWIDTH
%ENDREP   

%ENDMACRO

%MACRO MT_INVERT_ENDLINE 0
   add                  edi, [esp + .nDstPitch]
%ENDMACRO

%MACRO MT_INVERT_COMMON 1
invert8_ %+ %1:
   SET_XMMX             %1
   STACK                edi
   .pDst                PARAMETER(1)
   .nDstPitch           PARAMETER(2)
   .nWidth              PARAMETER(3)
   .nHeight             PARAMETER(4)
   
   movq                 mm7, [MT_BYTE_FF]  
%IFIDNI REGISTERS, xmm
   movdqa               xmm7, [MT_BYTE_FF]  
%ENDIF   
   
   mov                  edi, [esp + .pDst]
   
   MT_UNROLL_INPLACE_WIDTH    {MT_INVERT_UNROLLED %1}, MT_INVERT_ROLLED, MT_INVERT_ENDLINE
   
   ;sfence
      
   RETURN
%ENDMACRO

   MT_INVERT_COMMON     mmx
   MT_INVERT_COMMON     isse
   MT_INVERT_COMMON     3dnow
   MT_INVERT_COMMON     sse2
   MT_INVERT_COMMON     asse2
