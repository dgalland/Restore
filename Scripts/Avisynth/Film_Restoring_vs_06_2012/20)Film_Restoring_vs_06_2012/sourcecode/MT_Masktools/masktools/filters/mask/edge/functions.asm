%INCLUDE "common.asm"

SECTION .rodata
ALIGN 16
MT_BYTE_7F : TIMES 16 DB 0x80

;=============================================================================
; Code
;=============================================================================

SECTION .text

   mt_mangle Edge, sobel8_mmx
   mt_mangle Edge, sobel8_sse2
   mt_mangle Edge, roberts8_mmx
   mt_mangle Edge, roberts8_sse2
   mt_mangle Edge, laplace8_mmx
   mt_mangle Edge, laplace8_sse2
   mt_mangle Edge, convolution8_mmx
   mt_mangle Edge, convolution8_sse2
   mt_mangle Edge, morpho8_isse
   mt_mangle Edge, morpho8_sse2
   
%MACRO MT_SOBEL_ROLLED_INSIDE_SUB  15
   SET_XMMX                %1
   %7                      %1, LOAD  , %12, esi + %2, ebx, R6, R7
   %9                      %1, LOAD  , %13, esi + %2, ebx, R6, R7
   %6                      %1, ULOAD , %14, esi + %2, ebx, R6, R7
   %4                      %1, ULOAD , %15, esi + %2, ebx, R6, R7
   
   %11                     %12   , R5
   %11                     %13   , R5
   %11                     %14   , R5
   %11                     %15   , R5
   
   paddw                   %12   , %13
   paddw                   %14   , %15
   MOV                     %15   , %12
   psubusw                 %12   , %14
   psubusw                 %14   , %15
   paddw                   %12   , %14
   psrlw                   %12   , 1
%ENDMACRO

%MACRO MT_MORPHO_ROLLED_INSIDE_SUB   15
   SET_XMMX                %1
   LOAD                   %12   , [esi + ebx + %2]
   %3                      %1, ULOAD , %14, esi + %2, ebx, R6, R7
   %4                      %1, ULOAD , %15, esi + %2, ebx, R6, R7
   MOV                     %13   , %12
   pmaxub                  %12   , %14
   pminub                  %13   , %14
   %5                      %1, ULOAD , %14, esi + %2, ebx, R6, R7
   pmaxub                  %12   , %15
   pminub                  %13   , %15
   %6                      %1, ULOAD , %15, esi + %2, ebx, R6, R7
   pmaxub                  %12   , %14
   pminub                  %13   , %14
   %7                      %1, ULOAD , %14, esi + %2, ebx, R6, R7
   pmaxub                  %12   , %15
   pminub                  %13   , %15
   %8                      %1, ULOAD , %15, esi + %2, ebx, R6, R7
   pmaxub                  %12   , %14
   pminub                  %13   , %14
   %9                      %1, ULOAD , %14, esi + %2, ebx, R6, R7
   pmaxub                  %12   , %15
   pminub                  %13   , %15
   %10                     %1, ULOAD , %15, esi + %2, ebx, R6, R7
   pmaxub                  %12   , %14
   pminub                  %13   , %14
   pmaxub                  %12   , %15
   pminub                  %13   , %15
   
   psubb                   %12   , %13
   
   ; thresholding
   MOV                     %13   , %12
   psubb                   %13   , [MT_BYTE_7F]
   MOV                     %15   , %13
   pcmpgtb                 %13   , [esp + .nLow8]
   pcmpgtb                 %15   , [esp + .nHigh8]
   pand                    %12   , %13
   por                     %12   , %15
   
   STORE                   [edi + %2], %12
%ENDMACRO   

