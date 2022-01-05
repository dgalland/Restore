%INCLUDE "morphologic.asm"

SECTION .rodata

;=============================================================================
; Code
;=============================================================================

SECTION .text

   mt_mangle Expand, expand8_isse
   mt_mangle Expand, expand8_3dnow
   mt_mangle Expand, expand8_sse2
   mt_mangle Expand, expand8_asse2

MT_MORPHOLOGIC_FUNCTION isse, expand, MT_EXPAND_UNROLLED, MT_EXPAND_ROLLED
MT_MORPHOLOGIC_FUNCTION 3dnow, expand, MT_EXPAND_UNROLLED, MT_EXPAND_ROLLED
MT_MORPHOLOGIC_FUNCTION sse2, expand, MT_EXPAND_UNROLLED, MT_EXPAND_ROLLED
MT_MORPHOLOGIC_FUNCTION asse2, expand, MT_EXPAND_UNROLLED, MT_EXPAND_ROLLED
