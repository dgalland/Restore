%INCLUDE "morphologic.asm"

SECTION .rodata

;=============================================================================
; Code
;=============================================================================

SECTION .text

   mt_mangle Inpand, inpand8_isse
   mt_mangle Inpand, inpand8_3dnow
   mt_mangle Inpand, inpand8_sse2
   mt_mangle Inpand, inpand8_asse2

MT_MORPHOLOGIC_FUNCTION isse, inpand, MT_INPAND_UNROLLED, MT_INPAND_ROLLED
MT_MORPHOLOGIC_FUNCTION 3dnow, inpand, MT_INPAND_UNROLLED, MT_INPAND_ROLLED
MT_MORPHOLOGIC_FUNCTION sse2, inpand, MT_INPAND_UNROLLED, MT_INPAND_ROLLED
MT_MORPHOLOGIC_FUNCTION asse2, inpand, MT_INPAND_UNROLLED, MT_INPAND_ROLLED