%MACRO MT_ROBERTS_ROLLED_INSIDE_SUB  15
   SET_XMMX                %1
   ULOAD                   %12   , [esi + ebx + %2]
   %7                      %1, ULOAD , %13, esi + %2, ebx, R6, R7
   %9                      %1, ULOAD , %14, esi + %2, ebx, R6, R7
   %11                     %12   , R5
   %11                     %13   , R5
   %11                     %14   , R5
   psllw                   %12   , 1
   paddw                   %13   , %14
   MOV                     %15   , %12
   psubusw                 %12   , %13
   psubusw                 %13   , %15
   paddw                   %12   , %13
   psrlw                   %12   , 1
%ENDMACRO

%MACRO MT_LAPLACE_ROLLED_INSIDE_SUB  15
   SET_XMMX                %1
   %3                      %1, ULOAD , %12, esi + %2, ebx, R6, R7
   %4                      %1, ULOAD , %13, esi + %2, ebx, R6, R7
   %5                      %1, ULOAD , %14, esi + %2, ebx, R6, R7
   %6                      %1, ULOAD , %15, esi + %2, ebx, R6, R7
   %11                     %12   , R5
   %11                     %13   , R5
   %11                     %14   , R5
   %11                     %15   , R5
   paddw                   %12   , %13
   paddw                   %14   , %15
   paddw                   %12   , %14
   %7                      %1, ULOAD , %13, esi + %2, ebx, R6, R7
   %8                      %1, ULOAD , %14, esi + %2, ebx, R6, R7
   %9                      %1, ULOAD , %15, esi + %2, ebx, R6, R7
   %11                     %13   , R5
   %11                     %14   , R5
   %11                     %15   , R5
   paddw                   %13   , %14
   paddw                   %12   , %15
   paddw                   %12   , %13
   %10                     %1, ULOAD , %14, esi + %2, ebx, R6, R7
   ULOAD                   %13   , [esi + ebx + %2]
   %11                     %14   , R5
   %11                     %13   , R5
   paddw                   %12   , %14
   psllw                   %13   , 3
   MOV                     %14   , %12
   psubusw                 %12   , %13
   psubusw                 %13   , %14
   paddw                   %12   , %13
   psrlw                   %12   , 3
%ENDMACRO

%MACRO MT_CONVOLUTION_ROLLED_INSIDE_SUB  15
   SET_XMMX                %1
   %3                      %1, ULOAD , %12, esi + %2, ebx, R6, R7
   %4                      %1, ULOAD , %13, esi + %2, ebx, R6, R7
   %5                      %1, ULOAD , %14, esi + %2, ebx, R6, R7
   %6                      %1, ULOAD , %15, esi + %2, ebx, R6, R7
   %11                     %12   , R5
   %11                     %13   , R5
   %11                     %14   , R5
   %11                     %15   , R5
   pmullw                  %12   , [esp + .nTopLeft]
   pmullw                  %13   , [esp + .nTop]
   pmullw                  %14   , [esp + .nTopRight]
   pmullw                  %15   , [esp + .nLeft]
   paddw                   %12   , %13
   paddw                   %14   , %15
   paddw                   %12   , %14
   %7                      %1, ULOAD , %13, esi + %2, ebx, R6, R7
   %8                      %1, ULOAD , %14, esi + %2, ebx, R6, R7
   %9                      %1, ULOAD , %15, esi + %2, ebx, R6, R7
   %11                     %13   , R5
   %11                     %14   , R5
   %11                     %15   , R5
   pmullw                  %13   , [esp + .nRight]
   pmullw                  %14   , [esp + .nBottomLeft]
   pmullw                  %15   , [esp + .nBottom]
   paddw                   %13   , %14
   paddw                   %12   , %15
   paddw                   %12   , %13
   %10                     %1, ULOAD , %14, esi + %2, ebx, R6, R7
   ULOAD                   %13   , [esi + ebx + %2]
   %11                     %14   , R5
   %11                     %13   , R5
   pmullw                  %14   , [esp + .nBottomRight]
   pmullw                  %13   , [esp + .nCenter]
   
   paddw                   %12   , %14

   MOV                     %14   , %12
   MOV                     %15   , %13   
   
   psubw                   %12   , %13
   
   psubw                   %15   , %14
   
   MOV                     %14   , %15
   psraw                   %15   , 15
   
   pand                    %12   , %15
   pandn                   %15   , %14
   
   paddw                   %12   , %15
   psraw                   %12   , [esp + .nDivisor]
   
