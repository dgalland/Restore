;=============================================================================
; Macros and other preprocessor constants
;=============================================================================

%MACRO cglobal 1
    %IFDEF NO_PREFIXED_SYMBOL
        global %1
    %ELSE
        global _%1
        %DEFINE %1 _%1
    %ENDIF
%ENDMACRO

%MACRO mt_mangle 2
   %IFDEF NO_PREFIXED_SYMBOL
      global %1_%2
   %ELSE
      global _%1_%2
      %DEFINE %2 _%1_%2
   %ENDIF
%ENDMACRO

;=============================================================================
; common rodata stuff
;=============================================================================

SECTION .rodata ALIGN=16
ALIGN 16
   MT_MASK_LEFT_XMM  : DB 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
   MT_MASK_RIGHT_XMM : DB 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF
   MT_MASK_LEFT_MMX  : DB 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
   MT_MASK_RIGHT_MMX : DB 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF

;=============================================================================
; Macros
;=============================================================================

;=============================================================================
; Parameters/Stack handling
;=============================================================================

%DEFINE PARAM(x)    EQU _MT_STACK * 4 + x * 4 + _MT_PUSH_COUNT * 4
%DEFINE PSTACK(x)   EQU x * 4

%MACRO MT_PUSH 1-*
    %REP %0
        push        %1
        %ROTATE     1
    %ENDREP
%ENDMACRO

%MACRO MT_POP 1-*
    %REP %0
        %ROTATE     -1
        pop         %1
    %ENDREP
%ENDMACRO

%MACRO _PUSH_REGISTER 1
   %IFIDNI _REGISTERS,null
      %XDEFINE _REGISTERS %1
   %ELSE
      %XDEFINE _REGISTERS %1, _REGISTERS
   %ENDIF      
   push %1
%ENDMACRO   

%MACRO _POP_REGISTERS 0-*
   %REP %0
      pop %1
      %ROTATE 1
   %ENDREP
%ENDMACRO

%ASSIGN _BITS 32
%ASSIGN _BYTES _BITS / 8

%DEFINE PARAMETER(n) EQU (_HEAP_SIZE + _BYTES * (n + _REGISTER))
%DEFINE VARIABLE(n)  EQU (n + _BYTES * _REGISTER)

; syntax is [minimum heap size, [number of arguments]], [pushed registers list]
%MACRO STACK 0-*

   %ASSIGN _HEAP_SIZE 0
   %ASSIGN _USE_LOCAL 0
   %ASSIGN _REGISTER %0
   
   %IFNUM %1
      %ASSIGN _HEAP_SIZE %1
      %ASSIGN _REGISTER _REGISTER - 1
      %ROTATE 1
   %ENDIF
   
   %IFNUM %1
      %ASSIGN _REGISTER _REGISTER - 1
      mov eax, esp
      sub esp, %1 * _BYTES
      sub esp, 4
      and esp, ~15
      sub eax, esp
      mov [esp], eax
      add eax, esp
      
      ; copy parameters
      
      %ASSIGN offset 0
      %REP %1
         mov edx, [eax + _BYTES * (1 + offset)]
         mov [esp + _BYTES * (1 + offset)], edx
         %ASSIGN offset offset + 1
      %ENDREP
      
      %IF _HEAP_SIZE & 15
         %ERROR Heap size must be a multiple of 16 when you ask aligning the heap
      %ENDIF
      
      %ASSIGN _USE_LOCAL 1
      %ROTATE 1
   %ENDIF
      
   ; allocate heap
   %IF _HEAP_SIZE   
      sub esp, _HEAP_SIZE
   %ENDIF

   ;%ERROR _REGISTER
   
   ; save registers
   %XDEFINE _REGISTERS null
   %REP _REGISTER
      _PUSH_REGISTER %1
      %ROTATE 1
   %ENDREP
   
   ;%ERROR _REGISTERS
   
%ENDMACRO   

%MACRO RETURN 0

   ; restore registers
   %IFNIDNI _REGISTERS,null
      _POP_REGISTERS _REGISTERS
   %ENDIF
   
   ; deallocate heap
   %IF _HEAP_SIZE   
      add esp, _HEAP_SIZE
   %ENDIF   

   %IF _USE_LOCAL
      mov ecx, [esp]
      add esp, ecx   
   %ENDIF
   
   ret
   
%ENDMACRO
   
