%INCLUDE "support.asm"

;=============================================================================
; Code
;=============================================================================

SECTION .text

   mt_mangle Average, average8_isse
   mt_mangle Average, average8_3dnow
   mt_mangle Average, average8_sse2
   mt_mangle Average, average8_asse2
   
%MACRO MT_AVERAGE_ROLLED 0
   movq           mm0, [edi + eax]
   pavgb          mm0, [esi + eax]
   movq           [edi + eax], mm0
%ENDMACRO

%MACRO MT_AVERAGE_UNROLLED 1
   SET_XMMX       %1
   PREFETCH_RW    edi + eax + 256
   PREFETCH_R     esi + eax + 256

%ASSIGN offset 0
%REP 64 / (4 * RWIDTH)
   
   LOAD           R0, [edi + eax + offset + 0 * RWIDTH]
   LOAD           R1, [edi + eax + offset + 1 * RWIDTH]
   LOAD           R2, [edi + eax + offset + 2 * RWIDTH]
   LOAD           R3, [edi + eax + offset + 3 * RWIDTH]
   
%IFIDNI LOAD, ALOAD
   pavgb          R0, [esi + eax + offset + 0 * RWIDTH]
   pavgb          R1, [esi + eax + offset + 1 * RWIDTH]
   pavgb          R2, [esi + eax + offset + 2 * RWIDTH]
   pavgb          R3, [esi + eax + offset + 3 * RWIDTH]
%ELSE   
   LOAD           R4, [esi + eax + offset + 0 * RWIDTH]
   LOAD           R5, [esi + eax + offset + 1 * RWIDTH]
   LOAD           R6, [esi + eax + offset + 2 * RWIDTH]
   LOAD           R7, [esi + eax + offset + 3 * RWIDTH]
   pavgb          R0, R4
   pavgb          R1, R5
   pavgb          R2, R6
   pavgb          R3, R7
%ENDIF   

   STORE          [edi + eax + offset + 0 * RWIDTH], R0
   STORE          [edi + eax + offset + 1 * RWIDTH], R1
   STORE          [edi + eax + offset + 2 * RWIDTH], R2
   STORE          [edi + eax + offset + 3 * RWIDTH], R3
   
%ASSIGN offset offset + 4 * RWIDTH
%ENDREP

%ENDMACRO

%MACRO MT_AVERAGE_ENDLINE 0
   add                  edi, [esp + .nDstPitch]
   add                  esi, [esp + .nSrcPitch]
%ENDMACRO

MT_SUPPORT_COMMON    isse, average, AVERAGE
MT_SUPPORT_COMMON    3dnow, average, AVERAGE
MT_SUPPORT_COMMON    sse2, average, AVERAGE
MT_SUPPORT_COMMON    asse2, average, AVERAGE