%ENDMACRO


%MACRO MT_EDGE_THRESHOLDING 2
   SET_XMMX                %1
   packuswb                R0    , R1                            ; abs values
   MOV                     R1    , R0
   psubb                   R1    , [MT_BYTE_7F]
   MOV                     R3    , R1
   pcmpgtb                 R1    , [esp + .nLow8]
   pcmpgtb                 R3    , [esp + .nHigh8]
   pand                    R0    , R1
   por                     R0    , R3
   
   STORE                   [edi + %2], R0
%ENDMACRO   

%MACRO MT_EDGE_BIDOUILLE   2
   SET_XMMX                %1
   packuswb                R0    , R1                            ; abs values
   STORE                   [edi + %2], R0
%ENDMACRO   


%MACRO MT_SOBEL_ROLLED_INSIDE    10
   SET_XMMX                      %1
   MT_SOBEL_ROLLED_INSIDE_SUB    %1, %2, %3, %4, %5, %6, %7, %8, %9, %10, punpcklbw, R0, R1, R2, R3
   MT_SOBEL_ROLLED_INSIDE_SUB    %1, %2, %3, %4, %5, %6, %7, %8, %9, %10, punpckhbw, R1, R2, R3, R4
   MT_EDGE_THRESHOLDING          %1, %2
%ENDMACRO

%MACRO MT_ROBERTS_ROLLED_INSIDE  10
   SET_XMMX                      %1
   MT_ROBERTS_ROLLED_INSIDE_SUB  %1, %2, %3, %4, %5, %6, %7, %8, %9, %10, punpcklbw, R0, R1, R2, R3
   MT_ROBERTS_ROLLED_INSIDE_SUB  %1, %2, %3, %4, %5, %6, %7, %8, %9, %10, punpckhbw, R1, R2, R3, R4
   MT_EDGE_THRESHOLDING          %1, %2
%ENDMACRO

%MACRO MT_LAPLACE_ROLLED_INSIDE  10
   SET_XMMX                      %1
   MT_LAPLACE_ROLLED_INSIDE_SUB  %1, %2, %3, %4, %5, %6, %7, %8, %9, %10, punpcklbw, R0, R1, R2, R3
   MT_LAPLACE_ROLLED_INSIDE_SUB  %1, %2, %3, %4, %5, %6, %7, %8, %9, %10, punpckhbw, R1, R2, R3, R4
   MT_EDGE_THRESHOLDING          %1, %2
%ENDMACRO

%MACRO MT_CONVOLUTION_ROLLED_INSIDE 10
   SET_XMMX                         %1
   MT_CONVOLUTION_ROLLED_INSIDE_SUB %1, %2, %3, %4, %5, %6, %7, %8, %9, %10, punpcklbw, R0, R1, R2, R3
   MT_CONVOLUTION_ROLLED_INSIDE_SUB %1, %2, %3, %4, %5, %6, %7, %8, %9, %10, punpckhbw, R1, R2, R3, R4
   MT_EDGE_THRESHOLDING             %1, %2
%ENDMACRO

%MACRO MT_MORPHO_ROLLED_INSIDE      10
   SET_XMMX                         %1
   MT_MORPHO_ROLLED_INSIDE_SUB      %1, %2, %3, %4, %5, %6, %7, %8, %9, %10, , R0, R1, R2, R3
%ENDMACRO

%MACRO MT_EDGE_ROLLED            10
   SET_XMMX                      %1
   MT_ %+ %2 %+ _ROLLED_INSIDE   DOWNGRADED, 0 , %3, %4, %5, %6, %7, %8, %9, %10
%ENDMACRO

%MACRO MT_SOBEL_ROLLED           9
   MT_EDGE_ROLLED                %1, SOBEL, %2, %3, %4, %5, %6, %7, %8, %9
