%INCLUDE "common.asm"

SECTION .rodata
ALIGN 16
   MT_BYTE_80     : TIMES 16 DB 0x80

%MACRO MT_SUPPORT_DIFF_ROLLED 2
   SET_XMMX             %1
   LOAD                 R0, [edi + eax]
   LOAD                 R1, [esi + eax]
   
   psubb                R0, R7
   psubb                R1, R7
   %2                   R0, R1
   paddb                R0, R7
   
   STORE                [edi + eax], R0
%ENDMACRO

%MACRO MT_SUPPORT_DIFF 3
   SET_XMMX             %1
   LOAD                 R0, [edi + eax + %2]
   LOAD                 R4, [esi + eax + %2]

   LOAD                 R1, [edi + eax + %2 + RWIDTH]
   LOAD                 R5, [esi + eax + %2 + RWIDTH]

   psubb                R0, R7
   psubb                R4, R7

   psubb                R1, R7
   psubb                R5, R7

   %3                   R0, R4
   %3                   R1, R5

   paddb                R0, R7
   paddb                R1, R7
   
   STORE                [edi + eax + %2], R0
   STORE                [edi + eax + %2 + RWIDTH], R1
%ENDMACRO

%MACRO MT_SUPPORT_DIFF_UNROLLED 2
   SET_XMMX             %1
   PREFETCH_RW          edi + eax + 128
   PREFETCH_R           esi + eax + 128

%ASSIGN offset 0
%REP 64 / (RWIDTH * 2)
   MT_SUPPORT_DIFF      %1, offset, %2
   %ASSIGN offset offset + RWIDTH * 2
%ENDREP   
%ENDMACRO

%MACRO MT_SUPPORT_COMMON 3
%2 %+ 8_ %+ %1:
   SET_XMMX             %1
   STACK                edi, esi
   .pDst                PARAMETER(1)
   .nDstPitch           PARAMETER(2)
   .pSrc                PARAMETER(3)
   .nSrcPitch           PARAMETER(4)
   .nWidth              PARAMETER(5)
   .nHeight             PARAMETER(6)
   
   movq                 mm7, [MT_BYTE_80]
%IFIDNI REGISTERS,xmm
   movdqa               xmm7, [MT_BYTE_80]
%ENDIF   
   mov                  edi, [esp + .pDst]
   mov                  esi, [esp + .pSrc]
   
   MT_UNROLL_INPLACE_WIDTH    {MT_ %+ %3 %+ _UNROLLED %1}, MT_ %+ %3 %+ _ROLLED, MT_ %+ %3 %+ _ENDLINE
      
   RETURN
%ENDMACRO