%MACRO MT_SET_STACK 1-*
    %ASSIGN _MT_STACK %1
    %ASSIGN _MT_PUSH_COUNT %0 - 1
    %XDEFINE        MT_REGISTERS
    %IF (%0 > 1)
        %XDEFINE    MT_REGISTERS %2
    %ENDIF
    %IF (%0 > 2)
        %XDEFINE    MT_REGISTERS MT_REGISTERS, %3
    %ENDIF
    %IF (%0 > 3)
        %XDEFINE    MT_REGISTERS MT_REGISTERS, %4
    %ENDIF
    %IF (%0 > 4)
        %XDEFINE    MT_REGISTERS MT_REGISTERS, %5
    %ENDIF
    %IF (%0 > 1)
        MT_PUSH     MT_REGISTERS
    %ENDIF
    %IF (%1 > 0)
        sub         esp, _MT_STACK * 4
    %ENDIF
%ENDMACRO

%MACRO MT_REL_STACK 0-1
    %IF (%0 == 0)
         %IF (_MT_STACK > 0)
            add         esp, _MT_STACK * 4
         %ENDIF
    %ENDIF
    MT_POP          MT_REGISTERS
    %UNDEF          MT_REGISTERS
    ret
%ENDMACRO

%MACRO PUSHALL 0
	push      ebx
	push      edi
	push      esi
	push      ebp
%ENDMACRO

%MACRO POPALL 0
	pop       ebp
	pop       esi
	pop       edi
	pop       ebx
%ENDMACRO

%MACRO SHUFFLE 6
    pshufw          %1, %2, %3 * 1 + %4 * 4 + %5 * 16 + %6 * 64
%ENDMACRO

;=============================================================================
; XMMX handling
;=============================================================================

%MACRO SET_XMMX_MOVS_REGISTERS 1

%IFIDNI %1, mmx

%DEFINE MOV movq
%DEFINE HLOAD movd
%DEFINE HSTORE movd
%DEFINE LSHIFT psllq
%DEFINE RSHIFT psrlq
%DEFINE SBYTE 8
%DEFINE RWIDTH 8
%DEFINE RHWIDTH 4
%DEFINE R0 mm0
%DEFINE R1 mm1
%DEFINE R2 mm2
%DEFINE R3 mm3
%DEFINE R4 mm4
%DEFINE R5 mm5
%DEFINE R6 mm6
%DEFINE R7 mm7
%DEFINE REGISTERS mmx
%DEFINE DOWNGRADED %1

%DEFINE MT_MASK_LEFT MT_MASK_LEFT_MMX
%DEFINE MT_MASK_RIGHT MT_MASK_RIGHT_MMX

%ELIFIDNI %1, xmm

%DEFINE MOV movdqa
%DEFINE HLOAD movq
%DEFINE HSTORE movq
%DEFINE LSHIFT pslldq
%DEFINE RSHIFT psrldq
%DEFINE SBYTE 1
%DEFINE RWIDTH 16
%DEFINE RHWIDTH 8
%DEFINE R0 xmm0
%DEFINE R1 xmm1
%DEFINE R2 xmm2
%DEFINE R3 xmm3
%DEFINE R4 xmm4
%DEFINE R5 xmm5
%DEFINE R6 xmm6
%DEFINE R7 xmm7
%DEFINE REGISTERS xmm
%DEFINE DOWNGRADED isse

%DEFINE MT_MASK_LEFT MT_MASK_LEFT_XMM
%DEFINE MT_MASK_RIGHT MT_MASK_RIGHT_XMM

%ELSE

%ERROR unsupported register set : %1

%ENDIF

%ENDMACRO

%MACRO SET_XMMX 1

%IFIDNI %1, mmx

SET_XMMX_MOVS_REGISTERS mmx

%DEFINE PREFETCH_R MT_NOPREFETCH
%DEFINE PREFETCH_RW MT_NOPREFETCH
%DEFINE TSTORE movq

%ELIFIDNI %1, isse

SET_XMMX_MOVS_REGISTERS mmx

%DEFINE PREFETCH_R MT_PREFETCHT0
%DEFINE PREFETCH_RW MT_PREFETCHT0
%DEFINE TSTORE movq

%ELIFIDNI %1, 3dnow

SET_XMMX_MOVS_REGISTERS mmx

%DEFINE PREFETCH_R MT_PREFETCHT0
%DEFINE PREFETCH_RW MT_PREFETCHW
%DEFINE TSTORE movq

%ELSE

SET_XMMX_MOVS_REGISTERS xmm

%DEFINE PREFETCH_R MT_PREFETCHT0
%DEFINE PREFETCH_RW MT_PREFETCHT0