%ENDMACRO

%MACRO MT_ROBERTS_ROLLED         9
   MT_EDGE_ROLLED                %1, ROBERTS, %2, %3, %4, %5, %6, %7, %8, %9
%ENDMACRO

%MACRO MT_LAPLACE_ROLLED         9
   MT_EDGE_ROLLED                %1, LAPLACE, %2, %3, %4, %5, %6, %7, %8, %9
%ENDMACRO

%MACRO MT_CONVOLUTION_ROLLED     9
   MT_EDGE_ROLLED                %1, CONVOLUTION, %2, %3, %4, %5, %6, %7, %8, %9
%ENDMACRO

%MACRO MT_MORPHO_ROLLED          9
   MT_EDGE_ROLLED                %1, MORPHO, %2, %3, %4, %5, %6, %7, %8, %9
%ENDMACRO

%MACRO MT_EDGE_UNROLLED          10
   SET_XMMX                      %1
%ASSIGN offset 0
%REP 64 / RWIDTH
   MT_ %+ %2 %+ _ROLLED_INSIDE   %1, offset , %3, %4, %5, %6, %7, %8, %9, %10
%ASSIGN offset offset + RWIDTH
%ENDREP
%ENDMACRO

%MACRO MT_SOBEL_UNROLLED         9
   MT_EDGE_UNROLLED              %1, SOBEL, %2, %3, %4, %5, %6, %7, %8, %9
%ENDMACRO

%MACRO MT_ROBERTS_UNROLLED       9
   MT_EDGE_UNROLLED              %1, ROBERTS, %2, %3, %4, %5, %6, %7, %8, %9
%ENDMACRO

%MACRO MT_LAPLACE_UNROLLED       9
   MT_EDGE_UNROLLED              %1, LAPLACE, %2, %3, %4, %5, %6, %7, %8, %9
%ENDMACRO

%MACRO MT_CONVOLUTION_UNROLLED   9
   MT_EDGE_UNROLLED              %1, CONVOLUTION, %2, %3, %4, %5, %6, %7, %8, %9
%ENDMACRO

%MACRO MT_MORPHO_UNROLLED        9
   MT_EDGE_UNROLLED              %1, MORPHO, %2, %3, %4, %5, %6, %7, %8, %9
%ENDMACRO

%MACRO MT_EDGE_NEXT_LINE         0
   add                           esi, [esp + .nSrcPad]
   add                           edi, [esp + .nDstPad]
%ENDMACRO

%MACRO MT_EDGE_NEXT_8_PIXELS     0
   add                           esi, 8
   add                           edi, 8
%ENDMACRO

%MACRO MT_EDGE_NEXT_64_PIXELS    0
   add                           esi, 64
   add                           edi, 64
%ENDMACRO

%MACRO MT_EDGE_FUNCTION    3
%2 %+ 8_ %+ %1:
   SET_XMMX                %1
   STACK                   RWIDTH * 2 + 32, 9, esi, edi, ebx, ebp
   .pDst                   PARAMETER(1)
   .nDstPitch              PARAMETER(2)
   .pSrc                   PARAMETER(3)
   .nSrcPitch              PARAMETER(4)
   ; matrix
   .nLowThreshold          PARAMETER(6)
   .nHighThreshold         PARAMETER(7)
   .nWidth                 PARAMETER(8)
   .nHeight                PARAMETER(9)
   .nLow8                  VARIABLE(RWIDTH * 0)
   .nHigh8                 VARIABLE(RWIDTH * 1)
   .nTailWidth             VARIABLE(RWIDTH * 2 +  0)
   .nUnrolledWidth         VARIABLE(RWIDTH * 2 +  4)
   .nSrcPad                VARIABLE(RWIDTH * 2 +  8)
   .nDstPad                VARIABLE(RWIDTH * 2 + 12)
   .nHeightCount           VARIABLE(RWIDTH * 2 + 16)
   
   pxor                    mm5, mm5
   
   movd                    mm6, [esp + .nLowThreshold]
   movd                    mm7, [esp + .nHighThreshold]
   punpcklbw               mm6, mm6
   punpcklbw               mm7, mm7
   punpcklwd               mm6, mm6
   punpcklwd               mm7, mm7
   punpckldq               mm6, mm6
   punpckldq               mm7, mm7
   psubb                   mm6, [MT_BYTE_7F]
   psubb                   mm7, [MT_BYTE_7F]
   movq                    [esp + .nLow8], mm6
   movq                    [esp + .nHigh8], mm7
