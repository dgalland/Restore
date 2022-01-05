%INCLUDE "support.asm"

;=============================================================================
; Code
;=============================================================================

SECTION .text

   mt_mangle AddDiff, adddiff8_mmx
   mt_mangle AddDiff, adddiff8_isse
   mt_mangle AddDiff, adddiff8_3dnow
   mt_mangle AddDiff, adddiff8_sse2
   mt_mangle AddDiff, adddiff8_asse2
   
%MACRO MT_ADDDIFF_ROLLED 0
   MT_SUPPORT_DIFF_ROLLED mmx, paddsb
%ENDMACRO

%MACRO MT_ADDDIFF_UNROLLED 1
   MT_SUPPORT_DIFF_UNROLLED %1, paddsb
%ENDMACRO

%MACRO MT_ADDDIFF_ENDLINE 0
   add                  edi, [esp + .nDstPitch]
   add                  esi, [esp + .nSrcPitch]
%ENDMACRO

MT_SUPPORT_COMMON    mmx, adddiff, ADDDIFF
MT_SUPPORT_COMMON    isse, adddiff, ADDDIFF
MT_SUPPORT_COMMON    3dnow, adddiff, ADDDIFF
MT_SUPPORT_COMMON    sse2, adddiff, ADDDIFF
MT_SUPPORT_COMMON    asse2, adddiff, ADDDIFF
