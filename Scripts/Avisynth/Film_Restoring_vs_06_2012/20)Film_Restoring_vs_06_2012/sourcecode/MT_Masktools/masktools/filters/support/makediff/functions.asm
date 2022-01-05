%INCLUDE "support.asm"

;=============================================================================
; Code
;=============================================================================

SECTION .text

   mt_mangle MakeDiff, makediff8_mmx
   mt_mangle MakeDiff, makediff8_isse
   mt_mangle MakeDiff, makediff8_3dnow
   mt_mangle MakeDiff, makediff8_sse2
   mt_mangle MakeDiff, makediff8_asse2
   
%MACRO MT_MAKEDIFF_ROLLED 0
   MT_SUPPORT_DIFF_ROLLED mmx, psubsb
%ENDMACRO

%MACRO MT_MAKEDIFF_UNROLLED 1
   MT_SUPPORT_DIFF_UNROLLED %1, psubsb
%ENDMACRO

%MACRO MT_MAKEDIFF_ENDLINE 0
   add                  edi, [esp + .nDstPitch]
   add                  esi, [esp + .nSrcPitch]
%ENDMACRO

MT_SUPPORT_COMMON    mmx, makediff, MAKEDIFF
MT_SUPPORT_COMMON    isse, makediff, MAKEDIFF
MT_SUPPORT_COMMON    3dnow, makediff, MAKEDIFF
MT_SUPPORT_COMMON    sse2, makediff, MAKEDIFF
MT_SUPPORT_COMMON    asse2, makediff, MAKEDIFF
