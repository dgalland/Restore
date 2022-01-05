%INCLUDE "common.asm"

SECTION .rodata

;=============================================================================
; Code
;=============================================================================

SECTION .text

   cglobal memset8_mmx
   cglobal memset8_isse
   cglobal memset8_3dnow
   cglobal copy8_mmx
   cglobal copy8_isse
   cglobal copy8_3dnow
   cglobal stream_copy8_mmx
   cglobal stream_copy8_isse
   cglobal stream_copy8_3dnow
   cglobal prefetch_c
   cglobal prefetch_isse
   cglobal emms_mmx
   cglobal cpuid_asm
   cglobal cpu_test_asm
   
%MACRO MT_MEMSET_ROLLED 0
   movq                 [edi + eax], mm0
%ENDMACRO

;=============================================================================
; memset macros
;=============================================================================

%MACRO MT_MEMSET_UNROLLED 1
   ;%1                   edi + eax + 640
   movq                 [edi + eax], mm0
   movq                 [edi + eax + 8], mm0
   movq                 [edi + eax + 16], mm0
   movq                 [edi + eax + 24], mm0
   movq                 [edi + eax + 32], mm0
   movq                 [edi + eax + 40], mm0
   movq                 [edi + eax + 48], mm0
   movq                 [edi + eax + 56], mm0
%ENDMACRO

%MACRO MT_MEMSET_ENDLINE 0
   add                  edi, [esp + .nDstPitch]
%ENDMACRO

%MACRO MT_MEMSET_FUNCTION 2
%1 :
   STACK                edi
   .pDst                PARAMETER(1)
   .nDstPitch           PARAMETER(2)
   .nWidth              PARAMETER(3)
   .nHeight             PARAMETER(4)
   .nValue              PARAMETER(5)

   mov                  edi, [esp + .pDst]
   movd                 mm0, [esp + .nValue]
   punpcklbw            mm0, mm0
   punpcklwd            mm0, mm0
   punpckldq            mm0, mm0

   MT_UNROLL_INPLACE_WIDTH  {MT_MEMSET_UNROLLED %2}, MT_MEMSET_ROLLED, MT_MEMSET_ENDLINE
   
   RETURN
%ENDMACRO

;=============================================================================
; memset functions
;=============================================================================

MT_MEMSET_FUNCTION      memset8_mmx    , MT_NOPREFETCH
MT_MEMSET_FUNCTION      memset8_isse   , MT_PREFETCHT0
MT_MEMSET_FUNCTION      memset8_3dnow  , MT_PREFETCHW

;=============================================================================
; copy macros
;=============================================================================

%MACRO MT_COPY_ROLLED 0
   movq                 mm0, [esi + eax]
   movq                 [edi + eax], mm0
%ENDMACRO

%MACRO MT_COPY_UNROLLED 2
   %1                   esi + eax + 384
   ;%2                   edi + eax + 320
   movq                 mm0, [esi + eax]
   movq                 mm1, [esi + eax + 8]
   movq                 mm2, [esi + eax + 16]
   movq                 mm3, [esi + eax + 24]
   movq                 mm4, [esi + eax + 32]
   movq                 mm5, [esi + eax + 40]
   movq                 mm6, [esi + eax + 48]
   movq                 mm7, [esi + eax + 56]
   movq                 [edi + eax], mm0
   movq                 [edi + eax + 8], mm1
   movq                 [edi + eax + 16], mm2
   movq                 [edi + eax + 24], mm3
   movq                 [edi + eax + 32], mm4
   movq                 [edi + eax + 40], mm5
   movq                 [edi + eax + 48], mm6
   movq                 [edi + eax + 56], mm7
%ENDMACRO

%MACRO MT_COPY_ENDLINE 0
   add                  edi, [esp + .nDstPitch]
   add                  esi, [esp + .nSrcPitch]
%ENDMACRO

%MACRO MT_COPY_FUNCTION 3
%1 :
   STACK                esi, edi
   .pDst                PARAMETER(1)
   .nDstPitch           PARAMETER(2)
   .pSrc                PARAMETER(3)
   .nSrcPitch           PARAMETER(4)
   .nWidth              PARAMETER(5)
   .nHeight             PARAMETER(6)

   mov                  edi, [esp + .pDst]
   mov                  esi, [esp + .pSrc]
   
   MT_UNROLL_INPLACE_WIDTH  {MT_COPY_UNROLLED %2, %3}, MT_COPY_ROLLED, MT_COPY_ENDLINE
      
   RETURN
%ENDMACRO

;=============================================================================
; copy functions
;=============================================================================
   