%IFIDNI REGISTERS, xmm
   pxor                    xmm5, xmm5
   movq                    [esp + .nLow8 + 8], mm6
   movq                    [esp + .nHigh8 + 8], mm7
%ENDIF   
   
   mov                     edi, [esp + .pDst]
   mov                     esi, [esp + .pSrc]
   mov                     ebx, [esp + .nSrcPitch]
   mov                     ebp, [esp + .nDstPitch]
   sub                     esi, ebx                         ; positive offset needed for pitches
   
   mov                     ecx, [esp + .nWidth]
   sub                     ecx, ebx
   neg                     ecx
   mov                     [esp + .nSrcPad], ecx            ; padding src
   mov                     ecx, [esp + .nWidth]
   sub                     ecx, ebp
   neg                     ecx
   mov                     [esp + .nDstPad], ecx            ; padding dst
   
   MT_PROCESS_CONVOLUTION  %1, MT_ %+ %3 %+ _UNROLLED, MT_ %+ %3 %+ _ROLLED, MT_EDGE_NEXT_LINE, MT_EDGE_NEXT_8_PIXELS, MT_EDGE_NEXT_64_PIXELS
   
   RETURN
%ENDMACRO

MT_EDGE_FUNCTION        mmx, sobel, SOBEL
MT_EDGE_FUNCTION        sse2, sobel, SOBEL
   
MT_EDGE_FUNCTION        mmx, roberts, ROBERTS
MT_EDGE_FUNCTION        sse2, roberts, ROBERTS
   
MT_EDGE_FUNCTION        mmx, laplace, LAPLACE
MT_EDGE_FUNCTION        sse2, laplace, LAPLACE

MT_EDGE_FUNCTION        isse, morpho, MORPHO
MT_EDGE_FUNCTION        sse2, morpho, MORPHO
   
%MACRO MT_LOAD_WORD        3
   SET_XMMX                %1
   movd                    mm6, [eax + %2 * 2]
   punpcklwd               mm6, mm6
   punpckldq               mm6, mm6
   movq                    [esp + %3], mm6
%IFIDNI REGISTERS, xmm
   movq                    [esp + %3 + 8], mm6
%ENDIF
%ENDMACRO
   
