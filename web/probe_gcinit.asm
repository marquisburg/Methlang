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
    call main
    mov [rsp+32], rax
    extern gc_shutdown
    call gc_shutdown
    mov rcx, [rsp+32]
    extern ExitProcess
    call ExitProcess
