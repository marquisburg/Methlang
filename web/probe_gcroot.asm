section .data
msg:
    dq msg_chars
    dq 5
msg_chars:
    db 'h','e','l','l','o',0

section .text
global main
main:
    push rbp
    mov rbp, rsp
    mov eax, 0
    mov rsp, rbp
    pop rbp
    ret

global mainCRTStartup
mainCRTStartup:
    sub rsp, 40
    lea rcx, [rsp + 40]
    extern gc_init
    call gc_init
    lea rcx, [rel msg]
    extern gc_register_root
    call gc_register_root
    call main
    mov [rsp+32], rax
    extern gc_shutdown
    call gc_shutdown
    mov rcx, [rsp+32]
    extern ExitProcess
    call ExitProcess
