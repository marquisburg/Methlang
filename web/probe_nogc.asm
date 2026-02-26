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
    call main
    mov ecx, eax
    extern ExitProcess
    call ExitProcess