%ENDIF

%IFIDNI REGISTERS,mmx

%DEFINE LOAD movq
%DEFINE ALOAD movq
%DEFINE ULOAD movq
%DEFINE STORE movq
%DEFINE ASTORE movq
%DEFINE USTORE movq
%DEFINE HLOAD movd
%DEFINE HSTORE movd

%ELSE

%DEFINE HLOAD movq
%DEFINE HSTORE movq
%DEFINE ALOAD movdqa
%DEFINE ASTORE movdqa
%DEFINE USTORE movdqu
%DEFINE ATSTORE movntdq
%DEFINE UTSTORE movdqu

%IFIDNI %1, sse2

%DEFINE LOAD movdqu
%DEFINE ULOAD movdqu
%DEFINE STORE movdqu
%DEFINE TSTORE movdqu

%ELIFIDNI %1, asse2

%DEFINE LOAD movdqa
%DEFINE ULOAD movdqu
%DEFINE STORE movdqa
%DEFINE TSTORE movntdq

%ELIFIDNI %1, usse2

%DEFINE LOAD movdqu
%DEFINE ULOAD movdqu
%DEFINE STORE movdqu

%ELIFIDNI %1, sse3

%DEFINE LOAD lddqu
%DEFINE ULOAD lddqu
%DEFINE STORE movdqu

%ELIFIDNI %1, asse3

%DEFINE LOAD movdqa
%DEFINE ULOAD lddqu
%DEFINE STORE movdqa

%ELIFIDNI %1, usse3

%DEFINE LOAD lddqu
%DEFINE ULOAD lddqu
%DEFINE STORE movdqu

%ELSE

%ERROR unsupported extension set : %1

%ENDIF

%ENDIF

%ENDMACRO

;=============================================================================
; prefetchs
;=============================================================================

%MACRO MT_PREFETCHW  1
   prefetchw         [%1]
%ENDMACRO

%MACRO MT_PREFETCHT0 1
   prefetcht0        [%1]
%ENDMACRO

%MACRO MT_NOPREFETCH 1
%ENDMACRO

%MACRO MT_PREFETCHNTA 1
   prefetchnta       [%1]
%ENDMACRO


;=============================================================================
; unrolling stuff
;=============================================================================


; uses edx, eax, ecx, edi
; edi is the pointer on the data, eax contains the pitch, and ecx the height
; three parameters : code for the aligning loops ( tail - 8 bytes)
;                    code for the unrolled loop ( 64 bytes )
;                    size of the jump in the unrolled loop
; in these loops, edi + eax will be the current position
%MACRO MT_UNROLL_INPLACE 2
   mov                  eax, [esp + .nDstPitch]
   mov                  ecx, [esp + .nHeight]

   mul                  ecx               ; compute the total size
   sub                  eax, ebp
   
   mov                  edx, eax          ; save it into edx ( unrolled loop stop )
   mov                  ecx, eax          ; save it into ecx
   and                  edx, 0xFFFFFFC0   ; make the loop 64-bytes aligned
   xor                  eax, eax          ; reset eax ( loop counter )
   
   cmp                  eax, ecx
   je                   .end              ; if nothing, jump
   
.loop

   %1                                     ; unrolled code
   
   add                  eax, 64
   cmp                  eax, edx
   jl                   .loop
   
   cmp                  eax, ecx
   je                   .end
   
.tail

   %2                                     ; tail code

   add                  eax, 8
   cmp                  eax, ecx
   jl                   .tail
   
.end
   
%ENDMACRO   


; uses edx, eax, ecx
; ecx will be the height counter, eax the width one, and edx the loop stop
; three parameters : code for the unrolled loop ( 64 bytes )
;                    code for the aligning loops ( head & tail - 8 bytes)
;                    code for increasing pointers
%MACRO MT_UNROLL_INPLACE_WIDTH 3
   mov                  ecx, [esp + .nHeight];
   mov                  edx, [esp + .nWidth] ; save it into edx ( unrolled loop stop )
   and                  edx, 0xFFFFFFC0      ; make the loop 64-bytes aligned
   
%%loopy
   
   xor                  eax, eax          ; reset eax ( loop counter )
   
   cmp                  eax, edx          ; check that we can do 64 pixels 
   je                   %%no64
   
%%loop

   %1                                     ; unrolled code
   
   add                  eax, 64
   cmp                  eax, edx
   jl                   %%loop
   
%%no64   
   
   cmp                  eax, [esp + .nWidth]
   je                   %%end

