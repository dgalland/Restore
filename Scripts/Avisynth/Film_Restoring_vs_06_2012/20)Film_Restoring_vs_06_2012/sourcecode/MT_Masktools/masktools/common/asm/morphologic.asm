%INCLUDE "common.asm"

%MACRO MT_MORPHOLOGIC_ROLLED_INSIDE_XMM 13
   SET_XMMX                %1
   MT_LOAD_NORMAL          %1, ULOAD , R0, esi + %5, ebx, R6, R7
   %7                      %1, ULOAD , R1, esi + %5, ebx, R6, R7
   %12                     %1, ULOAD , R2, esi + %5, ebx, R6, R7
   %6                      %1, ULOAD , R3, esi + %5, ebx, R6, R7
   MOV                     R4, R0
   %2                      R0, R2
   %2                      R1, R3
   %8                      %1, ULOAD , R2, esi + %5, ebx, R6, R7
   %9                      %1, ULOAD , R3, esi + %5, ebx, R6, R7
   %4                      R4, R5
   %2                      R0, R2
   %2                      R1, R3
   %10                     %1, ULOAD , R2, esi + %5, ebx, R6, R7
   %11                     %1, ULOAD , R3, esi + %5, ebx, R6, R7
   %2                      R0, R2
   %2                      R1, R3
   %13                     %1, ULOAD , R2, esi + %5, ebx, R6, R7
   %2                      R0, R1
   %2                      R0, R2
   %3                      R0, R4
   USTORE                  [edi + %5], R0
%ENDMACRO

%MACRO MT_MORPHOLOGIC_ROLLED_INSIDE    13
   SET_XMMX                %1
%IFIDNI REGISTERS, xmm
   MT_MORPHOLOGIC_ROLLED_INSIDE_XMM %1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12, %13
%ELSE   
   MT_LOAD_NORMAL          %1, LOAD  , R0, esi + %5, ebx, R6, R7
   MOV                     R1, R0
   %6                      %1, %2    , R0, esi + %5, ebx, R6, R7
   %7                      %1, %2    , R0, esi + %5, ebx, R6, R7
   %4                      R1, R5
   %8                      %1, %2    , R0, esi + %5, ebx, R6, R7
   %9                      %1, %2    , R0, esi + %5, ebx, R6, R7
   %10                     %1, %2    , R0, esi + %5, ebx, R6, R7
   %11                     %1, %2    , R0, esi + %5, ebx, R6, R7
   %12                     %1, %2    , R0, esi + %5, ebx, R6, R7
   %13                     %1, %2    , R0, esi + %5, ebx, R6, R7
   %3                      R0, R1
   STORE                   [edi + %5], R0
%ENDIF   
%ENDMACRO

%MACRO MT_MORPHOLOGIC_ROLLED     12
   MT_MORPHOLOGIC_ROLLED_INSIDE  %1, %2, %3, %4, 0 , %5, %6, %7, %8, %9, %10, %11, %12
%ENDMACRO

%DEFINE MT_EXPAND_OPERATORS      pmaxub, pminub, paddusb
%DEFINE MT_INPAND_OPERATORS      pminub, pmaxub, psubusb

%MACRO MT_EXPAND_ROLLED          9
   SET_XMMX                      %1
   MT_MORPHOLOGIC_ROLLED         DOWNGRADED, MT_EXPAND_OPERATORS, %2, %3, %4, %5, %6, %7, %8, %9
%ENDMACRO

%MACRO MT_INPAND_ROLLED          9
   SET_XMMX                      %1
   MT_MORPHOLOGIC_ROLLED         DOWNGRADED, MT_INPAND_OPERATORS, %2, %3, %4, %5, %6, %7, %8, %9
%ENDMACRO

%MACRO MT_MORPHOLOGIC_UNROLLED   12
   SET_XMMX                      %1
%ASSIGN offset 0
%REP 64 / RWIDTH
   MT_MORPHOLOGIC_ROLLED_INSIDE  %1, %2, %3, %4, offset, %5, %6, %7, %8, %9, %10, %11, %12
   %ASSIGN offset offset + RWIDTH
%ENDREP      
%ENDMACRO

%MACRO MT_EXPAND_UNROLLED        9
   MT_MORPHOLOGIC_UNROLLED       %1, MT_EXPAND_OPERATORS, %2, %3, %4, %5, %6, %7, %8, %9
%ENDMACRO

%MACRO MT_INPAND_UNROLLED        9
   MT_MORPHOLOGIC_UNROLLED       %1, MT_INPAND_OPERATORS, %2, %3, %4, %5, %6, %7, %8, %9
%ENDMACRO

%MACRO MT_MORPHOLOGIC_NEXT_LINE  0
   add                           esi, [esp + .nSrcPad]
   add                           edi, [esp + .nDstPad]
%ENDMACRO

%MACRO MT_MORPHOLOGIC_NEXT_8_PIXELS 0
   add                           esi, 8
   add                           edi, 8
%ENDMACRO

%MACRO MT_MORPHOLOGIC_NEXT_64_PIXELS 0
   add                           esi, 64
   add                           edi, 64
%ENDMACRO

%MACRO MT_MORPHOLOGIC_FUNCTION  4
%2 %+ 8_ %+ %1:
   SET_XMMX                %1
   STACK                   20, esi, edi, ebx, ebp
   .pDst                   PARAMETER(1)
   .nDstPitch              PARAMETER(2)
   .pSrc                   PARAMETER(3)
   .nSrcPitch              PARAMETER(4)
   .nMaxDeviation          PARAMETER(5)
   .nWidth                 PARAMETER(8)
   .nHeight                PARAMETER(9)
   .nTailWidth             VARIABLE(0)
   .nUnrolledWidth         VARIABLE(4)
   .nSrcPad                VARIABLE(8)
   .nDstPad                VARIABLE(12)
   .nHeightCount           VARIABLE(16)
   
   movd                    mm5, [esp + .nMaxDeviation]
   punpcklbw               mm5, mm5
   punpcklwd               mm5, mm5
   punpckldq               mm5, mm5
%IFIDNI REGISTERS, xmm
   movq2dq                 xmm5, mm5
   punpcklbw               xmm5, xmm5
%ENDIF
   
   mov                     edi, [esp + .pDst]
   mov                     esi, [esp + .pSrc]
   mov                     ebx, [esp + .nSrcPitch]
   mov                     ebp, [esp + .nDstPitch]
   sub                     esi, ebx                         ; positive offset needed for pitches
   
   mov                     ecx, [esp + .nWidth]
   sub                     ecx, ebx
   neg                     ecx
   mov                     [esp + .nSrcPad], ecx
   mov                     ecx, [esp + .nWidth]
   sub                     ecx, ebp
   neg                     ecx
   mov                     [esp + .nDstPad], ecx
   
   MT_PROCESS_CONVOLUTION  %1, %3, %4, MT_MORPHOLOGIC_NEXT_LINE, MT_MORPHOLOGIC_NEXT_8_PIXELS, MT_MORPHOLOGIC_NEXT_64_PIXELS
   
   RETURN
%ENDMACRO