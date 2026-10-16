; Copyright (c) 2026 DialogueOutline contributors
; SPDX-License-Identifier: MIT
OPTION CASEMAP:NONE
EXTERN SoraOutlineCallbackWithScale:PROC
EXTERN gSoraTrampoline:QWORD
PUBLIC SoraHookBridge

.code
; Mid-function entry RSP is 16-byte aligned. No synthetic return address is used.
; Preserve live volatile state before the callback, then let the original INC set flags.
SoraHookBridge PROC
    pushfq
    push rax
    push rcx
    push rdx
    push r8
    push r9
    push r10
    push r11
    sub rsp, 90h
    movdqu xmmword ptr [rsp+20h], xmm0
    movdqu xmmword ptr [rsp+30h], xmm1
    movdqu xmmword ptr [rsp+40h], xmm2
    movdqu xmmword ptr [rsp+50h], xmm3
    movdqu xmmword ptr [rsp+60h], xmm4
    movdqu xmmword ptr [rsp+70h], xmm5
    stmxcsr dword ptr [rsp+80h]
    mov rcx, rsi
    mov rdx, rdi
    mov r8, rbx
    movss xmm3, xmm8 ; Fourth Win64 argument: native glyph construction scale.
    call SoraOutlineCallbackWithScale
    ldmxcsr dword ptr [rsp+80h]
    movdqu xmm0, xmmword ptr [rsp+20h]
    movdqu xmm1, xmmword ptr [rsp+30h]
    movdqu xmm2, xmmword ptr [rsp+40h]
    movdqu xmm3, xmmword ptr [rsp+50h]
    movdqu xmm4, xmmword ptr [rsp+60h]
    movdqu xmm5, xmmword ptr [rsp+70h]
    add rsp, 90h
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdx
    pop rcx
    pop rax
    popfq
    jmp qword ptr [gSoraTrampoline]
SoraHookBridge ENDP
END