%%tail

   %2                                     ; tail code ( same as head )

   add                  eax, 8
   cmp                  eax, [esp + .nWidth]
   jl                   %%tail
   
%%end

   %3                                     ; change line code
   dec                  ecx
   jnz                  %%loopy


   
%ENDMACRO   
   
;=============================================================================
; convolution & border stuff
;=============================================================================

%MACRO MT_LOAD_NORMAL         7
   %2                         %3, [%4 + %5]
%ENDMACRO

%MACRO MT_LOAD_OFFSET         8
   SET_XMMX                   %1
   %IFNIDN %2, ULOAD
      ULOAD                   %4, [%6]
      ULOAD                   %5, [%7]
      pand                    %5, %4
      %8                      %4, SBYTE      ; drop the byte
      paddb                   %4, %5
      %2                      %3, %4
   %ELSE
      ULOAD                   %3, [%6]
      ULOAD                   %5, [%7]
      pand                    %5, %3
      %8                      %3, SBYTE      ; drop the byte
      paddb                   %3, %5
   %ENDIF
%ENDMACRO

   
;=============================================================================
;left border
;=============================================================================

%MACRO MT_LOAD_LEFT_BORDER    7
   SET_XMMX                   %1
   MT_LOAD_OFFSET             %1, %2, %3, %6, %7, %4 + %5, MT_MASK_LEFT, LSHIFT
%ENDMACRO

%MACRO MT_LOAD_LEFT_NORMAL    7
   %2                         %3, [%4 + %5 - 1]
%ENDMACRO

;=============================================================================
;right border
;=============================================================================

%MACRO MT_LOAD_RIGHT_BORDER   7
   SET_XMMX                   %1
   MT_LOAD_OFFSET             %1, %2, %3, %6, %7, %4 + %5, MT_MASK_RIGHT, RSHIFT
%ENDMACRO

%MACRO MT_LOAD_RIGHT_NORMAL   7
   %2                         %3, [%4 + %5 + 1]
%ENDMACRO

;=============================================================================
; top border
;=============================================================================

%MACRO MT_LOAD_UP_BORDER      7
   %2                         %3, [%4 + %5]
%ENDMACRO

%MACRO MT_LOAD_UP_NORMAL      7
   %2                         %3, [%4]
%ENDMACRO

;=============================================================================
; bottom border
;=============================================================================

%MACRO MT_LOAD_DOWN_BORDER    7
   %2                         %3, [%4 + %5]
%ENDMACRO

%MACRO MT_LOAD_DOWN_NORMAL    7
   %2                         %3, [%4 + 2 * %5]
%ENDMACRO

;=============================================================================
; top left
;=============================================================================

%MACRO MT_LOAD_UP_LEFT_BORDER_UP_LEFT 7
   SET_XMMX                   %1
   MT_LOAD_OFFSET             %1, %2, %3, %6, %7, %4 + %5, MT_MASK_LEFT, LSHIFT
%ENDMACRO

%MACRO MT_LOAD_UP_LEFT_BORDER_LEFT 7
   SET_XMMX                   %1
   MT_LOAD_OFFSET             %1, %2, %3, %6, %7, %4, MT_MASK_LEFT, LSHIFT
%ENDMACRO

%MACRO MT_LOAD_UP_LEFT_BORDER_UP 7
   %2                         %3, [%4 + %5 - 1]
%ENDMACRO

%MACRO MT_LOAD_UP_LEFT_NORMAL 7
   %2                         %3, [%4 - 1]
%ENDMACRO

;=============================================================================
; top right
;=============================================================================

%MACRO MT_LOAD_UP_RIGHT_BORDER_UP_RIGHT 7
   SET_XMMX                   %1
   MT_LOAD_OFFSET             %1, %2, %3, %6, %7, %4 + %5, MT_MASK_RIGHT, RSHIFT
%ENDMACRO

%MACRO MT_LOAD_UP_RIGHT_BORDER_RIGHT 7
   SET_XMMX                   %1
   MT_LOAD_OFFSET             %1, %2, %3, %6, %7, %4, MT_MASK_RIGHT, RSHIFT
%ENDMACRO

%MACRO MT_LOAD_UP_RIGHT_BORDER_UP 7
   %2                         %3, [%4 + %5 + 1]
%ENDMACRO

%MACRO MT_LOAD_UP_RIGHT_NORMAL 7
   %2                         %3, [%4 + 1]
%ENDMACRO

;=============================================================================
; bottom left
;=============================================================================