%MACRO MT_CONVOLUTION_FUNCTION 1
convolution8_ %+ %1:
   SET_XMMX                %1
   STACK                   RWIDTH * 12 + 32, 9, esi, edi, ebx, ebp
   .pDst                   PARAMETER(1)
   .nDstPitch              PARAMETER(2)
   .pSrc                   PARAMETER(3)
   .nSrcPitch              PARAMETER(4)
   .matrix                 PARAMETER(5)
   .nLowThreshold          PARAMETER(6)
   .nHighThreshold         PARAMETER(7)
   .nWidth                 PARAMETER(8)
   .nHeight                PARAMETER(9)
   .nLow8                  VARIABLE(RWIDTH *  0)
   .nHigh8                 VARIABLE(RWIDTH *  1)
   .nTopLeft               VARIABLE(RWIDTH *  2)
   .nTop                   VARIABLE(RWIDTH *  3)
   .nTopRight              VARIABLE(RWIDTH *  4)
   .nLeft                  VARIABLE(RWIDTH *  5)
   .nRight                 VARIABLE(RWIDTH *  6)
   .nBottomLeft            VARIABLE(RWIDTH *  7)
   .nBottom                VARIABLE(RWIDTH *  8)
   .nBottomRight           VARIABLE(RWIDTH *  9)
   .nCenter                VARIABLE(RWIDTH * 10)
   .nDivisor               VARIABLE(RWIDTH * 11)
   .nTailWidth             VARIABLE(RWIDTH * 12 +  0)
   .nUnrolledWidth         VARIABLE(RWIDTH * 12 +  4)
   .nSrcPad                VARIABLE(RWIDTH * 12 +  8)
   .nDstPad                VARIABLE(RWIDTH * 12 + 12)
   .nHeightCount           VARIABLE(RWIDTH * 12 + 16)
   
   pxor                    mm5, mm5
   
   movd                    mm6, [esp + .nLowThreshold]
   movd                    mm7, [esp + .nHighThreshold]
   punpcklbw               mm6, mm6
   punpcklbw               mm7, mm7
   punpcklwd               mm6, mm6
   punpcklwd               mm7, mm7
   punpckldq               mm6, mm6
   punpckldq               mm7, mm7
   psubb                   mm6, [MT_BYTE_7F]
   psubb                   mm7, [MT_BYTE_7F]
   movq                    [esp + .nLow8], mm6
   movq                    [esp + .nHigh8], mm7
%IFIDNI REGISTERS, xmm
   pxor                    xmm5, xmm5   
   movq                    [esp + .nLow8 + 8], mm6
   movq                    [esp + .nHigh8 + 8], mm7
%ENDIF   
       
   mov                     eax, [esp + .matrix]

   MT_LOAD_WORD            %1, 0, .nTopLeft
   MT_LOAD_WORD            %1, 1, .nTop
   MT_LOAD_WORD            %1, 2, .nTopRight
   MT_LOAD_WORD            %1, 3, .nLeft
   MT_LOAD_WORD            %1, 5, .nRight
   MT_LOAD_WORD            %1, 6, .nBottomLeft
   MT_LOAD_WORD            %1, 7, .nBottom
   MT_LOAD_WORD            %1, 8, .nBottomRight

   movd                    mm6, [eax + 8]
   punpcklwd               mm6, mm6
   punpckldq               mm6, mm6
   pxor                    mm7, mm7
   psubw                   mm7, mm6
   movq                    [esp + .nCenter], mm7
%IFIDNI REGISTERS, xmm
   movq                    [esp + .nCenter + 8], mm7
%ENDIF   
   ;MT_LOAD_WORD            4, .nCenter
   
   mov                     cx, [eax + 18]
   xor                     eax, eax
   mov                     [esp + .nDivisor + 4], eax
%IFIDNI REGISTERS, xmm
   movq                    [esp + .nDivisor + 8], mm5
%ENDIF   
.loop_div
   inc                     eax
   shr                     cx, 1
   jnz                     .loop_div
   dec                     eax
   mov                     [esp + .nDivisor], eax
   
   mov                     edi, [esp + .pDst]
   mov                     esi, [esp + .pSrc]
   mov                     ebx, [esp + .nSrcPitch]
   mov                     ebp, [esp + .nDstPitch]
   sub                     esi, ebx                         ; positive offset needed for pitches
   
   mov                     ecx, [esp + .nWidth]
   sub                     ecx, ebx
   neg                     ecx
   mov                     [esp + .nSrcPad], ecx            ; padding src
   mov                     ecx, [esp + .nWidth]
   sub                     ecx, ebp
   neg                     ecx
   mov                     [esp + .nDstPad], ecx            ; padding dst
   
   MT_PROCESS_CONVOLUTION  %1, MT_CONVOLUTION_UNROLLED, MT_CONVOLUTION_ROLLED, MT_EDGE_NEXT_LINE, MT_EDGE_NEXT_8_PIXELS, MT_EDGE_NEXT_64_PIXELS
   
   RETURN
%ENDMACRO
   
MT_CONVOLUTION_FUNCTION mmx
MT_CONVOLUTION_FUNCTION sse2