MT_COPY_FUNCTION        copy8_mmx   , MT_NOPREFETCH   , MT_NOPREFETCH
MT_COPY_FUNCTION        copy8_isse  , MT_PREFETCHNTA  , MT_PREFETCHT0
MT_COPY_FUNCTION        copy8_3dnow , MT_PREFETCHNTA  , MT_PREFETCHT0

;=============================================================================
; stream copy macros
;=============================================================================

%MACRO MT_STREAM_COPY_ROLLED 0
   movq                 mm0, [esi + eax]
   movq                 [edi + eax], mm0
%ENDMACRO

%MACRO MT_STREAM_COPY_UNROLLED 1
   %1                   esi + eax + 384
   movq                 mm0, [esi + eax]
   movq                 mm1, [esi + eax + 8]
   movq                 mm2, [esi + eax + 16]
   movq                 mm3, [esi + eax + 24]
   movq                 mm4, [esi + eax + 32]
   movq                 mm5, [esi + eax + 40]
   movq                 mm6, [esi + eax + 48]
   movq                 mm7, [esi + eax + 56]
   movntq               [edi + eax], mm0
   movntq               [edi + eax + 8], mm1
   movntq               [edi + eax + 16], mm2
   movntq               [edi + eax + 24], mm3
   movntq               [edi + eax + 32], mm4
   movntq               [edi + eax + 40], mm5
   movntq               [edi + eax + 48], mm6
   movntq               [edi + eax + 56], mm7
%ENDMACRO

%MACRO MT_STREAM_COPY_FUNCTION 2
%1 :
   STACK                esi, edi, ebp
   .pDst                PARAMETER(1)
   .pSrc                PARAMETER(2)
   .nDstPitch           PARAMETER(3)
   .nHeight             PARAMETER(4)

   mov                  edi, [esp + .pDst]
   mov                  esi, [esp + .pSrc]
   
   xor                  ebp, ebp
   
   MT_UNROLL_INPLACE    {MT_STREAM_COPY_UNROLLED %2}, MT_STREAM_COPY_ROLLED
      
   RETURN
%ENDMACRO

MT_STREAM_COPY_FUNCTION stream_copy8_mmx   , MT_NOPREFETCH
MT_STREAM_COPY_FUNCTION stream_copy8_isse  , MT_PREFETCHNTA
MT_STREAM_COPY_FUNCTION stream_copy8_3dnow , MT_PREFETCHNTA

prefetch_c:
   STACK                ebx
   .pData               PARAMETER(1)
   .nData               PARAMETER(2)
   
   mov                  eax, [esp + .pData]
   mov                  edx, [esp + .nData]
   
   xor                  ecx, ecx
   
.loop

   mov                  ebx, [eax + ecx]
   mov                  ebx, [eax + ecx + 64]
   mov                  ebx, [eax + ecx + 128]
   mov                  ebx, [eax + ecx + 192]
   
   add                  ecx, 256
   cmp                  ecx, edx
   jl                   .loop
   
   RETURN
   
   
prefetch_isse:
   STACK
   .pData               PARAMETER(1)
   .nData               PARAMETER(2)
   
   mov                  eax, [esp + .pData]
   mov                  edx, [esp + .nData]
   
   xor                  ecx, ecx
   
.loop

   prefetcht0           [eax + ecx]
   prefetcht0           [eax + ecx + 64]
   prefetcht0           [eax + ecx + 128]
   prefetcht0           [eax + ecx + 192]
   
   add                  ecx, 256
   cmp                  ecx, edx
   jl                   .loop
   
   RETURN
   
emms_mmx:
   STACK
   
   emms
   
   RETURN
   
cpuid_asm:
    push    ebp
    mov     ebp,    esp
    push    ebx
    push    esi
    push    edi
    
    mov     eax,    [ebp +  8]
    cpuid

    mov     esi,    [ebp + 12]
    mov     [esi],  eax

    mov     esi,    [ebp + 16]
    mov     [esi],  ebx

    mov     esi,    [ebp + 20]
    mov     [esi],  ecx

    mov     esi,    [ebp + 24]
    mov     [esi],  edx

    pop     edi
    pop     esi
    pop     ebx
    pop     ebp
    ret

cpu_test_asm:
    pushfd
    push    ebx
    push    ebp
    push    esi
    push    edi

    pushfd
    pop     eax
    mov     ebx, eax
    xor     eax, 0x200000
    push    eax
    popfd
    pushfd
    pop     eax
    xor     eax, ebx
    
    pop     edi
    pop     esi
    pop     ebp
    pop     ebx
    popfd
    ret