%MACRO MT_LOAD_DOWN_LEFT_BORDER_LEFT 7
   SET_XMMX                   %1
   MT_LOAD_OFFSET             %1, %2, %3, %6, %7, %4 + 2 * %5, MT_MASK_LEFT, LSHIFT
%ENDMACRO

%MACRO MT_LOAD_DOWN_LEFT_BORDER_DOWN_LEFT 7
   SET_XMMX                   %1
   MT_LOAD_OFFSET             %1, %2, %3, %6, %7, %4 + %5, MT_MASK_LEFT, LSHIFT
%ENDMACRO

%MACRO MT_LOAD_DOWN_LEFT_BORDER_DOWN 7
   %2                         %3, [%4 + %5 - 1]
%ENDMACRO

%MACRO MT_LOAD_DOWN_LEFT_NORMAL 7
   %2                         %3, [%4 + 2 * %5 - 1]
%ENDMACRO

;=============================================================================
; bottom right
;=============================================================================

%MACRO MT_LOAD_DOWN_RIGHT_BORDER_RIGHT 7
   SET_XMMX                   %1
   MT_LOAD_OFFSET             %1, %2, %3, %6, %7, %4 + 2 * %5, MT_MASK_RIGHT, RSHIFT
%ENDMACRO

%MACRO MT_LOAD_DOWN_RIGHT_BORDER_DOWN_RIGHT 7
   SET_XMMX                   %1
   MT_LOAD_OFFSET             %1, %2, %3, %6, %7, %4 + %5, MT_MASK_RIGHT, RSHIFT
%ENDMACRO

%MACRO MT_LOAD_DOWN_RIGHT_BORDER_DOWN 7
   %2                         %3, [%4 + %5 + 1]
%ENDMACRO

%MACRO MT_LOAD_DOWN_RIGHT_NORMAL 7
   %2                         %3, [%4 + 2 * %5 + 1]
%ENDMACRO

; combos

%DEFINE MT_TOP_LEFT     MT_LOAD_UP_LEFT_BORDER_UP_LEFT, MT_LOAD_UP_BORDER, MT_LOAD_UP_RIGHT_BORDER_UP      , MT_LOAD_LEFT_BORDER, MT_LOAD_RIGHT_NORMAL, MT_LOAD_DOWN_LEFT_BORDER_LEFT     , MT_LOAD_DOWN_NORMAL, MT_LOAD_DOWN_RIGHT_NORMAL
%DEFINE MT_TOP          MT_LOAD_UP_LEFT_BORDER_UP     , MT_LOAD_UP_BORDER, MT_LOAD_UP_RIGHT_BORDER_UP      , MT_LOAD_LEFT_NORMAL, MT_LOAD_RIGHT_NORMAL, MT_LOAD_DOWN_LEFT_NORMAL          , MT_LOAD_DOWN_NORMAL, MT_LOAD_DOWN_RIGHT_NORMAL
%DEFINE MT_TOP_RIGHT    MT_LOAD_UP_LEFT_BORDER_UP     , MT_LOAD_UP_BORDER, MT_LOAD_UP_RIGHT_BORDER_UP_RIGHT, MT_LOAD_LEFT_NORMAL, MT_LOAD_RIGHT_BORDER, MT_LOAD_DOWN_LEFT_NORMAL          , MT_LOAD_DOWN_NORMAL, MT_LOAD_DOWN_RIGHT_BORDER_RIGHT

%DEFINE MT_LEFT         MT_LOAD_UP_LEFT_BORDER_LEFT   , MT_LOAD_UP_NORMAL, MT_LOAD_UP_RIGHT_NORMAL         , MT_LOAD_LEFT_BORDER, MT_LOAD_RIGHT_NORMAL, MT_LOAD_DOWN_LEFT_BORDER_LEFT     , MT_LOAD_DOWN_NORMAL, MT_LOAD_DOWN_RIGHT_NORMAL
%DEFINE MT_NORMAL       MT_LOAD_UP_LEFT_NORMAL        , MT_LOAD_UP_NORMAL, MT_LOAD_UP_RIGHT_NORMAL         , MT_LOAD_LEFT_NORMAL, MT_LOAD_RIGHT_NORMAL, MT_LOAD_DOWN_LEFT_NORMAL          , MT_LOAD_DOWN_NORMAL, MT_LOAD_DOWN_RIGHT_NORMAL
%DEFINE MT_RIGHT        MT_LOAD_UP_LEFT_NORMAL        , MT_LOAD_UP_NORMAL, MT_LOAD_UP_RIGHT_BORDER_RIGHT   , MT_LOAD_LEFT_NORMAL, MT_LOAD_RIGHT_BORDER, MT_LOAD_DOWN_LEFT_NORMAL          , MT_LOAD_DOWN_NORMAL, MT_LOAD_DOWN_RIGHT_BORDER_RIGHT

