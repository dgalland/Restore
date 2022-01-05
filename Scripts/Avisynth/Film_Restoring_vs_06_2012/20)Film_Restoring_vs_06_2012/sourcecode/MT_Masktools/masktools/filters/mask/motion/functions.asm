%INCLUDE "common.asm"

SECTION .rodata
ALIGN 16
MT_BYTE_7F  : TIMES 16 DB 0x7F

;=============================================================================
; Code
;=============================================================================

SECTION .text

   mt_mangle Motion, sad8_isse
   mt_mangle Motion, sad8_3dnow
   
   mt_mangle Motion, motion8_mmx
   mt_mangle Motion, motion8_isse
   mt_mangle Motion, motion8_3dnow

%MACRO MT_SAD_ROLLED 0
   movq              mm1, [esi + eax]
   psadbw            mm1, [edi + eax]
   paddd             mm0, mm1
%ENDMACRO

%MACRO MT_SAD_UNROLLED 1
   %1                esi + eax + 192
   %1                edi + eax + 192
   
   movq              mm1, [esi + eax]
   movq              mm2, [esi + eax + 8]
   movq              mm3, [esi + eax + 16]
   movq              mm4, [esi + eax + 24]
   psadbw            mm1, [edi + eax]
   movq              mm5, [esi + eax + 32]
   psadbw            mm2, [edi + eax + 8]
   movq              mm6, [esi + eax + 40]
   psadbw            mm3, [edi + eax + 16]
   paddd             mm1, mm2
   movq              mm7, [esi + eax + 48]
   psadbw            mm4, [edi + eax + 24]
   movq              mm2, [esi + eax + 56]
   paddd             mm3, mm4
   psadbw            mm5, [edi + eax + 32]
   psadbw            mm6, [edi + eax + 40]
   psadbw            mm7, [edi + eax + 48]
   paddd             mm5, mm6
   psadbw            mm2, [edi + eax + 56]
   paddd             mm3, mm5
   paddd             mm2, mm7
   paddd             mm1, mm3
   paddd             mm0, mm2
   paddd             mm0, mm1
%ENDMACRO

%MACRO MT_SAD_ENDLINE 0
   add                  edi, [esp + .nDstPitch]
   add                  esi, [esp + .nSrcPitch]
%ENDMACRO

%MACRO MT_SAD_COMMON  1
   STACK                esi, edi
   .pDst                PARAMETER(1)
   .nDstPitch           PARAMETER(2)
   .pSrc                PARAMETER(3)
   .nSrcPitch           PARAMETER(4)
   .nWidth              PARAMETER(5)
   .nHeight             PARAMETER(6)

   pxor                 mm0, mm0
   
   mov                  edi, [esp + .pDst]
   mov                  esi, [esp + .pSrc]
   
   MT_UNROLL_INPLACE_WIDTH  {MT_SAD_UNROLLED %1}, MT_SAD_ROLLED, MT_SAD_ENDLINE
   
   movd                 eax, mm0
      
   RETURN
%ENDMACRO

sad8_mmx:
   MT_SAD_COMMON  MT_NOPREFETCH
   
sad8_isse:
   MT_SAD_COMMON  MT_PREFETCHNTA
   
sad8_3dnow:
   MT_SAD_COMMON  MT_PREFETCHNTA
   
%MACRO MT_MOTION_ROLLED_INSIDE   1
   movq                          mm0, [esi + eax + %1]
   movq                          mm1, [edi + eax + %1]
   psubusb                       mm0, mm1
   psubusb                       mm1, [esi + eax + %1]
   paddb                         mm0, mm1

   movq                          mm1, mm0
   psubb                         mm1, [MT_BYTE_7F]
   movq                          mm3, mm1
   pcmpgtb                       mm1, mm6
   pcmpgtb                       mm3, mm7
   pand                          mm0, mm1
   por                           mm0, mm3
   
   movq                          [edi + eax + %1], mm0
%ENDMACRO

%MACRO MT_MOTION_ROLLED          0
   MT_MOTION_ROLLED_INSIDE       0
%ENDMACRO

%MACRO MT_MOTION_UNROLLED        2
   MT_MOTION_ROLLED_INSIDE       0 
   MT_MOTION_ROLLED_INSIDE       8 
   MT_MOTION_ROLLED_INSIDE       16
   MT_MOTION_ROLLED_INSIDE       24
   MT_MOTION_ROLLED_INSIDE       32
   MT_MOTION_ROLLED_INSIDE       40
   MT_MOTION_ROLLED_INSIDE       48
   MT_MOTION_ROLLED_INSIDE       56
%ENDMACRO

%MACRO MT_MOTION_NEXT_LINE 0
   add                           esi, [esp + .nSrcPitch]
   add                           edi, [esp + .nDstPitch]
%ENDMACRO

%MACRO MT_MOTION_FUNCTION   2
   STACK                esi, edi
   .pDst                PARAMETER(1)
   .nDstPitch           PARAMETER(2)
   .pSrc                PARAMETER(3)
   .nSrcPitch           PARAMETER(4)
   .nLowThreshold       PARAMETER(5)
   .nHighThreshold      PARAMETER(6)
   .nWidth              PARAMETER(7)
   .nHeight             PARAMETER(8)

   mov                  edi, [esp + .pDst]
   mov                  esi, [esp + .pSrc]
   
   movd                 mm6, [esp + .nHighThreshold]
   movd                 mm7, [esp + .nLowThreshold]
   punpcklbw            mm6, mm6
   punpcklbw            mm7, mm7
   punpcklwd            mm6, mm6
   punpcklwd            mm7, mm7
   punpckldq            mm6, mm6
   punpckldq            mm7, mm7
   psubb                mm6, [MT_BYTE_7F]
   psubb                mm7, [MT_BYTE_7F]
      
   MT_UNROLL_INPLACE_WIDTH  {MT_MOTION_UNROLLED %1, %2}, MT_MOTION_ROLLED, MT_MOTION_NEXT_LINE
   
   RETURN
%ENDMACRO


motion8_mmx:
   MT_MOTION_FUNCTION   MT_NOPREFETCH, MT_NOPREFETCH
   
motion8_isse:
   MT_MOTION_FUNCTION   MT_PREFETCHT0, MT_PREFETCHT0
   
motion8_3dnow:
   MT_MOTION_FUNCTION   MT_PREFETCHW, MT_NOPREFETCHT0