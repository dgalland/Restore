;*****************************************************************************
;* cpu-32.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2003-2008 x264 project
;*
;* Authors: Laurent Aimar <fenrir@via.ecp.fr>
;*
;* This program is free software; you can redistribute it and/or modify
;* it under the terms of the GNU General Public License as published by
;* the Free Software Foundation; either version 2 of the License, or
;* (at your option) any later version.
;*
;* This program is distributed in the hope that it will be useful,
;* but WITHOUT ANY WARRANTY; without even the implied warranty of
;* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;* GNU General Public License for more details.
;*
;* You should have received a copy of the GNU General Public License
;* along with this program; if not, write to the Free Software
;* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
;*****************************************************************************

%include "x86inc.asm"

SECTION .text

;-----------------------------------------------------------------------------
; int x264_cpu_cpuid_test( void )
; return 0 if unsupported
;-----------------------------------------------------------------------------
cglobal x264_cpu_cpuid_test
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

;-----------------------------------------------------------------------------
; int x264_cpu_cpuid( int op, int *eax, int *ebx, int *ecx, int *edx )
;-----------------------------------------------------------------------------
cglobal x264_cpu_cpuid, 0,6
    mov     eax,    r0m
    cpuid
    mov     esi,    r1m
    mov     [esi],  eax
    mov     esi,    r2m
    mov     [esi],  ebx
    mov     esi,    r3m
    mov     [esi],  ecx
    mov     esi,    r4m
    mov     [esi],  edx
    RET

;-----------------------------------------------------------------------------
; void x264_emms( void )
;-----------------------------------------------------------------------------
cglobal x264_emms
    emms
    ret

;-----------------------------------------------------------------------------
; void x264_stack_align( void (*func)(void*), void *arg );
;-----------------------------------------------------------------------------
cglobal x264_stack_align
    push ebp
    mov  ebp, esp
    sub  esp, 4
    and  esp, ~15
    mov  ecx, [ebp+8]
    mov  edx, [ebp+12]
    mov  [esp], edx
    call ecx
    leave
    ret