%DEFINE MT_BOTTOM_LEFT  MT_LOAD_UP_LEFT_BORDER_LEFT   , MT_LOAD_UP_NORMAL, MT_LOAD_UP_RIGHT_NORMAL         , MT_LOAD_LEFT_BORDER, MT_LOAD_RIGHT_NORMAL, MT_LOAD_DOWN_LEFT_BORDER_DOWN_LEFT, MT_LOAD_DOWN_BORDER, MT_LOAD_DOWN_RIGHT_BORDER_DOWN
%DEFINE MT_BOTTOM       MT_LOAD_UP_LEFT_NORMAL        , MT_LOAD_UP_NORMAL, MT_LOAD_UP_RIGHT_NORMAL         , MT_LOAD_LEFT_NORMAL, MT_LOAD_RIGHT_NORMAL, MT_LOAD_DOWN_LEFT_BORDER_DOWN     , MT_LOAD_DOWN_BORDER, MT_LOAD_DOWN_RIGHT_BORDER_DOWN
%DEFINE MT_BOTTOM_RIGHT MT_LOAD_UP_LEFT_NORMAL        , MT_LOAD_UP_NORMAL, MT_LOAD_UP_RIGHT_BORDER_RIGHT   , MT_LOAD_LEFT_NORMAL, MT_LOAD_RIGHT_BORDER, MT_LOAD_DOWN_LEFT_BORDER_DOWN     , MT_LOAD_DOWN_BORDER, MT_LOAD_DOWN_RIGHT_BORDER_DOWN_RIGHT

;=============================================================================
; processing
;=============================================================================

%MACRO MT_PROCESS_LINE     8
   SET_XMMX                %1
   xor                     eax, eax                         ; loop counter resetted
   
   ; left
   %3                      %1, %4                           ; rolled code, left
   
   add                     eax, 8
   %7                                                       ; next 8 pixels
   
%%loopx:

   %2                      %1, %5                           ; unrolled code, center
   add                     eax, 64
   %8                                                       ; next 64 pixels
   cmp                     eax, [esp + .nUnrolledWidth]
   jl                      %%loopx
   
   cmp                     eax, [esp + .nTailWidth]
   je                      %%endx
   
%%tailx:

   %3                      %1, %5                           ; rolled code, center
   add                     eax, 8
   %7                                                       ; next 8 pixels
   cmp                     eax, [esp + .nTailWidth]
   jl                      %%tailx
  
%%endx:   
   ; right
   
   %3                      %1, %6                           ; rolled code, right
   %7                                                       ; next 8 pixels
%ENDMACRO   
   
%MACRO MT_PROCESS_CONVOLUTION 6
   mov                     eax, [esp + .nWidth]
   sub                     eax, 8
   mov                     [esp + .nTailWidth], eax         ; row length, minus 8 last bytes
   sub                     eax, 8                           ; row length, minus 8 first bytes
   and                     eax, 0xFFFFFFC0                  ; multiple of 64
   add                     eax, 8                           ; add back the 8 first bytes
   mov                     [esp + .nUnrolledWidth], eax     ; row length, rounded to the last unrolled loop

   ; top line

   MT_PROCESS_LINE         %1, %2, %3, {MT_TOP_LEFT}, {MT_TOP}, {MT_TOP_RIGHT}, %5, %6
   
   %4                                                       ; next line
   
   ; center
   mov                     eax, [esp + .nHeight]
   sub                     eax, 2                           ; -2 : top & bottom lines
   
.loopy

   mov                     [esp + .nHeightCount], eax       ; save height counter

   MT_PROCESS_LINE         %1, %2, %3, {MT_LEFT}, {MT_NORMAL}, {MT_RIGHT}, %5, %6

   %4                                                       ; next line
   
   mov                     eax, [esp + .nHeightCount]       ; load height counter
   dec                     eax
   jnz                     .loopy
   
   ; bottom line
   
   MT_PROCESS_LINE         %1, %2, %3, {MT_BOTTOM_LEFT}, {MT_BOTTOM}, {MT_BOTTOM_RIGHT}, %5, %6
%ENDMACRO