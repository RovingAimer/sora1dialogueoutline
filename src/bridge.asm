; Copyright (c) 2026 DialogueOutline contributors
; SPDX-License-Identifier: MIT
OPTION CASEMAP:NONE
EXTERN SoraOutlineCallbackWithScale:PROC
EXTERN gSoraTrampoline:QWORD
EXTERN SoraLayoutPrepare:PROC
EXTERN SoraLayoutPosition:PROC
EXTERN SoraLayoutSpacing:PROC
EXTERN SoraLayoutDestroy:PROC
EXTERN gSoraPrepareTrampoline:QWORD
EXTERN gSoraPositionTrampoline:QWORD
EXTERN gSoraSpacingTrampoline:QWORD
EXTERN gSoraDestroyTrampoline:QWORD
PUBLIC SoraHookBridge
PUBLIC SoraPrepareBridge
PUBLIC SoraPositionBridge
PUBLIC SoraSpacingBridge
PUBLIC SoraDestroyBridge

; Entry bridges have RSP%16=8; the draw mid-function bridge has RSP%16=0.
; Replay prologues/cmp only after restoring the original RSP and volatile state.
LAYOUT_BRIDGE MACRO procname, callback, target, reserve, bodyInR8, callerInRdx
procname PROC
    pushfq
    push rax
    push rcx
    push rdx
    push r8
    push r9
    push r10
    push r11
    sub rsp, reserve
    movdqu xmmword ptr [rsp+20h], xmm0
    movdqu xmmword ptr [rsp+30h], xmm1
    movdqu xmmword ptr [rsp+40h], xmm2
    movdqu xmmword ptr [rsp+50h], xmm3
    movdqu xmmword ptr [rsp+60h], xmm4
    movdqu xmmword ptr [rsp+70h], xmm5
    stmxcsr dword ptr [rsp+80h]
    IF bodyInR8
        mov rcx, r8
    ENDIF
    IF callerInRdx
        ; Eight pushes plus this entry bridge's 98h reserve: original RSP+0.
        mov rdx, qword ptr [rsp+reserve+40h]
    ENDIF
    call callback
    ldmxcsr dword ptr [rsp+80h]
    movdqu xmm0, xmmword ptr [rsp+20h]
    movdqu xmm1, xmmword ptr [rsp+30h]
    movdqu xmm2, xmmword ptr [rsp+40h]
    movdqu xmm3, xmmword ptr [rsp+50h]
    movdqu xmm4, xmmword ptr [rsp+60h]
    movdqu xmm5, xmmword ptr [rsp+70h]
    add rsp, reserve
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdx
    pop rcx
    pop rax
    popfq
    jmp qword ptr [target]
procname ENDP
ENDM

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
LAYOUT_BRIDGE SoraPrepareBridge, SoraLayoutPrepare, gSoraPrepareTrampoline, 98h, 0, 1
LAYOUT_BRIDGE SoraPositionBridge, SoraLayoutPosition, gSoraPositionTrampoline, 90h, 1, 0
LAYOUT_BRIDGE SoraSpacingBridge, SoraLayoutSpacing, gSoraSpacingTrampoline, 90h, 0, 0
LAYOUT_BRIDGE SoraDestroyBridge, SoraLayoutDestroy, gSoraDestroyTrampoline, 98h, 0, 0
END
