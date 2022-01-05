%INCLUDE "common.asm"

;=============================================================================
; Code
;=============================================================================

SECTION .text

   mt_mangle Clamp, clamp8_isse
   mt_mangle Clamp, clamp8_3dnow
   mt_mangle Clamp, clamp8_sse2
   mt_mangle Clamp, clamp8_asse2
   
%MACRO MT_CLAMP_ROLLED 0
   movq        mm0, [esi + eax]
   movq        mm1, [ebx + eax]
   
   paddusb     mm0, mm6             ; overshoot
   psubusb     mm1, mm7             ; undershoot
   
   pminub      mm0, [edi + eax]
   pmaxub      mm0, mm1
   
   movq        [edi + eax], mm0
%ENDMACRO

%MACRO MT_CLAMP 2
   SET_XMMX    %1
   
   LOAD        R0, [esi + eax + %2 + 0 * RWIDTH]
   LOAD        R1, [esi + eax + %2 + 1 * RWIDTH]
   LOAD        R2, [ebx + eax + %2 + 0 * RWIDTH]
   LOAD        R3, [ebx + eax + %2 + 1 * RWIDTH]
   
   paddusb     R0, R6
   paddusb     R1, R6
   psubusb     R2, R7
   psubusb     R3, R7
   
%IFIDNI LOAD, ALOAD
   pminub      R0, [edi + eax + %2 + 0 * RWIDTH]
   pminub      R1, [edi + eax + %2 + 1 * RWIDTH]
%ELSE
   LOAD        R4, [edi + eax + %2 + 0 * RWIDTH]
   LOAD        R5, [edi + eax + %2 + 1 * RWIDTH]
   pminub      R0, R4
   pminub      R1, R5
%ENDIF   
   pmaxub      R0, R2
   pmaxub      R1, R3
   
   STORE       [edi + eax + %2 + 0 * RWIDTH], R0
   STORE       [edi + eax + %2 + 1 * RWIDTH], R1
%ENDMACRO   

%MACRO MT_CLAMP_UNROLLED 1
   SET_XMMX    %1
   
   PREFETCH_RW edi + eax + 64
   PREFETCH_R  esi + eax + 64
   PREFETCH_R  ebx + eax + 64
   
%ASSIGN offset 0
%REP 64 / (RWIDTH * 2)
   MT_CLAMP    %1, offset
   %ASSIGN offset offset + RWIDTH * 2
%ENDREP   
%ENDMACRO

%MACRO MT_CLAMP_ENDLINE 0
   add                  edi, [esp + .nDstPitch]
   add                  esi, [esp + .nSrc1Pitch]
   add                  ebx, [esp + .nSrc2Pitch]
%ENDMACRO

%MACRO MT_CLAMP_COMMON  2
%2 %+ 8_ %+ %1:
   SET_XMMX             %1
   STACK                edi, esi, ebx
   .pDst                PARAMETER(1)
   .nDstPitch           PARAMETER(2)
   .pSrc1               PARAMETER(3)
   .nSrc1Pitch          PARAMETER(4)
   .pSrc2               PARAMETER(5)
   .nSrc2Pitch          PARAMETER(6)
   .nWidth              PARAMETER(7)
   .nHeight             PARAMETER(8)
   .nOvershoot          PARAMETER(9)
   .nUndershoot         PARAMETER(10)
   
   mov                  edi, [esp + .pDst]
   mov                  esi, [esp + .pSrc1]
   mov                  ebx, [esp + .pSrc2]
   
   movd                 mm6, [esp + .nOvershoot]
   movd                 mm7, [esp + .nUndershoot]
   punpcklbw            mm6, mm6
   punpcklbw            mm7, mm7
   punpcklwd            mm6, mm6
   punpcklwd            mm7, mm7
   punpckldq            mm6, mm6
   punpckldq            mm7, mm7
%IFIDNI REGISTERS, xmm
   movq2dq              xmm6, mm6   
   movq2dq              xmm7, mm7   
   punpcklbw            xmm6, xmm6
   punpcklbw            xmm7, xmm7
%ENDIF   
   
   MT_UNROLL_INPLACE_WIDTH    {MT_CLAMP_UNROLLED %1}, MT_CLAMP_ROLLED, MT_CLAMP_ENDLINE
      
   RETURN
%ENDMACRO

MT_CLAMP_COMMON      isse, clamp
MT_CLAMP_COMMON      3dnow, clamp
MT_CLAMP_COMMON      sse2, clamp
MT_CLAMP_COMMON      asse2, clamp
