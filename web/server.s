


bits 64

section .text

















































































































































































































































section .text
    extern puts
    extern putchar
    extern getchar

global cstr

cstr:
    push rbp
    mov rbp, rsp
    sub rsp, 80

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_0:

    mov rax, qword [rbp - 8]
    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    mov rax, qword [rax]
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    mov [rbp - 32], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_errdefer_ok_1
    jmp ir_errdefer_end_2
ir_errdefer_ok_1:
ir_errdefer_end_2:
    mov rax, [rbp - 24]
    jmp Lcstr_exit
Lcstr_exit:

    mov rsp, rbp
    pop rbp
    ret

global print

print:
    push rbp
    mov rbp, rsp
    sub rsp, 272

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_3:
    mov rax, 0

    mov qword [rbp - 16], rax
ir_while_4:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_6
    jmp ir_nonnull_7
ir_trap_null_6:

    sub rsp, 32

    lea rax, [rel Lstr_struct73]
    mov rcx, rax
    call puts
    add rsp, 32


    extern exit

    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_7:

    mov rax, qword [rbp - 16]
    mov [rbp - 24], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 24]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 32], rax
    mov rax, [rbp - 32]
    movzx rax, byte [rax]
    mov [rbp - 40], rax
    mov rax, [rbp - 40]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    test rax, rax
    jz ir_while_end_5

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_8
    jmp ir_nonnull_9
ir_trap_null_8:

    sub rsp, 32

    lea rax, [rel Lstr_struct75]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_9:

    mov rax, qword [rbp - 16]
    mov [rbp - 56], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 56]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 64], rax
    mov rax, [rbp - 64]
    movzx rax, byte [rax]
    mov [rbp - 72], rax

    sub rsp, 32
    mov rax, [rbp - 72]
    mov rcx, rax
    call putchar
    add rsp, 32


    mov [rbp - 80], rax

    mov rax, qword [rbp - 16]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 88], rax
    mov rax, [rbp - 88]

    mov qword [rbp - 16], rax
    jmp ir_while_4
ir_while_end_5:
    mov rax, 0
    mov [rbp - 96], rax
ir_errdefer_ok_10:
ir_errdefer_end_11:
    jmp Lprint_exit
Lprint_exit:

    mov rsp, rbp
    pop rbp
    ret

global println

println:
    push rbp
    mov rbp, rsp
    sub rsp, 64

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_12:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    call puts
    add rsp, 32


    mov [rbp - 16], rax
    mov rax, 0
    mov [rbp - 24], rax
ir_errdefer_ok_13:
ir_errdefer_end_14:
    jmp Lprintln_exit
Lprintln_exit:

    mov rsp, rbp
    pop rbp
    ret

global newline

newline:
    push rbp
    mov rbp, rsp
    sub rsp, 48


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_15:

    sub rsp, 32
    mov rax, 10
    mov rcx, rax
    call putchar
    add rsp, 32


    mov [rbp - 8], rax
    mov rax, 0
    mov [rbp - 16], rax
ir_errdefer_ok_16:
ir_errdefer_end_17:
    jmp Lnewline_exit
Lnewline_exit:

    mov rsp, rbp
    pop rbp
    ret

global print_int

print_int:
    push rbp
    mov rbp, rsp
    sub rsp, 752

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_18:

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 104], rax
    mov rax, [rbp - 104]
    test rax, rax
    jz ir_if_next_20

    sub rsp, 32
    mov rax, 45
    mov rcx, rax
    call putchar
    add rsp, 32


    mov [rbp - 112], rax
    mov rax, 0
    push rax

    mov rax, qword [rbp - 8]
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 120], rax
    mov rax, [rbp - 120]

    mov qword [rbp - 8], rax
    jmp ir_if_end_19
ir_if_next_20:
ir_if_end_19:

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 128], rax
    mov rax, [rbp - 128]
    test rax, rax
    jz ir_if_next_22

    sub rsp, 32
    mov rax, 48
    mov rcx, rax
    call putchar
    add rsp, 32


    mov [rbp - 136], rax
    mov rax, 0
    mov [rbp - 144], rax
ir_errdefer_ok_23:
ir_errdefer_end_24:
    jmp Lprint_int_exit
ir_if_next_22:
ir_if_end_21:
    mov rax, 0

    mov dword [rbp - 92], eax
ir_while_25:

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setg al
    movzx rax, al
    mov [rbp - 152], rax
    mov rax, [rbp - 152]
    test rax, rax
    jz ir_while_end_26
    mov rax, 48
    push rax

    mov rax, qword [rbp - 8]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 160], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 10
    mov r10, rax
    pop rax
    cqo
    idiv r10
    mov [rbp - 168], rax
    mov rax, [rbp - 168]
    push rax
    mov rax, 10
    mov r10, rax
    pop rax
    imul rax, r10
    mov [rbp - 176], rax
    mov rax, [rbp - 160]
    push rax
    mov rax, [rbp - 176]
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 184], rax

    movsxd rax, dword [rbp - 92]
    push rax
    mov rax, 20
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 192], rax
    mov rax, [rbp - 192]
    test rax, rax
    jz ir_trap_bounds_27
    jmp ir_in_bounds_28
ir_trap_bounds_27:

    sub rsp, 32

    lea rax, [rel Lstr_struct77]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_28:

    movsxd rax, dword [rbp - 92]
    push rax
    mov rax, 4
    mov r10, rax
    pop rax
    imul rax, r10
    mov [rbp - 200], rax

    lea rax, [rbp - 88]
    push rax
    mov rax, [rbp - 200]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 208], rax
    mov rax, [rbp - 208]
    push rax
    mov rax, [rbp - 184]
    mov rcx, rax
    pop rax
    mov dword [rax], ecx

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 10
    mov r10, rax
    pop rax
    cqo
    idiv r10
    mov [rbp - 224], rax
    mov rax, [rbp - 224]

    mov qword [rbp - 8], rax

    movsxd rax, dword [rbp - 92]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 232], rax
    mov rax, [rbp - 232]

    mov dword [rbp - 92], eax
    jmp ir_while_25
ir_while_end_26:

    movsxd rax, dword [rbp - 92]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 240], rax
    mov rax, [rbp - 240]

    mov dword [rbp - 96], eax
ir_while_29:

    movsxd rax, dword [rbp - 96]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setge al
    movzx rax, al
    mov [rbp - 248], rax
    mov rax, [rbp - 248]
    test rax, rax
    jz ir_while_end_30

    movsxd rax, dword [rbp - 96]
    push rax
    mov rax, 20
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 256], rax
    mov rax, [rbp - 256]
    test rax, rax
    jz ir_trap_bounds_31
    jmp ir_in_bounds_32
ir_trap_bounds_31:

    sub rsp, 32

    lea rax, [rel Lstr_struct79]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_32:

    movsxd rax, dword [rbp - 96]
    push rax
    mov rax, 4
    mov r10, rax
    pop rax
    imul rax, r10
    mov [rbp - 264], rax

    lea rax, [rbp - 88]
    push rax
    mov rax, [rbp - 264]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 272], rax
    mov rax, [rbp - 272]
    mov eax, dword [rax]
    mov [rbp - 280], rax

    sub rsp, 32
    mov rax, [rbp - 280]
    mov rcx, rax
    call putchar
    add rsp, 32


    mov [rbp - 288], rax

    movsxd rax, dword [rbp - 96]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 296], rax
    mov rax, [rbp - 296]

    mov dword [rbp - 96], eax
    jmp ir_while_29
ir_while_end_30:
    mov rax, 0
    mov [rbp - 304], rax
ir_errdefer_ok_33:
ir_errdefer_end_34:
    jmp Lprint_int_exit
Lprint_int_exit:

    mov rsp, rbp
    pop rbp
    ret

global println_int

println_int:
    push rbp
    mov rbp, rsp
    sub rsp, 80

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_35:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    call print_int
    add rsp, 32

    mov [rbp - 16], rax

    sub rsp, 32
    mov rax, 10
    mov rcx, rax
    call putchar
    add rsp, 32


    mov [rbp - 24], rax
    mov rax, 0
    mov [rbp - 32], rax
ir_errdefer_ok_36:
ir_errdefer_end_37:
    jmp Lprintln_int_exit
Lprintln_int_exit:

    mov rsp, rbp
    pop rbp
    ret
    extern fopen
    extern fclose
    extern fread
    extern fwrite
    extern fputs
    extern fgets
    extern fflush
    extern __acrt_iob_func

global get_stdin

get_stdin:
    push rbp
    mov rbp, rsp
    sub rsp, 48


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_38:

    sub rsp, 32
    mov rax, 0
    mov rcx, rax
    call __acrt_iob_func
    add rsp, 32

    mov [rbp - 8], rax
    mov rax, [rbp - 8]
    mov [rbp - 16], rax
    mov rax, [rbp - 8]
    test rax, rax
    jz ir_errdefer_ok_39
    jmp ir_errdefer_end_40
ir_errdefer_ok_39:
ir_errdefer_end_40:
    mov rax, [rbp - 8]
    jmp Lget_stdin_exit
Lget_stdin_exit:

    mov rsp, rbp
    pop rbp
    ret

global get_stdout

get_stdout:
    push rbp
    mov rbp, rsp
    sub rsp, 48


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_41:

    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call __acrt_iob_func
    add rsp, 32

    mov [rbp - 8], rax
    mov rax, [rbp - 8]
    mov [rbp - 16], rax
    mov rax, [rbp - 8]
    test rax, rax
    jz ir_errdefer_ok_42
    jmp ir_errdefer_end_43
ir_errdefer_ok_42:
ir_errdefer_end_43:
    mov rax, [rbp - 8]
    jmp Lget_stdout_exit
Lget_stdout_exit:

    mov rsp, rbp
    pop rbp
    ret

global get_stderr

get_stderr:
    push rbp
    mov rbp, rsp
    sub rsp, 48


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_44:

    sub rsp, 32
    mov rax, 2
    mov rcx, rax
    call __acrt_iob_func
    add rsp, 32

    mov [rbp - 8], rax
    mov rax, [rbp - 8]
    mov [rbp - 16], rax
    mov rax, [rbp - 8]
    test rax, rax
    jz ir_errdefer_ok_45
    jmp ir_errdefer_end_46
ir_errdefer_ok_45:
ir_errdefer_end_46:
    mov rax, [rbp - 8]
    jmp Lget_stderr_exit
Lget_stderr_exit:

    mov rsp, rbp
    pop rbp
    ret

global print_err

print_err:
    push rbp
    mov rbp, rsp
    sub rsp, 80

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_47:

    sub rsp, 32
    call get_stderr
    add rsp, 32

    mov [rbp - 16], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 16]
    mov rdx, rax
    call fputs
    add rsp, 32


    mov [rbp - 24], rax
    mov rax, 0
    mov [rbp - 32], rax
ir_errdefer_ok_48:
ir_errdefer_end_49:
    jmp Lprint_err_exit
Lprint_err_exit:

    mov rsp, rbp
    pop rbp
    ret

global println_err

println_err:
    push rbp
    mov rbp, rsp
    sub rsp, 160

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_50:

    sub rsp, 32
    call get_stderr
    add rsp, 32

    mov [rbp - 16], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 16]
    mov rdx, rax
    call fputs
    add rsp, 32


    mov [rbp - 24], rax

    sub rsp, 32

    lea rax, [rel Lstr_struct81]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 32], rax

    sub rsp, 32
    call get_stderr
    add rsp, 32

    mov [rbp - 40], rax

    sub rsp, 32
    mov rax, [rbp - 32]
    mov rcx, rax
    mov rax, [rbp - 40]
    mov rdx, rax
    call fputs
    add rsp, 32


    mov [rbp - 48], rax
    mov rax, 0
    mov [rbp - 56], rax
ir_errdefer_ok_51:
ir_errdefer_end_52:
    jmp Lprintln_err_exit
Lprintln_err_exit:

    mov rsp, rbp
    pop rbp
    ret
    extern malloc
    extern calloc
    extern realloc
    extern free
    extern memset
    extern memcpy
    extern memmove
    extern memcmp

global alloc_zeroed

alloc_zeroed:
    push rbp
    mov rbp, rsp
    sub rsp, 64

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_53:

    sub rsp, 32
    mov rax, 1
    mov rcx, rax

    mov rax, qword [rbp - 8]
    mov rdx, rax
    call calloc
    add rsp, 32

    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    mov [rbp - 24], rax
    mov rax, [rbp - 16]
    test rax, rax
    jz ir_errdefer_ok_54
    jmp ir_errdefer_end_55
ir_errdefer_ok_54:
ir_errdefer_end_55:
    mov rax, [rbp - 16]
    jmp Lalloc_zeroed_exit
Lalloc_zeroed_exit:

    mov rsp, rbp
    pop rbp
    ret

global buf_dup

buf_dup:
    push rbp
    mov rbp, rsp
    sub rsp, 160

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_56:

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax
    call malloc
    add rsp, 32

    mov [rbp - 32], rax
    mov rax, [rbp - 32]

    mov qword [rbp - 24], rax

    mov rax, qword [rbp - 24]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 40], rax
    mov rax, [rbp - 40]
    test rax, rax
    jz ir_if_next_58
    mov rax, 0
    mov [rbp - 48], rax
ir_errdefer_ok_59:
ir_errdefer_end_60:
    mov rax, 0
    jmp Lbuf_dup_exit
ir_if_next_58:
ir_if_end_57:

    sub rsp, 32

    mov rax, qword [rbp - 24]
    mov rcx, rax

    mov rax, qword [rbp - 8]
    mov rdx, rax

    mov rax, qword [rbp - 16]
    mov r8, rax
    call memcpy
    add rsp, 32

    mov [rbp - 56], rax

    mov rax, qword [rbp - 24]
    mov [rbp - 64], rax
    mov rax, [rbp - 64]
    test rax, rax
    jz ir_errdefer_ok_61
    jmp ir_errdefer_end_62
ir_errdefer_ok_61:
ir_errdefer_end_62:

    mov rax, qword [rbp - 24]
    jmp Lbuf_dup_exit
Lbuf_dup_exit:

    mov rsp, rbp
    pop rbp
    ret

global INFINITE

INFINITE:
    push rbp
    mov rbp, rsp
    sub rsp, 32


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_63:
    mov rax, 0
    mov [rbp - 8], rax
ir_errdefer_ok_64:
ir_errdefer_end_65:
    mov rax, 0
    jmp LINFINITE_exit
LINFINITE_exit:

    mov rsp, rbp
    pop rbp
    ret

global WAIT_OBJECT_0

WAIT_OBJECT_0:
    push rbp
    mov rbp, rsp
    sub rsp, 32


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_66:
    mov rax, 0
    mov [rbp - 8], rax
ir_errdefer_ok_67:
ir_errdefer_end_68:
    mov rax, 0
    jmp LWAIT_OBJECT_0_exit
LWAIT_OBJECT_0_exit:

    mov rsp, rbp
    pop rbp
    ret

global WAIT_TIMEOUT

WAIT_TIMEOUT:
    push rbp
    mov rbp, rsp
    sub rsp, 32


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_69:
    mov rax, 258
    mov [rbp - 8], rax
    jmp ir_errdefer_end_71
ir_errdefer_ok_70:
ir_errdefer_end_71:
    mov rax, 258
    jmp LWAIT_TIMEOUT_exit
LWAIT_TIMEOUT_exit:

    mov rsp, rbp
    pop rbp
    ret

global WAIT_FAILED

WAIT_FAILED:
    push rbp
    mov rbp, rsp
    sub rsp, 32


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_72:
    mov rax, 0
    mov [rbp - 8], rax
ir_errdefer_ok_73:
ir_errdefer_end_74:
    mov rax, 0
    jmp LWAIT_FAILED_exit
LWAIT_FAILED_exit:

    mov rsp, rbp
    pop rbp
    ret
    extern CreateThread
    extern CloseHandle
    extern WaitForSingleObject
    extern GetCurrentThreadId
    extern Sleep
    extern CreateMutexA
    extern ReleaseMutex
    extern gc_thread_attach
    extern gc_thread_detach
    extern _InterlockedCompareExchange
    extern _InterlockedExchange
    extern _InterlockedIncrement
    extern _InterlockedDecrement

global thread_close

thread_close:
    push rbp
    mov rbp, rsp
    sub rsp, 64

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_75:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    call CloseHandle
    add rsp, 32


    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    mov [rbp - 24], rax
    mov rax, [rbp - 16]
    test rax, rax
    jz ir_errdefer_ok_76
    jmp ir_errdefer_end_77
ir_errdefer_ok_76:
ir_errdefer_end_77:
    mov rax, [rbp - 16]
    jmp Lthread_close_exit
Lthread_close_exit:

    mov rsp, rbp
    pop rbp
    ret

global thread_join

thread_join:
    push rbp
    mov rbp, rsp
    sub rsp, 64

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_78:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax

    movsxd rax, dword [rbp - 16]
    mov rdx, rax
    call WaitForSingleObject
    add rsp, 32


    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    mov [rbp - 32], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_errdefer_ok_79
    jmp ir_errdefer_end_80
ir_errdefer_ok_79:
ir_errdefer_end_80:
    mov rax, [rbp - 24]
    jmp Lthread_join_exit
Lthread_join_exit:

    mov rsp, rbp
    pop rbp
    ret

global thread_join_infinite

thread_join_infinite:
    push rbp
    mov rbp, rsp
    sub rsp, 80

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_81:

    sub rsp, 32
    call INFINITE
    add rsp, 32


    mov [rbp - 16], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 16]
    mov rdx, rax
    call WaitForSingleObject
    add rsp, 32


    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    mov [rbp - 32], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_errdefer_ok_82
    jmp ir_errdefer_end_83
ir_errdefer_ok_82:
ir_errdefer_end_83:
    mov rax, [rbp - 24]
    jmp Lthread_join_infinite_exit
Lthread_join_infinite_exit:

    mov rsp, rbp
    pop rbp
    ret

global thread_detach

thread_detach:
    push rbp
    mov rbp, rsp
    sub rsp, 64

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_84:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    call CloseHandle
    add rsp, 32


    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    mov [rbp - 24], rax
    mov rax, [rbp - 16]
    test rax, rax
    jz ir_errdefer_ok_85
    jmp ir_errdefer_end_86
ir_errdefer_ok_85:
ir_errdefer_end_86:
    mov rax, [rbp - 16]
    jmp Lthread_detach_exit
Lthread_detach_exit:

    mov rsp, rbp
    pop rbp
    ret

global thread_sleep_ms

thread_sleep_ms:
    push rbp
    mov rbp, rsp
    sub rsp, 64

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_87:

    sub rsp, 32

    movsxd rax, dword [rbp - 8]
    mov rcx, rax
    call Sleep
    add rsp, 32

    mov [rbp - 16], rax
    mov rax, 0
    mov [rbp - 24], rax
ir_errdefer_ok_88:
ir_errdefer_end_89:
    jmp Lthread_sleep_ms_exit
Lthread_sleep_ms_exit:

    mov rsp, rbp
    pop rbp
    ret

global thread_gc_attach

thread_gc_attach:
    push rbp
    mov rbp, rsp
    sub rsp, 48


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_90:

    sub rsp, 32
    call gc_thread_attach
    add rsp, 32


    mov [rbp - 8], rax
    mov rax, 0
    mov [rbp - 16], rax
ir_errdefer_ok_91:
ir_errdefer_end_92:
    jmp Lthread_gc_attach_exit
Lthread_gc_attach_exit:

    mov rsp, rbp
    pop rbp
    ret

global thread_gc_detach

thread_gc_detach:
    push rbp
    mov rbp, rsp
    sub rsp, 48


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_93:

    sub rsp, 32
    call gc_thread_detach
    add rsp, 32


    mov [rbp - 8], rax
    mov rax, 0
    mov [rbp - 16], rax
ir_errdefer_ok_94:
ir_errdefer_end_95:
    jmp Lthread_gc_detach_exit
Lthread_gc_detach_exit:

    mov rsp, rbp
    pop rbp
    ret

global mutex_create

mutex_create:
    push rbp
    mov rbp, rsp
    sub rsp, 48


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_96:

    sub rsp, 32
    mov rax, 0
    mov rcx, rax
    mov rax, 0
    mov rdx, rax
    mov rax, 0
    mov r8, rax
    call CreateMutexA
    add rsp, 32

    mov [rbp - 8], rax
    mov rax, [rbp - 8]
    mov [rbp - 16], rax
    mov rax, [rbp - 8]
    test rax, rax
    jz ir_errdefer_ok_97
    jmp ir_errdefer_end_98
ir_errdefer_ok_97:
ir_errdefer_end_98:
    mov rax, [rbp - 8]
    jmp Lmutex_create_exit
Lmutex_create_exit:

    mov rsp, rbp
    pop rbp
    ret

global mutex_create_owned

mutex_create_owned:
    push rbp
    mov rbp, rsp
    sub rsp, 48


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_99:

    sub rsp, 32
    mov rax, 0
    mov rcx, rax
    mov rax, 1
    mov rdx, rax
    mov rax, 0
    mov r8, rax
    call CreateMutexA
    add rsp, 32

    mov [rbp - 8], rax
    mov rax, [rbp - 8]
    mov [rbp - 16], rax
    mov rax, [rbp - 8]
    test rax, rax
    jz ir_errdefer_ok_100
    jmp ir_errdefer_end_101
ir_errdefer_ok_100:
ir_errdefer_end_101:
    mov rax, [rbp - 8]
    jmp Lmutex_create_owned_exit
Lmutex_create_owned_exit:

    mov rsp, rbp
    pop rbp
    ret

global mutex_lock

mutex_lock:
    push rbp
    mov rbp, rsp
    sub rsp, 160

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_102:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax

    movsxd rax, dword [rbp - 16]
    mov rdx, rax
    call WaitForSingleObject
    add rsp, 32


    mov [rbp - 32], rax
    mov rax, [rbp - 32]

    mov dword [rbp - 20], eax

    sub rsp, 32
    call WAIT_OBJECT_0
    add rsp, 32


    mov [rbp - 40], rax

    movsxd rax, dword [rbp - 20]
    push rax
    mov rax, [rbp - 40]
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    test rax, rax
    jz ir_if_next_104
    mov rax, 1
    mov [rbp - 56], rax
    jmp ir_errdefer_end_106
ir_errdefer_ok_105:
ir_errdefer_end_106:
    mov rax, 1
    jmp Lmutex_lock_exit
ir_if_next_104:
ir_if_end_103:
    mov rax, 0
    mov [rbp - 64], rax
ir_errdefer_ok_107:
ir_errdefer_end_108:
    mov rax, 0
    jmp Lmutex_lock_exit
Lmutex_lock_exit:

    mov rsp, rbp
    pop rbp
    ret

global mutex_lock_infinite

mutex_lock_infinite:
    push rbp
    mov rbp, rsp
    sub rsp, 80

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_109:

    sub rsp, 32
    call INFINITE
    add rsp, 32


    mov [rbp - 16], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 16]
    mov rdx, rax
    call mutex_lock
    add rsp, 32


    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    mov [rbp - 32], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_errdefer_ok_110
    jmp ir_errdefer_end_111
ir_errdefer_ok_110:
ir_errdefer_end_111:
    mov rax, [rbp - 24]
    jmp Lmutex_lock_infinite_exit
Lmutex_lock_infinite_exit:

    mov rsp, rbp
    pop rbp
    ret

global mutex_unlock

mutex_unlock:
    push rbp
    mov rbp, rsp
    sub rsp, 64

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_112:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    call ReleaseMutex
    add rsp, 32


    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    mov [rbp - 24], rax
    mov rax, [rbp - 16]
    test rax, rax
    jz ir_errdefer_ok_113
    jmp ir_errdefer_end_114
ir_errdefer_ok_113:
ir_errdefer_end_114:
    mov rax, [rbp - 16]
    jmp Lmutex_unlock_exit
Lmutex_unlock_exit:

    mov rsp, rbp
    pop rbp
    ret

global mutex_close

mutex_close:
    push rbp
    mov rbp, rsp
    sub rsp, 64

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_115:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    call CloseHandle
    add rsp, 32


    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    mov [rbp - 24], rax
    mov rax, [rbp - 16]
    test rax, rax
    jz ir_errdefer_ok_116
    jmp ir_errdefer_end_117
ir_errdefer_ok_116:
ir_errdefer_end_117:
    mov rax, [rbp - 16]
    jmp Lmutex_close_exit
Lmutex_close_exit:

    mov rsp, rbp
    pop rbp
    ret

global atomic_compare_exchange_i32

atomic_compare_exchange_i32:
    push rbp
    mov rbp, rsp
    sub rsp, 80

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx

    mov [rbp - 24], r8


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_118:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax

    movsxd rax, dword [rbp - 16]
    mov rdx, rax

    movsxd rax, dword [rbp - 24]
    mov r8, rax
    call _InterlockedCompareExchange
    add rsp, 32


    mov [rbp - 32], rax
    mov rax, [rbp - 32]
    mov [rbp - 40], rax
    mov rax, [rbp - 32]
    test rax, rax
    jz ir_errdefer_ok_119
    jmp ir_errdefer_end_120
ir_errdefer_ok_119:
ir_errdefer_end_120:
    mov rax, [rbp - 32]
    jmp Latomic_compare_exchange_i32_exit
Latomic_compare_exchange_i32_exit:

    mov rsp, rbp
    pop rbp
    ret

global atomic_exchange_i32

atomic_exchange_i32:
    push rbp
    mov rbp, rsp
    sub rsp, 64

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_121:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax

    movsxd rax, dword [rbp - 16]
    mov rdx, rax
    call _InterlockedExchange
    add rsp, 32


    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    mov [rbp - 32], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_errdefer_ok_122
    jmp ir_errdefer_end_123
ir_errdefer_ok_122:
ir_errdefer_end_123:
    mov rax, [rbp - 24]
    jmp Latomic_exchange_i32_exit
Latomic_exchange_i32_exit:

    mov rsp, rbp
    pop rbp
    ret

global atomic_inc_i32

atomic_inc_i32:
    push rbp
    mov rbp, rsp
    sub rsp, 64

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_124:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    call _InterlockedIncrement
    add rsp, 32


    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    mov [rbp - 24], rax
    mov rax, [rbp - 16]
    test rax, rax
    jz ir_errdefer_ok_125
    jmp ir_errdefer_end_126
ir_errdefer_ok_125:
ir_errdefer_end_126:
    mov rax, [rbp - 16]
    jmp Latomic_inc_i32_exit
Latomic_inc_i32_exit:

    mov rsp, rbp
    pop rbp
    ret

global atomic_dec_i32

atomic_dec_i32:
    push rbp
    mov rbp, rsp
    sub rsp, 64

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_127:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    call _InterlockedDecrement
    add rsp, 32


    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    mov [rbp - 24], rax
    mov rax, [rbp - 16]
    test rax, rax
    jz ir_errdefer_ok_128
    jmp ir_errdefer_end_129
ir_errdefer_ok_128:
ir_errdefer_end_129:
    mov rax, [rbp - 16]
    jmp Latomic_dec_i32_exit
Latomic_dec_i32_exit:

    mov rsp, rbp
    pop rbp
    ret

global spin_try_lock

spin_try_lock:
    push rbp
    mov rbp, rsp
    sub rsp, 112

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_130:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, 1
    mov rdx, rax
    mov rax, 0
    mov r8, rax
    call _InterlockedCompareExchange
    add rsp, 32


    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_if_next_132
    mov rax, 1
    mov [rbp - 32], rax
    jmp ir_errdefer_end_134
ir_errdefer_ok_133:
ir_errdefer_end_134:
    mov rax, 1
    jmp Lspin_try_lock_exit
ir_if_next_132:
ir_if_end_131:
    mov rax, 0
    mov [rbp - 40], rax
ir_errdefer_ok_135:
ir_errdefer_end_136:
    mov rax, 0
    jmp Lspin_try_lock_exit
Lspin_try_lock_exit:

    mov rsp, rbp
    pop rbp
    ret

global spin_lock

spin_lock:
    push rbp
    mov rbp, rsp
    sub rsp, 112

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_137:
ir_while_138:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    call spin_try_lock
    add rsp, 32


    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_while_end_139

    sub rsp, 32
    mov rax, 0
    mov rcx, rax
    call Sleep
    add rsp, 32

    mov [rbp - 32], rax
    jmp ir_while_138
ir_while_end_139:
    mov rax, 0
    mov [rbp - 40], rax
ir_errdefer_ok_140:
ir_errdefer_end_141:
    jmp Lspin_lock_exit
Lspin_lock_exit:

    mov rsp, rbp
    pop rbp
    ret

global spin_unlock

spin_unlock:
    push rbp
    mov rbp, rsp
    sub rsp, 64

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_142:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, 0
    mov rdx, rax
    call _InterlockedExchange
    add rsp, 32


    mov [rbp - 16], rax
    mov rax, 0
    mov [rbp - 24], rax
ir_errdefer_ok_143:
ir_errdefer_end_144:
    jmp Lspin_unlock_exit
Lspin_unlock_exit:

    mov rsp, rbp
    pop rbp
    ret

global AF_INET

AF_INET:
    push rbp
    mov rbp, rsp
    sub rsp, 32


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_145:
    mov rax, 2
    mov [rbp - 8], rax
    jmp ir_errdefer_end_147
ir_errdefer_ok_146:
ir_errdefer_end_147:
    mov rax, 2
    jmp LAF_INET_exit
LAF_INET_exit:

    mov rsp, rbp
    pop rbp
    ret

global SOCK_STREAM

SOCK_STREAM:
    push rbp
    mov rbp, rsp
    sub rsp, 32


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_148:
    mov rax, 1
    mov [rbp - 8], rax
    jmp ir_errdefer_end_150
ir_errdefer_ok_149:
ir_errdefer_end_150:
    mov rax, 1
    jmp LSOCK_STREAM_exit
LSOCK_STREAM_exit:

    mov rsp, rbp
    pop rbp
    ret

global SOCK_DGRAM

SOCK_DGRAM:
    push rbp
    mov rbp, rsp
    sub rsp, 32


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_151:
    mov rax, 2
    mov [rbp - 8], rax
    jmp ir_errdefer_end_153
ir_errdefer_ok_152:
ir_errdefer_end_153:
    mov rax, 2
    jmp LSOCK_DGRAM_exit
LSOCK_DGRAM_exit:

    mov rsp, rbp
    pop rbp
    ret

global IPPROTO_TCP

IPPROTO_TCP:
    push rbp
    mov rbp, rsp
    sub rsp, 32


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_154:
    mov rax, 6
    mov [rbp - 8], rax
    jmp ir_errdefer_end_156
ir_errdefer_ok_155:
ir_errdefer_end_156:
    mov rax, 6
    jmp LIPPROTO_TCP_exit
LIPPROTO_TCP_exit:

    mov rsp, rbp
    pop rbp
    ret

global IPPROTO_UDP

IPPROTO_UDP:
    push rbp
    mov rbp, rsp
    sub rsp, 32


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_157:
    mov rax, 17
    mov [rbp - 8], rax
    jmp ir_errdefer_end_159
ir_errdefer_ok_158:
ir_errdefer_end_159:
    mov rax, 17
    jmp LIPPROTO_UDP_exit
LIPPROTO_UDP_exit:

    mov rsp, rbp
    pop rbp
    ret

global SOL_SOCKET

SOL_SOCKET:
    push rbp
    mov rbp, rsp
    sub rsp, 32


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_160:
    mov rax, 0
    mov [rbp - 8], rax
ir_errdefer_ok_161:
ir_errdefer_end_162:
    mov rax, 0
    jmp LSOL_SOCKET_exit
LSOL_SOCKET_exit:

    mov rsp, rbp
    pop rbp
    ret

global SO_REUSEADDR

SO_REUSEADDR:
    push rbp
    mov rbp, rsp
    sub rsp, 32


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_163:
    mov rax, 4
    mov [rbp - 8], rax
    jmp ir_errdefer_end_165
ir_errdefer_ok_164:
ir_errdefer_end_165:
    mov rax, 4
    jmp LSO_REUSEADDR_exit
LSO_REUSEADDR_exit:

    mov rsp, rbp
    pop rbp
    ret

global SD_RECEIVE

SD_RECEIVE:
    push rbp
    mov rbp, rsp
    sub rsp, 32


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_166:
    mov rax, 0
    mov [rbp - 8], rax
ir_errdefer_ok_167:
ir_errdefer_end_168:
    mov rax, 0
    jmp LSD_RECEIVE_exit
LSD_RECEIVE_exit:

    mov rsp, rbp
    pop rbp
    ret

global SD_SEND

SD_SEND:
    push rbp
    mov rbp, rsp
    sub rsp, 32


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_169:
    mov rax, 1
    mov [rbp - 8], rax
    jmp ir_errdefer_end_171
ir_errdefer_ok_170:
ir_errdefer_end_171:
    mov rax, 1
    jmp LSD_SEND_exit
LSD_SEND_exit:

    mov rsp, rbp
    pop rbp
    ret

global SD_BOTH

SD_BOTH:
    push rbp
    mov rbp, rsp
    sub rsp, 32


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_172:
    mov rax, 2
    mov [rbp - 8], rax
    jmp ir_errdefer_end_174
ir_errdefer_ok_173:
ir_errdefer_end_174:
    mov rax, 2
    jmp LSD_BOTH_exit
LSD_BOTH_exit:

    mov rsp, rbp
    pop rbp
    ret

global INADDR_ANY

INADDR_ANY:
    push rbp
    mov rbp, rsp
    sub rsp, 32


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_175:
    mov rax, 0
    mov [rbp - 8], rax
ir_errdefer_ok_176:
ir_errdefer_end_177:
    mov rax, 0
    jmp LINADDR_ANY_exit
LINADDR_ANY_exit:

    mov rsp, rbp
    pop rbp
    ret

global INVALID_SOCKET

INVALID_SOCKET:
    push rbp
    mov rbp, rsp
    sub rsp, 48


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_178:
    mov rax, -1
    mov [rbp - 8], rax
    mov rax, -1
    mov [rbp - 16], rax
    jmp ir_errdefer_end_180
ir_errdefer_ok_179:
ir_errdefer_end_180:
    mov rax, [rbp - 8]
    jmp LINVALID_SOCKET_exit
LINVALID_SOCKET_exit:

    mov rsp, rbp
    pop rbp
    ret

global SOCKET_ERROR

SOCKET_ERROR:
    push rbp
    mov rbp, rsp
    sub rsp, 48


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_181:
    mov rax, -1
    mov [rbp - 8], rax
    mov rax, -1
    mov [rbp - 16], rax
    jmp ir_errdefer_end_183
ir_errdefer_ok_182:
ir_errdefer_end_183:
    mov rax, [rbp - 8]
    jmp LSOCKET_ERROR_exit
LSOCKET_ERROR_exit:

    mov rsp, rbp
    pop rbp
    ret
    extern WSAStartup
    extern WSACleanup
    extern WSAGetLastError
    extern socket
    extern closesocket
    extern shutdown
    extern connect
    extern bind
    extern listen
    extern accept
    extern setsockopt
    extern send
    extern recv
    extern htons
    extern htonl
    extern ntohs
    extern ntohl
    extern inet_addr

global net_init

net_init:
    push rbp
    mov rbp, rsp
    sub rsp, 480


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_184:
    lea rax, [rel net_ref_lock]
    mov [rbp - 24], rax

    sub rsp, 32
    mov rax, [rbp - 24]
    mov rcx, rax
    call spin_lock
    add rsp, 32

    mov [rbp - 32], rax

    movsxd rax, dword [rel net_ref_count]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setg al
    movzx rax, al
    mov [rbp - 40], rax
    mov rax, [rbp - 40]
    test rax, rax
    jz ir_if_next_186

    movsxd rax, dword [rel net_ref_count]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 48], rax
    mov rax, [rbp - 48]

    mov dword [rel net_ref_count], eax
    lea rax, [rel net_ref_lock]
    mov [rbp - 56], rax

    sub rsp, 32
    mov rax, [rbp - 56]
    mov rcx, rax
    call spin_unlock
    add rsp, 32

    mov [rbp - 64], rax
    mov rax, 0
    mov [rbp - 72], rax
ir_errdefer_ok_187:
ir_errdefer_end_188:
    mov rax, 0
    jmp Lnet_init_exit
ir_if_next_186:
ir_if_end_185:

    sub rsp, 32
    mov rax, 408
    mov rcx, rax
    call malloc
    add rsp, 32

    mov [rbp - 80], rax
    mov rax, [rbp - 80]

    mov qword [rbp - 8], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 88], rax
    mov rax, [rbp - 88]
    test rax, rax
    jz ir_if_next_190
    lea rax, [rel net_ref_lock]
    mov [rbp - 96], rax

    sub rsp, 32
    mov rax, [rbp - 96]
    mov rcx, rax
    call spin_unlock
    add rsp, 32

    mov [rbp - 104], rax
    mov rax, -1
    mov [rbp - 112], rax
    mov rax, -1
    mov [rbp - 120], rax
    jmp ir_errdefer_end_192
ir_errdefer_ok_191:
ir_errdefer_end_192:
    mov rax, [rbp - 112]
    jmp Lnet_init_exit
ir_if_next_190:
ir_if_end_189:

    sub rsp, 32
    mov rax, 514
    mov rcx, rax

    mov rax, qword [rbp - 8]
    mov rdx, rax
    call WSAStartup
    add rsp, 32


    mov [rbp - 128], rax
    mov rax, [rbp - 128]

    mov dword [rbp - 12], eax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    call free
    add rsp, 32

    mov [rbp - 136], rax

    movsxd rax, dword [rbp - 12]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 144], rax
    mov rax, [rbp - 144]
    test rax, rax
    jz ir_if_next_194
    mov rax, 1

    mov dword [rel net_ref_count], eax
    jmp ir_if_end_193
ir_if_next_194:
ir_if_end_193:
    lea rax, [rel net_ref_lock]
    mov [rbp - 152], rax

    sub rsp, 32
    mov rax, [rbp - 152]
    mov rcx, rax
    call spin_unlock
    add rsp, 32

    mov [rbp - 160], rax

    movsxd rax, dword [rbp - 12]
    mov [rbp - 168], rax
    mov rax, [rbp - 168]
    test rax, rax
    jz ir_errdefer_ok_195
    jmp ir_errdefer_end_196
ir_errdefer_ok_195:
ir_errdefer_end_196:

    movsxd rax, dword [rbp - 12]
    jmp Lnet_init_exit
Lnet_init_exit:

    mov rsp, rbp
    pop rbp
    ret

global net_cleanup

net_cleanup:
    push rbp
    mov rbp, rsp
    sub rsp, 368


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_197:
    lea rax, [rel net_ref_lock]
    mov [rbp - 16], rax

    sub rsp, 32
    mov rax, [rbp - 16]
    mov rcx, rax
    call spin_lock
    add rsp, 32

    mov [rbp - 24], rax

    movsxd rax, dword [rel net_ref_count]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setle al
    movzx rax, al
    mov [rbp - 32], rax
    mov rax, [rbp - 32]
    test rax, rax
    jz ir_if_next_199
    lea rax, [rel net_ref_lock]
    mov [rbp - 40], rax

    sub rsp, 32
    mov rax, [rbp - 40]
    mov rcx, rax
    call spin_unlock
    add rsp, 32

    mov [rbp - 48], rax
    mov rax, 0
    mov [rbp - 56], rax
ir_errdefer_ok_200:
ir_errdefer_end_201:
    mov rax, 0
    jmp Lnet_cleanup_exit
ir_if_next_199:
ir_if_end_198:

    movsxd rax, dword [rel net_ref_count]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 64], rax
    mov rax, [rbp - 64]

    mov dword [rel net_ref_count], eax

    movsxd rax, dword [rel net_ref_count]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setg al
    movzx rax, al
    mov [rbp - 72], rax
    mov rax, [rbp - 72]
    test rax, rax
    jz ir_if_next_203
    lea rax, [rel net_ref_lock]
    mov [rbp - 80], rax

    sub rsp, 32
    mov rax, [rbp - 80]
    mov rcx, rax
    call spin_unlock
    add rsp, 32

    mov [rbp - 88], rax
    mov rax, 0
    mov [rbp - 96], rax
ir_errdefer_ok_204:
ir_errdefer_end_205:
    mov rax, 0
    jmp Lnet_cleanup_exit
ir_if_next_203:
ir_if_end_202:

    sub rsp, 32
    call WSACleanup
    add rsp, 32


    mov [rbp - 104], rax
    mov rax, [rbp - 104]

    mov dword [rbp - 4], eax
    lea rax, [rel net_ref_lock]
    mov [rbp - 112], rax

    sub rsp, 32
    mov rax, [rbp - 112]
    mov rcx, rax
    call spin_unlock
    add rsp, 32

    mov [rbp - 120], rax

    movsxd rax, dword [rbp - 4]
    mov [rbp - 128], rax
    mov rax, [rbp - 128]
    test rax, rax
    jz ir_errdefer_ok_206
    jmp ir_errdefer_end_207
ir_errdefer_ok_206:
ir_errdefer_end_207:

    movsxd rax, dword [rbp - 4]
    jmp Lnet_cleanup_exit
Lnet_cleanup_exit:

    mov rsp, rbp
    pop rbp
    ret

global net_last_error

net_last_error:
    push rbp
    mov rbp, rsp
    sub rsp, 48


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_208:

    sub rsp, 32
    call WSAGetLastError
    add rsp, 32


    mov [rbp - 8], rax
    mov rax, [rbp - 8]
    mov [rbp - 16], rax
    mov rax, [rbp - 8]
    test rax, rax
    jz ir_errdefer_ok_209
    jmp ir_errdefer_end_210
ir_errdefer_ok_209:
ir_errdefer_end_210:
    mov rax, [rbp - 8]
    jmp Lnet_last_error_exit
Lnet_last_error_exit:

    mov rsp, rbp
    pop rbp
    ret

global socket_tcp

socket_tcp:
    push rbp
    mov rbp, rsp
    sub rsp, 128


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_211:

    sub rsp, 32
    call AF_INET
    add rsp, 32


    mov [rbp - 8], rax

    sub rsp, 32
    call SOCK_STREAM
    add rsp, 32


    mov [rbp - 16], rax

    sub rsp, 32
    call IPPROTO_TCP
    add rsp, 32


    mov [rbp - 24], rax

    sub rsp, 32
    mov rax, [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 16]
    mov rdx, rax
    mov rax, [rbp - 24]
    mov r8, rax
    call socket
    add rsp, 32

    mov [rbp - 32], rax
    mov rax, [rbp - 32]
    mov [rbp - 40], rax
    mov rax, [rbp - 32]
    test rax, rax
    jz ir_errdefer_ok_212
    jmp ir_errdefer_end_213
ir_errdefer_ok_212:
ir_errdefer_end_213:
    mov rax, [rbp - 32]
    jmp Lsocket_tcp_exit
Lsocket_tcp_exit:

    mov rsp, rbp
    pop rbp
    ret

global socket_udp

socket_udp:
    push rbp
    mov rbp, rsp
    sub rsp, 128


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_214:

    sub rsp, 32
    call AF_INET
    add rsp, 32


    mov [rbp - 8], rax

    sub rsp, 32
    call SOCK_DGRAM
    add rsp, 32


    mov [rbp - 16], rax

    sub rsp, 32
    call IPPROTO_UDP
    add rsp, 32


    mov [rbp - 24], rax

    sub rsp, 32
    mov rax, [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 16]
    mov rdx, rax
    mov rax, [rbp - 24]
    mov r8, rax
    call socket
    add rsp, 32

    mov [rbp - 32], rax
    mov rax, [rbp - 32]
    mov [rbp - 40], rax
    mov rax, [rbp - 32]
    test rax, rax
    jz ir_errdefer_ok_215
    jmp ir_errdefer_end_216
ir_errdefer_ok_215:
ir_errdefer_end_216:
    mov rax, [rbp - 32]
    jmp Lsocket_udp_exit
Lsocket_udp_exit:

    mov rsp, rbp
    pop rbp
    ret

global net_is_initialized

net_is_initialized:
    push rbp
    mov rbp, rsp
    sub rsp, 160


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_217:
    mov rax, 0

    mov dword [rbp - 4], eax
    lea rax, [rel net_ref_lock]
    mov [rbp - 16], rax

    sub rsp, 32
    mov rax, [rbp - 16]
    mov rcx, rax
    call spin_lock
    add rsp, 32

    mov [rbp - 24], rax

    movsxd rax, dword [rel net_ref_count]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setg al
    movzx rax, al
    mov [rbp - 32], rax
    mov rax, [rbp - 32]
    test rax, rax
    jz ir_if_next_219
    mov rax, 1

    mov dword [rbp - 4], eax
    jmp ir_if_end_218
ir_if_next_219:
ir_if_end_218:
    lea rax, [rel net_ref_lock]
    mov [rbp - 40], rax

    sub rsp, 32
    mov rax, [rbp - 40]
    mov rcx, rax
    call spin_unlock
    add rsp, 32

    mov [rbp - 48], rax

    movsxd rax, dword [rbp - 4]
    mov [rbp - 56], rax
    mov rax, [rbp - 56]
    test rax, rax
    jz ir_errdefer_ok_220
    jmp ir_errdefer_end_221
ir_errdefer_ok_220:
ir_errdefer_end_221:

    movsxd rax, dword [rbp - 4]
    jmp Lnet_is_initialized_exit
Lnet_is_initialized_exit:

    mov rsp, rbp
    pop rbp
    ret

global sockaddr_in

sockaddr_in:
    push rbp
    mov rbp, rsp
    sub rsp, 896

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_222:

    sub rsp, 32
    mov rax, 16
    mov rcx, rax
    call malloc
    add rsp, 32

    mov [rbp - 40], rax
    mov rax, [rbp - 40]

    mov qword [rbp - 24], rax

    mov rax, qword [rbp - 24]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    test rax, rax
    jz ir_if_next_224
    mov rax, 0
    mov [rbp - 56], rax
ir_errdefer_ok_225:
ir_errdefer_end_226:
    mov rax, 0
    jmp Lsockaddr_in_exit
ir_if_next_224:
ir_if_end_223:

    sub rsp, 32

    mov rax, qword [rbp - 24]
    mov rcx, rax
    mov rax, 0
    mov rdx, rax
    mov rax, 16
    mov r8, rax
    call memset
    add rsp, 32

    mov [rbp - 64], rax

    mov rax, qword [rbp - 24]
    test rax, rax
    jz ir_trap_null_227
    jmp ir_nonnull_228
ir_trap_null_227:

    sub rsp, 32

    lea rax, [rel Lstr_struct83]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_228:
    mov rax, 0
    mov [rbp - 72], rax

    mov rax, qword [rbp - 24]
    mov [rbp - 80], rax
    mov rax, [rbp - 80]
    push rax
    mov rax, 2
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    mov rax, qword [rbp - 24]
    test rax, rax
    jz ir_trap_null_229
    jmp ir_nonnull_230
ir_trap_null_229:

    sub rsp, 32

    lea rax, [rel Lstr_struct85]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_230:
    mov rax, 1
    mov [rbp - 96], rax

    mov rax, qword [rbp - 24]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 104], rax
    mov rax, [rbp - 104]
    push rax
    mov rax, 0
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    sub rsp, 32

    movsxd rax, dword [rbp - 16]
    mov rcx, rax
    call htons
    add rsp, 32


    mov [rbp - 120], rax
    mov rax, [rbp - 120]

    mov dword [rbp - 28], eax

    mov rax, qword [rbp - 24]
    test rax, rax
    jz ir_trap_null_231
    jmp ir_nonnull_232
ir_trap_null_231:

    sub rsp, 32

    lea rax, [rel Lstr_struct87]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_232:
    mov rax, 2
    mov [rbp - 128], rax

    mov rax, qword [rbp - 24]
    push rax
    mov rax, 2
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 136], rax
    mov rax, [rbp - 136]
    push rax

    movsxd rax, dword [rbp - 28]
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    movsxd rax, dword [rbp - 28]
    push rax
    mov rax, 256
    mov r10, rax
    pop rax
    cqo
    idiv r10
    mov [rbp - 152], rax

    mov rax, qword [rbp - 24]
    test rax, rax
    jz ir_trap_null_233
    jmp ir_nonnull_234
ir_trap_null_233:

    sub rsp, 32

    lea rax, [rel Lstr_struct89]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_234:
    mov rax, 3
    mov [rbp - 160], rax

    mov rax, qword [rbp - 24]
    push rax
    mov rax, 3
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 168], rax
    mov rax, [rbp - 168]
    push rax
    mov rax, [rbp - 152]
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    call inet_addr
    add rsp, 32


    mov [rbp - 184], rax
    mov rax, [rbp - 184]

    mov dword [rbp - 32], eax

    mov rax, qword [rbp - 24]
    test rax, rax
    jz ir_trap_null_235
    jmp ir_nonnull_236
ir_trap_null_235:

    sub rsp, 32

    lea rax, [rel Lstr_struct91]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_236:
    mov rax, 4
    mov [rbp - 192], rax

    mov rax, qword [rbp - 24]
    push rax
    mov rax, 4
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 200], rax
    mov rax, [rbp - 200]
    push rax

    movsxd rax, dword [rbp - 32]
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    movsxd rax, dword [rbp - 32]
    push rax
    mov rax, 256
    mov r10, rax
    pop rax
    cqo
    idiv r10
    mov [rbp - 216], rax

    mov rax, qword [rbp - 24]
    test rax, rax
    jz ir_trap_null_237
    jmp ir_nonnull_238
ir_trap_null_237:

    sub rsp, 32

    lea rax, [rel Lstr_struct93]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_238:
    mov rax, 5
    mov [rbp - 224], rax

    mov rax, qword [rbp - 24]
    push rax
    mov rax, 5
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 232], rax
    mov rax, [rbp - 232]
    push rax
    mov rax, [rbp - 216]
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    movsxd rax, dword [rbp - 32]
    push rax
    mov rax, 65536
    mov r10, rax
    pop rax
    cqo
    idiv r10
    mov [rbp - 248], rax

    mov rax, qword [rbp - 24]
    test rax, rax
    jz ir_trap_null_239
    jmp ir_nonnull_240
ir_trap_null_239:

    sub rsp, 32

    lea rax, [rel Lstr_struct95]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_240:
    mov rax, 6
    mov [rbp - 256], rax

    mov rax, qword [rbp - 24]
    push rax
    mov rax, 6
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 264], rax
    mov rax, [rbp - 264]
    push rax
    mov rax, [rbp - 248]
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    movsxd rax, dword [rbp - 32]
    push rax
    mov rax, 16777216
    mov r10, rax
    pop rax
    cqo
    idiv r10
    mov [rbp - 280], rax

    mov rax, qword [rbp - 24]
    test rax, rax
    jz ir_trap_null_241
    jmp ir_nonnull_242
ir_trap_null_241:

    sub rsp, 32

    lea rax, [rel Lstr_struct97]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_242:
    mov rax, 7
    mov [rbp - 288], rax

    mov rax, qword [rbp - 24]
    push rax
    mov rax, 7
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 296], rax
    mov rax, [rbp - 296]
    push rax
    mov rax, [rbp - 280]
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    mov rax, qword [rbp - 24]
    mov [rbp - 312], rax
    mov rax, [rbp - 312]
    test rax, rax
    jz ir_errdefer_ok_243
    jmp ir_errdefer_end_244
ir_errdefer_ok_243:
ir_errdefer_end_244:

    mov rax, qword [rbp - 24]
    jmp Lsockaddr_in_exit
Lsockaddr_in_exit:

    mov rsp, rbp
    pop rbp
    ret

global sockaddr_in_any

sockaddr_in_any:
    push rbp
    mov rbp, rsp
    sub rsp, 496

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_245:

    sub rsp, 32
    mov rax, 16
    mov rcx, rax
    call malloc
    add rsp, 32

    mov [rbp - 32], rax
    mov rax, [rbp - 32]

    mov qword [rbp - 16], rax

    mov rax, qword [rbp - 16]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 40], rax
    mov rax, [rbp - 40]
    test rax, rax
    jz ir_if_next_247
    mov rax, 0
    mov [rbp - 48], rax
ir_errdefer_ok_248:
ir_errdefer_end_249:
    mov rax, 0
    jmp Lsockaddr_in_any_exit
ir_if_next_247:
ir_if_end_246:

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax
    mov rax, 0
    mov rdx, rax
    mov rax, 16
    mov r8, rax
    call memset
    add rsp, 32

    mov [rbp - 56], rax

    mov rax, qword [rbp - 16]
    test rax, rax
    jz ir_trap_null_250
    jmp ir_nonnull_251
ir_trap_null_250:

    sub rsp, 32

    lea rax, [rel Lstr_struct99]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_251:
    mov rax, 0
    mov [rbp - 64], rax

    mov rax, qword [rbp - 16]
    mov [rbp - 72], rax
    mov rax, [rbp - 72]
    push rax
    mov rax, 2
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    mov rax, qword [rbp - 16]
    test rax, rax
    jz ir_trap_null_252
    jmp ir_nonnull_253
ir_trap_null_252:

    sub rsp, 32

    lea rax, [rel Lstr_struct101]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_253:
    mov rax, 1
    mov [rbp - 88], rax

    mov rax, qword [rbp - 16]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 96], rax
    mov rax, [rbp - 96]
    push rax
    mov rax, 0
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    sub rsp, 32

    movsxd rax, dword [rbp - 8]
    mov rcx, rax
    call htons
    add rsp, 32


    mov [rbp - 112], rax
    mov rax, [rbp - 112]

    mov dword [rbp - 20], eax

    mov rax, qword [rbp - 16]
    test rax, rax
    jz ir_trap_null_254
    jmp ir_nonnull_255
ir_trap_null_254:

    sub rsp, 32

    lea rax, [rel Lstr_struct103]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_255:
    mov rax, 2
    mov [rbp - 120], rax

    mov rax, qword [rbp - 16]
    push rax
    mov rax, 2
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 128], rax
    mov rax, [rbp - 128]
    push rax

    movsxd rax, dword [rbp - 20]
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    movsxd rax, dword [rbp - 20]
    push rax
    mov rax, 256
    mov r10, rax
    pop rax
    cqo
    idiv r10
    mov [rbp - 144], rax

    mov rax, qword [rbp - 16]
    test rax, rax
    jz ir_trap_null_256
    jmp ir_nonnull_257
ir_trap_null_256:

    sub rsp, 32

    lea rax, [rel Lstr_struct105]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_257:
    mov rax, 3
    mov [rbp - 152], rax

    mov rax, qword [rbp - 16]
    push rax
    mov rax, 3
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 160], rax
    mov rax, [rbp - 160]
    push rax
    mov rax, [rbp - 144]
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    mov rax, qword [rbp - 16]
    mov [rbp - 176], rax
    mov rax, [rbp - 176]
    test rax, rax
    jz ir_errdefer_ok_258
    jmp ir_errdefer_end_259
ir_errdefer_ok_258:
ir_errdefer_end_259:

    mov rax, qword [rbp - 16]
    jmp Lsockaddr_in_any_exit
Lsockaddr_in_any_exit:

    mov rsp, rbp
    pop rbp
    ret

global set_reuseaddr

set_reuseaddr:
    push rbp
    mov rbp, rsp
    sub rsp, 688

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_260:

    movsxd rax, dword [rbp - 16]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 32], rax
    mov rax, [rbp - 32]
    test rax, rax
    jz ir_if_next_262
    mov rax, 1
    mov [rbp - 40], rax
    jmp ir_in_bounds_264
ir_trap_bounds_263:

    sub rsp, 32

    lea rax, [rel Lstr_struct107]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_264:
    mov rax, 0
    mov [rbp - 48], rax

    lea rax, [rbp - 20]
    mov [rbp - 56], rax
    mov rax, [rbp - 56]
    push rax
    mov rax, 1
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    jmp ir_if_end_261
ir_if_next_262:
    mov rax, 1
    mov [rbp - 72], rax
    jmp ir_in_bounds_266
ir_trap_bounds_265:

    sub rsp, 32

    lea rax, [rel Lstr_struct109]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_266:
    mov rax, 0
    mov [rbp - 80], rax

    lea rax, [rbp - 20]
    mov [rbp - 88], rax
    mov rax, [rbp - 88]
    push rax
    mov rax, 0
    mov rcx, rax
    pop rax
    mov byte [rax], cl
ir_if_end_261:
    mov rax, 1
    mov [rbp - 104], rax
    jmp ir_in_bounds_268
ir_trap_bounds_267:

    sub rsp, 32

    lea rax, [rel Lstr_struct111]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_268:
    mov rax, 1
    mov [rbp - 112], rax

    lea rax, [rbp - 20]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 120], rax
    mov rax, [rbp - 120]
    push rax
    mov rax, 0
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    mov rax, 1
    mov [rbp - 136], rax
    jmp ir_in_bounds_270
ir_trap_bounds_269:

    sub rsp, 32

    lea rax, [rel Lstr_struct113]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_270:
    mov rax, 2
    mov [rbp - 144], rax

    lea rax, [rbp - 20]
    push rax
    mov rax, 2
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 152], rax
    mov rax, [rbp - 152]
    push rax
    mov rax, 0
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    mov rax, 1
    mov [rbp - 168], rax
    jmp ir_in_bounds_272
ir_trap_bounds_271:

    sub rsp, 32

    lea rax, [rel Lstr_struct115]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_272:
    mov rax, 3
    mov [rbp - 176], rax

    lea rax, [rbp - 20]
    push rax
    mov rax, 3
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 184], rax
    mov rax, [rbp - 184]
    push rax
    mov rax, 0
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    sub rsp, 32
    call SOL_SOCKET
    add rsp, 32


    mov [rbp - 200], rax

    sub rsp, 32
    call SO_REUSEADDR
    add rsp, 32


    mov [rbp - 208], rax
    mov rax, 1
    mov [rbp - 216], rax
    jmp ir_in_bounds_274
ir_trap_bounds_273:

    sub rsp, 32

    lea rax, [rel Lstr_struct117]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_274:
    mov rax, 0
    mov [rbp - 224], rax

    lea rax, [rbp - 20]
    mov [rbp - 232], rax

    sub rsp, 48
    mov rax, 4
    mov [rsp + 32], rax

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 200]
    mov rdx, rax
    mov rax, [rbp - 208]
    mov r8, rax
    mov rax, [rbp - 232]
    mov r9, rax
    call setsockopt
    add rsp, 48


    mov [rbp - 240], rax
    mov rax, [rbp - 240]
    mov [rbp - 248], rax
    mov rax, [rbp - 240]
    test rax, rax
    jz ir_errdefer_ok_275
    jmp ir_errdefer_end_276
ir_errdefer_ok_275:
ir_errdefer_end_276:
    mov rax, [rbp - 240]
    jmp Lset_reuseaddr_exit
Lset_reuseaddr_exit:

    mov rsp, rbp
    pop rbp
    ret

global send_all

send_all:
    push rbp
    mov rbp, rsp
    sub rsp, 304

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx

    mov [rbp - 24], r8


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_277:
    mov rax, 0

    mov dword [rbp - 28], eax
ir_while_278:

    movsxd rax, dword [rbp - 28]
    push rax

    movsxd rax, dword [rbp - 24]
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 40], rax
    mov rax, [rbp - 40]
    test rax, rax
    jz ir_while_end_279

    mov rax, qword [rbp - 16]
    test rax, rax
    jz ir_trap_null_280
    jmp ir_nonnull_281
ir_trap_null_280:

    sub rsp, 32

    lea rax, [rel Lstr_struct119]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_281:

    movsxd rax, dword [rbp - 28]
    mov [rbp - 48], rax

    mov rax, qword [rbp - 16]
    push rax
    mov rax, [rbp - 48]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 56], rax

    movsxd rax, dword [rbp - 24]
    push rax

    movsxd rax, dword [rbp - 28]
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 64], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 56]
    mov rdx, rax
    mov rax, [rbp - 64]
    mov r8, rax
    mov rax, 0
    mov r9, rax
    call send
    add rsp, 32


    mov [rbp - 72], rax
    mov rax, [rbp - 72]

    mov dword [rbp - 32], eax

    movsxd rax, dword [rbp - 32]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setle al
    movzx rax, al
    mov [rbp - 80], rax
    mov rax, [rbp - 80]
    test rax, rax
    jz ir_if_next_283
    mov rax, -1
    mov [rbp - 88], rax
    mov rax, -1
    mov [rbp - 96], rax
    jmp ir_errdefer_end_285
ir_errdefer_ok_284:
ir_errdefer_end_285:
    mov rax, [rbp - 88]
    jmp Lsend_all_exit
ir_if_next_283:
ir_if_end_282:

    movsxd rax, dword [rbp - 28]
    push rax

    movsxd rax, dword [rbp - 32]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 104], rax
    mov rax, [rbp - 104]

    mov dword [rbp - 28], eax
    jmp ir_while_278
ir_while_end_279:

    movsxd rax, dword [rbp - 28]
    mov [rbp - 112], rax
    mov rax, [rbp - 112]
    test rax, rax
    jz ir_errdefer_ok_286
    jmp ir_errdefer_end_287
ir_errdefer_ok_286:
ir_errdefer_end_287:

    movsxd rax, dword [rbp - 28]
    jmp Lsend_all_exit
Lsend_all_exit:

    mov rsp, rbp
    pop rbp
    ret
    extern atoi
    extern atol

global digit_to_char

digit_to_char:
    push rbp
    mov rbp, rsp
    sub rsp, 64

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_288:

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 48
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    mov [rbp - 24], rax
    mov rax, [rbp - 16]
    test rax, rax
    jz ir_errdefer_ok_289
    jmp ir_errdefer_end_290
ir_errdefer_ok_289:
ir_errdefer_end_290:
    mov rax, [rbp - 16]
    jmp Ldigit_to_char_exit
Ldigit_to_char_exit:

    mov rsp, rbp
    pop rbp
    ret

global char_to_digit

char_to_digit:
    push rbp
    mov rbp, rsp
    sub rsp, 64

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_291:

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 48
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    mov [rbp - 24], rax
    mov rax, [rbp - 16]
    test rax, rax
    jz ir_errdefer_ok_292
    jmp ir_errdefer_end_293
ir_errdefer_ok_292:
ir_errdefer_end_293:
    mov rax, [rbp - 16]
    jmp Lchar_to_digit_exit
Lchar_to_digit_exit:

    mov rsp, rbp
    pop rbp
    ret

global is_digit

is_digit:
    push rbp
    mov rbp, rsp
    sub rsp, 112

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_294:

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 48
    mov r10, rax
    pop rax
    cmp rax, r10
    setge al
    movzx rax, al
    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    test rax, rax
    jz ir_if_next_296

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 57
    mov r10, rax
    pop rax
    cmp rax, r10
    setle al
    movzx rax, al
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_if_next_298
    mov rax, 1
    mov [rbp - 32], rax
    jmp ir_errdefer_end_300
ir_errdefer_ok_299:
ir_errdefer_end_300:
    mov rax, 1
    jmp Lis_digit_exit
ir_if_next_298:
ir_if_end_297:
    jmp ir_if_end_295
ir_if_next_296:
ir_if_end_295:
    mov rax, 0
    mov [rbp - 40], rax
ir_errdefer_ok_301:
ir_errdefer_end_302:
    mov rax, 0
    jmp Lis_digit_exit
Lis_digit_exit:

    mov rsp, rbp
    pop rbp
    ret

global is_upper

is_upper:
    push rbp
    mov rbp, rsp
    sub rsp, 112

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_303:

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 65
    mov r10, rax
    pop rax
    cmp rax, r10
    setge al
    movzx rax, al
    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    test rax, rax
    jz ir_if_next_305

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 90
    mov r10, rax
    pop rax
    cmp rax, r10
    setle al
    movzx rax, al
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_if_next_307
    mov rax, 1
    mov [rbp - 32], rax
    jmp ir_errdefer_end_309
ir_errdefer_ok_308:
ir_errdefer_end_309:
    mov rax, 1
    jmp Lis_upper_exit
ir_if_next_307:
ir_if_end_306:
    jmp ir_if_end_304
ir_if_next_305:
ir_if_end_304:
    mov rax, 0
    mov [rbp - 40], rax
ir_errdefer_ok_310:
ir_errdefer_end_311:
    mov rax, 0
    jmp Lis_upper_exit
Lis_upper_exit:

    mov rsp, rbp
    pop rbp
    ret

global is_lower

is_lower:
    push rbp
    mov rbp, rsp
    sub rsp, 112

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_312:

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 97
    mov r10, rax
    pop rax
    cmp rax, r10
    setge al
    movzx rax, al
    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    test rax, rax
    jz ir_if_next_314

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 122
    mov r10, rax
    pop rax
    cmp rax, r10
    setle al
    movzx rax, al
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_if_next_316
    mov rax, 1
    mov [rbp - 32], rax
    jmp ir_errdefer_end_318
ir_errdefer_ok_317:
ir_errdefer_end_318:
    mov rax, 1
    jmp Lis_lower_exit
ir_if_next_316:
ir_if_end_315:
    jmp ir_if_end_313
ir_if_next_314:
ir_if_end_313:
    mov rax, 0
    mov [rbp - 40], rax
ir_errdefer_ok_319:
ir_errdefer_end_320:
    mov rax, 0
    jmp Lis_lower_exit
Lis_lower_exit:

    mov rsp, rbp
    pop rbp
    ret

global is_alpha

is_alpha:
    push rbp
    mov rbp, rsp
    sub rsp, 176

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_321:

    sub rsp, 32

    movsxd rax, dword [rbp - 8]
    mov rcx, rax
    call is_upper
    add rsp, 32


    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_if_next_323
    mov rax, 1
    mov [rbp - 32], rax
    jmp ir_errdefer_end_325
ir_errdefer_ok_324:
ir_errdefer_end_325:
    mov rax, 1
    jmp Lis_alpha_exit
ir_if_next_323:
ir_if_end_322:

    sub rsp, 32

    movsxd rax, dword [rbp - 8]
    mov rcx, rax
    call is_lower
    add rsp, 32


    mov [rbp - 40], rax
    mov rax, [rbp - 40]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    test rax, rax
    jz ir_if_next_327
    mov rax, 1
    mov [rbp - 56], rax
    jmp ir_errdefer_end_329
ir_errdefer_ok_328:
ir_errdefer_end_329:
    mov rax, 1
    jmp Lis_alpha_exit
ir_if_next_327:
ir_if_end_326:
    mov rax, 0
    mov [rbp - 64], rax
ir_errdefer_ok_330:
ir_errdefer_end_331:
    mov rax, 0
    jmp Lis_alpha_exit
Lis_alpha_exit:

    mov rsp, rbp
    pop rbp
    ret

global is_alnum

is_alnum:
    push rbp
    mov rbp, rsp
    sub rsp, 176

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_332:

    sub rsp, 32

    movsxd rax, dword [rbp - 8]
    mov rcx, rax
    call is_alpha
    add rsp, 32


    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_if_next_334
    mov rax, 1
    mov [rbp - 32], rax
    jmp ir_errdefer_end_336
ir_errdefer_ok_335:
ir_errdefer_end_336:
    mov rax, 1
    jmp Lis_alnum_exit
ir_if_next_334:
ir_if_end_333:

    sub rsp, 32

    movsxd rax, dword [rbp - 8]
    mov rcx, rax
    call is_digit
    add rsp, 32


    mov [rbp - 40], rax
    mov rax, [rbp - 40]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    test rax, rax
    jz ir_if_next_338
    mov rax, 1
    mov [rbp - 56], rax
    jmp ir_errdefer_end_340
ir_errdefer_ok_339:
ir_errdefer_end_340:
    mov rax, 1
    jmp Lis_alnum_exit
ir_if_next_338:
ir_if_end_337:
    mov rax, 0
    mov [rbp - 64], rax
ir_errdefer_ok_341:
ir_errdefer_end_342:
    mov rax, 0
    jmp Lis_alnum_exit
Lis_alnum_exit:

    mov rsp, rbp
    pop rbp
    ret

global is_space

is_space:
    push rbp
    mov rbp, rsp
    sub rsp, 320

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_343:

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 32
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    test rax, rax
    jz ir_if_next_345
    mov rax, 1
    mov [rbp - 24], rax
    jmp ir_errdefer_end_347
ir_errdefer_ok_346:
ir_errdefer_end_347:
    mov rax, 1
    jmp Lis_space_exit
ir_if_next_345:
ir_if_end_344:

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 9
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 32], rax
    mov rax, [rbp - 32]
    test rax, rax
    jz ir_if_next_349
    mov rax, 1
    mov [rbp - 40], rax
    jmp ir_errdefer_end_351
ir_errdefer_ok_350:
ir_errdefer_end_351:
    mov rax, 1
    jmp Lis_space_exit
ir_if_next_349:
ir_if_end_348:

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 10
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    test rax, rax
    jz ir_if_next_353
    mov rax, 1
    mov [rbp - 56], rax
    jmp ir_errdefer_end_355
ir_errdefer_ok_354:
ir_errdefer_end_355:
    mov rax, 1
    jmp Lis_space_exit
ir_if_next_353:
ir_if_end_352:

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 13
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 64], rax
    mov rax, [rbp - 64]
    test rax, rax
    jz ir_if_next_357
    mov rax, 1
    mov [rbp - 72], rax
    jmp ir_errdefer_end_359
ir_errdefer_ok_358:
ir_errdefer_end_359:
    mov rax, 1
    jmp Lis_space_exit
ir_if_next_357:
ir_if_end_356:

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 12
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 80], rax
    mov rax, [rbp - 80]
    test rax, rax
    jz ir_if_next_361
    mov rax, 1
    mov [rbp - 88], rax
    jmp ir_errdefer_end_363
ir_errdefer_ok_362:
ir_errdefer_end_363:
    mov rax, 1
    jmp Lis_space_exit
ir_if_next_361:
ir_if_end_360:

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 11
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 96], rax
    mov rax, [rbp - 96]
    test rax, rax
    jz ir_if_next_365
    mov rax, 1
    mov [rbp - 104], rax
    jmp ir_errdefer_end_367
ir_errdefer_ok_366:
ir_errdefer_end_367:
    mov rax, 1
    jmp Lis_space_exit
ir_if_next_365:
ir_if_end_364:
    mov rax, 0
    mov [rbp - 112], rax
ir_errdefer_ok_368:
ir_errdefer_end_369:
    mov rax, 0
    jmp Lis_space_exit
Lis_space_exit:

    mov rsp, rbp
    pop rbp
    ret

global to_lower

to_lower:
    push rbp
    mov rbp, rsp
    sub rsp, 128

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_370:

    sub rsp, 32

    movsxd rax, dword [rbp - 8]
    mov rcx, rax
    call is_upper
    add rsp, 32


    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_if_next_372

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 32
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 32], rax
    mov rax, [rbp - 32]
    mov [rbp - 40], rax
    mov rax, [rbp - 32]
    test rax, rax
    jz ir_errdefer_ok_373
    jmp ir_errdefer_end_374
ir_errdefer_ok_373:
ir_errdefer_end_374:
    mov rax, [rbp - 32]
    jmp Lto_lower_exit
ir_if_next_372:
ir_if_end_371:

    movsxd rax, dword [rbp - 8]
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    test rax, rax
    jz ir_errdefer_ok_375
    jmp ir_errdefer_end_376
ir_errdefer_ok_375:
ir_errdefer_end_376:

    movsxd rax, dword [rbp - 8]
    jmp Lto_lower_exit
Lto_lower_exit:

    mov rsp, rbp
    pop rbp
    ret

global to_upper

to_upper:
    push rbp
    mov rbp, rsp
    sub rsp, 128

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_377:

    sub rsp, 32

    movsxd rax, dword [rbp - 8]
    mov rcx, rax
    call is_lower
    add rsp, 32


    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_if_next_379

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 32
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 32], rax
    mov rax, [rbp - 32]
    mov [rbp - 40], rax
    mov rax, [rbp - 32]
    test rax, rax
    jz ir_errdefer_ok_380
    jmp ir_errdefer_end_381
ir_errdefer_ok_380:
ir_errdefer_end_381:
    mov rax, [rbp - 32]
    jmp Lto_upper_exit
ir_if_next_379:
ir_if_end_378:

    movsxd rax, dword [rbp - 8]
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    test rax, rax
    jz ir_errdefer_ok_382
    jmp ir_errdefer_end_383
ir_errdefer_ok_382:
ir_errdefer_end_383:

    movsxd rax, dword [rbp - 8]
    jmp Lto_upper_exit
Lto_upper_exit:

    mov rsp, rbp
    pop rbp
    ret

global strlen

strlen:
    push rbp
    mov rbp, rsp
    sub rsp, 176

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_384:
    mov rax, 0

    mov qword [rbp - 16], rax
ir_while_385:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_387
    jmp ir_nonnull_388
ir_trap_null_387:

    sub rsp, 32

    lea rax, [rel Lstr_struct121]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_388:

    mov rax, qword [rbp - 16]
    mov [rbp - 24], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 24]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 32], rax
    mov rax, [rbp - 32]
    movzx rax, byte [rax]
    mov [rbp - 40], rax
    mov rax, [rbp - 40]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    test rax, rax
    jz ir_while_end_386

    mov rax, qword [rbp - 16]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 56], rax
    mov rax, [rbp - 56]

    mov qword [rbp - 16], rax
    jmp ir_while_385
ir_while_end_386:

    mov rax, qword [rbp - 16]
    mov [rbp - 64], rax
    mov rax, [rbp - 64]
    test rax, rax
    jz ir_errdefer_ok_389
    jmp ir_errdefer_end_390
ir_errdefer_ok_389:
ir_errdefer_end_390:

    mov rax, qword [rbp - 16]
    jmp Lstrlen_exit
Lstrlen_exit:

    mov rsp, rbp
    pop rbp
    ret

global streq

streq:
    push rbp
    mov rbp, rsp
    sub rsp, 480

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_391:
    mov rax, 0

    mov qword [rbp - 24], rax
ir_while_392:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_394
    jmp ir_nonnull_395
ir_trap_null_394:

    sub rsp, 32

    lea rax, [rel Lstr_struct123]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_395:

    mov rax, qword [rbp - 24]
    mov [rbp - 32], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 32]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 40], rax
    mov rax, [rbp - 40]
    movzx rax, byte [rax]
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 56], rax
    mov rax, [rbp - 56]
    test rax, rax
    jz ir_while_end_393

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_398
    jmp ir_nonnull_399
ir_trap_null_398:

    sub rsp, 32

    lea rax, [rel Lstr_struct125]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_399:

    mov rax, qword [rbp - 24]
    mov [rbp - 64], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 64]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 72], rax
    mov rax, [rbp - 72]
    movzx rax, byte [rax]
    mov [rbp - 80], rax

    mov rax, qword [rbp - 16]
    test rax, rax
    jz ir_trap_null_400
    jmp ir_nonnull_401
ir_trap_null_400:

    sub rsp, 32

    lea rax, [rel Lstr_struct127]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_401:

    mov rax, qword [rbp - 24]
    mov [rbp - 88], rax

    mov rax, qword [rbp - 16]
    push rax
    mov rax, [rbp - 88]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 96], rax
    mov rax, [rbp - 96]
    movzx rax, byte [rax]
    mov [rbp - 104], rax
    mov rax, [rbp - 80]
    push rax
    mov rax, [rbp - 104]
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 112], rax
    mov rax, [rbp - 112]
    test rax, rax
    jz ir_if_next_397
    mov rax, 0
    mov [rbp - 120], rax
ir_errdefer_ok_402:
ir_errdefer_end_403:
    mov rax, 0
    jmp Lstreq_exit
ir_if_next_397:
ir_if_end_396:

    mov rax, qword [rbp - 24]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 128], rax
    mov rax, [rbp - 128]

    mov qword [rbp - 24], rax
    jmp ir_while_392
ir_while_end_393:

    mov rax, qword [rbp - 16]
    test rax, rax
    jz ir_trap_null_406
    jmp ir_nonnull_407
ir_trap_null_406:

    sub rsp, 32

    lea rax, [rel Lstr_struct129]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_407:

    mov rax, qword [rbp - 24]
    mov [rbp - 136], rax

    mov rax, qword [rbp - 16]
    push rax
    mov rax, [rbp - 136]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 144], rax
    mov rax, [rbp - 144]
    movzx rax, byte [rax]
    mov [rbp - 152], rax
    mov rax, [rbp - 152]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 160], rax
    mov rax, [rbp - 160]
    test rax, rax
    jz ir_if_next_405
    mov rax, 0
    mov [rbp - 168], rax
ir_errdefer_ok_408:
ir_errdefer_end_409:
    mov rax, 0
    jmp Lstreq_exit
ir_if_next_405:
ir_if_end_404:
    mov rax, 1
    mov [rbp - 176], rax
    jmp ir_errdefer_end_411
ir_errdefer_ok_410:
ir_errdefer_end_411:
    mov rax, 1
    jmp Lstreq_exit
Lstreq_exit:

    mov rsp, rbp
    pop rbp
    ret

global strncmp

strncmp:
    push rbp
    mov rbp, rsp
    sub rsp, 960

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx

    mov [rbp - 24], r8

    mov [rbp - 32], r9


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_412:

    movsxd rax, dword [rbp - 16]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    test rax, rax
    jz ir_if_next_414
    mov rax, -1
    mov [rbp - 56], rax
    mov rax, -1
    mov [rbp - 64], rax
    jmp ir_errdefer_end_416
ir_errdefer_ok_415:
ir_errdefer_end_416:
    mov rax, [rbp - 56]
    jmp Lstrncmp_exit
ir_if_next_414:
ir_if_end_413:

    movsxd rax, dword [rbp - 32]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 72], rax
    mov rax, [rbp - 72]
    test rax, rax
    jz ir_if_next_418
    mov rax, -1
    mov [rbp - 80], rax
    mov rax, -1
    mov [rbp - 88], rax
    jmp ir_errdefer_end_420
ir_errdefer_ok_419:
ir_errdefer_end_420:
    mov rax, [rbp - 80]
    jmp Lstrncmp_exit
ir_if_next_418:
ir_if_end_417:

    movsxd rax, dword [rbp - 16]
    push rax

    movsxd rax, dword [rbp - 32]
    mov r10, rax
    pop rax
    cmp rax, r10
    setg al
    movzx rax, al
    mov [rbp - 96], rax
    mov rax, [rbp - 96]
    test rax, rax
    jz ir_if_next_422
    mov rax, -1
    mov [rbp - 104], rax
    mov rax, -1
    mov [rbp - 112], rax
    jmp ir_errdefer_end_424
ir_errdefer_ok_423:
ir_errdefer_end_424:
    mov rax, [rbp - 104]
    jmp Lstrncmp_exit
ir_if_next_422:
ir_if_end_421:
    mov rax, 0

    mov dword [rbp - 36], eax
ir_while_425:

    mov rax, qword [rbp - 24]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 120], rax
    mov rax, [rbp - 120]
    mov rax, qword [rax]
    mov [rbp - 128], rax

    movsxd rax, dword [rbp - 36]
    push rax
    mov rax, [rbp - 128]
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 136], rax
    mov rax, [rbp - 136]
    test rax, rax
    jz ir_while_end_426

    movsxd rax, dword [rbp - 16]
    push rax

    movsxd rax, dword [rbp - 36]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 144], rax
    mov rax, [rbp - 144]
    push rax

    movsxd rax, dword [rbp - 32]
    mov r10, rax
    pop rax
    cmp rax, r10
    setge al
    movzx rax, al
    mov [rbp - 152], rax
    mov rax, [rbp - 152]
    test rax, rax
    jz ir_if_next_428
    mov rax, -1
    mov [rbp - 160], rax
    mov rax, -1
    mov [rbp - 168], rax
    jmp ir_errdefer_end_430
ir_errdefer_ok_429:
ir_errdefer_end_430:
    mov rax, [rbp - 160]
    jmp Lstrncmp_exit
ir_if_next_428:
ir_if_end_427:

    movsxd rax, dword [rbp - 16]
    push rax

    movsxd rax, dword [rbp - 36]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 176], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_433
    jmp ir_nonnull_434
ir_trap_null_433:

    sub rsp, 32

    lea rax, [rel Lstr_struct131]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_434:
    mov rax, [rbp - 176]
    mov [rbp - 184], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 176]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 192], rax
    mov rax, [rbp - 192]
    movzx rax, byte [rax]
    mov [rbp - 200], rax

    mov rax, qword [rbp - 24]
    mov [rbp - 208], rax
    mov rax, [rbp - 208]
    mov rax, qword [rax]
    mov [rbp - 216], rax
    mov rax, [rbp - 216]
    test rax, rax
    jz ir_trap_null_435
    jmp ir_nonnull_436
ir_trap_null_435:

    sub rsp, 32

    lea rax, [rel Lstr_struct133]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_436:

    movsxd rax, dword [rbp - 36]
    mov [rbp - 224], rax
    mov rax, [rbp - 216]
    push rax
    mov rax, [rbp - 224]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 232], rax
    mov rax, [rbp - 232]
    movzx rax, byte [rax]
    mov [rbp - 240], rax
    mov rax, [rbp - 200]
    push rax
    mov rax, [rbp - 240]
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 248], rax
    mov rax, [rbp - 248]
    test rax, rax
    jz ir_if_next_432

    movsxd rax, dword [rbp - 16]
    push rax

    movsxd rax, dword [rbp - 36]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 256], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_437
    jmp ir_nonnull_438
ir_trap_null_437:

    sub rsp, 32

    lea rax, [rel Lstr_struct135]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_438:
    mov rax, [rbp - 256]
    mov [rbp - 264], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 256]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 272], rax
    mov rax, [rbp - 272]
    movzx rax, byte [rax]
    mov [rbp - 280], rax

    mov rax, qword [rbp - 24]
    mov [rbp - 288], rax
    mov rax, [rbp - 288]
    mov rax, qword [rax]
    mov [rbp - 296], rax
    mov rax, [rbp - 296]
    test rax, rax
    jz ir_trap_null_439
    jmp ir_nonnull_440
ir_trap_null_439:

    sub rsp, 32

    lea rax, [rel Lstr_struct137]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_440:

    movsxd rax, dword [rbp - 36]
    mov [rbp - 304], rax
    mov rax, [rbp - 296]
    push rax
    mov rax, [rbp - 304]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 312], rax
    mov rax, [rbp - 312]
    movzx rax, byte [rax]
    mov [rbp - 320], rax
    mov rax, [rbp - 280]
    push rax
    mov rax, [rbp - 320]
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 328], rax
    mov rax, [rbp - 328]
    mov [rbp - 336], rax
    mov rax, [rbp - 328]
    test rax, rax
    jz ir_errdefer_ok_441
    jmp ir_errdefer_end_442
ir_errdefer_ok_441:
ir_errdefer_end_442:
    mov rax, [rbp - 328]
    jmp Lstrncmp_exit
ir_if_next_432:
ir_if_end_431:

    movsxd rax, dword [rbp - 36]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 344], rax
    mov rax, [rbp - 344]

    mov dword [rbp - 36], eax
    jmp ir_while_425
ir_while_end_426:
    mov rax, 0
    mov [rbp - 352], rax
ir_errdefer_ok_443:
ir_errdefer_end_444:
    mov rax, 0
    jmp Lstrncmp_exit
Lstrncmp_exit:

    mov rsp, rbp
    pop rbp
    ret
    extern gc_init
    extern AllocConsole
    extern GetStdHandle
    extern WriteFile

global is_get

is_get:
    push rbp
    mov rbp, rsp
    sub rsp, 560

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_445:

    movsxd rax, dword [rbp - 16]
    push rax
    mov rax, 4
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_if_next_447
    mov rax, 0
    mov [rbp - 32], rax
ir_errdefer_ok_448:
ir_errdefer_end_449:
    mov rax, 0
    jmp Lis_get_exit
ir_if_next_447:
ir_if_end_446:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_452
    jmp ir_nonnull_453
ir_trap_null_452:

    sub rsp, 32

    lea rax, [rel Lstr_struct139]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_453:
    mov rax, 0
    mov [rbp - 40], rax

    mov rax, qword [rbp - 8]
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    movzx rax, byte [rax]
    mov [rbp - 56], rax
    mov rax, [rbp - 56]
    push rax
    mov rax, 71
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 64], rax
    mov rax, [rbp - 64]
    test rax, rax
    jz ir_if_next_451
    mov rax, 0
    mov [rbp - 72], rax
ir_errdefer_ok_454:
ir_errdefer_end_455:
    mov rax, 0
    jmp Lis_get_exit
ir_if_next_451:
ir_if_end_450:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_458
    jmp ir_nonnull_459
ir_trap_null_458:

    sub rsp, 32

    lea rax, [rel Lstr_struct141]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_459:
    mov rax, 1
    mov [rbp - 80], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 88], rax
    mov rax, [rbp - 88]
    movzx rax, byte [rax]
    mov [rbp - 96], rax
    mov rax, [rbp - 96]
    push rax
    mov rax, 69
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 104], rax
    mov rax, [rbp - 104]
    test rax, rax
    jz ir_if_next_457
    mov rax, 0
    mov [rbp - 112], rax
ir_errdefer_ok_460:
ir_errdefer_end_461:
    mov rax, 0
    jmp Lis_get_exit
ir_if_next_457:
ir_if_end_456:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_464
    jmp ir_nonnull_465
ir_trap_null_464:

    sub rsp, 32

    lea rax, [rel Lstr_struct143]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_465:
    mov rax, 2
    mov [rbp - 120], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 2
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 128], rax
    mov rax, [rbp - 128]
    movzx rax, byte [rax]
    mov [rbp - 136], rax
    mov rax, [rbp - 136]
    push rax
    mov rax, 84
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 144], rax
    mov rax, [rbp - 144]
    test rax, rax
    jz ir_if_next_463
    mov rax, 0
    mov [rbp - 152], rax
ir_errdefer_ok_466:
ir_errdefer_end_467:
    mov rax, 0
    jmp Lis_get_exit
ir_if_next_463:
ir_if_end_462:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_470
    jmp ir_nonnull_471
ir_trap_null_470:

    sub rsp, 32

    lea rax, [rel Lstr_struct145]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_471:
    mov rax, 3
    mov [rbp - 160], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 3
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 168], rax
    mov rax, [rbp - 168]
    movzx rax, byte [rax]
    mov [rbp - 176], rax
    mov rax, [rbp - 176]
    push rax
    mov rax, 32
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 184], rax
    mov rax, [rbp - 184]
    test rax, rax
    jz ir_if_next_469
    mov rax, 0
    mov [rbp - 192], rax
ir_errdefer_ok_472:
ir_errdefer_end_473:
    mov rax, 0
    jmp Lis_get_exit
ir_if_next_469:
ir_if_end_468:
    mov rax, 1
    mov [rbp - 200], rax
    jmp ir_errdefer_end_475
ir_errdefer_ok_474:
ir_errdefer_end_475:
    mov rax, 1
    jmp Lis_get_exit
Lis_get_exit:

    mov rsp, rbp
    pop rbp
    ret

global is_root

is_root:
    push rbp
    mov rbp, rsp
    sub rsp, 320

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_476:

    movsxd rax, dword [rbp - 16]
    push rax
    mov rax, 6
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_if_next_478
    mov rax, 0
    mov [rbp - 32], rax
ir_errdefer_ok_479:
ir_errdefer_end_480:
    mov rax, 0
    jmp Lis_root_exit
ir_if_next_478:
ir_if_end_477:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_483
    jmp ir_nonnull_484
ir_trap_null_483:

    sub rsp, 32

    lea rax, [rel Lstr_struct147]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_484:
    mov rax, 4
    mov [rbp - 40], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 4
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    movzx rax, byte [rax]
    mov [rbp - 56], rax
    mov rax, [rbp - 56]
    push rax
    mov rax, 47
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 64], rax
    mov rax, [rbp - 64]
    test rax, rax
    jz ir_if_next_482
    mov rax, 0
    mov [rbp - 72], rax
ir_errdefer_ok_485:
ir_errdefer_end_486:
    mov rax, 0
    jmp Lis_root_exit
ir_if_next_482:
ir_if_end_481:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_489
    jmp ir_nonnull_490
ir_trap_null_489:

    sub rsp, 32

    lea rax, [rel Lstr_struct149]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_490:
    mov rax, 5
    mov [rbp - 80], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 5
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 88], rax
    mov rax, [rbp - 88]
    movzx rax, byte [rax]
    mov [rbp - 96], rax
    mov rax, [rbp - 96]
    push rax
    mov rax, 32
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 104], rax
    mov rax, [rbp - 104]
    test rax, rax
    jz ir_if_next_488
    mov rax, 0
    mov [rbp - 112], rax
ir_errdefer_ok_491:
ir_errdefer_end_492:
    mov rax, 0
    jmp Lis_root_exit
ir_if_next_488:
ir_if_end_487:
    mov rax, 1
    mov [rbp - 120], rax
    jmp ir_errdefer_end_494
ir_errdefer_ok_493:
ir_errdefer_end_494:
    mov rax, 1
    jmp Lis_root_exit
Lis_root_exit:

    mov rsp, rbp
    pop rbp
    ret

global is_health

is_health:
    push rbp
    mov rbp, rsp
    sub rsp, 1152

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_495:

    movsxd rax, dword [rbp - 16]
    push rax
    mov rax, 12
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_if_next_497
    mov rax, 0
    mov [rbp - 32], rax
ir_errdefer_ok_498:
ir_errdefer_end_499:
    mov rax, 0
    jmp Lis_health_exit
ir_if_next_497:
ir_if_end_496:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_502
    jmp ir_nonnull_503
ir_trap_null_502:

    sub rsp, 32

    lea rax, [rel Lstr_struct151]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_503:
    mov rax, 4
    mov [rbp - 40], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 4
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    movzx rax, byte [rax]
    mov [rbp - 56], rax
    mov rax, [rbp - 56]
    push rax
    mov rax, 47
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 64], rax
    mov rax, [rbp - 64]
    test rax, rax
    jz ir_if_next_501
    mov rax, 0
    mov [rbp - 72], rax
ir_errdefer_ok_504:
ir_errdefer_end_505:
    mov rax, 0
    jmp Lis_health_exit
ir_if_next_501:
ir_if_end_500:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_508
    jmp ir_nonnull_509
ir_trap_null_508:

    sub rsp, 32

    lea rax, [rel Lstr_struct153]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_509:
    mov rax, 5
    mov [rbp - 80], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 5
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 88], rax
    mov rax, [rbp - 88]
    movzx rax, byte [rax]
    mov [rbp - 96], rax
    mov rax, [rbp - 96]
    push rax
    mov rax, 104
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 104], rax
    mov rax, [rbp - 104]
    test rax, rax
    jz ir_if_next_507
    mov rax, 0
    mov [rbp - 112], rax
ir_errdefer_ok_510:
ir_errdefer_end_511:
    mov rax, 0
    jmp Lis_health_exit
ir_if_next_507:
ir_if_end_506:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_514
    jmp ir_nonnull_515
ir_trap_null_514:

    sub rsp, 32

    lea rax, [rel Lstr_struct155]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_515:
    mov rax, 6
    mov [rbp - 120], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 6
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 128], rax
    mov rax, [rbp - 128]
    movzx rax, byte [rax]
    mov [rbp - 136], rax
    mov rax, [rbp - 136]
    push rax
    mov rax, 101
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 144], rax
    mov rax, [rbp - 144]
    test rax, rax
    jz ir_if_next_513
    mov rax, 0
    mov [rbp - 152], rax
ir_errdefer_ok_516:
ir_errdefer_end_517:
    mov rax, 0
    jmp Lis_health_exit
ir_if_next_513:
ir_if_end_512:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_520
    jmp ir_nonnull_521
ir_trap_null_520:

    sub rsp, 32

    lea rax, [rel Lstr_struct157]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_521:
    mov rax, 7
    mov [rbp - 160], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 7
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 168], rax
    mov rax, [rbp - 168]
    movzx rax, byte [rax]
    mov [rbp - 176], rax
    mov rax, [rbp - 176]
    push rax
    mov rax, 97
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 184], rax
    mov rax, [rbp - 184]
    test rax, rax
    jz ir_if_next_519
    mov rax, 0
    mov [rbp - 192], rax
ir_errdefer_ok_522:
ir_errdefer_end_523:
    mov rax, 0
    jmp Lis_health_exit
ir_if_next_519:
ir_if_end_518:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_526
    jmp ir_nonnull_527
ir_trap_null_526:

    sub rsp, 32

    lea rax, [rel Lstr_struct159]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_527:
    mov rax, 8
    mov [rbp - 200], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 208], rax
    mov rax, [rbp - 208]
    movzx rax, byte [rax]
    mov [rbp - 216], rax
    mov rax, [rbp - 216]
    push rax
    mov rax, 108
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 224], rax
    mov rax, [rbp - 224]
    test rax, rax
    jz ir_if_next_525
    mov rax, 0
    mov [rbp - 232], rax
ir_errdefer_ok_528:
ir_errdefer_end_529:
    mov rax, 0
    jmp Lis_health_exit
ir_if_next_525:
ir_if_end_524:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_532
    jmp ir_nonnull_533
ir_trap_null_532:

    sub rsp, 32

    lea rax, [rel Lstr_struct161]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_533:
    mov rax, 9
    mov [rbp - 240], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 9
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 248], rax
    mov rax, [rbp - 248]
    movzx rax, byte [rax]
    mov [rbp - 256], rax
    mov rax, [rbp - 256]
    push rax
    mov rax, 116
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 264], rax
    mov rax, [rbp - 264]
    test rax, rax
    jz ir_if_next_531
    mov rax, 0
    mov [rbp - 272], rax
ir_errdefer_ok_534:
ir_errdefer_end_535:
    mov rax, 0
    jmp Lis_health_exit
ir_if_next_531:
ir_if_end_530:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_538
    jmp ir_nonnull_539
ir_trap_null_538:

    sub rsp, 32

    lea rax, [rel Lstr_struct163]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_539:
    mov rax, 10
    mov [rbp - 280], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 10
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 288], rax
    mov rax, [rbp - 288]
    movzx rax, byte [rax]
    mov [rbp - 296], rax
    mov rax, [rbp - 296]
    push rax
    mov rax, 104
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 304], rax
    mov rax, [rbp - 304]
    test rax, rax
    jz ir_if_next_537
    mov rax, 0
    mov [rbp - 312], rax
ir_errdefer_ok_540:
ir_errdefer_end_541:
    mov rax, 0
    jmp Lis_health_exit
ir_if_next_537:
ir_if_end_536:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_544
    jmp ir_nonnull_545
ir_trap_null_544:

    sub rsp, 32

    lea rax, [rel Lstr_struct165]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_545:
    mov rax, 11
    mov [rbp - 320], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 11
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 328], rax
    mov rax, [rbp - 328]
    movzx rax, byte [rax]
    mov [rbp - 336], rax
    mov rax, [rbp - 336]
    push rax
    mov rax, 32
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 344], rax
    mov rax, [rbp - 344]
    test rax, rax
    jz ir_sc_false_548
ir_sc_rhs_546:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_550
    jmp ir_nonnull_551
ir_trap_null_550:

    sub rsp, 32

    lea rax, [rel Lstr_struct167]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_551:
    mov rax, 11
    mov [rbp - 352], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 11
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 360], rax
    mov rax, [rbp - 360]
    movzx rax, byte [rax]
    mov [rbp - 368], rax
    mov rax, [rbp - 368]
    push rax
    mov rax, 63
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 376], rax
    mov rax, [rbp - 376]
    test rax, rax
    jz ir_sc_false_548
ir_sc_true_547:
    mov rax, 1
    mov [rbp - 384], rax
    jmp ir_sc_end_549
ir_sc_false_548:
    mov rax, 0
    mov [rbp - 384], rax
ir_sc_end_549:
    mov rax, [rbp - 384]
    test rax, rax
    jz ir_if_next_543
    mov rax, 0
    mov [rbp - 400], rax
ir_errdefer_ok_552:
ir_errdefer_end_553:
    mov rax, 0
    jmp Lis_health_exit
ir_if_next_543:
ir_if_end_542:
    mov rax, 1
    mov [rbp - 408], rax
    jmp ir_errdefer_end_555
ir_errdefer_ok_554:
ir_errdefer_end_555:
    mov rax, 1
    jmp Lis_health_exit
Lis_health_exit:

    mov rsp, rbp
    pop rbp
    ret

global is_post

is_post:
    push rbp
    mov rbp, rsp
    sub rsp, 672

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_556:

    movsxd rax, dword [rbp - 16]
    push rax
    mov rax, 5
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_if_next_558
    mov rax, 0
    mov [rbp - 32], rax
ir_errdefer_ok_559:
ir_errdefer_end_560:
    mov rax, 0
    jmp Lis_post_exit
ir_if_next_558:
ir_if_end_557:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_563
    jmp ir_nonnull_564
ir_trap_null_563:

    sub rsp, 32

    lea rax, [rel Lstr_struct169]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_564:
    mov rax, 0
    mov [rbp - 40], rax

    mov rax, qword [rbp - 8]
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    movzx rax, byte [rax]
    mov [rbp - 56], rax
    mov rax, [rbp - 56]
    push rax
    mov rax, 80
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 64], rax
    mov rax, [rbp - 64]
    test rax, rax
    jz ir_if_next_562
    mov rax, 0
    mov [rbp - 72], rax
ir_errdefer_ok_565:
ir_errdefer_end_566:
    mov rax, 0
    jmp Lis_post_exit
ir_if_next_562:
ir_if_end_561:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_569
    jmp ir_nonnull_570
ir_trap_null_569:

    sub rsp, 32

    lea rax, [rel Lstr_struct171]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_570:
    mov rax, 1
    mov [rbp - 80], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 88], rax
    mov rax, [rbp - 88]
    movzx rax, byte [rax]
    mov [rbp - 96], rax
    mov rax, [rbp - 96]
    push rax
    mov rax, 79
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 104], rax
    mov rax, [rbp - 104]
    test rax, rax
    jz ir_if_next_568
    mov rax, 0
    mov [rbp - 112], rax
ir_errdefer_ok_571:
ir_errdefer_end_572:
    mov rax, 0
    jmp Lis_post_exit
ir_if_next_568:
ir_if_end_567:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_575
    jmp ir_nonnull_576
ir_trap_null_575:

    sub rsp, 32

    lea rax, [rel Lstr_struct173]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_576:
    mov rax, 2
    mov [rbp - 120], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 2
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 128], rax
    mov rax, [rbp - 128]
    movzx rax, byte [rax]
    mov [rbp - 136], rax
    mov rax, [rbp - 136]
    push rax
    mov rax, 83
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 144], rax
    mov rax, [rbp - 144]
    test rax, rax
    jz ir_if_next_574
    mov rax, 0
    mov [rbp - 152], rax
ir_errdefer_ok_577:
ir_errdefer_end_578:
    mov rax, 0
    jmp Lis_post_exit
ir_if_next_574:
ir_if_end_573:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_581
    jmp ir_nonnull_582
ir_trap_null_581:

    sub rsp, 32

    lea rax, [rel Lstr_struct175]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_582:
    mov rax, 3
    mov [rbp - 160], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 3
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 168], rax
    mov rax, [rbp - 168]
    movzx rax, byte [rax]
    mov [rbp - 176], rax
    mov rax, [rbp - 176]
    push rax
    mov rax, 84
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 184], rax
    mov rax, [rbp - 184]
    test rax, rax
    jz ir_if_next_580
    mov rax, 0
    mov [rbp - 192], rax
ir_errdefer_ok_583:
ir_errdefer_end_584:
    mov rax, 0
    jmp Lis_post_exit
ir_if_next_580:
ir_if_end_579:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_587
    jmp ir_nonnull_588
ir_trap_null_587:

    sub rsp, 32

    lea rax, [rel Lstr_struct177]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_588:
    mov rax, 4
    mov [rbp - 200], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 4
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 208], rax
    mov rax, [rbp - 208]
    movzx rax, byte [rax]
    mov [rbp - 216], rax
    mov rax, [rbp - 216]
    push rax
    mov rax, 32
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 224], rax
    mov rax, [rbp - 224]
    test rax, rax
    jz ir_if_next_586
    mov rax, 0
    mov [rbp - 232], rax
ir_errdefer_ok_589:
ir_errdefer_end_590:
    mov rax, 0
    jmp Lis_post_exit
ir_if_next_586:
ir_if_end_585:
    mov rax, 1
    mov [rbp - 240], rax
    jmp ir_errdefer_end_592
ir_errdefer_ok_591:
ir_errdefer_end_592:
    mov rax, 1
    jmp Lis_post_exit
Lis_post_exit:

    mov rsp, rbp
    pop rbp
    ret

global is_forum

is_forum:
    push rbp
    mov rbp, rsp
    sub rsp, 928

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx

    mov [rbp - 24], r8


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_593:

    movsxd rax, dword [rbp - 24]
    push rax
    mov rax, 6
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 32], rax
    mov rax, [rbp - 32]
    push rax

    movsxd rax, dword [rbp - 16]
    mov r10, rax
    pop rax
    cmp rax, r10
    setg al
    movzx rax, al
    mov [rbp - 40], rax
    mov rax, [rbp - 40]
    test rax, rax
    jz ir_if_next_595
    mov rax, 0
    mov [rbp - 48], rax
ir_errdefer_ok_596:
ir_errdefer_end_597:
    mov rax, 0
    jmp Lis_forum_exit
ir_if_next_595:
ir_if_end_594:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_600
    jmp ir_nonnull_601
ir_trap_null_600:

    sub rsp, 32

    lea rax, [rel Lstr_struct179]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_601:

    movsxd rax, dword [rbp - 24]
    mov [rbp - 56], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 56]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 64], rax
    mov rax, [rbp - 64]
    movzx rax, byte [rax]
    mov [rbp - 72], rax
    mov rax, [rbp - 72]
    push rax
    mov rax, 47
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 80], rax
    mov rax, [rbp - 80]
    test rax, rax
    jz ir_if_next_599
    mov rax, 0
    mov [rbp - 88], rax
ir_errdefer_ok_602:
ir_errdefer_end_603:
    mov rax, 0
    jmp Lis_forum_exit
ir_if_next_599:
ir_if_end_598:

    movsxd rax, dword [rbp - 24]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 96], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_606
    jmp ir_nonnull_607
ir_trap_null_606:

    sub rsp, 32

    lea rax, [rel Lstr_struct181]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_607:
    mov rax, [rbp - 96]
    mov [rbp - 104], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 96]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 112], rax
    mov rax, [rbp - 112]
    movzx rax, byte [rax]
    mov [rbp - 120], rax
    mov rax, [rbp - 120]
    push rax
    mov rax, 102
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 128], rax
    mov rax, [rbp - 128]
    test rax, rax
    jz ir_if_next_605
    mov rax, 0
    mov [rbp - 136], rax
ir_errdefer_ok_608:
ir_errdefer_end_609:
    mov rax, 0
    jmp Lis_forum_exit
ir_if_next_605:
ir_if_end_604:

    movsxd rax, dword [rbp - 24]
    push rax
    mov rax, 2
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 144], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_612
    jmp ir_nonnull_613
ir_trap_null_612:

    sub rsp, 32

    lea rax, [rel Lstr_struct183]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_613:
    mov rax, [rbp - 144]
    mov [rbp - 152], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 144]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 160], rax
    mov rax, [rbp - 160]
    movzx rax, byte [rax]
    mov [rbp - 168], rax
    mov rax, [rbp - 168]
    push rax
    mov rax, 111
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 176], rax
    mov rax, [rbp - 176]
    test rax, rax
    jz ir_if_next_611
    mov rax, 0
    mov [rbp - 184], rax
ir_errdefer_ok_614:
ir_errdefer_end_615:
    mov rax, 0
    jmp Lis_forum_exit
ir_if_next_611:
ir_if_end_610:

    movsxd rax, dword [rbp - 24]
    push rax
    mov rax, 3
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 192], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_618
    jmp ir_nonnull_619
ir_trap_null_618:

    sub rsp, 32

    lea rax, [rel Lstr_struct185]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_619:
    mov rax, [rbp - 192]
    mov [rbp - 200], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 192]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 208], rax
    mov rax, [rbp - 208]
    movzx rax, byte [rax]
    mov [rbp - 216], rax
    mov rax, [rbp - 216]
    push rax
    mov rax, 114
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 224], rax
    mov rax, [rbp - 224]
    test rax, rax
    jz ir_if_next_617
    mov rax, 0
    mov [rbp - 232], rax
ir_errdefer_ok_620:
ir_errdefer_end_621:
    mov rax, 0
    jmp Lis_forum_exit
ir_if_next_617:
ir_if_end_616:

    movsxd rax, dword [rbp - 24]
    push rax
    mov rax, 4
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 240], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_624
    jmp ir_nonnull_625
ir_trap_null_624:

    sub rsp, 32

    lea rax, [rel Lstr_struct187]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_625:
    mov rax, [rbp - 240]
    mov [rbp - 248], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 240]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 256], rax
    mov rax, [rbp - 256]
    movzx rax, byte [rax]
    mov [rbp - 264], rax
    mov rax, [rbp - 264]
    push rax
    mov rax, 117
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 272], rax
    mov rax, [rbp - 272]
    test rax, rax
    jz ir_if_next_623
    mov rax, 0
    mov [rbp - 280], rax
ir_errdefer_ok_626:
ir_errdefer_end_627:
    mov rax, 0
    jmp Lis_forum_exit
ir_if_next_623:
ir_if_end_622:

    movsxd rax, dword [rbp - 24]
    push rax
    mov rax, 5
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 288], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_630
    jmp ir_nonnull_631
ir_trap_null_630:

    sub rsp, 32

    lea rax, [rel Lstr_struct189]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_631:
    mov rax, [rbp - 288]
    mov [rbp - 296], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 288]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 304], rax
    mov rax, [rbp - 304]
    movzx rax, byte [rax]
    mov [rbp - 312], rax
    mov rax, [rbp - 312]
    push rax
    mov rax, 109
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 320], rax
    mov rax, [rbp - 320]
    test rax, rax
    jz ir_if_next_629
    mov rax, 0
    mov [rbp - 328], rax
ir_errdefer_ok_632:
ir_errdefer_end_633:
    mov rax, 0
    jmp Lis_forum_exit
ir_if_next_629:
ir_if_end_628:
    mov rax, 1
    mov [rbp - 336], rax
    jmp ir_errdefer_end_635
ir_errdefer_ok_634:
ir_errdefer_end_635:
    mov rax, 1
    jmp Lis_forum_exit
Lis_forum_exit:

    mov rsp, rbp
    pop rbp
    ret

global get_thread_id

get_thread_id:
    push rbp
    mov rbp, rsp
    sub rsp, 1936

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx

    mov [rbp - 24], r8


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_636:

    movsxd rax, dword [rbp - 24]
    push rax
    mov rax, 6
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 40], rax
    mov rax, [rbp - 40]

    mov dword [rbp - 28], eax

    movsxd rax, dword [rbp - 28]
    push rax

    movsxd rax, dword [rbp - 16]
    mov r10, rax
    pop rax
    cmp rax, r10
    setge al
    movzx rax, al
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    test rax, rax
    jz ir_if_next_638
    mov rax, 0
    mov [rbp - 56], rax
ir_errdefer_ok_639:
ir_errdefer_end_640:
    mov rax, 0
    jmp Lget_thread_id_exit
ir_if_next_638:
ir_if_end_637:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_643
    jmp ir_nonnull_644
ir_trap_null_643:

    sub rsp, 32

    lea rax, [rel Lstr_struct191]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_644:

    movsxd rax, dword [rbp - 28]
    mov [rbp - 64], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 64]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 72], rax
    mov rax, [rbp - 72]
    movzx rax, byte [rax]
    mov [rbp - 80], rax
    mov rax, [rbp - 80]
    push rax
    mov rax, 63
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 88], rax
    mov rax, [rbp - 88]
    test rax, rax
    jz ir_if_next_642
    mov rax, 0
    mov [rbp - 96], rax
ir_errdefer_ok_645:
ir_errdefer_end_646:
    mov rax, 0
    jmp Lget_thread_id_exit
ir_if_next_642:
ir_if_end_641:

    movsxd rax, dword [rbp - 28]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 104], rax
    mov rax, [rbp - 104]

    mov dword [rbp - 28], eax

    movsxd rax, dword [rbp - 28]
    push rax
    mov rax, 7
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 112], rax
    mov rax, [rbp - 112]
    push rax

    movsxd rax, dword [rbp - 16]
    mov r10, rax
    pop rax
    cmp rax, r10
    setg al
    movzx rax, al
    mov [rbp - 120], rax
    mov rax, [rbp - 120]
    test rax, rax
    jz ir_if_next_648
    mov rax, 0
    mov [rbp - 128], rax
ir_errdefer_ok_649:
ir_errdefer_end_650:
    mov rax, 0
    jmp Lget_thread_id_exit
ir_if_next_648:
ir_if_end_647:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_653
    jmp ir_nonnull_654
ir_trap_null_653:

    sub rsp, 32

    lea rax, [rel Lstr_struct193]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_654:

    movsxd rax, dword [rbp - 28]
    mov [rbp - 136], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 136]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 144], rax
    mov rax, [rbp - 144]
    movzx rax, byte [rax]
    mov [rbp - 152], rax
    mov rax, [rbp - 152]
    push rax
    mov rax, 116
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 160], rax
    mov rax, [rbp - 160]
    test rax, rax
    jz ir_sc_rhs_655
    jmp ir_sc_true_656
ir_sc_rhs_655:

    movsxd rax, dword [rbp - 28]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 168], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_659
    jmp ir_nonnull_660
ir_trap_null_659:

    sub rsp, 32

    lea rax, [rel Lstr_struct195]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_660:
    mov rax, [rbp - 168]
    mov [rbp - 176], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 168]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 184], rax
    mov rax, [rbp - 184]
    movzx rax, byte [rax]
    mov [rbp - 192], rax
    mov rax, [rbp - 192]
    push rax
    mov rax, 104
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 200], rax
    mov rax, [rbp - 200]
    test rax, rax
    jz ir_sc_false_657
ir_sc_true_656:
    mov rax, 1
    mov [rbp - 208], rax
    jmp ir_sc_end_658
ir_sc_false_657:
    mov rax, 0
    mov [rbp - 208], rax
ir_sc_end_658:
    mov rax, [rbp - 208]
    test rax, rax
    jz ir_sc_rhs_661
    jmp ir_sc_true_662
ir_sc_rhs_661:

    movsxd rax, dword [rbp - 28]
    push rax
    mov rax, 2
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 224], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_665
    jmp ir_nonnull_666
ir_trap_null_665:

    sub rsp, 32

    lea rax, [rel Lstr_struct197]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_666:
    mov rax, [rbp - 224]
    mov [rbp - 232], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 224]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 240], rax
    mov rax, [rbp - 240]
    movzx rax, byte [rax]
    mov [rbp - 248], rax
    mov rax, [rbp - 248]
    push rax
    mov rax, 114
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 256], rax
    mov rax, [rbp - 256]
    test rax, rax
    jz ir_sc_false_663
ir_sc_true_662:
    mov rax, 1
    mov [rbp - 264], rax
    jmp ir_sc_end_664
ir_sc_false_663:
    mov rax, 0
    mov [rbp - 264], rax
ir_sc_end_664:
    mov rax, [rbp - 264]
    test rax, rax
    jz ir_sc_rhs_667
    jmp ir_sc_true_668
ir_sc_rhs_667:

    movsxd rax, dword [rbp - 28]
    push rax
    mov rax, 3
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 280], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_671
    jmp ir_nonnull_672
ir_trap_null_671:

    sub rsp, 32

    lea rax, [rel Lstr_struct199]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_672:
    mov rax, [rbp - 280]
    mov [rbp - 288], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 280]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 296], rax
    mov rax, [rbp - 296]
    movzx rax, byte [rax]
    mov [rbp - 304], rax
    mov rax, [rbp - 304]
    push rax
    mov rax, 101
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 312], rax
    mov rax, [rbp - 312]
    test rax, rax
    jz ir_sc_false_669
ir_sc_true_668:
    mov rax, 1
    mov [rbp - 320], rax
    jmp ir_sc_end_670
ir_sc_false_669:
    mov rax, 0
    mov [rbp - 320], rax
ir_sc_end_670:
    mov rax, [rbp - 320]
    test rax, rax
    jz ir_sc_rhs_673
    jmp ir_sc_true_674
ir_sc_rhs_673:

    movsxd rax, dword [rbp - 28]
    push rax
    mov rax, 4
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 336], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_677
    jmp ir_nonnull_678
ir_trap_null_677:

    sub rsp, 32

    lea rax, [rel Lstr_struct201]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_678:
    mov rax, [rbp - 336]
    mov [rbp - 344], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 336]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 352], rax
    mov rax, [rbp - 352]
    movzx rax, byte [rax]
    mov [rbp - 360], rax
    mov rax, [rbp - 360]
    push rax
    mov rax, 97
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 368], rax
    mov rax, [rbp - 368]
    test rax, rax
    jz ir_sc_false_675
ir_sc_true_674:
    mov rax, 1
    mov [rbp - 376], rax
    jmp ir_sc_end_676
ir_sc_false_675:
    mov rax, 0
    mov [rbp - 376], rax
ir_sc_end_676:
    mov rax, [rbp - 376]
    test rax, rax
    jz ir_sc_rhs_679
    jmp ir_sc_true_680
ir_sc_rhs_679:

    movsxd rax, dword [rbp - 28]
    push rax
    mov rax, 5
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 392], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_683
    jmp ir_nonnull_684
ir_trap_null_683:

    sub rsp, 32

    lea rax, [rel Lstr_struct203]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_684:
    mov rax, [rbp - 392]
    mov [rbp - 400], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 392]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 408], rax
    mov rax, [rbp - 408]
    movzx rax, byte [rax]
    mov [rbp - 416], rax
    mov rax, [rbp - 416]
    push rax
    mov rax, 100
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 424], rax
    mov rax, [rbp - 424]
    test rax, rax
    jz ir_sc_false_681
ir_sc_true_680:
    mov rax, 1
    mov [rbp - 432], rax
    jmp ir_sc_end_682
ir_sc_false_681:
    mov rax, 0
    mov [rbp - 432], rax
ir_sc_end_682:
    mov rax, [rbp - 432]
    test rax, rax
    jz ir_sc_rhs_685
    jmp ir_sc_true_686
ir_sc_rhs_685:

    movsxd rax, dword [rbp - 28]
    push rax
    mov rax, 6
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 448], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_689
    jmp ir_nonnull_690
ir_trap_null_689:

    sub rsp, 32

    lea rax, [rel Lstr_struct205]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_690:
    mov rax, [rbp - 448]
    mov [rbp - 456], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 448]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 464], rax
    mov rax, [rbp - 464]
    movzx rax, byte [rax]
    mov [rbp - 472], rax
    mov rax, [rbp - 472]
    push rax
    mov rax, 61
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 480], rax
    mov rax, [rbp - 480]
    test rax, rax
    jz ir_sc_false_687
ir_sc_true_686:
    mov rax, 1
    mov [rbp - 488], rax
    jmp ir_sc_end_688
ir_sc_false_687:
    mov rax, 0
    mov [rbp - 488], rax
ir_sc_end_688:
    mov rax, [rbp - 488]
    test rax, rax
    jz ir_if_next_652
    mov rax, 0
    mov [rbp - 504], rax
ir_errdefer_ok_691:
ir_errdefer_end_692:
    mov rax, 0
    jmp Lget_thread_id_exit
ir_if_next_652:
ir_if_end_651:

    movsxd rax, dword [rbp - 28]
    push rax
    mov rax, 7
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 512], rax
    mov rax, [rbp - 512]

    mov dword [rbp - 28], eax
    mov rax, 0

    mov dword [rbp - 32], eax
ir_while_693:

    movsxd rax, dword [rbp - 28]
    push rax

    movsxd rax, dword [rbp - 16]
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 520], rax
    mov rax, [rbp - 520]
    test rax, rax
    jz ir_sc_false_697
ir_sc_rhs_695:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_699
    jmp ir_nonnull_700
ir_trap_null_699:

    sub rsp, 32

    lea rax, [rel Lstr_struct207]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_700:

    movsxd rax, dword [rbp - 28]
    mov [rbp - 528], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 528]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 536], rax
    mov rax, [rbp - 536]
    movzx rax, byte [rax]
    mov [rbp - 544], rax
    mov rax, [rbp - 544]
    push rax
    mov rax, 48
    mov r10, rax
    pop rax
    cmp rax, r10
    setge al
    movzx rax, al
    mov [rbp - 552], rax
    mov rax, [rbp - 552]
    test rax, rax
    jz ir_sc_false_697
ir_sc_true_696:
    mov rax, 1
    mov [rbp - 560], rax
    jmp ir_sc_end_698
ir_sc_false_697:
    mov rax, 0
    mov [rbp - 560], rax
ir_sc_end_698:
    mov rax, [rbp - 560]
    test rax, rax
    jz ir_sc_false_703
ir_sc_rhs_701:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_705
    jmp ir_nonnull_706
ir_trap_null_705:

    sub rsp, 32

    lea rax, [rel Lstr_struct209]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_706:

    movsxd rax, dword [rbp - 28]
    mov [rbp - 576], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 576]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 584], rax
    mov rax, [rbp - 584]
    movzx rax, byte [rax]
    mov [rbp - 592], rax
    mov rax, [rbp - 592]
    push rax
    mov rax, 57
    mov r10, rax
    pop rax
    cmp rax, r10
    setle al
    movzx rax, al
    mov [rbp - 600], rax
    mov rax, [rbp - 600]
    test rax, rax
    jz ir_sc_false_703
ir_sc_true_702:
    mov rax, 1
    mov [rbp - 608], rax
    jmp ir_sc_end_704
ir_sc_false_703:
    mov rax, 0
    mov [rbp - 608], rax
ir_sc_end_704:
    mov rax, [rbp - 608]
    test rax, rax
    jz ir_while_end_694

    movsxd rax, dword [rbp - 32]
    push rax
    mov rax, 10
    mov r10, rax
    pop rax
    imul rax, r10
    mov [rbp - 624], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_707
    jmp ir_nonnull_708
ir_trap_null_707:

    sub rsp, 32

    lea rax, [rel Lstr_struct211]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_708:

    movsxd rax, dword [rbp - 28]
    mov [rbp - 632], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 632]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 640], rax
    mov rax, [rbp - 640]
    movzx rax, byte [rax]
    mov [rbp - 648], rax
    mov rax, [rbp - 648]
    push rax
    mov rax, 48
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 656], rax
    mov rax, [rbp - 624]
    push rax
    mov rax, [rbp - 656]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 664], rax
    mov rax, [rbp - 664]

    mov dword [rbp - 32], eax

    movsxd rax, dword [rbp - 28]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 672], rax
    mov rax, [rbp - 672]

    mov dword [rbp - 28], eax
    jmp ir_while_693
ir_while_end_694:

    movsxd rax, dword [rbp - 32]
    mov [rbp - 680], rax
    mov rax, [rbp - 680]
    test rax, rax
    jz ir_errdefer_ok_709
    jmp ir_errdefer_end_710
ir_errdefer_ok_709:
ir_errdefer_end_710:

    movsxd rax, dword [rbp - 32]
    jmp Lget_thread_id_exit
Lget_thread_id_exit:

    mov rsp, rbp
    pop rbp
    ret

global is_demo

is_demo:
    push rbp
    mov rbp, rsp
    sub rsp, 928

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_711:

    movsxd rax, dword [rbp - 16]
    push rax
    mov rax, 9
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_if_next_713
    mov rax, 0
    mov [rbp - 32], rax
ir_errdefer_ok_714:
ir_errdefer_end_715:
    mov rax, 0
    jmp Lis_demo_exit
ir_if_next_713:
ir_if_end_712:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_718
    jmp ir_nonnull_719
ir_trap_null_718:

    sub rsp, 32

    lea rax, [rel Lstr_struct213]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_719:
    mov rax, 4
    mov [rbp - 40], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 4
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    movzx rax, byte [rax]
    mov [rbp - 56], rax
    mov rax, [rbp - 56]
    push rax
    mov rax, 47
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 64], rax
    mov rax, [rbp - 64]
    test rax, rax
    jz ir_if_next_717
    mov rax, 0
    mov [rbp - 72], rax
ir_errdefer_ok_720:
ir_errdefer_end_721:
    mov rax, 0
    jmp Lis_demo_exit
ir_if_next_717:
ir_if_end_716:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_724
    jmp ir_nonnull_725
ir_trap_null_724:

    sub rsp, 32

    lea rax, [rel Lstr_struct215]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_725:
    mov rax, 5
    mov [rbp - 80], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 5
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 88], rax
    mov rax, [rbp - 88]
    movzx rax, byte [rax]
    mov [rbp - 96], rax
    mov rax, [rbp - 96]
    push rax
    mov rax, 100
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 104], rax
    mov rax, [rbp - 104]
    test rax, rax
    jz ir_if_next_723
    mov rax, 0
    mov [rbp - 112], rax
ir_errdefer_ok_726:
ir_errdefer_end_727:
    mov rax, 0
    jmp Lis_demo_exit
ir_if_next_723:
ir_if_end_722:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_730
    jmp ir_nonnull_731
ir_trap_null_730:

    sub rsp, 32

    lea rax, [rel Lstr_struct217]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_731:
    mov rax, 6
    mov [rbp - 120], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 6
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 128], rax
    mov rax, [rbp - 128]
    movzx rax, byte [rax]
    mov [rbp - 136], rax
    mov rax, [rbp - 136]
    push rax
    mov rax, 101
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 144], rax
    mov rax, [rbp - 144]
    test rax, rax
    jz ir_if_next_729
    mov rax, 0
    mov [rbp - 152], rax
ir_errdefer_ok_732:
ir_errdefer_end_733:
    mov rax, 0
    jmp Lis_demo_exit
ir_if_next_729:
ir_if_end_728:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_736
    jmp ir_nonnull_737
ir_trap_null_736:

    sub rsp, 32

    lea rax, [rel Lstr_struct219]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_737:
    mov rax, 7
    mov [rbp - 160], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 7
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 168], rax
    mov rax, [rbp - 168]
    movzx rax, byte [rax]
    mov [rbp - 176], rax
    mov rax, [rbp - 176]
    push rax
    mov rax, 109
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 184], rax
    mov rax, [rbp - 184]
    test rax, rax
    jz ir_if_next_735
    mov rax, 0
    mov [rbp - 192], rax
ir_errdefer_ok_738:
ir_errdefer_end_739:
    mov rax, 0
    jmp Lis_demo_exit
ir_if_next_735:
ir_if_end_734:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_742
    jmp ir_nonnull_743
ir_trap_null_742:

    sub rsp, 32

    lea rax, [rel Lstr_struct221]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_743:
    mov rax, 8
    mov [rbp - 200], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 208], rax
    mov rax, [rbp - 208]
    movzx rax, byte [rax]
    mov [rbp - 216], rax
    mov rax, [rbp - 216]
    push rax
    mov rax, 111
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 224], rax
    mov rax, [rbp - 224]
    test rax, rax
    jz ir_if_next_741
    mov rax, 0
    mov [rbp - 232], rax
ir_errdefer_ok_744:
ir_errdefer_end_745:
    mov rax, 0
    jmp Lis_demo_exit
ir_if_next_741:
ir_if_end_740:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_748
    jmp ir_nonnull_749
ir_trap_null_748:

    sub rsp, 32

    lea rax, [rel Lstr_struct223]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_749:
    mov rax, 9
    mov [rbp - 240], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 9
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 248], rax
    mov rax, [rbp - 248]
    movzx rax, byte [rax]
    mov [rbp - 256], rax
    mov rax, [rbp - 256]
    push rax
    mov rax, 32
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 264], rax
    mov rax, [rbp - 264]
    test rax, rax
    jz ir_sc_false_752
ir_sc_rhs_750:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_754
    jmp ir_nonnull_755
ir_trap_null_754:

    sub rsp, 32

    lea rax, [rel Lstr_struct225]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_755:
    mov rax, 9
    mov [rbp - 272], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 9
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 280], rax
    mov rax, [rbp - 280]
    movzx rax, byte [rax]
    mov [rbp - 288], rax
    mov rax, [rbp - 288]
    push rax
    mov rax, 63
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 296], rax
    mov rax, [rbp - 296]
    test rax, rax
    jz ir_sc_false_752
ir_sc_true_751:
    mov rax, 1
    mov [rbp - 304], rax
    jmp ir_sc_end_753
ir_sc_false_752:
    mov rax, 0
    mov [rbp - 304], rax
ir_sc_end_753:
    mov rax, [rbp - 304]
    test rax, rax
    jz ir_if_next_747
    mov rax, 0
    mov [rbp - 320], rax
ir_errdefer_ok_756:
ir_errdefer_end_757:
    mov rax, 0
    jmp Lis_demo_exit
ir_if_next_747:
ir_if_end_746:
    mov rax, 1
    mov [rbp - 328], rax
    jmp ir_errdefer_end_759
ir_errdefer_ok_758:
ir_errdefer_end_759:
    mov rax, 1
    jmp Lis_demo_exit
Lis_demo_exit:

    mov rsp, rbp
    pop rbp
    ret

global is_benchmarks

is_benchmarks:
    push rbp
    mov rbp, rsp
    sub rsp, 1616

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_760:

    movsxd rax, dword [rbp - 16]
    push rax
    mov rax, 16
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_if_next_762
    mov rax, 0
    mov [rbp - 32], rax
ir_errdefer_ok_763:
ir_errdefer_end_764:
    mov rax, 0
    jmp Lis_benchmarks_exit
ir_if_next_762:
ir_if_end_761:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_767
    jmp ir_nonnull_768
ir_trap_null_767:

    sub rsp, 32

    lea rax, [rel Lstr_struct227]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_768:
    mov rax, 4
    mov [rbp - 40], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 4
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    movzx rax, byte [rax]
    mov [rbp - 56], rax
    mov rax, [rbp - 56]
    push rax
    mov rax, 47
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 64], rax
    mov rax, [rbp - 64]
    test rax, rax
    jz ir_if_next_766
    mov rax, 0
    mov [rbp - 72], rax
ir_errdefer_ok_769:
ir_errdefer_end_770:
    mov rax, 0
    jmp Lis_benchmarks_exit
ir_if_next_766:
ir_if_end_765:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_773
    jmp ir_nonnull_774
ir_trap_null_773:

    sub rsp, 32

    lea rax, [rel Lstr_struct229]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_774:
    mov rax, 5
    mov [rbp - 80], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 5
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 88], rax
    mov rax, [rbp - 88]
    movzx rax, byte [rax]
    mov [rbp - 96], rax
    mov rax, [rbp - 96]
    push rax
    mov rax, 98
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 104], rax
    mov rax, [rbp - 104]
    test rax, rax
    jz ir_if_next_772
    mov rax, 0
    mov [rbp - 112], rax
ir_errdefer_ok_775:
ir_errdefer_end_776:
    mov rax, 0
    jmp Lis_benchmarks_exit
ir_if_next_772:
ir_if_end_771:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_779
    jmp ir_nonnull_780
ir_trap_null_779:

    sub rsp, 32

    lea rax, [rel Lstr_struct231]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_780:
    mov rax, 6
    mov [rbp - 120], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 6
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 128], rax
    mov rax, [rbp - 128]
    movzx rax, byte [rax]
    mov [rbp - 136], rax
    mov rax, [rbp - 136]
    push rax
    mov rax, 101
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 144], rax
    mov rax, [rbp - 144]
    test rax, rax
    jz ir_if_next_778
    mov rax, 0
    mov [rbp - 152], rax
ir_errdefer_ok_781:
ir_errdefer_end_782:
    mov rax, 0
    jmp Lis_benchmarks_exit
ir_if_next_778:
ir_if_end_777:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_785
    jmp ir_nonnull_786
ir_trap_null_785:

    sub rsp, 32

    lea rax, [rel Lstr_struct233]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_786:
    mov rax, 7
    mov [rbp - 160], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 7
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 168], rax
    mov rax, [rbp - 168]
    movzx rax, byte [rax]
    mov [rbp - 176], rax
    mov rax, [rbp - 176]
    push rax
    mov rax, 110
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 184], rax
    mov rax, [rbp - 184]
    test rax, rax
    jz ir_if_next_784
    mov rax, 0
    mov [rbp - 192], rax
ir_errdefer_ok_787:
ir_errdefer_end_788:
    mov rax, 0
    jmp Lis_benchmarks_exit
ir_if_next_784:
ir_if_end_783:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_791
    jmp ir_nonnull_792
ir_trap_null_791:

    sub rsp, 32

    lea rax, [rel Lstr_struct235]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_792:
    mov rax, 8
    mov [rbp - 200], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 208], rax
    mov rax, [rbp - 208]
    movzx rax, byte [rax]
    mov [rbp - 216], rax
    mov rax, [rbp - 216]
    push rax
    mov rax, 99
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 224], rax
    mov rax, [rbp - 224]
    test rax, rax
    jz ir_if_next_790
    mov rax, 0
    mov [rbp - 232], rax
ir_errdefer_ok_793:
ir_errdefer_end_794:
    mov rax, 0
    jmp Lis_benchmarks_exit
ir_if_next_790:
ir_if_end_789:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_797
    jmp ir_nonnull_798
ir_trap_null_797:

    sub rsp, 32

    lea rax, [rel Lstr_struct237]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_798:
    mov rax, 9
    mov [rbp - 240], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 9
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 248], rax
    mov rax, [rbp - 248]
    movzx rax, byte [rax]
    mov [rbp - 256], rax
    mov rax, [rbp - 256]
    push rax
    mov rax, 104
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 264], rax
    mov rax, [rbp - 264]
    test rax, rax
    jz ir_if_next_796
    mov rax, 0
    mov [rbp - 272], rax
ir_errdefer_ok_799:
ir_errdefer_end_800:
    mov rax, 0
    jmp Lis_benchmarks_exit
ir_if_next_796:
ir_if_end_795:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_803
    jmp ir_nonnull_804
ir_trap_null_803:

    sub rsp, 32

    lea rax, [rel Lstr_struct239]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_804:
    mov rax, 10
    mov [rbp - 280], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 10
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 288], rax
    mov rax, [rbp - 288]
    movzx rax, byte [rax]
    mov [rbp - 296], rax
    mov rax, [rbp - 296]
    push rax
    mov rax, 109
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 304], rax
    mov rax, [rbp - 304]
    test rax, rax
    jz ir_if_next_802
    mov rax, 0
    mov [rbp - 312], rax
ir_errdefer_ok_805:
ir_errdefer_end_806:
    mov rax, 0
    jmp Lis_benchmarks_exit
ir_if_next_802:
ir_if_end_801:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_809
    jmp ir_nonnull_810
ir_trap_null_809:

    sub rsp, 32

    lea rax, [rel Lstr_struct241]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_810:
    mov rax, 11
    mov [rbp - 320], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 11
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 328], rax
    mov rax, [rbp - 328]
    movzx rax, byte [rax]
    mov [rbp - 336], rax
    mov rax, [rbp - 336]
    push rax
    mov rax, 97
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 344], rax
    mov rax, [rbp - 344]
    test rax, rax
    jz ir_if_next_808
    mov rax, 0
    mov [rbp - 352], rax
ir_errdefer_ok_811:
ir_errdefer_end_812:
    mov rax, 0
    jmp Lis_benchmarks_exit
ir_if_next_808:
ir_if_end_807:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_815
    jmp ir_nonnull_816
ir_trap_null_815:

    sub rsp, 32

    lea rax, [rel Lstr_struct243]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_816:
    mov rax, 12
    mov [rbp - 360], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 12
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 368], rax
    mov rax, [rbp - 368]
    movzx rax, byte [rax]
    mov [rbp - 376], rax
    mov rax, [rbp - 376]
    push rax
    mov rax, 114
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 384], rax
    mov rax, [rbp - 384]
    test rax, rax
    jz ir_if_next_814
    mov rax, 0
    mov [rbp - 392], rax
ir_errdefer_ok_817:
ir_errdefer_end_818:
    mov rax, 0
    jmp Lis_benchmarks_exit
ir_if_next_814:
ir_if_end_813:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_821
    jmp ir_nonnull_822
ir_trap_null_821:

    sub rsp, 32

    lea rax, [rel Lstr_struct245]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_822:
    mov rax, 13
    mov [rbp - 400], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 13
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 408], rax
    mov rax, [rbp - 408]
    movzx rax, byte [rax]
    mov [rbp - 416], rax
    mov rax, [rbp - 416]
    push rax
    mov rax, 107
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 424], rax
    mov rax, [rbp - 424]
    test rax, rax
    jz ir_if_next_820
    mov rax, 0
    mov [rbp - 432], rax
ir_errdefer_ok_823:
ir_errdefer_end_824:
    mov rax, 0
    jmp Lis_benchmarks_exit
ir_if_next_820:
ir_if_end_819:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_827
    jmp ir_nonnull_828
ir_trap_null_827:

    sub rsp, 32

    lea rax, [rel Lstr_struct247]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_828:
    mov rax, 14
    mov [rbp - 440], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 14
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 448], rax
    mov rax, [rbp - 448]
    movzx rax, byte [rax]
    mov [rbp - 456], rax
    mov rax, [rbp - 456]
    push rax
    mov rax, 115
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 464], rax
    mov rax, [rbp - 464]
    test rax, rax
    jz ir_if_next_826
    mov rax, 0
    mov [rbp - 472], rax
ir_errdefer_ok_829:
ir_errdefer_end_830:
    mov rax, 0
    jmp Lis_benchmarks_exit
ir_if_next_826:
ir_if_end_825:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_833
    jmp ir_nonnull_834
ir_trap_null_833:

    sub rsp, 32

    lea rax, [rel Lstr_struct249]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_834:
    mov rax, 15
    mov [rbp - 480], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 15
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 488], rax
    mov rax, [rbp - 488]
    movzx rax, byte [rax]
    mov [rbp - 496], rax
    mov rax, [rbp - 496]
    push rax
    mov rax, 32
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 504], rax
    mov rax, [rbp - 504]
    test rax, rax
    jz ir_sc_false_837
ir_sc_rhs_835:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_839
    jmp ir_nonnull_840
ir_trap_null_839:

    sub rsp, 32

    lea rax, [rel Lstr_struct251]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_840:
    mov rax, 15
    mov [rbp - 512], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 15
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 520], rax
    mov rax, [rbp - 520]
    movzx rax, byte [rax]
    mov [rbp - 528], rax
    mov rax, [rbp - 528]
    push rax
    mov rax, 63
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 536], rax
    mov rax, [rbp - 536]
    test rax, rax
    jz ir_sc_false_837
ir_sc_true_836:
    mov rax, 1
    mov [rbp - 544], rax
    jmp ir_sc_end_838
ir_sc_false_837:
    mov rax, 0
    mov [rbp - 544], rax
ir_sc_end_838:
    mov rax, [rbp - 544]
    test rax, rax
    jz ir_if_next_832
    mov rax, 0
    mov [rbp - 560], rax
ir_errdefer_ok_841:
ir_errdefer_end_842:
    mov rax, 0
    jmp Lis_benchmarks_exit
ir_if_next_832:
ir_if_end_831:
    mov rax, 1
    mov [rbp - 568], rax
    jmp ir_errdefer_end_844
ir_errdefer_ok_843:
ir_errdefer_end_844:
    mov rax, 1
    jmp Lis_benchmarks_exit
Lis_benchmarks_exit:

    mov rsp, rbp
    pop rbp
    ret

global is_docs

is_docs:
    push rbp
    mov rbp, rsp
    sub rsp, 928

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_845:

    movsxd rax, dword [rbp - 16]
    push rax
    mov rax, 9
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_if_next_847
    mov rax, 0
    mov [rbp - 32], rax
ir_errdefer_ok_848:
ir_errdefer_end_849:
    mov rax, 0
    jmp Lis_docs_exit
ir_if_next_847:
ir_if_end_846:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_852
    jmp ir_nonnull_853
ir_trap_null_852:

    sub rsp, 32

    lea rax, [rel Lstr_struct253]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_853:
    mov rax, 4
    mov [rbp - 40], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 4
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    movzx rax, byte [rax]
    mov [rbp - 56], rax
    mov rax, [rbp - 56]
    push rax
    mov rax, 47
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 64], rax
    mov rax, [rbp - 64]
    test rax, rax
    jz ir_if_next_851
    mov rax, 0
    mov [rbp - 72], rax
ir_errdefer_ok_854:
ir_errdefer_end_855:
    mov rax, 0
    jmp Lis_docs_exit
ir_if_next_851:
ir_if_end_850:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_858
    jmp ir_nonnull_859
ir_trap_null_858:

    sub rsp, 32

    lea rax, [rel Lstr_struct255]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_859:
    mov rax, 5
    mov [rbp - 80], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 5
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 88], rax
    mov rax, [rbp - 88]
    movzx rax, byte [rax]
    mov [rbp - 96], rax
    mov rax, [rbp - 96]
    push rax
    mov rax, 100
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 104], rax
    mov rax, [rbp - 104]
    test rax, rax
    jz ir_if_next_857
    mov rax, 0
    mov [rbp - 112], rax
ir_errdefer_ok_860:
ir_errdefer_end_861:
    mov rax, 0
    jmp Lis_docs_exit
ir_if_next_857:
ir_if_end_856:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_864
    jmp ir_nonnull_865
ir_trap_null_864:

    sub rsp, 32

    lea rax, [rel Lstr_struct257]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_865:
    mov rax, 6
    mov [rbp - 120], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 6
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 128], rax
    mov rax, [rbp - 128]
    movzx rax, byte [rax]
    mov [rbp - 136], rax
    mov rax, [rbp - 136]
    push rax
    mov rax, 111
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 144], rax
    mov rax, [rbp - 144]
    test rax, rax
    jz ir_if_next_863
    mov rax, 0
    mov [rbp - 152], rax
ir_errdefer_ok_866:
ir_errdefer_end_867:
    mov rax, 0
    jmp Lis_docs_exit
ir_if_next_863:
ir_if_end_862:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_870
    jmp ir_nonnull_871
ir_trap_null_870:

    sub rsp, 32

    lea rax, [rel Lstr_struct259]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_871:
    mov rax, 7
    mov [rbp - 160], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 7
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 168], rax
    mov rax, [rbp - 168]
    movzx rax, byte [rax]
    mov [rbp - 176], rax
    mov rax, [rbp - 176]
    push rax
    mov rax, 99
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 184], rax
    mov rax, [rbp - 184]
    test rax, rax
    jz ir_if_next_869
    mov rax, 0
    mov [rbp - 192], rax
ir_errdefer_ok_872:
ir_errdefer_end_873:
    mov rax, 0
    jmp Lis_docs_exit
ir_if_next_869:
ir_if_end_868:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_876
    jmp ir_nonnull_877
ir_trap_null_876:

    sub rsp, 32

    lea rax, [rel Lstr_struct261]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_877:
    mov rax, 8
    mov [rbp - 200], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 208], rax
    mov rax, [rbp - 208]
    movzx rax, byte [rax]
    mov [rbp - 216], rax
    mov rax, [rbp - 216]
    push rax
    mov rax, 115
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 224], rax
    mov rax, [rbp - 224]
    test rax, rax
    jz ir_if_next_875
    mov rax, 0
    mov [rbp - 232], rax
ir_errdefer_ok_878:
ir_errdefer_end_879:
    mov rax, 0
    jmp Lis_docs_exit
ir_if_next_875:
ir_if_end_874:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_882
    jmp ir_nonnull_883
ir_trap_null_882:

    sub rsp, 32

    lea rax, [rel Lstr_struct263]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_883:
    mov rax, 9
    mov [rbp - 240], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 9
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 248], rax
    mov rax, [rbp - 248]
    movzx rax, byte [rax]
    mov [rbp - 256], rax
    mov rax, [rbp - 256]
    push rax
    mov rax, 32
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 264], rax
    mov rax, [rbp - 264]
    test rax, rax
    jz ir_sc_false_886
ir_sc_rhs_884:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_888
    jmp ir_nonnull_889
ir_trap_null_888:

    sub rsp, 32

    lea rax, [rel Lstr_struct265]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_889:
    mov rax, 9
    mov [rbp - 272], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 9
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 280], rax
    mov rax, [rbp - 280]
    movzx rax, byte [rax]
    mov [rbp - 288], rax
    mov rax, [rbp - 288]
    push rax
    mov rax, 63
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 296], rax
    mov rax, [rbp - 296]
    test rax, rax
    jz ir_sc_false_886
ir_sc_true_885:
    mov rax, 1
    mov [rbp - 304], rax
    jmp ir_sc_end_887
ir_sc_false_886:
    mov rax, 0
    mov [rbp - 304], rax
ir_sc_end_887:
    mov rax, [rbp - 304]
    test rax, rax
    jz ir_if_next_881
    mov rax, 0
    mov [rbp - 320], rax
ir_errdefer_ok_890:
ir_errdefer_end_891:
    mov rax, 0
    jmp Lis_docs_exit
ir_if_next_881:
ir_if_end_880:
    mov rax, 1
    mov [rbp - 328], rax
    jmp ir_errdefer_end_893
ir_errdefer_ok_892:
ir_errdefer_end_893:
    mov rax, 1
    jmp Lis_docs_exit
Lis_docs_exit:

    mov rsp, rbp
    pop rbp
    ret

global hex_char_val

hex_char_val:
    push rbp
    mov rbp, rsp
    sub rsp, 528

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_894:

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 48
    mov r10, rax
    pop rax
    cmp rax, r10
    setge al
    movzx rax, al
    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    test rax, rax
    jz ir_sc_false_899
ir_sc_rhs_897:

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 57
    mov r10, rax
    pop rax
    cmp rax, r10
    setle al
    movzx rax, al
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_sc_false_899
ir_sc_true_898:
    mov rax, 1
    mov [rbp - 32], rax
    jmp ir_sc_end_900
ir_sc_false_899:
    mov rax, 0
    mov [rbp - 32], rax
ir_sc_end_900:
    mov rax, [rbp - 32]
    test rax, rax
    jz ir_if_next_896

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 48
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    mov [rbp - 56], rax
    mov rax, [rbp - 48]
    test rax, rax
    jz ir_errdefer_ok_901
    jmp ir_errdefer_end_902
ir_errdefer_ok_901:
ir_errdefer_end_902:
    mov rax, [rbp - 48]
    jmp Lhex_char_val_exit
ir_if_next_896:
ir_if_end_895:

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 97
    mov r10, rax
    pop rax
    cmp rax, r10
    setge al
    movzx rax, al
    mov [rbp - 64], rax
    mov rax, [rbp - 64]
    test rax, rax
    jz ir_sc_false_907
ir_sc_rhs_905:

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 102
    mov r10, rax
    pop rax
    cmp rax, r10
    setle al
    movzx rax, al
    mov [rbp - 72], rax
    mov rax, [rbp - 72]
    test rax, rax
    jz ir_sc_false_907
ir_sc_true_906:
    mov rax, 1
    mov [rbp - 80], rax
    jmp ir_sc_end_908
ir_sc_false_907:
    mov rax, 0
    mov [rbp - 80], rax
ir_sc_end_908:
    mov rax, [rbp - 80]
    test rax, rax
    jz ir_if_next_904

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 97
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 96], rax
    mov rax, [rbp - 96]
    push rax
    mov rax, 10
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 104], rax
    mov rax, [rbp - 104]
    mov [rbp - 112], rax
    mov rax, [rbp - 104]
    test rax, rax
    jz ir_errdefer_ok_909
    jmp ir_errdefer_end_910
ir_errdefer_ok_909:
ir_errdefer_end_910:
    mov rax, [rbp - 104]
    jmp Lhex_char_val_exit
ir_if_next_904:
ir_if_end_903:

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 65
    mov r10, rax
    pop rax
    cmp rax, r10
    setge al
    movzx rax, al
    mov [rbp - 120], rax
    mov rax, [rbp - 120]
    test rax, rax
    jz ir_sc_false_915
ir_sc_rhs_913:

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 70
    mov r10, rax
    pop rax
    cmp rax, r10
    setle al
    movzx rax, al
    mov [rbp - 128], rax
    mov rax, [rbp - 128]
    test rax, rax
    jz ir_sc_false_915
ir_sc_true_914:
    mov rax, 1
    mov [rbp - 136], rax
    jmp ir_sc_end_916
ir_sc_false_915:
    mov rax, 0
    mov [rbp - 136], rax
ir_sc_end_916:
    mov rax, [rbp - 136]
    test rax, rax
    jz ir_if_next_912

    movsxd rax, dword [rbp - 8]
    push rax
    mov rax, 65
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 152], rax
    mov rax, [rbp - 152]
    push rax
    mov rax, 10
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 160], rax
    mov rax, [rbp - 160]
    mov [rbp - 168], rax
    mov rax, [rbp - 160]
    test rax, rax
    jz ir_errdefer_ok_917
    jmp ir_errdefer_end_918
ir_errdefer_ok_917:
ir_errdefer_end_918:
    mov rax, [rbp - 160]
    jmp Lhex_char_val_exit
ir_if_next_912:
ir_if_end_911:
    mov rax, -1
    mov [rbp - 176], rax
    mov rax, -1
    mov [rbp - 184], rax
    jmp ir_errdefer_end_920
ir_errdefer_ok_919:
ir_errdefer_end_920:
    mov rax, [rbp - 176]
    jmp Lhex_char_val_exit
Lhex_char_val_exit:

    mov rsp, rbp
    pop rbp
    ret

global extract_form_value

extract_form_value:
    push rbp
    mov rbp, rsp
    sub rsp, 2336

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx

    mov [rbp - 24], r8

    mov [rbp - 32], r9

    mov rax, [rbp + 48]
    mov [rbp - 40], rax

    mov rax, [rbp + 56]
    mov [rbp - 48], rax

    mov rax, [rbp + 64]
    mov [rbp - 56], rax


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_921:

    movsxd rax, dword [rbp - 16]

    mov dword [rbp - 60], eax

    movsxd rax, dword [rbp - 16]
    push rax

    movsxd rax, dword [rbp - 24]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 96], rax
    mov rax, [rbp - 96]

    mov dword [rbp - 64], eax
ir_while_922:

    movsxd rax, dword [rbp - 60]
    push rax

    movsxd rax, dword [rbp - 40]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 104], rax
    mov rax, [rbp - 104]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 112], rax
    mov rax, [rbp - 112]
    push rax

    movsxd rax, dword [rbp - 64]
    mov r10, rax
    pop rax
    cmp rax, r10
    setle al
    movzx rax, al
    mov [rbp - 120], rax
    mov rax, [rbp - 120]
    test rax, rax
    jz ir_while_end_923
    mov rax, 1

    mov dword [rbp - 68], eax
    mov rax, 0

    mov dword [rbp - 72], eax
ir_while_924:

    movsxd rax, dword [rbp - 72]
    push rax

    movsxd rax, dword [rbp - 40]
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 128], rax
    mov rax, [rbp - 128]
    test rax, rax
    jz ir_while_end_925

    movsxd rax, dword [rbp - 60]
    push rax

    movsxd rax, dword [rbp - 72]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 136], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_928
    jmp ir_nonnull_929
ir_trap_null_928:

    sub rsp, 32

    lea rax, [rel Lstr_struct267]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_929:
    mov rax, [rbp - 136]
    mov [rbp - 144], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 136]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 152], rax
    mov rax, [rbp - 152]
    movzx rax, byte [rax]
    mov [rbp - 160], rax

    mov rax, qword [rbp - 32]
    test rax, rax
    jz ir_trap_null_930
    jmp ir_nonnull_931
ir_trap_null_930:

    sub rsp, 32

    lea rax, [rel Lstr_struct269]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_931:

    movsxd rax, dword [rbp - 72]
    mov [rbp - 168], rax

    mov rax, qword [rbp - 32]
    push rax
    mov rax, [rbp - 168]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 176], rax
    mov rax, [rbp - 176]
    movzx rax, byte [rax]
    mov [rbp - 184], rax
    mov rax, [rbp - 160]
    push rax
    mov rax, [rbp - 184]
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 192], rax
    mov rax, [rbp - 192]
    test rax, rax
    jz ir_if_next_927
    mov rax, 0

    mov dword [rbp - 68], eax
    jmp ir_while_end_925
ir_if_next_927:
ir_if_end_926:

    movsxd rax, dword [rbp - 72]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 200], rax
    mov rax, [rbp - 200]

    mov dword [rbp - 72], eax
    jmp ir_while_924
ir_while_end_925:

    movsxd rax, dword [rbp - 68]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 208], rax
    mov rax, [rbp - 208]
    test rax, rax
    jz ir_sc_false_936
ir_sc_rhs_934:

    movsxd rax, dword [rbp - 60]
    push rax

    movsxd rax, dword [rbp - 40]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 216], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_938
    jmp ir_nonnull_939
ir_trap_null_938:

    sub rsp, 32

    lea rax, [rel Lstr_struct271]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_939:
    mov rax, [rbp - 216]
    mov [rbp - 224], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 216]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 232], rax
    mov rax, [rbp - 232]
    movzx rax, byte [rax]
    mov [rbp - 240], rax
    mov rax, [rbp - 240]
    push rax
    mov rax, 61
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 248], rax
    mov rax, [rbp - 248]
    test rax, rax
    jz ir_sc_false_936
ir_sc_true_935:
    mov rax, 1
    mov [rbp - 256], rax
    jmp ir_sc_end_937
ir_sc_false_936:
    mov rax, 0
    mov [rbp - 256], rax
ir_sc_end_937:
    mov rax, [rbp - 256]
    test rax, rax
    jz ir_if_next_933

    movsxd rax, dword [rbp - 60]
    push rax

    movsxd rax, dword [rbp - 40]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 272], rax
    mov rax, [rbp - 272]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 280], rax
    mov rax, [rbp - 280]

    mov dword [rbp - 60], eax
    mov rax, 0

    mov dword [rbp - 76], eax
ir_while_940:

    movsxd rax, dword [rbp - 60]
    push rax

    movsxd rax, dword [rbp - 64]
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 288], rax
    mov rax, [rbp - 288]
    test rax, rax
    jz ir_sc_false_944
ir_sc_rhs_942:

    movsxd rax, dword [rbp - 56]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 296], rax

    movsxd rax, dword [rbp - 76]
    push rax
    mov rax, [rbp - 296]
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 304], rax
    mov rax, [rbp - 304]
    test rax, rax
    jz ir_sc_false_944
ir_sc_true_943:
    mov rax, 1
    mov [rbp - 312], rax
    jmp ir_sc_end_945
ir_sc_false_944:
    mov rax, 0
    mov [rbp - 312], rax
ir_sc_end_945:
    mov rax, [rbp - 312]
    test rax, rax
    jz ir_while_end_941

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_948
    jmp ir_nonnull_949
ir_trap_null_948:

    sub rsp, 32

    lea rax, [rel Lstr_struct273]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_949:

    movsxd rax, dword [rbp - 60]
    mov [rbp - 328], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 328]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 336], rax
    mov rax, [rbp - 336]
    movzx rax, byte [rax]
    mov [rbp - 344], rax
    mov rax, [rbp - 344]
    push rax
    mov rax, 38
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 352], rax
    mov rax, [rbp - 352]
    test rax, rax
    jz ir_if_next_947
    jmp ir_while_end_941
ir_if_next_947:
ir_if_end_946:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_952
    jmp ir_nonnull_953
ir_trap_null_952:

    sub rsp, 32

    lea rax, [rel Lstr_struct275]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_953:

    movsxd rax, dword [rbp - 60]
    mov [rbp - 360], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 360]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 368], rax
    mov rax, [rbp - 368]
    movzx rax, byte [rax]
    mov [rbp - 376], rax
    mov rax, [rbp - 376]
    push rax
    mov rax, 43
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 384], rax
    mov rax, [rbp - 384]
    test rax, rax
    jz ir_if_next_951

    mov rax, qword [rbp - 48]
    test rax, rax
    jz ir_trap_null_954
    jmp ir_nonnull_955
ir_trap_null_954:

    sub rsp, 32

    lea rax, [rel Lstr_struct277]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_955:

    movsxd rax, dword [rbp - 76]
    mov [rbp - 392], rax

    mov rax, qword [rbp - 48]
    push rax
    mov rax, [rbp - 392]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 400], rax
    mov rax, [rbp - 400]
    push rax
    mov rax, 32
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    movsxd rax, dword [rbp - 76]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 416], rax
    mov rax, [rbp - 416]

    mov dword [rbp - 76], eax

    movsxd rax, dword [rbp - 60]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 424], rax
    mov rax, [rbp - 424]

    mov dword [rbp - 60], eax
    jmp ir_if_end_950
ir_if_next_951:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_957
    jmp ir_nonnull_958
ir_trap_null_957:

    sub rsp, 32

    lea rax, [rel Lstr_struct279]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_958:

    movsxd rax, dword [rbp - 60]
    mov [rbp - 432], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 432]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 440], rax
    mov rax, [rbp - 440]
    movzx rax, byte [rax]
    mov [rbp - 448], rax
    mov rax, [rbp - 448]
    push rax
    mov rax, 37
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 456], rax
    mov rax, [rbp - 456]
    test rax, rax
    jz ir_sc_false_961
ir_sc_rhs_959:

    movsxd rax, dword [rbp - 60]
    push rax
    mov rax, 2
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 464], rax
    mov rax, [rbp - 464]
    push rax

    movsxd rax, dword [rbp - 64]
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 472], rax
    mov rax, [rbp - 472]
    test rax, rax
    jz ir_sc_false_961
ir_sc_true_960:
    mov rax, 1
    mov [rbp - 480], rax
    jmp ir_sc_end_962
ir_sc_false_961:
    mov rax, 0
    mov [rbp - 480], rax
ir_sc_end_962:
    mov rax, [rbp - 480]
    test rax, rax
    jz ir_if_next_956

    movsxd rax, dword [rbp - 60]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 496], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_963
    jmp ir_nonnull_964
ir_trap_null_963:

    sub rsp, 32

    lea rax, [rel Lstr_struct281]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_964:
    mov rax, [rbp - 496]
    mov [rbp - 504], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 496]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 512], rax
    mov rax, [rbp - 512]
    movzx rax, byte [rax]
    mov [rbp - 520], rax

    sub rsp, 32
    mov rax, [rbp - 520]
    mov rcx, rax
    call hex_char_val
    add rsp, 32


    mov [rbp - 528], rax
    mov rax, [rbp - 528]

    mov dword [rbp - 80], eax

    movsxd rax, dword [rbp - 60]
    push rax
    mov rax, 2
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 536], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_965
    jmp ir_nonnull_966
ir_trap_null_965:

    sub rsp, 32

    lea rax, [rel Lstr_struct283]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_966:
    mov rax, [rbp - 536]
    mov [rbp - 544], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 536]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 552], rax
    mov rax, [rbp - 552]
    movzx rax, byte [rax]
    mov [rbp - 560], rax

    sub rsp, 32
    mov rax, [rbp - 560]
    mov rcx, rax
    call hex_char_val
    add rsp, 32


    mov [rbp - 568], rax
    mov rax, [rbp - 568]

    mov dword [rbp - 84], eax

    movsxd rax, dword [rbp - 80]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setge al
    movzx rax, al
    mov [rbp - 576], rax
    mov rax, [rbp - 576]
    test rax, rax
    jz ir_sc_false_971
ir_sc_rhs_969:

    movsxd rax, dword [rbp - 84]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setge al
    movzx rax, al
    mov [rbp - 584], rax
    mov rax, [rbp - 584]
    test rax, rax
    jz ir_sc_false_971
ir_sc_true_970:
    mov rax, 1
    mov [rbp - 592], rax
    jmp ir_sc_end_972
ir_sc_false_971:
    mov rax, 0
    mov [rbp - 592], rax
ir_sc_end_972:
    mov rax, [rbp - 592]
    test rax, rax
    jz ir_if_next_968

    movsxd rax, dword [rbp - 80]
    push rax
    mov rax, 4
    mov r10, rax
    pop rax
    mov rcx, r10
    shl rax, cl
    mov [rbp - 608], rax
    mov rax, [rbp - 608]
    push rax

    movsxd rax, dword [rbp - 84]
    mov r10, rax
    pop rax
    or rax, r10
    mov [rbp - 616], rax

    mov rax, qword [rbp - 48]
    test rax, rax
    jz ir_trap_null_973
    jmp ir_nonnull_974
ir_trap_null_973:

    sub rsp, 32

    lea rax, [rel Lstr_struct285]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_974:

    movsxd rax, dword [rbp - 76]
    mov [rbp - 624], rax

    mov rax, qword [rbp - 48]
    push rax
    mov rax, [rbp - 624]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 632], rax
    mov rax, [rbp - 632]
    push rax
    mov rax, [rbp - 616]
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    movsxd rax, dword [rbp - 76]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 648], rax
    mov rax, [rbp - 648]

    mov dword [rbp - 76], eax

    movsxd rax, dword [rbp - 60]
    push rax
    mov rax, 3
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 656], rax
    mov rax, [rbp - 656]

    mov dword [rbp - 60], eax
    jmp ir_if_end_967
ir_if_next_968:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_975
    jmp ir_nonnull_976
ir_trap_null_975:

    sub rsp, 32

    lea rax, [rel Lstr_struct287]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_976:

    movsxd rax, dword [rbp - 60]
    mov [rbp - 664], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 664]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 672], rax
    mov rax, [rbp - 672]
    movzx rax, byte [rax]
    mov [rbp - 680], rax

    mov rax, qword [rbp - 48]
    test rax, rax
    jz ir_trap_null_977
    jmp ir_nonnull_978
ir_trap_null_977:

    sub rsp, 32

    lea rax, [rel Lstr_struct289]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_978:

    movsxd rax, dword [rbp - 76]
    mov [rbp - 688], rax

    mov rax, qword [rbp - 48]
    push rax
    mov rax, [rbp - 688]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 696], rax
    mov rax, [rbp - 696]
    push rax
    mov rax, [rbp - 680]
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    movsxd rax, dword [rbp - 76]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 712], rax
    mov rax, [rbp - 712]

    mov dword [rbp - 76], eax

    movsxd rax, dword [rbp - 60]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 720], rax
    mov rax, [rbp - 720]

    mov dword [rbp - 60], eax
ir_if_end_967:
    jmp ir_if_end_950
ir_if_next_956:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_979
    jmp ir_nonnull_980
ir_trap_null_979:

    sub rsp, 32

    lea rax, [rel Lstr_struct291]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_980:

    movsxd rax, dword [rbp - 60]
    mov [rbp - 728], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 728]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 736], rax
    mov rax, [rbp - 736]
    movzx rax, byte [rax]
    mov [rbp - 744], rax

    mov rax, qword [rbp - 48]
    test rax, rax
    jz ir_trap_null_981
    jmp ir_nonnull_982
ir_trap_null_981:

    sub rsp, 32

    lea rax, [rel Lstr_struct293]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_982:

    movsxd rax, dword [rbp - 76]
    mov [rbp - 752], rax

    mov rax, qword [rbp - 48]
    push rax
    mov rax, [rbp - 752]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 760], rax
    mov rax, [rbp - 760]
    push rax
    mov rax, [rbp - 744]
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    movsxd rax, dword [rbp - 76]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 776], rax
    mov rax, [rbp - 776]

    mov dword [rbp - 76], eax

    movsxd rax, dword [rbp - 60]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 784], rax
    mov rax, [rbp - 784]

    mov dword [rbp - 60], eax
ir_if_end_950:
    jmp ir_while_940
ir_while_end_941:

    mov rax, qword [rbp - 48]
    test rax, rax
    jz ir_trap_null_983
    jmp ir_nonnull_984
ir_trap_null_983:

    sub rsp, 32

    lea rax, [rel Lstr_struct295]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_984:

    movsxd rax, dword [rbp - 76]
    mov [rbp - 792], rax

    mov rax, qword [rbp - 48]
    push rax
    mov rax, [rbp - 792]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 800], rax
    mov rax, [rbp - 800]
    push rax
    mov rax, 0
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    movsxd rax, dword [rbp - 76]
    mov [rbp - 816], rax
    mov rax, [rbp - 816]
    test rax, rax
    jz ir_errdefer_ok_985
    jmp ir_errdefer_end_986
ir_errdefer_ok_985:
ir_errdefer_end_986:

    movsxd rax, dword [rbp - 76]
    jmp Lextract_form_value_exit
ir_if_next_933:
ir_if_end_932:

    movsxd rax, dword [rbp - 60]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 824], rax
    mov rax, [rbp - 824]

    mov dword [rbp - 60], eax
    jmp ir_while_922
ir_while_end_923:
    mov rax, 0
    mov [rbp - 832], rax
ir_errdefer_ok_987:
ir_errdefer_end_988:
    mov rax, 0
    jmp Lextract_form_value_exit
Lextract_form_value_exit:

    mov rsp, rbp
    pop rbp
    ret

global dbg

dbg:
    push rbp
    mov rbp, rsp
    sub rsp, 208

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_989:

    movsxd rax, dword [rel dbg_on]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_if_next_991
    mov rax, 0
    mov [rbp - 32], rax
ir_errdefer_ok_992:
ir_errdefer_end_993:
    jmp Ldbg_exit
ir_if_next_991:
ir_if_end_990:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    call print
    add rsp, 32

    mov [rbp - 40], rax

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax
    call print_int
    add rsp, 32

    mov [rbp - 48], rax

    sub rsp, 32
    mov rax, 10
    mov rcx, rax
    call putchar
    add rsp, 32


    mov [rbp - 56], rax

    sub rsp, 32
    call get_stdout
    add rsp, 32

    mov [rbp - 64], rax

    sub rsp, 32
    mov rax, [rbp - 64]
    mov rcx, rax
    call fflush
    add rsp, 32


    mov [rbp - 72], rax
    mov rax, 0
    mov [rbp - 80], rax
ir_errdefer_ok_994:
ir_errdefer_end_995:
    jmp Ldbg_exit
Ldbg_exit:

    mov rsp, rbp
    pop rbp
    ret

global dbg_msg

dbg_msg:
    push rbp
    mov rbp, rsp
    sub rsp, 160

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_996:

    movsxd rax, dword [rel dbg_on]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    test rax, rax
    jz ir_if_next_998
    mov rax, 0
    mov [rbp - 24], rax
ir_errdefer_ok_999:
ir_errdefer_end_1000:
    jmp Ldbg_msg_exit
ir_if_next_998:
ir_if_end_997:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    call println
    add rsp, 32

    mov [rbp - 32], rax

    sub rsp, 32
    call get_stdout
    add rsp, 32

    mov [rbp - 40], rax

    sub rsp, 32
    mov rax, [rbp - 40]
    mov rcx, rax
    call fflush
    add rsp, 32


    mov [rbp - 48], rax
    mov rax, 0
    mov [rbp - 56], rax
ir_errdefer_ok_1001:
ir_errdefer_end_1002:
    jmp Ldbg_msg_exit
Ldbg_msg_exit:

    mov rsp, rbp
    pop rbp
    ret

global int_to_dec

int_to_dec:
    push rbp
    mov rbp, rsp
    sub rsp, 848

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_1003:
    mov rax, 0

    mov dword [rbp - 100], eax

    mov rax, qword [rbp - 16]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 128], rax
    mov rax, [rbp - 128]
    test rax, rax
    jz ir_if_next_1005

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1006
    jmp ir_nonnull_1007
ir_trap_null_1006:

    sub rsp, 32

    lea rax, [rel Lstr_struct297]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1007:
    mov rax, 0
    mov [rbp - 136], rax

    mov rax, qword [rbp - 8]
    mov [rbp - 144], rax
    mov rax, [rbp - 144]
    push rax
    mov rax, 48
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    mov rax, 1
    mov [rbp - 160], rax
    jmp ir_errdefer_end_1009
ir_errdefer_ok_1008:
ir_errdefer_end_1009:
    mov rax, 1
    jmp Lint_to_dec_exit
ir_if_next_1005:
ir_if_end_1004:
ir_while_1010:

    mov rax, qword [rbp - 16]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setg al
    movzx rax, al
    mov [rbp - 168], rax
    mov rax, [rbp - 168]
    test rax, rax
    jz ir_while_end_1011

    mov rax, qword [rbp - 16]
    push rax
    mov rax, 10
    mov r10, rax
    pop rax
    cqo
    idiv r10
    mov [rbp - 176], rax
    mov rax, [rbp - 176]
    push rax
    mov rax, 10
    mov r10, rax
    pop rax
    imul rax, r10
    mov [rbp - 184], rax

    mov rax, qword [rbp - 16]
    push rax
    mov rax, [rbp - 184]
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 192], rax
    mov rax, [rbp - 192]

    mov qword [rbp - 112], rax

    movsxd rax, dword [rbp - 100]
    push rax
    mov rax, 20
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 200], rax
    mov rax, [rbp - 200]
    test rax, rax
    jz ir_trap_bounds_1012
    jmp ir_in_bounds_1013
ir_trap_bounds_1012:

    sub rsp, 32

    lea rax, [rel Lstr_struct299]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1013:

    movsxd rax, dword [rbp - 100]
    push rax
    mov rax, 4
    mov r10, rax
    pop rax
    imul rax, r10
    mov [rbp - 208], rax

    lea rax, [rbp - 96]
    push rax
    mov rax, [rbp - 208]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 216], rax
    mov rax, [rbp - 216]
    push rax

    mov rax, qword [rbp - 112]
    mov rcx, rax
    pop rax
    mov dword [rax], ecx

    movsxd rax, dword [rbp - 100]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 232], rax
    mov rax, [rbp - 232]

    mov dword [rbp - 100], eax

    mov rax, qword [rbp - 16]
    push rax
    mov rax, 10
    mov r10, rax
    pop rax
    cqo
    idiv r10
    mov [rbp - 240], rax
    mov rax, [rbp - 240]

    mov qword [rbp - 16], rax
    jmp ir_while_1010
ir_while_end_1011:

    movsxd rax, dword [rbp - 100]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 248], rax
    mov rax, [rbp - 248]

    mov dword [rbp - 116], eax
ir_while_1014:

    movsxd rax, dword [rbp - 116]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setge al
    movzx rax, al
    mov [rbp - 256], rax
    mov rax, [rbp - 256]
    test rax, rax
    jz ir_while_end_1015

    movsxd rax, dword [rbp - 116]
    push rax
    mov rax, 20
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 264], rax
    mov rax, [rbp - 264]
    test rax, rax
    jz ir_trap_bounds_1016
    jmp ir_in_bounds_1017
ir_trap_bounds_1016:

    sub rsp, 32

    lea rax, [rel Lstr_struct301]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1017:

    movsxd rax, dword [rbp - 116]
    push rax
    mov rax, 4
    mov r10, rax
    pop rax
    imul rax, r10
    mov [rbp - 272], rax

    lea rax, [rbp - 96]
    push rax
    mov rax, [rbp - 272]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 280], rax
    mov rax, [rbp - 280]
    mov eax, dword [rax]
    mov [rbp - 288], rax

    sub rsp, 32
    mov rax, [rbp - 288]
    mov rcx, rax
    call digit_to_char
    add rsp, 32


    mov [rbp - 296], rax

    movsxd rax, dword [rbp - 100]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 304], rax
    mov rax, [rbp - 304]
    push rax

    movsxd rax, dword [rbp - 116]
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 312], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1018
    jmp ir_nonnull_1019
ir_trap_null_1018:

    sub rsp, 32

    lea rax, [rel Lstr_struct303]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1019:
    mov rax, [rbp - 312]
    mov [rbp - 320], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 312]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 328], rax
    mov rax, [rbp - 328]
    push rax
    mov rax, [rbp - 296]
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    movsxd rax, dword [rbp - 116]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 344], rax
    mov rax, [rbp - 344]

    mov dword [rbp - 116], eax
    jmp ir_while_1014
ir_while_end_1015:

    movsxd rax, dword [rbp - 100]
    mov [rbp - 352], rax
    mov rax, [rbp - 352]
    test rax, rax
    jz ir_errdefer_ok_1020
    jmp ir_errdefer_end_1021
ir_errdefer_ok_1020:
ir_errdefer_end_1021:

    movsxd rax, dword [rbp - 100]
    jmp Lint_to_dec_exit
Lint_to_dec_exit:

    mov rsp, rbp
    pop rbp
    ret

global con_writeln

con_writeln:
    push rbp
    mov rbp, rsp
    sub rsp, 560

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_1022:
    mov rax, -11
    mov [rbp - 32], rax

    sub rsp, 32
    mov rax, -11
    mov rcx, rax
    call GetStdHandle
    add rsp, 32

    mov [rbp - 40], rax
    mov rax, [rbp - 40]

    mov qword [rbp - 16], rax
    mov rax, -1
    mov [rbp - 48], rax

    mov rax, qword [rbp - 16]
    push rax
    mov rax, -1
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 56], rax
    mov rax, [rbp - 56]
    test rax, rax
    jz ir_sc_false_1027
ir_sc_rhs_1025:

    mov rax, qword [rbp - 16]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 64], rax
    mov rax, [rbp - 64]
    test rax, rax
    jz ir_sc_false_1027
ir_sc_true_1026:
    mov rax, 1
    mov [rbp - 72], rax
    jmp ir_sc_end_1028
ir_sc_false_1027:
    mov rax, 0
    mov [rbp - 72], rax
ir_sc_end_1028:
    mov rax, [rbp - 72]
    test rax, rax
    jz ir_if_next_1024

    mov rax, qword [rbp - 8]
    mov [rbp - 88], rax
    mov rax, [rbp - 88]
    mov rax, qword [rax]
    mov [rbp - 96], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 104], rax
    mov rax, [rbp - 104]
    mov rax, qword [rax]
    mov [rbp - 112], rax
    mov rax, 1
    mov [rbp - 120], rax
    jmp ir_in_bounds_1030
ir_trap_bounds_1029:

    sub rsp, 32

    lea rax, [rel Lstr_struct305]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1030:
    mov rax, 0
    mov [rbp - 128], rax

    lea rax, [rbp - 20]
    mov [rbp - 136], rax

    sub rsp, 48
    mov rax, 0
    mov [rsp + 32], rax

    mov rax, qword [rbp - 16]
    mov rcx, rax
    mov rax, [rbp - 96]
    mov rdx, rax
    mov rax, [rbp - 112]
    mov r8, rax
    mov rax, [rbp - 136]
    mov r9, rax
    call WriteFile
    add rsp, 48


    mov [rbp - 144], rax

    lea rax, [rel crlf]
    mov [rbp - 152], rax
    mov rax, [rbp - 152]
    mov rax, qword [rax]
    mov [rbp - 160], rax
    mov rax, 1
    mov [rbp - 168], rax
    jmp ir_in_bounds_1032
ir_trap_bounds_1031:

    sub rsp, 32

    lea rax, [rel Lstr_struct307]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1032:
    mov rax, 0
    mov [rbp - 176], rax

    lea rax, [rbp - 20]
    mov [rbp - 184], rax

    sub rsp, 48
    mov rax, 0
    mov [rsp + 32], rax

    mov rax, qword [rbp - 16]
    mov rcx, rax
    mov rax, [rbp - 160]
    mov rdx, rax
    mov rax, 2
    mov r8, rax
    mov rax, [rbp - 184]
    mov r9, rax
    call WriteFile
    add rsp, 48


    mov [rbp - 192], rax
    jmp ir_if_end_1023
ir_if_next_1024:
ir_if_end_1023:
    mov rax, 0
    mov [rbp - 200], rax
ir_errdefer_ok_1033:
ir_errdefer_end_1034:
    jmp Lcon_writeln_exit
Lcon_writeln_exit:

    mov rsp, rbp
    pop rbp
    ret

global parse_content_length

parse_content_length:
    push rbp
    mov rbp, rsp
    sub rsp, 1552

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_1035:
    mov rax, 0

    mov dword [rbp - 20], eax
ir_while_1036:

    movsxd rax, dword [rbp - 20]
    push rax
    mov rax, 15
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 56], rax
    mov rax, [rbp - 56]
    push rax

    movsxd rax, dword [rbp - 16]
    mov r10, rax
    pop rax
    cmp rax, r10
    setle al
    movzx rax, al
    mov [rbp - 64], rax
    mov rax, [rbp - 64]
    test rax, rax
    jz ir_while_end_1037
    mov rax, 1

    mov dword [rbp - 24], eax
    mov rax, 0

    mov dword [rbp - 28], eax
ir_while_1038:

    movsxd rax, dword [rbp - 28]
    push rax
    mov rax, 15
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 72], rax
    mov rax, [rbp - 72]
    test rax, rax
    jz ir_while_end_1039

    movsxd rax, dword [rbp - 20]
    push rax

    movsxd rax, dword [rbp - 28]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 80], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1040
    jmp ir_nonnull_1041
ir_trap_null_1040:

    sub rsp, 32

    lea rax, [rel Lstr_struct309]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1041:
    mov rax, [rbp - 80]
    mov [rbp - 88], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 80]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 96], rax
    mov rax, [rbp - 96]
    movzx rax, byte [rax]
    mov [rbp - 104], rax
    mov rax, [rbp - 104]

    mov dword [rbp - 32], eax

    lea rax, [rel hdr_content_length]
    mov [rbp - 112], rax
    mov rax, [rbp - 112]
    mov rax, qword [rax]
    mov [rbp - 120], rax
    mov rax, [rbp - 120]
    test rax, rax
    jz ir_trap_null_1042
    jmp ir_nonnull_1043
ir_trap_null_1042:

    sub rsp, 32

    lea rax, [rel Lstr_struct311]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1043:

    movsxd rax, dword [rbp - 28]
    mov [rbp - 128], rax
    mov rax, [rbp - 120]
    push rax
    mov rax, [rbp - 128]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 136], rax
    mov rax, [rbp - 136]
    movzx rax, byte [rax]
    mov [rbp - 144], rax
    mov rax, [rbp - 144]

    mov dword [rbp - 36], eax

    lea rax, [rel hdr_content_length_lo]
    mov [rbp - 152], rax
    mov rax, [rbp - 152]
    mov rax, qword [rax]
    mov [rbp - 160], rax
    mov rax, [rbp - 160]
    test rax, rax
    jz ir_trap_null_1044
    jmp ir_nonnull_1045
ir_trap_null_1044:

    sub rsp, 32

    lea rax, [rel Lstr_struct313]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1045:

    movsxd rax, dword [rbp - 28]
    mov [rbp - 168], rax
    mov rax, [rbp - 160]
    push rax
    mov rax, [rbp - 168]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 176], rax
    mov rax, [rbp - 176]
    movzx rax, byte [rax]
    mov [rbp - 184], rax
    mov rax, [rbp - 184]

    mov dword [rbp - 40], eax

    movsxd rax, dword [rbp - 32]
    push rax

    movsxd rax, dword [rbp - 36]
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 192], rax
    mov rax, [rbp - 192]
    test rax, rax
    jz ir_sc_false_1050
ir_sc_rhs_1048:

    movsxd rax, dword [rbp - 32]
    push rax

    movsxd rax, dword [rbp - 40]
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 200], rax
    mov rax, [rbp - 200]
    test rax, rax
    jz ir_sc_false_1050
ir_sc_true_1049:
    mov rax, 1
    mov [rbp - 208], rax
    jmp ir_sc_end_1051
ir_sc_false_1050:
    mov rax, 0
    mov [rbp - 208], rax
ir_sc_end_1051:
    mov rax, [rbp - 208]
    test rax, rax
    jz ir_if_next_1047
    mov rax, 0

    mov dword [rbp - 24], eax
    jmp ir_while_end_1039
ir_if_next_1047:
ir_if_end_1046:

    movsxd rax, dword [rbp - 28]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 224], rax
    mov rax, [rbp - 224]

    mov dword [rbp - 28], eax
    jmp ir_while_1038
ir_while_end_1039:

    movsxd rax, dword [rbp - 24]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 232], rax
    mov rax, [rbp - 232]
    test rax, rax
    jz ir_if_next_1053

    movsxd rax, dword [rbp - 20]
    push rax
    mov rax, 15
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 240], rax
    mov rax, [rbp - 240]

    mov dword [rbp - 20], eax
ir_while_1054:

    movsxd rax, dword [rbp - 20]
    push rax

    movsxd rax, dword [rbp - 16]
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 248], rax
    mov rax, [rbp - 248]
    test rax, rax
    jz ir_sc_false_1058
ir_sc_rhs_1056:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1060
    jmp ir_nonnull_1061
ir_trap_null_1060:

    sub rsp, 32

    lea rax, [rel Lstr_struct315]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1061:

    movsxd rax, dword [rbp - 20]
    mov [rbp - 256], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 256]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 264], rax
    mov rax, [rbp - 264]
    movzx rax, byte [rax]
    mov [rbp - 272], rax
    mov rax, [rbp - 272]
    push rax
    mov rax, 32
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 280], rax
    mov rax, [rbp - 280]
    test rax, rax
    jz ir_sc_rhs_1062
    jmp ir_sc_true_1063
ir_sc_rhs_1062:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1066
    jmp ir_nonnull_1067
ir_trap_null_1066:

    sub rsp, 32

    lea rax, [rel Lstr_struct317]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1067:

    movsxd rax, dword [rbp - 20]
    mov [rbp - 288], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 288]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 296], rax
    mov rax, [rbp - 296]
    movzx rax, byte [rax]
    mov [rbp - 304], rax
    mov rax, [rbp - 304]
    push rax
    mov rax, 9
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 312], rax
    mov rax, [rbp - 312]
    test rax, rax
    jz ir_sc_false_1064
ir_sc_true_1063:
    mov rax, 1
    mov [rbp - 320], rax
    jmp ir_sc_end_1065
ir_sc_false_1064:
    mov rax, 0
    mov [rbp - 320], rax
ir_sc_end_1065:
    mov rax, [rbp - 320]
    test rax, rax
    jz ir_sc_false_1058
ir_sc_true_1057:
    mov rax, 1
    mov [rbp - 336], rax
    jmp ir_sc_end_1059
ir_sc_false_1058:
    mov rax, 0
    mov [rbp - 336], rax
ir_sc_end_1059:
    mov rax, [rbp - 336]
    test rax, rax
    jz ir_while_end_1055

    movsxd rax, dword [rbp - 20]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 352], rax
    mov rax, [rbp - 352]

    mov dword [rbp - 20], eax
    jmp ir_while_1054
ir_while_end_1055:
    mov rax, 0

    mov dword [rbp - 44], eax
ir_while_1068:

    movsxd rax, dword [rbp - 20]
    push rax

    movsxd rax, dword [rbp - 16]
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 360], rax
    mov rax, [rbp - 360]
    test rax, rax
    jz ir_sc_false_1072
ir_sc_rhs_1070:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1074
    jmp ir_nonnull_1075
ir_trap_null_1074:

    sub rsp, 32

    lea rax, [rel Lstr_struct319]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1075:

    movsxd rax, dword [rbp - 20]
    mov [rbp - 368], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 368]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 376], rax
    mov rax, [rbp - 376]
    movzx rax, byte [rax]
    mov [rbp - 384], rax
    mov rax, [rbp - 384]
    push rax
    mov rax, 48
    mov r10, rax
    pop rax
    cmp rax, r10
    setge al
    movzx rax, al
    mov [rbp - 392], rax
    mov rax, [rbp - 392]
    test rax, rax
    jz ir_sc_false_1072
ir_sc_true_1071:
    mov rax, 1
    mov [rbp - 400], rax
    jmp ir_sc_end_1073
ir_sc_false_1072:
    mov rax, 0
    mov [rbp - 400], rax
ir_sc_end_1073:
    mov rax, [rbp - 400]
    test rax, rax
    jz ir_sc_false_1078
ir_sc_rhs_1076:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1080
    jmp ir_nonnull_1081
ir_trap_null_1080:

    sub rsp, 32

    lea rax, [rel Lstr_struct321]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1081:

    movsxd rax, dword [rbp - 20]
    mov [rbp - 416], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 416]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 424], rax
    mov rax, [rbp - 424]
    movzx rax, byte [rax]
    mov [rbp - 432], rax
    mov rax, [rbp - 432]
    push rax
    mov rax, 57
    mov r10, rax
    pop rax
    cmp rax, r10
    setle al
    movzx rax, al
    mov [rbp - 440], rax
    mov rax, [rbp - 440]
    test rax, rax
    jz ir_sc_false_1078
ir_sc_true_1077:
    mov rax, 1
    mov [rbp - 448], rax
    jmp ir_sc_end_1079
ir_sc_false_1078:
    mov rax, 0
    mov [rbp - 448], rax
ir_sc_end_1079:
    mov rax, [rbp - 448]
    test rax, rax
    jz ir_while_end_1069

    movsxd rax, dword [rbp - 44]
    push rax
    mov rax, 10
    mov r10, rax
    pop rax
    imul rax, r10
    mov [rbp - 464], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1082
    jmp ir_nonnull_1083
ir_trap_null_1082:

    sub rsp, 32

    lea rax, [rel Lstr_struct323]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1083:

    movsxd rax, dword [rbp - 20]
    mov [rbp - 472], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 472]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 480], rax
    mov rax, [rbp - 480]
    movzx rax, byte [rax]
    mov [rbp - 488], rax
    mov rax, [rbp - 488]
    push rax
    mov rax, 48
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 496], rax
    mov rax, [rbp - 464]
    push rax
    mov rax, [rbp - 496]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 504], rax
    mov rax, [rbp - 504]

    mov dword [rbp - 44], eax

    movsxd rax, dword [rbp - 20]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 512], rax
    mov rax, [rbp - 512]

    mov dword [rbp - 20], eax
    jmp ir_while_1068
ir_while_end_1069:

    movsxd rax, dword [rbp - 44]
    mov [rbp - 520], rax
    mov rax, [rbp - 520]
    test rax, rax
    jz ir_errdefer_ok_1084
    jmp ir_errdefer_end_1085
ir_errdefer_ok_1084:
ir_errdefer_end_1085:

    movsxd rax, dword [rbp - 44]
    jmp Lparse_content_length_exit
ir_if_next_1053:
ir_if_end_1052:

    movsxd rax, dword [rbp - 20]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 528], rax
    mov rax, [rbp - 528]

    mov dword [rbp - 20], eax
    jmp ir_while_1036
ir_while_end_1037:
    mov rax, 0
    mov [rbp - 536], rax
ir_errdefer_ok_1086:
ir_errdefer_end_1087:
    mov rax, 0
    jmp Lparse_content_length_exit
Lparse_content_length_exit:

    mov rsp, rbp
    pop rbp
    ret

global find_body_start

find_body_start:
    push rbp
    mov rbp, rsp
    sub rsp, 912

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_1088:

    movsxd rax, dword [rel dbg_on]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 32], rax
    mov rax, [rbp - 32]
    test rax, rax
    jz ir_if_next_1090

    sub rsp, 32

    lea rax, [rel dbg_fbs_ok]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 40], rax

    sub rsp, 32
    mov rax, [rbp - 40]
    mov rcx, rax
    call dbg_msg
    add rsp, 32

    mov [rbp - 48], rax
    jmp ir_if_end_1089
ir_if_next_1090:
ir_if_end_1089:

    movsxd rax, dword [rel dbg_on]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 56], rax
    mov rax, [rbp - 56]
    test rax, rax
    jz ir_if_next_1092

    sub rsp, 32

    lea rax, [rel dbg_fbs_enter]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 64], rax

    sub rsp, 32
    mov rax, [rbp - 64]
    mov rcx, rax

    movsxd rax, dword [rbp - 16]
    mov rdx, rax
    call dbg
    add rsp, 32

    mov [rbp - 72], rax
    jmp ir_if_end_1091
ir_if_next_1092:
ir_if_end_1091:
    mov rax, 0

    mov dword [rbp - 20], eax
ir_while_1093:

    movsxd rax, dword [rbp - 20]
    push rax
    mov rax, 4
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 80], rax
    mov rax, [rbp - 80]
    push rax

    movsxd rax, dword [rbp - 16]
    mov r10, rax
    pop rax
    cmp rax, r10
    setle al
    movzx rax, al
    mov [rbp - 88], rax
    mov rax, [rbp - 88]
    test rax, rax
    jz ir_while_end_1094

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1097
    jmp ir_nonnull_1098
ir_trap_null_1097:

    sub rsp, 32

    lea rax, [rel Lstr_struct325]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1098:

    movsxd rax, dword [rbp - 20]
    mov [rbp - 96], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 96]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 104], rax

    sub rsp, 32

    lea rax, [rel pat_crlf2]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 112], rax

    sub rsp, 32
    mov rax, [rbp - 104]
    mov rcx, rax
    mov rax, [rbp - 112]
    mov rdx, rax
    mov rax, 4
    mov r8, rax
    call memcmp
    add rsp, 32


    mov [rbp - 120], rax
    mov rax, [rbp - 120]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 128], rax
    mov rax, [rbp - 128]
    test rax, rax
    jz ir_if_next_1096

    movsxd rax, dword [rel dbg_on]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 136], rax
    mov rax, [rbp - 136]
    test rax, rax
    jz ir_if_next_1100

    sub rsp, 32

    lea rax, [rel dbg_fbs_exit]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 144], rax

    movsxd rax, dword [rbp - 20]
    push rax
    mov rax, 4
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 152], rax

    sub rsp, 32
    mov rax, [rbp - 144]
    mov rcx, rax
    mov rax, [rbp - 152]
    mov rdx, rax
    call dbg
    add rsp, 32

    mov [rbp - 160], rax
    jmp ir_if_end_1099
ir_if_next_1100:
ir_if_end_1099:

    movsxd rax, dword [rbp - 20]
    push rax
    mov rax, 4
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 168], rax
    mov rax, [rbp - 168]
    mov [rbp - 176], rax
    mov rax, [rbp - 168]
    test rax, rax
    jz ir_errdefer_ok_1101
    jmp ir_errdefer_end_1102
ir_errdefer_ok_1101:
ir_errdefer_end_1102:
    mov rax, [rbp - 168]
    jmp Lfind_body_start_exit
ir_if_next_1096:
ir_if_end_1095:

    movsxd rax, dword [rbp - 20]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 184], rax
    mov rax, [rbp - 184]

    mov dword [rbp - 20], eax
    jmp ir_while_1093
ir_while_end_1094:
    mov rax, 0

    mov dword [rbp - 20], eax
ir_while_1103:

    movsxd rax, dword [rbp - 20]
    push rax
    mov rax, 2
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 192], rax
    mov rax, [rbp - 192]
    push rax

    movsxd rax, dword [rbp - 16]
    mov r10, rax
    pop rax
    cmp rax, r10
    setle al
    movzx rax, al
    mov [rbp - 200], rax
    mov rax, [rbp - 200]
    test rax, rax
    jz ir_while_end_1104

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1107
    jmp ir_nonnull_1108
ir_trap_null_1107:

    sub rsp, 32

    lea rax, [rel Lstr_struct327]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1108:

    movsxd rax, dword [rbp - 20]
    mov [rbp - 208], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 208]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 216], rax

    sub rsp, 32

    lea rax, [rel pat_lf2]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 224], rax

    sub rsp, 32
    mov rax, [rbp - 216]
    mov rcx, rax
    mov rax, [rbp - 224]
    mov rdx, rax
    mov rax, 2
    mov r8, rax
    call memcmp
    add rsp, 32


    mov [rbp - 232], rax
    mov rax, [rbp - 232]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 240], rax
    mov rax, [rbp - 240]
    test rax, rax
    jz ir_if_next_1106

    movsxd rax, dword [rel dbg_on]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 248], rax
    mov rax, [rbp - 248]
    test rax, rax
    jz ir_if_next_1110

    sub rsp, 32

    lea rax, [rel dbg_fbs_exit]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 256], rax

    movsxd rax, dword [rbp - 20]
    push rax
    mov rax, 2
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 264], rax

    sub rsp, 32
    mov rax, [rbp - 256]
    mov rcx, rax
    mov rax, [rbp - 264]
    mov rdx, rax
    call dbg
    add rsp, 32

    mov [rbp - 272], rax
    jmp ir_if_end_1109
ir_if_next_1110:
ir_if_end_1109:

    movsxd rax, dword [rbp - 20]
    push rax
    mov rax, 2
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 280], rax
    mov rax, [rbp - 280]
    mov [rbp - 288], rax
    mov rax, [rbp - 280]
    test rax, rax
    jz ir_errdefer_ok_1111
    jmp ir_errdefer_end_1112
ir_errdefer_ok_1111:
ir_errdefer_end_1112:
    mov rax, [rbp - 280]
    jmp Lfind_body_start_exit
ir_if_next_1106:
ir_if_end_1105:

    movsxd rax, dword [rbp - 20]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 296], rax
    mov rax, [rbp - 296]

    mov dword [rbp - 20], eax
    jmp ir_while_1103
ir_while_end_1104:

    movsxd rax, dword [rel dbg_on]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 304], rax
    mov rax, [rbp - 304]
    test rax, rax
    jz ir_if_next_1114

    sub rsp, 32

    lea rax, [rel dbg_fbs_exit]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 312], rax

    sub rsp, 32
    mov rax, [rbp - 312]
    mov rcx, rax

    movsxd rax, dword [rbp - 16]
    mov rdx, rax
    call dbg
    add rsp, 32

    mov [rbp - 320], rax
    jmp ir_if_end_1113
ir_if_next_1114:
ir_if_end_1113:

    movsxd rax, dword [rbp - 16]
    mov [rbp - 328], rax
    mov rax, [rbp - 328]
    test rax, rax
    jz ir_errdefer_ok_1115
    jmp ir_errdefer_end_1116
ir_errdefer_ok_1115:
ir_errdefer_end_1116:

    movsxd rax, dword [rbp - 16]
    jmp Lfind_body_start_exit
Lfind_body_start_exit:

    mov rsp, rbp
    pop rbp
    ret

global send_html_escaped

send_html_escaped:
    push rbp
    mov rbp, rsp
    sub rsp, 960

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx

    mov [rbp - 24], r8


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_1117:
    mov rax, 0

    mov dword [rbp - 28], eax
ir_while_1118:

    movsxd rax, dword [rbp - 28]
    push rax

    movsxd rax, dword [rbp - 24]
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 40], rax
    mov rax, [rbp - 40]
    test rax, rax
    jz ir_while_end_1119

    mov rax, qword [rbp - 16]
    test rax, rax
    jz ir_trap_null_1120
    jmp ir_nonnull_1121
ir_trap_null_1120:

    sub rsp, 32

    lea rax, [rel Lstr_struct329]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1121:

    movsxd rax, dword [rbp - 28]
    mov [rbp - 48], rax

    mov rax, qword [rbp - 16]
    push rax
    mov rax, [rbp - 48]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 56], rax
    mov rax, [rbp - 56]
    movzx rax, byte [rax]
    mov [rbp - 64], rax
    mov rax, [rbp - 64]

    mov dword [rbp - 32], eax

    movsxd rax, dword [rbp - 32]
    push rax
    mov rax, 38
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 72], rax
    mov rax, [rbp - 72]
    test rax, rax
    jz ir_if_next_1123

    lea rax, [rel html_amp]
    mov [rbp - 80], rax
    mov rax, [rbp - 80]
    mov rax, qword [rax]
    mov [rbp - 88], rax

    lea rax, [rel html_amp]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 96], rax
    mov rax, [rbp - 96]
    mov rax, qword [rax]
    mov [rbp - 104], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 88]
    mov rdx, rax
    mov rax, [rbp - 104]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 112], rax
    jmp ir_if_end_1122
ir_if_next_1123:

    movsxd rax, dword [rbp - 32]
    push rax
    mov rax, 60
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 120], rax
    mov rax, [rbp - 120]
    test rax, rax
    jz ir_if_next_1124

    lea rax, [rel html_lt]
    mov [rbp - 128], rax
    mov rax, [rbp - 128]
    mov rax, qword [rax]
    mov [rbp - 136], rax

    lea rax, [rel html_lt]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 144], rax
    mov rax, [rbp - 144]
    mov rax, qword [rax]
    mov [rbp - 152], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 136]
    mov rdx, rax
    mov rax, [rbp - 152]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 160], rax
    jmp ir_if_end_1122
ir_if_next_1124:

    movsxd rax, dword [rbp - 32]
    push rax
    mov rax, 62
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 168], rax
    mov rax, [rbp - 168]
    test rax, rax
    jz ir_if_next_1125

    lea rax, [rel html_gt]
    mov [rbp - 176], rax
    mov rax, [rbp - 176]
    mov rax, qword [rax]
    mov [rbp - 184], rax

    lea rax, [rel html_gt]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 192], rax
    mov rax, [rbp - 192]
    mov rax, qword [rax]
    mov [rbp - 200], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 184]
    mov rdx, rax
    mov rax, [rbp - 200]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 208], rax
    jmp ir_if_end_1122
ir_if_next_1125:

    movsxd rax, dword [rbp - 32]
    push rax
    mov rax, 34
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 216], rax
    mov rax, [rbp - 216]
    test rax, rax
    jz ir_if_next_1126

    lea rax, [rel html_quot]
    mov [rbp - 224], rax
    mov rax, [rbp - 224]
    mov rax, qword [rax]
    mov [rbp - 232], rax

    lea rax, [rel html_quot]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 240], rax
    mov rax, [rbp - 240]
    mov rax, qword [rax]
    mov [rbp - 248], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 232]
    mov rdx, rax
    mov rax, [rbp - 248]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 256], rax
    jmp ir_if_end_1122
ir_if_next_1126:

    movsxd rax, dword [rbp - 32]
    push rax
    mov rax, 39
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 264], rax
    mov rax, [rbp - 264]
    test rax, rax
    jz ir_if_next_1127

    lea rax, [rel html_apos]
    mov [rbp - 272], rax
    mov rax, [rbp - 272]
    mov rax, qword [rax]
    mov [rbp - 280], rax

    lea rax, [rel html_apos]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 288], rax
    mov rax, [rbp - 288]
    mov rax, qword [rax]
    mov [rbp - 296], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 280]
    mov rdx, rax
    mov rax, [rbp - 296]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 304], rax
    jmp ir_if_end_1122
ir_if_next_1127:

    mov rax, qword [rbp - 16]
    test rax, rax
    jz ir_trap_null_1128
    jmp ir_nonnull_1129
ir_trap_null_1128:

    sub rsp, 32

    lea rax, [rel Lstr_struct331]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1129:

    movsxd rax, dword [rbp - 28]
    mov [rbp - 312], rax

    mov rax, qword [rbp - 16]
    push rax
    mov rax, [rbp - 312]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 320], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 320]
    mov rdx, rax
    mov rax, 1
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 328], rax
ir_if_end_1122:

    movsxd rax, dword [rbp - 28]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 336], rax
    mov rax, [rbp - 336]

    mov dword [rbp - 28], eax
    jmp ir_while_1118
ir_while_end_1119:
    mov rax, 0
    mov [rbp - 344], rax
ir_errdefer_ok_1130:
ir_errdefer_end_1131:
    jmp Lsend_html_escaped_exit
Lsend_html_escaped_exit:

    mov rsp, rbp
    pop rbp
    ret

global build_posts_filename

build_posts_filename:
    push rbp
    mov rbp, rsp
    sub rsp, 736

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_1132:

    lea rax, [rel fn_posts_prefix]
    mov [rbp - 32], rax
    mov rax, [rbp - 32]
    mov rax, qword [rax]
    mov [rbp - 40], rax

    lea rax, [rel fn_posts_prefix]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    mov rax, qword [rax]
    mov [rbp - 56], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 40]
    mov rdx, rax
    mov rax, [rbp - 56]
    mov r8, rax
    call memcpy
    add rsp, 32

    mov [rbp - 64], rax

    lea rax, [rel fn_posts_prefix]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 72], rax
    mov rax, [rbp - 72]
    mov rax, qword [rax]
    mov [rbp - 80], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1133
    jmp ir_nonnull_1134
ir_trap_null_1133:

    sub rsp, 32

    lea rax, [rel Lstr_struct333]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1134:
    mov rax, [rbp - 80]
    mov [rbp - 88], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 80]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 96], rax

    sub rsp, 32
    mov rax, [rbp - 96]
    mov rcx, rax

    movsxd rax, dword [rbp - 16]
    mov rdx, rax
    call int_to_dec
    add rsp, 32


    mov [rbp - 104], rax
    mov rax, [rbp - 104]

    mov dword [rbp - 20], eax

    lea rax, [rel fn_posts_prefix]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 112], rax
    mov rax, [rbp - 112]
    mov rax, qword [rax]
    mov [rbp - 120], rax
    mov rax, [rbp - 120]
    push rax

    movsxd rax, dword [rbp - 20]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 128], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1135
    jmp ir_nonnull_1136
ir_trap_null_1135:

    sub rsp, 32

    lea rax, [rel Lstr_struct335]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1136:
    mov rax, [rbp - 128]
    mov [rbp - 136], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 128]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 144], rax

    lea rax, [rel fn_posts_suffix]
    mov [rbp - 152], rax
    mov rax, [rbp - 152]
    mov rax, qword [rax]
    mov [rbp - 160], rax

    lea rax, [rel fn_posts_suffix]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 168], rax
    mov rax, [rbp - 168]
    mov rax, qword [rax]
    mov [rbp - 176], rax

    sub rsp, 32
    mov rax, [rbp - 144]
    mov rcx, rax
    mov rax, [rbp - 160]
    mov rdx, rax
    mov rax, [rbp - 176]
    mov r8, rax
    call memcpy
    add rsp, 32

    mov [rbp - 184], rax

    lea rax, [rel fn_posts_prefix]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 192], rax
    mov rax, [rbp - 192]
    mov rax, qword [rax]
    mov [rbp - 200], rax
    mov rax, [rbp - 200]
    push rax

    movsxd rax, dword [rbp - 20]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 208], rax

    lea rax, [rel fn_posts_suffix]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 216], rax
    mov rax, [rbp - 216]
    mov rax, qword [rax]
    mov [rbp - 224], rax
    mov rax, [rbp - 208]
    push rax
    mov rax, [rbp - 224]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 232], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1137
    jmp ir_nonnull_1138
ir_trap_null_1137:

    sub rsp, 32

    lea rax, [rel Lstr_struct337]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1138:
    mov rax, [rbp - 232]
    mov [rbp - 240], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 232]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 248], rax
    mov rax, [rbp - 248]
    push rax
    mov rax, 0
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    mov rax, 0
    mov [rbp - 264], rax
ir_errdefer_ok_1139:
ir_errdefer_end_1140:
    jmp Lbuild_posts_filename_exit
Lbuild_posts_filename_exit:

    mov rsp, rbp
    pop rbp
    ret

global build_forum_thread_url

build_forum_thread_url:
    push rbp
    mov rbp, rsp
    sub rsp, 1200

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_1141:

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1142
    jmp ir_nonnull_1143
ir_trap_null_1142:

    sub rsp, 32

    lea rax, [rel Lstr_struct339]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1143:
    mov rax, 0
    mov [rbp - 32], rax

    mov rax, qword [rbp - 8]
    mov [rbp - 40], rax
    mov rax, [rbp - 40]
    push rax
    mov rax, 47
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1144
    jmp ir_nonnull_1145
ir_trap_null_1144:

    sub rsp, 32

    lea rax, [rel Lstr_struct341]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1145:
    mov rax, 1
    mov [rbp - 56], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 64], rax
    mov rax, [rbp - 64]
    push rax
    mov rax, 102
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1146
    jmp ir_nonnull_1147
ir_trap_null_1146:

    sub rsp, 32

    lea rax, [rel Lstr_struct343]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1147:
    mov rax, 2
    mov [rbp - 80], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 2
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 88], rax
    mov rax, [rbp - 88]
    push rax
    mov rax, 111
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1148
    jmp ir_nonnull_1149
ir_trap_null_1148:

    sub rsp, 32

    lea rax, [rel Lstr_struct345]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1149:
    mov rax, 3
    mov [rbp - 104], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 3
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 112], rax
    mov rax, [rbp - 112]
    push rax
    mov rax, 114
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1150
    jmp ir_nonnull_1151
ir_trap_null_1150:

    sub rsp, 32

    lea rax, [rel Lstr_struct347]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1151:
    mov rax, 4
    mov [rbp - 128], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 4
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 136], rax
    mov rax, [rbp - 136]
    push rax
    mov rax, 117
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1152
    jmp ir_nonnull_1153
ir_trap_null_1152:

    sub rsp, 32

    lea rax, [rel Lstr_struct349]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1153:
    mov rax, 5
    mov [rbp - 152], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 5
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 160], rax
    mov rax, [rbp - 160]
    push rax
    mov rax, 109
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1154
    jmp ir_nonnull_1155
ir_trap_null_1154:

    sub rsp, 32

    lea rax, [rel Lstr_struct351]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1155:
    mov rax, 6
    mov [rbp - 176], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 6
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 184], rax
    mov rax, [rbp - 184]
    push rax
    mov rax, 63
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1156
    jmp ir_nonnull_1157
ir_trap_null_1156:

    sub rsp, 32

    lea rax, [rel Lstr_struct353]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1157:
    mov rax, 7
    mov [rbp - 200], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 7
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 208], rax
    mov rax, [rbp - 208]
    push rax
    mov rax, 116
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1158
    jmp ir_nonnull_1159
ir_trap_null_1158:

    sub rsp, 32

    lea rax, [rel Lstr_struct355]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1159:
    mov rax, 8
    mov [rbp - 224], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 232], rax
    mov rax, [rbp - 232]
    push rax
    mov rax, 104
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1160
    jmp ir_nonnull_1161
ir_trap_null_1160:

    sub rsp, 32

    lea rax, [rel Lstr_struct357]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1161:
    mov rax, 9
    mov [rbp - 248], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 9
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 256], rax
    mov rax, [rbp - 256]
    push rax
    mov rax, 114
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1162
    jmp ir_nonnull_1163
ir_trap_null_1162:

    sub rsp, 32

    lea rax, [rel Lstr_struct359]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1163:
    mov rax, 10
    mov [rbp - 272], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 10
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 280], rax
    mov rax, [rbp - 280]
    push rax
    mov rax, 101
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1164
    jmp ir_nonnull_1165
ir_trap_null_1164:

    sub rsp, 32

    lea rax, [rel Lstr_struct361]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1165:
    mov rax, 11
    mov [rbp - 296], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 11
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 304], rax
    mov rax, [rbp - 304]
    push rax
    mov rax, 97
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1166
    jmp ir_nonnull_1167
ir_trap_null_1166:

    sub rsp, 32

    lea rax, [rel Lstr_struct363]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1167:
    mov rax, 12
    mov [rbp - 320], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 12
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 328], rax
    mov rax, [rbp - 328]
    push rax
    mov rax, 100
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1168
    jmp ir_nonnull_1169
ir_trap_null_1168:

    sub rsp, 32

    lea rax, [rel Lstr_struct365]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1169:
    mov rax, 13
    mov [rbp - 344], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 13
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 352], rax
    mov rax, [rbp - 352]
    push rax
    mov rax, 61
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1170
    jmp ir_nonnull_1171
ir_trap_null_1170:

    sub rsp, 32

    lea rax, [rel Lstr_struct367]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1171:
    mov rax, 14
    mov [rbp - 368], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 14
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 376], rax

    sub rsp, 32
    mov rax, [rbp - 376]
    mov rcx, rax

    movsxd rax, dword [rbp - 16]
    mov rdx, rax
    call int_to_dec
    add rsp, 32


    mov [rbp - 384], rax
    mov rax, [rbp - 384]

    mov dword [rbp - 20], eax
    mov rax, 14
    push rax

    movsxd rax, dword [rbp - 20]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 392], rax

    mov rax, qword [rbp - 8]
    test rax, rax
    jz ir_trap_null_1172
    jmp ir_nonnull_1173
ir_trap_null_1172:

    sub rsp, 32

    lea rax, [rel Lstr_struct369]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1173:
    mov rax, [rbp - 392]
    mov [rbp - 400], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, [rbp - 392]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 408], rax
    mov rax, [rbp - 408]
    push rax
    mov rax, 0
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    mov rax, 0
    mov [rbp - 424], rax
ir_errdefer_ok_1174:
ir_errdefer_end_1175:
    jmp Lbuild_forum_thread_url_exit
Lbuild_forum_thread_url_exit:

    mov rsp, rbp
    pop rbp
    ret

global count_threads_impl

count_threads_impl:
    push rbp
    mov rbp, rsp
    sub rsp, 432


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_1176:

    sub rsp, 32

    lea rax, [rel fn_threads]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 88], rax

    sub rsp, 32

    lea rax, [rel mode_r]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 96], rax

    sub rsp, 32
    mov rax, [rbp - 88]
    mov rcx, rax
    mov rax, [rbp - 96]
    mov rdx, rax
    call fopen
    add rsp, 32

    mov [rbp - 104], rax
    mov rax, [rbp - 104]

    mov qword [rbp - 8], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 112], rax
    mov rax, [rbp - 112]
    test rax, rax
    jz ir_if_next_1178
    mov rax, 0
    mov [rbp - 120], rax
ir_errdefer_ok_1179:
ir_errdefer_end_1180:
    mov rax, 0
    jmp Lcount_threads_impl_exit
ir_if_next_1178:
ir_if_end_1177:
    mov rax, 0

    mov dword [rbp - 12], eax
ir_while_1181:
    mov rax, 1
    mov [rbp - 128], rax
    jmp ir_in_bounds_1184
ir_trap_bounds_1183:

    sub rsp, 32

    lea rax, [rel Lstr_struct371]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1184:
    mov rax, 0
    mov [rbp - 136], rax

    lea rax, [rbp - 80]
    mov [rbp - 144], rax

    sub rsp, 32
    mov rax, [rbp - 144]
    mov rcx, rax
    mov rax, 64
    mov rdx, rax

    mov rax, qword [rbp - 8]
    mov r8, rax
    call fgets
    add rsp, 32

    mov [rbp - 152], rax
    mov rax, [rbp - 152]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 160], rax
    mov rax, [rbp - 160]
    test rax, rax
    jz ir_while_end_1182

    movsxd rax, dword [rbp - 12]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 168], rax
    mov rax, [rbp - 168]

    mov dword [rbp - 12], eax
    jmp ir_while_1181
ir_while_end_1182:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    call fclose
    add rsp, 32


    mov [rbp - 176], rax

    movsxd rax, dword [rbp - 12]
    mov [rbp - 184], rax
    mov rax, [rbp - 184]
    test rax, rax
    jz ir_errdefer_ok_1185
    jmp ir_errdefer_end_1186
ir_errdefer_ok_1185:
ir_errdefer_end_1186:

    movsxd rax, dword [rbp - 12]
    jmp Lcount_threads_impl_exit
Lcount_threads_impl_exit:

    mov rsp, rbp
    pop rbp
    ret

global count_threads

count_threads:
    push rbp
    mov rbp, rsp
    sub rsp, 160


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_1187:

    sub rsp, 32

    mov rax, qword [rel file_mutex]
    mov rcx, rax
    call mutex_lock_infinite
    add rsp, 32


    mov [rbp - 16], rax
    mov rax, [rbp - 16]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    test rax, rax
    jz ir_if_next_1189
    mov rax, 0
    mov [rbp - 32], rax
ir_errdefer_ok_1190:
ir_errdefer_end_1191:
    mov rax, 0
    jmp Lcount_threads_exit
ir_if_next_1189:
ir_if_end_1188:

    sub rsp, 32
    call count_threads_impl
    add rsp, 32


    mov [rbp - 40], rax
    mov rax, [rbp - 40]

    mov dword [rbp - 4], eax

    sub rsp, 32

    mov rax, qword [rel file_mutex]
    mov rcx, rax
    call mutex_unlock
    add rsp, 32


    mov [rbp - 48], rax

    movsxd rax, dword [rbp - 4]
    mov [rbp - 56], rax
    mov rax, [rbp - 56]
    test rax, rax
    jz ir_errdefer_ok_1192
    jmp ir_errdefer_end_1193
ir_errdefer_ok_1192:
ir_errdefer_end_1193:

    movsxd rax, dword [rbp - 4]
    jmp Lcount_threads_exit
Lcount_threads_exit:

    mov rsp, rbp
    pop rbp
    ret

global send_redirect

send_redirect:
    push rbp
    mov rbp, rsp
    sub rsp, 432

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_1194:

    lea rax, [rel REDIRECT_302]
    mov [rbp - 32], rax
    mov rax, [rbp - 32]
    mov rax, qword [rax]
    mov [rbp - 40], rax

    lea rax, [rel REDIRECT_302]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    mov rax, qword [rax]
    mov [rbp - 56], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 40]
    mov rdx, rax
    mov rax, [rbp - 56]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 64], rax
    mov rax, 0

    mov dword [rbp - 20], eax
ir_while_1195:

    mov rax, qword [rbp - 16]
    test rax, rax
    jz ir_trap_null_1197
    jmp ir_nonnull_1198
ir_trap_null_1197:

    sub rsp, 32

    lea rax, [rel Lstr_struct373]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1198:

    movsxd rax, dword [rbp - 20]
    mov [rbp - 72], rax

    mov rax, qword [rbp - 16]
    push rax
    mov rax, [rbp - 72]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 80], rax
    mov rax, [rbp - 80]
    movzx rax, byte [rax]
    mov [rbp - 88], rax
    mov rax, [rbp - 88]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 96], rax
    mov rax, [rbp - 96]
    test rax, rax
    jz ir_while_end_1196

    movsxd rax, dword [rbp - 20]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 104], rax
    mov rax, [rbp - 104]

    mov dword [rbp - 20], eax
    jmp ir_while_1195
ir_while_end_1196:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax

    mov rax, qword [rbp - 16]
    mov rdx, rax

    movsxd rax, dword [rbp - 20]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 112], rax

    lea rax, [rel REDIRECT_END]
    mov [rbp - 120], rax
    mov rax, [rbp - 120]
    mov rax, qword [rax]
    mov [rbp - 128], rax

    lea rax, [rel REDIRECT_END]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 136], rax
    mov rax, [rbp - 136]
    mov rax, qword [rax]
    mov [rbp - 144], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 128]
    mov rdx, rax
    mov rax, [rbp - 144]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 152], rax
    mov rax, 0
    mov [rbp - 160], rax
ir_errdefer_ok_1199:
ir_errdefer_end_1200:
    jmp Lsend_redirect_exit
Lsend_redirect_exit:

    mov rsp, rbp
    pop rbp
    ret

global serve_forum_index

serve_forum_index:
    push rbp
    mov rbp, rsp
    sub rsp, 3712

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_1201:







    lea rax, [rel FORUM_HEADER]
    push rax

    lea rax, [rel FORUM_INDEX_START]
    mov r10, rax
    pop rax
    mov rcx, [rax + 8]
    add rcx, [r10 + 8]
    sub rsp, 24
    mov [rsp], r10
    mov [rsp + 8], rax
    mov [rsp + 16], rcx
    mov rcx, rcx
    add rcx, 17
    sub rsp, 40
    extern gc_alloc
    call gc_alloc
    add rsp, 40
    mov rcx, [rsp + 16]
    mov rdx, [rsp + 8]
    mov rsi, [rsp]
    add rsp, 24
    lea r8, [rax + 16]
    mov [rax], r8
    mov [rax + 8], rcx
    mov r9, [rdx + 8]
    mov rdi, [rdx]
    test r9, r9
    jz Lconcat_left_done374
Lconcat_left_loop375:
    mov r11b, [rdi]
    mov [r8], r11b
    inc rdi
    inc r8
    dec r9
    jnz Lconcat_left_loop375
Lconcat_left_done374:
    mov r9, [rsi + 8]
    mov rdi, [rsi]
    test r9, r9
    jz Lconcat_right_done376
Lconcat_right_loop377:
    mov r11b, [rdi]
    mov [r8], r11b
    inc rdi
    inc r8
    dec r9
    jnz Lconcat_right_loop377
Lconcat_right_done376:
    mov byte [r8], 0
    push rax

    lea rax, [rel FORUM_CSS]
    mov r10, rax
    pop rax
    mov rcx, [rax + 8]
    add rcx, [r10 + 8]
    sub rsp, 24
    mov [rsp], r10
    mov [rsp + 8], rax
    mov [rsp + 16], rcx
    mov rcx, rcx
    add rcx, 17
    sub rsp, 40
    extern gc_alloc
    call gc_alloc
    add rsp, 40
    mov rcx, [rsp + 16]
    mov rdx, [rsp + 8]
    mov rsi, [rsp]
    add rsp, 24
    lea r8, [rax + 16]
    mov [rax], r8
    mov [rax + 8], rcx
    mov r9, [rdx + 8]
    mov rdi, [rdx]
    test r9, r9
    jz Lconcat_left_done378
Lconcat_left_loop379:
    mov r11b, [rdi]
    mov [r8], r11b
    inc rdi
    inc r8
    dec r9
    jnz Lconcat_left_loop379
Lconcat_left_done378:
    mov r9, [rsi + 8]
    mov rdi, [rsi]
    test r9, r9
    jz Lconcat_right_done380
Lconcat_right_loop381:
    mov r11b, [rdi]
    mov [r8], r11b
    inc rdi
    inc r8
    dec r9
    jnz Lconcat_right_loop381
Lconcat_right_done380:
    mov byte [r8], 0
    push rax

    lea rax, [rel FORUM_INDEX_BODY]
    mov r10, rax
    pop rax
    mov rcx, [rax + 8]
    add rcx, [r10 + 8]
    sub rsp, 24
    mov [rsp], r10
    mov [rsp + 8], rax
    mov [rsp + 16], rcx
    mov rcx, rcx
    add rcx, 17
    sub rsp, 40
    extern gc_alloc
    call gc_alloc
    add rsp, 40
    mov rcx, [rsp + 16]
    mov rdx, [rsp + 8]
    mov rsi, [rsp]
    add rsp, 24
    lea r8, [rax + 16]
    mov [rax], r8
    mov [rax + 8], rcx
    mov r9, [rdx + 8]
    mov rdi, [rdx]
    test r9, r9
    jz Lconcat_left_done382
Lconcat_left_loop383:
    mov r11b, [rdi]
    mov [r8], r11b
    inc rdi
    inc r8
    dec r9
    jnz Lconcat_left_loop383
Lconcat_left_done382:
    mov r9, [rsi + 8]
    mov rdi, [rsi]
    test r9, r9
    jz Lconcat_right_done384
Lconcat_right_loop385:
    mov r11b, [rdi]
    mov [r8], r11b
    inc rdi
    inc r8
    dec r9
    jnz Lconcat_right_loop385
Lconcat_right_done384:
    mov byte [r8], 0
    mov [rbp - 1184], rax
    mov rax, [rbp - 1184]

    mov rcx, [rax]
    mov [rbp - 32], rcx
    mov rcx, [rax + 8]
    mov [rbp - 24], rcx

    lea rax, [rbp - 32]
    mov [rbp - 1192], rax
    mov rax, [rbp - 1192]
    mov rax, qword [rax]
    mov [rbp - 1200], rax

    lea rax, [rbp - 32]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1208], rax
    mov rax, [rbp - 1208]
    mov rax, qword [rax]
    mov [rbp - 1216], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 1200]
    mov rdx, rax
    mov rax, [rbp - 1216]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 1224], rax

    sub rsp, 32

    mov rax, qword [rel file_mutex]
    mov rcx, rax
    call mutex_lock_infinite
    add rsp, 32


    mov [rbp - 1232], rax
    mov rax, [rbp - 1232]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 1240], rax
    mov rax, [rbp - 1240]
    test rax, rax
    jz ir_if_next_1203
    mov rax, 0
    mov [rbp - 1248], rax
ir_errdefer_ok_1204:
ir_errdefer_end_1205:
    jmp Lserve_forum_index_exit
ir_if_next_1203:
ir_if_end_1202:

    sub rsp, 32

    lea rax, [rel fn_threads]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 1256], rax

    sub rsp, 32

    lea rax, [rel mode_r]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 1264], rax

    sub rsp, 32
    mov rax, [rbp - 1256]
    mov rcx, rax
    mov rax, [rbp - 1264]
    mov rdx, rax
    call fopen
    add rsp, 32

    mov [rbp - 1272], rax
    mov rax, [rbp - 1272]

    mov qword [rbp - 40], rax

    mov rax, qword [rbp - 40]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 1280], rax
    mov rax, [rbp - 1280]
    test rax, rax
    jz ir_if_next_1207
    mov rax, 1

    mov dword [rbp - 556], eax
ir_while_1208:
    mov rax, 1
    mov [rbp - 1288], rax
    jmp ir_in_bounds_1211
ir_trap_bounds_1210:

    sub rsp, 32

    lea rax, [rel Lstr_struct387]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1211:
    mov rax, 0
    mov [rbp - 1296], rax

    lea rax, [rbp - 552]
    mov [rbp - 1304], rax

    sub rsp, 32
    mov rax, [rbp - 1304]
    mov rcx, rax
    mov rax, 512
    mov rdx, rax

    mov rax, qword [rbp - 40]
    mov r8, rax
    call fgets
    add rsp, 32

    mov [rbp - 1312], rax
    mov rax, [rbp - 1312]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 1320], rax
    mov rax, [rbp - 1320]
    test rax, rax
    jz ir_while_end_1209
    mov rax, 0

    mov dword [rbp - 560], eax
ir_while_1212:

    movsxd rax, dword [rbp - 560]
    push rax
    mov rax, 512
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 1328], rax
    mov rax, [rbp - 1328]
    test rax, rax
    jz ir_trap_bounds_1214
    jmp ir_in_bounds_1215
ir_trap_bounds_1214:

    sub rsp, 32

    lea rax, [rel Lstr_struct389]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1215:

    movsxd rax, dword [rbp - 560]
    mov [rbp - 1336], rax

    lea rax, [rbp - 552]
    push rax
    mov rax, [rbp - 1336]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1344], rax
    mov rax, [rbp - 1344]
    movzx rax, byte [rax]
    mov [rbp - 1352], rax
    mov rax, [rbp - 1352]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 1360], rax
    mov rax, [rbp - 1360]
    test rax, rax
    jz ir_sc_false_1218
ir_sc_rhs_1216:

    movsxd rax, dword [rbp - 560]
    push rax
    mov rax, 512
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 1368], rax
    mov rax, [rbp - 1368]
    test rax, rax
    jz ir_trap_bounds_1220
    jmp ir_in_bounds_1221
ir_trap_bounds_1220:

    sub rsp, 32

    lea rax, [rel Lstr_struct391]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1221:

    movsxd rax, dword [rbp - 560]
    mov [rbp - 1376], rax

    lea rax, [rbp - 552]
    push rax
    mov rax, [rbp - 1376]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1384], rax
    mov rax, [rbp - 1384]
    movzx rax, byte [rax]
    mov [rbp - 1392], rax
    mov rax, [rbp - 1392]
    push rax
    mov rax, 10
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 1400], rax
    mov rax, [rbp - 1400]
    test rax, rax
    jz ir_sc_false_1218
ir_sc_true_1217:
    mov rax, 1
    mov [rbp - 1408], rax
    jmp ir_sc_end_1219
ir_sc_false_1218:
    mov rax, 0
    mov [rbp - 1408], rax
ir_sc_end_1219:
    mov rax, [rbp - 1408]
    test rax, rax
    jz ir_sc_false_1224
ir_sc_rhs_1222:

    movsxd rax, dword [rbp - 560]
    push rax
    mov rax, 512
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 1424], rax
    mov rax, [rbp - 1424]
    test rax, rax
    jz ir_trap_bounds_1226
    jmp ir_in_bounds_1227
ir_trap_bounds_1226:

    sub rsp, 32

    lea rax, [rel Lstr_struct393]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1227:

    movsxd rax, dword [rbp - 560]
    mov [rbp - 1432], rax

    lea rax, [rbp - 552]
    push rax
    mov rax, [rbp - 1432]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1440], rax
    mov rax, [rbp - 1440]
    movzx rax, byte [rax]
    mov [rbp - 1448], rax
    mov rax, [rbp - 1448]
    push rax
    mov rax, 13
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 1456], rax
    mov rax, [rbp - 1456]
    test rax, rax
    jz ir_sc_false_1224
ir_sc_true_1223:
    mov rax, 1
    mov [rbp - 1464], rax
    jmp ir_sc_end_1225
ir_sc_false_1224:
    mov rax, 0
    mov [rbp - 1464], rax
ir_sc_end_1225:
    mov rax, [rbp - 1464]
    test rax, rax
    jz ir_while_end_1213

    movsxd rax, dword [rbp - 560]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1480], rax
    mov rax, [rbp - 1480]

    mov dword [rbp - 560], eax
    jmp ir_while_1212
ir_while_end_1213:

    movsxd rax, dword [rbp - 560]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setg al
    movzx rax, al
    mov [rbp - 1488], rax
    mov rax, [rbp - 1488]
    test rax, rax
    jz ir_if_next_1229
    mov rax, 1
    mov [rbp - 1496], rax
    jmp ir_in_bounds_1231
ir_trap_bounds_1230:

    sub rsp, 32

    lea rax, [rel Lstr_struct395]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1231:
    mov rax, 0
    mov [rbp - 1504], rax

    lea rax, [rbp - 1160]
    mov [rbp - 1512], rax

    lea rax, [rel FORUM_LI_OPEN]
    mov [rbp - 1520], rax
    mov rax, [rbp - 1520]
    mov rax, qword [rax]
    mov [rbp - 1528], rax

    lea rax, [rel FORUM_LI_OPEN]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1536], rax
    mov rax, [rbp - 1536]
    mov rax, qword [rax]
    mov [rbp - 1544], rax

    sub rsp, 32
    mov rax, [rbp - 1512]
    mov rcx, rax
    mov rax, [rbp - 1528]
    mov rdx, rax
    mov rax, [rbp - 1544]
    mov r8, rax
    call memcpy
    add rsp, 32

    mov [rbp - 1552], rax

    lea rax, [rel FORUM_LI_OPEN]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1560], rax
    mov rax, [rbp - 1560]
    mov rax, qword [rax]
    mov [rbp - 1568], rax
    mov rax, [rbp - 1568]
    push rax
    mov rax, 600
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 1576], rax
    mov rax, [rbp - 1576]
    test rax, rax
    jz ir_trap_bounds_1232
    jmp ir_in_bounds_1233
ir_trap_bounds_1232:

    sub rsp, 32

    lea rax, [rel Lstr_struct397]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1233:
    mov rax, [rbp - 1568]
    mov [rbp - 1584], rax

    lea rax, [rbp - 1160]
    push rax
    mov rax, [rbp - 1568]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1592], rax

    sub rsp, 32
    mov rax, [rbp - 1592]
    mov rcx, rax

    movsxd rax, dword [rbp - 556]
    mov rdx, rax
    call int_to_dec
    add rsp, 32


    mov [rbp - 1600], rax
    mov rax, [rbp - 1600]

    mov dword [rbp - 1164], eax

    lea rax, [rel FORUM_LI_OPEN]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1608], rax
    mov rax, [rbp - 1608]
    mov rax, qword [rax]
    mov [rbp - 1616], rax
    mov rax, [rbp - 1616]
    push rax

    movsxd rax, dword [rbp - 1164]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1624], rax
    mov rax, [rbp - 1624]
    push rax
    mov rax, 600
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 1632], rax
    mov rax, [rbp - 1632]
    test rax, rax
    jz ir_trap_bounds_1234
    jmp ir_in_bounds_1235
ir_trap_bounds_1234:

    sub rsp, 32

    lea rax, [rel Lstr_struct399]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1235:
    mov rax, [rbp - 1624]
    mov [rbp - 1640], rax

    lea rax, [rbp - 1160]
    push rax
    mov rax, [rbp - 1624]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1648], rax

    lea rax, [rel FORUM_LI_MID]
    mov [rbp - 1656], rax
    mov rax, [rbp - 1656]
    mov rax, qword [rax]
    mov [rbp - 1664], rax

    lea rax, [rel FORUM_LI_MID]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1672], rax
    mov rax, [rbp - 1672]
    mov rax, qword [rax]
    mov [rbp - 1680], rax

    sub rsp, 32
    mov rax, [rbp - 1648]
    mov rcx, rax
    mov rax, [rbp - 1664]
    mov rdx, rax
    mov rax, [rbp - 1680]
    mov r8, rax
    call memcpy
    add rsp, 32

    mov [rbp - 1688], rax

    lea rax, [rel FORUM_LI_OPEN]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1696], rax
    mov rax, [rbp - 1696]
    mov rax, qword [rax]
    mov [rbp - 1704], rax
    mov rax, [rbp - 1704]
    push rax

    movsxd rax, dword [rbp - 1164]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1712], rax

    lea rax, [rel FORUM_LI_MID]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1720], rax
    mov rax, [rbp - 1720]
    mov rax, qword [rax]
    mov [rbp - 1728], rax
    mov rax, [rbp - 1712]
    push rax
    mov rax, [rbp - 1728]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1736], rax
    mov rax, [rbp - 1736]

    mov dword [rbp - 1168], eax

    movsxd rax, dword [rbp - 560]

    mov dword [rbp - 1172], eax

    movsxd rax, dword [rbp - 1172]
    push rax
    mov rax, 200
    mov r10, rax
    pop rax
    cmp rax, r10
    setg al
    movzx rax, al
    mov [rbp - 1744], rax
    mov rax, [rbp - 1744]
    test rax, rax
    jz ir_if_next_1237
    mov rax, 200

    mov dword [rbp - 1172], eax
    jmp ir_if_end_1236
ir_if_next_1237:
ir_if_end_1236:
    mov rax, 1
    mov [rbp - 1752], rax
    jmp ir_in_bounds_1239
ir_trap_bounds_1238:

    sub rsp, 32

    lea rax, [rel Lstr_struct401]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1239:
    mov rax, 0
    mov [rbp - 1760], rax

    lea rax, [rbp - 1160]
    mov [rbp - 1768], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 1768]
    mov rdx, rax

    movsxd rax, dword [rbp - 1168]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 1776], rax
    mov rax, 1
    mov [rbp - 1784], rax
    jmp ir_in_bounds_1241
ir_trap_bounds_1240:

    sub rsp, 32

    lea rax, [rel Lstr_struct403]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1241:
    mov rax, 0
    mov [rbp - 1792], rax

    lea rax, [rbp - 552]
    mov [rbp - 1800], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 1800]
    mov rdx, rax

    movsxd rax, dword [rbp - 1172]
    mov r8, rax
    call send_html_escaped
    add rsp, 32

    mov [rbp - 1808], rax

    lea rax, [rel FORUM_LI_CLOSE]
    mov [rbp - 1816], rax
    mov rax, [rbp - 1816]
    mov rax, qword [rax]
    mov [rbp - 1824], rax

    lea rax, [rel FORUM_LI_CLOSE]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1832], rax
    mov rax, [rbp - 1832]
    mov rax, qword [rax]
    mov [rbp - 1840], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 1824]
    mov rdx, rax
    mov rax, [rbp - 1840]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 1848], rax
    jmp ir_if_end_1228
ir_if_next_1229:
ir_if_end_1228:

    movsxd rax, dword [rbp - 556]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1856], rax
    mov rax, [rbp - 1856]

    mov dword [rbp - 556], eax
    jmp ir_while_1208
ir_while_end_1209:

    sub rsp, 32

    mov rax, qword [rbp - 40]
    mov rcx, rax
    call fclose
    add rsp, 32


    mov [rbp - 1864], rax
    jmp ir_if_end_1206
ir_if_next_1207:
ir_if_end_1206:

    sub rsp, 32

    mov rax, qword [rel file_mutex]
    mov rcx, rax
    call mutex_unlock
    add rsp, 32


    mov [rbp - 1872], rax

    lea rax, [rel FORUM_INDEX_END]
    mov [rbp - 1880], rax
    mov rax, [rbp - 1880]
    mov rax, qword [rax]
    mov [rbp - 1888], rax

    lea rax, [rel FORUM_INDEX_END]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1896], rax
    mov rax, [rbp - 1896]
    mov rax, qword [rax]
    mov [rbp - 1904], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 1888]
    mov rdx, rax
    mov rax, [rbp - 1904]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 1912], rax

    movsxd rax, dword [rbp - 16]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 1920], rax
    mov rax, [rbp - 1920]
    test rax, rax
    jz ir_if_next_1243

    lea rax, [rel FORUM_ERR_EMPTY]
    mov [rbp - 1928], rax
    mov rax, [rbp - 1928]
    mov rax, qword [rax]
    mov [rbp - 1936], rax

    lea rax, [rel FORUM_ERR_EMPTY]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1944], rax
    mov rax, [rbp - 1944]
    mov rax, qword [rax]
    mov [rbp - 1952], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 1936]
    mov rdx, rax
    mov rax, [rbp - 1952]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 1960], rax
    jmp ir_if_end_1242
ir_if_next_1243:
ir_if_end_1242:

    lea rax, [rel FORUM_FORM]
    mov [rbp - 1968], rax
    mov rax, [rbp - 1968]
    mov rax, qword [rax]
    mov [rbp - 1976], rax

    lea rax, [rel FORUM_FORM]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1984], rax
    mov rax, [rbp - 1984]
    mov rax, qword [rax]
    mov [rbp - 1992], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 1976]
    mov rdx, rax
    mov rax, [rbp - 1992]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 2000], rax
    mov rax, 0
    mov [rbp - 2008], rax
ir_errdefer_ok_1244:
ir_errdefer_end_1245:
    jmp Lserve_forum_index_exit
Lserve_forum_index_exit:

    mov rsp, rbp
    pop rbp
    ret

global serve_forum_thread

serve_forum_thread:
    push rbp
    mov rbp, rsp
    mov rax, 7344
    extern ___chkstk_ms
    sub rsp, 32
    call ___chkstk_ms
    add rsp, 32
    sub rsp, rax

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_1246:







    lea rax, [rel FORUM_HEADER]
    push rax

    lea rax, [rel FORUM_THREAD_START]
    mov r10, rax
    pop rax
    mov rcx, [rax + 8]
    add rcx, [r10 + 8]
    sub rsp, 24
    mov [rsp], r10
    mov [rsp + 8], rax
    mov [rsp + 16], rcx
    mov rcx, rcx
    add rcx, 17
    sub rsp, 40
    extern gc_alloc
    call gc_alloc
    add rsp, 40
    mov rcx, [rsp + 16]
    mov rdx, [rsp + 8]
    mov rsi, [rsp]
    add rsp, 24
    lea r8, [rax + 16]
    mov [rax], r8
    mov [rax + 8], rcx
    mov r9, [rdx + 8]
    mov rdi, [rdx]
    test r9, r9
    jz Lconcat_left_done404
Lconcat_left_loop405:
    mov r11b, [rdi]
    mov [r8], r11b
    inc rdi
    inc r8
    dec r9
    jnz Lconcat_left_loop405
Lconcat_left_done404:
    mov r9, [rsi + 8]
    mov rdi, [rsi]
    test r9, r9
    jz Lconcat_right_done406
Lconcat_right_loop407:
    mov r11b, [rdi]
    mov [r8], r11b
    inc rdi
    inc r8
    dec r9
    jnz Lconcat_right_loop407
Lconcat_right_done406:
    mov byte [r8], 0
    push rax

    lea rax, [rel FORUM_CSS]
    mov r10, rax
    pop rax
    mov rcx, [rax + 8]
    add rcx, [r10 + 8]
    sub rsp, 24
    mov [rsp], r10
    mov [rsp + 8], rax
    mov [rsp + 16], rcx
    mov rcx, rcx
    add rcx, 17
    sub rsp, 40
    extern gc_alloc
    call gc_alloc
    add rsp, 40
    mov rcx, [rsp + 16]
    mov rdx, [rsp + 8]
    mov rsi, [rsp]
    add rsp, 24
    lea r8, [rax + 16]
    mov [rax], r8
    mov [rax + 8], rcx
    mov r9, [rdx + 8]
    mov rdi, [rdx]
    test r9, r9
    jz Lconcat_left_done408
Lconcat_left_loop409:
    mov r11b, [rdi]
    mov [r8], r11b
    inc rdi
    inc r8
    dec r9
    jnz Lconcat_left_loop409
Lconcat_left_done408:
    mov r9, [rsi + 8]
    mov rdi, [rsi]
    test r9, r9
    jz Lconcat_right_done410
Lconcat_right_loop411:
    mov r11b, [rdi]
    mov [r8], r11b
    inc rdi
    inc r8
    dec r9
    jnz Lconcat_right_loop411
Lconcat_right_done410:
    mov byte [r8], 0
    push rax

    lea rax, [rel FORUM_THREAD_HEAD_END]
    mov r10, rax
    pop rax
    mov rcx, [rax + 8]
    add rcx, [r10 + 8]
    sub rsp, 24
    mov [rsp], r10
    mov [rsp + 8], rax
    mov [rsp + 16], rcx
    mov rcx, rcx
    add rcx, 17
    sub rsp, 40
    extern gc_alloc
    call gc_alloc
    add rsp, 40
    mov rcx, [rsp + 16]
    mov rdx, [rsp + 8]
    mov rsi, [rsp]
    add rsp, 24
    lea r8, [rax + 16]
    mov [rax], r8
    mov [rax + 8], rcx
    mov r9, [rdx + 8]
    mov rdi, [rdx]
    test r9, r9
    jz Lconcat_left_done412
Lconcat_left_loop413:
    mov r11b, [rdi]
    mov [r8], r11b
    inc rdi
    inc r8
    dec r9
    jnz Lconcat_left_loop413
Lconcat_left_done412:
    mov r9, [rsi + 8]
    mov rdi, [rsi]
    test r9, r9
    jz Lconcat_right_done414
Lconcat_right_loop415:
    mov r11b, [rdi]
    mov [r8], r11b
    inc rdi
    inc r8
    dec r9
    jnz Lconcat_right_loop415
Lconcat_right_done414:
    mov byte [r8], 0
    mov [rbp - 896], rax
    mov rax, [rbp - 896]

    mov rcx, [rax]
    mov [rbp - 32], rcx
    mov rcx, [rax + 8]
    mov [rbp - 24], rcx

    lea rax, [rbp - 32]
    mov [rbp - 904], rax
    mov rax, [rbp - 904]
    mov rax, qword [rax]
    mov [rbp - 912], rax

    lea rax, [rbp - 32]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 920], rax
    mov rax, [rbp - 920]
    mov rax, qword [rax]
    mov [rbp - 928], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 912]
    mov rdx, rax
    mov rax, [rbp - 928]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 936], rax

    sub rsp, 32

    mov rax, qword [rel file_mutex]
    mov rcx, rax
    call mutex_lock_infinite
    add rsp, 32


    mov [rbp - 944], rax
    mov rax, [rbp - 944]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 952], rax
    mov rax, [rbp - 952]
    test rax, rax
    jz ir_if_next_1248
    mov rax, 0
    mov [rbp - 960], rax
ir_errdefer_ok_1249:
ir_errdefer_end_1250:
    jmp Lserve_forum_thread_exit
ir_if_next_1248:
ir_if_end_1247:
    mov rax, 1
    mov [rbp - 968], rax
    jmp ir_in_bounds_1252
ir_trap_bounds_1251:

    sub rsp, 32

    lea rax, [rel Lstr_struct417]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1252:
    mov rax, 0
    mov [rbp - 976], rax

    lea rax, [rbp - 288]
    mov [rbp - 984], rax
    mov rax, [rbp - 984]
    push rax
    mov rax, 85
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    mov rax, 1
    mov [rbp - 1000], rax
    jmp ir_in_bounds_1254
ir_trap_bounds_1253:

    sub rsp, 32

    lea rax, [rel Lstr_struct419]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1254:
    mov rax, 1
    mov [rbp - 1008], rax

    lea rax, [rbp - 288]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1016], rax
    mov rax, [rbp - 1016]
    push rax
    mov rax, 110
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    mov rax, 1
    mov [rbp - 1032], rax
    jmp ir_in_bounds_1256
ir_trap_bounds_1255:

    sub rsp, 32

    lea rax, [rel Lstr_struct421]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1256:
    mov rax, 2
    mov [rbp - 1040], rax

    lea rax, [rbp - 288]
    push rax
    mov rax, 2
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1048], rax
    mov rax, [rbp - 1048]
    push rax
    mov rax, 107
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    mov rax, 1
    mov [rbp - 1064], rax
    jmp ir_in_bounds_1258
ir_trap_bounds_1257:

    sub rsp, 32

    lea rax, [rel Lstr_struct423]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1258:
    mov rax, 3
    mov [rbp - 1072], rax

    lea rax, [rbp - 288]
    push rax
    mov rax, 3
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1080], rax
    mov rax, [rbp - 1080]
    push rax
    mov rax, 110
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    mov rax, 1
    mov [rbp - 1096], rax
    jmp ir_in_bounds_1260
ir_trap_bounds_1259:

    sub rsp, 32

    lea rax, [rel Lstr_struct425]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1260:
    mov rax, 4
    mov [rbp - 1104], rax

    lea rax, [rbp - 288]
    push rax
    mov rax, 4
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1112], rax
    mov rax, [rbp - 1112]
    push rax
    mov rax, 111
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    mov rax, 1
    mov [rbp - 1128], rax
    jmp ir_in_bounds_1262
ir_trap_bounds_1261:

    sub rsp, 32

    lea rax, [rel Lstr_struct427]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1262:
    mov rax, 5
    mov [rbp - 1136], rax

    lea rax, [rbp - 288]
    push rax
    mov rax, 5
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1144], rax
    mov rax, [rbp - 1144]
    push rax
    mov rax, 119
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    mov rax, 1
    mov [rbp - 1160], rax
    jmp ir_in_bounds_1264
ir_trap_bounds_1263:

    sub rsp, 32

    lea rax, [rel Lstr_struct429]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1264:
    mov rax, 6
    mov [rbp - 1168], rax

    lea rax, [rbp - 288]
    push rax
    mov rax, 6
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1176], rax
    mov rax, [rbp - 1176]
    push rax
    mov rax, 110
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    mov rax, 1
    mov [rbp - 1192], rax
    jmp ir_in_bounds_1266
ir_trap_bounds_1265:

    sub rsp, 32

    lea rax, [rel Lstr_struct431]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1266:
    mov rax, 7
    mov [rbp - 1200], rax

    lea rax, [rbp - 288]
    push rax
    mov rax, 7
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1208], rax
    mov rax, [rbp - 1208]
    push rax
    mov rax, 0
    mov rcx, rax
    pop rax
    mov byte [rax], cl

    sub rsp, 32

    lea rax, [rel fn_threads]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 1224], rax

    sub rsp, 32

    lea rax, [rel mode_r]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 1232], rax

    sub rsp, 32
    mov rax, [rbp - 1224]
    mov rcx, rax
    mov rax, [rbp - 1232]
    mov rdx, rax
    call fopen
    add rsp, 32

    mov [rbp - 1240], rax
    mov rax, [rbp - 1240]

    mov qword [rbp - 296], rax

    mov rax, qword [rbp - 296]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 1248], rax
    mov rax, [rbp - 1248]
    test rax, rax
    jz ir_if_next_1268
    mov rax, 1

    mov dword [rbp - 812], eax
ir_while_1269:
    mov rax, 1
    mov [rbp - 1256], rax
    jmp ir_in_bounds_1272
ir_trap_bounds_1271:

    sub rsp, 32

    lea rax, [rel Lstr_struct433]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1272:
    mov rax, 0
    mov [rbp - 1264], rax

    lea rax, [rbp - 808]
    mov [rbp - 1272], rax

    sub rsp, 32
    mov rax, [rbp - 1272]
    mov rcx, rax
    mov rax, 512
    mov rdx, rax

    mov rax, qword [rbp - 296]
    mov r8, rax
    call fgets
    add rsp, 32

    mov [rbp - 1280], rax
    mov rax, [rbp - 1280]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 1288], rax
    mov rax, [rbp - 1288]
    test rax, rax
    jz ir_while_end_1270

    movsxd rax, dword [rbp - 812]
    push rax

    movsxd rax, dword [rbp - 16]
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 1296], rax
    mov rax, [rbp - 1296]
    test rax, rax
    jz ir_if_next_1274
    mov rax, 0

    mov dword [rbp - 816], eax
ir_while_1275:

    movsxd rax, dword [rbp - 816]
    push rax
    mov rax, 512
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 1304], rax
    mov rax, [rbp - 1304]
    test rax, rax
    jz ir_trap_bounds_1277
    jmp ir_in_bounds_1278
ir_trap_bounds_1277:

    sub rsp, 32

    lea rax, [rel Lstr_struct435]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1278:

    movsxd rax, dword [rbp - 816]
    mov [rbp - 1312], rax

    lea rax, [rbp - 808]
    push rax
    mov rax, [rbp - 1312]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1320], rax
    mov rax, [rbp - 1320]
    movzx rax, byte [rax]
    mov [rbp - 1328], rax
    mov rax, [rbp - 1328]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 1336], rax
    mov rax, [rbp - 1336]
    test rax, rax
    jz ir_sc_false_1281
ir_sc_rhs_1279:

    movsxd rax, dword [rbp - 816]
    push rax
    mov rax, 512
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 1344], rax
    mov rax, [rbp - 1344]
    test rax, rax
    jz ir_trap_bounds_1283
    jmp ir_in_bounds_1284
ir_trap_bounds_1283:

    sub rsp, 32

    lea rax, [rel Lstr_struct437]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1284:

    movsxd rax, dword [rbp - 816]
    mov [rbp - 1352], rax

    lea rax, [rbp - 808]
    push rax
    mov rax, [rbp - 1352]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1360], rax
    mov rax, [rbp - 1360]
    movzx rax, byte [rax]
    mov [rbp - 1368], rax
    mov rax, [rbp - 1368]
    push rax
    mov rax, 10
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 1376], rax
    mov rax, [rbp - 1376]
    test rax, rax
    jz ir_sc_false_1281
ir_sc_true_1280:
    mov rax, 1
    mov [rbp - 1384], rax
    jmp ir_sc_end_1282
ir_sc_false_1281:
    mov rax, 0
    mov [rbp - 1384], rax
ir_sc_end_1282:
    mov rax, [rbp - 1384]
    test rax, rax
    jz ir_sc_false_1287
ir_sc_rhs_1285:

    movsxd rax, dword [rbp - 816]
    push rax
    mov rax, 512
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 1400], rax
    mov rax, [rbp - 1400]
    test rax, rax
    jz ir_trap_bounds_1289
    jmp ir_in_bounds_1290
ir_trap_bounds_1289:

    sub rsp, 32

    lea rax, [rel Lstr_struct439]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1290:

    movsxd rax, dword [rbp - 816]
    mov [rbp - 1408], rax

    lea rax, [rbp - 808]
    push rax
    mov rax, [rbp - 1408]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1416], rax
    mov rax, [rbp - 1416]
    movzx rax, byte [rax]
    mov [rbp - 1424], rax
    mov rax, [rbp - 1424]
    push rax
    mov rax, 13
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 1432], rax
    mov rax, [rbp - 1432]
    test rax, rax
    jz ir_sc_false_1287
ir_sc_true_1286:
    mov rax, 1
    mov [rbp - 1440], rax
    jmp ir_sc_end_1288
ir_sc_false_1287:
    mov rax, 0
    mov [rbp - 1440], rax
ir_sc_end_1288:
    mov rax, [rbp - 1440]
    test rax, rax
    jz ir_while_end_1276

    movsxd rax, dword [rbp - 816]
    push rax
    mov rax, 255
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 1456], rax
    mov rax, [rbp - 1456]
    test rax, rax
    jz ir_if_next_1292

    movsxd rax, dword [rbp - 816]
    push rax
    mov rax, 512
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 1464], rax
    mov rax, [rbp - 1464]
    test rax, rax
    jz ir_trap_bounds_1293
    jmp ir_in_bounds_1294
ir_trap_bounds_1293:

    sub rsp, 32

    lea rax, [rel Lstr_struct441]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1294:

    movsxd rax, dword [rbp - 816]
    mov [rbp - 1472], rax

    lea rax, [rbp - 808]
    push rax
    mov rax, [rbp - 1472]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1480], rax
    mov rax, [rbp - 1480]
    movzx rax, byte [rax]
    mov [rbp - 1488], rax

    movsxd rax, dword [rbp - 816]
    push rax
    mov rax, 256
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 1496], rax
    mov rax, [rbp - 1496]
    test rax, rax
    jz ir_trap_bounds_1295
    jmp ir_in_bounds_1296
ir_trap_bounds_1295:

    sub rsp, 32

    lea rax, [rel Lstr_struct443]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1296:

    movsxd rax, dword [rbp - 816]
    mov [rbp - 1504], rax

    lea rax, [rbp - 288]
    push rax
    mov rax, [rbp - 1504]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1512], rax
    mov rax, [rbp - 1512]
    push rax
    mov rax, [rbp - 1488]
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    jmp ir_if_end_1291
ir_if_next_1292:
ir_if_end_1291:

    movsxd rax, dword [rbp - 816]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1528], rax
    mov rax, [rbp - 1528]

    mov dword [rbp - 816], eax
    jmp ir_while_1275
ir_while_end_1276:

    movsxd rax, dword [rbp - 816]
    push rax
    mov rax, 256
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 1536], rax
    mov rax, [rbp - 1536]
    test rax, rax
    jz ir_if_next_1298

    movsxd rax, dword [rbp - 816]
    push rax
    mov rax, 256
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 1544], rax
    mov rax, [rbp - 1544]
    test rax, rax
    jz ir_trap_bounds_1299
    jmp ir_in_bounds_1300
ir_trap_bounds_1299:

    sub rsp, 32

    lea rax, [rel Lstr_struct445]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1300:

    movsxd rax, dword [rbp - 816]
    mov [rbp - 1552], rax

    lea rax, [rbp - 288]
    push rax
    mov rax, [rbp - 1552]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1560], rax
    mov rax, [rbp - 1560]
    push rax
    mov rax, 0
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    jmp ir_if_end_1297
ir_if_next_1298:
    mov rax, 1
    mov [rbp - 1576], rax
    jmp ir_in_bounds_1302
ir_trap_bounds_1301:

    sub rsp, 32

    lea rax, [rel Lstr_struct447]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1302:
    mov rax, 255
    mov [rbp - 1584], rax

    lea rax, [rbp - 288]
    push rax
    mov rax, 255
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1592], rax
    mov rax, [rbp - 1592]
    push rax
    mov rax, 0
    mov rcx, rax
    pop rax
    mov byte [rax], cl
ir_if_end_1297:
    jmp ir_while_end_1270
ir_if_next_1274:
ir_if_end_1273:

    movsxd rax, dword [rbp - 812]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1608], rax
    mov rax, [rbp - 1608]

    mov dword [rbp - 812], eax
    jmp ir_while_1269
ir_while_end_1270:

    sub rsp, 32

    mov rax, qword [rbp - 296]
    mov rcx, rax
    call fclose
    add rsp, 32


    mov [rbp - 1616], rax
    jmp ir_if_end_1267
ir_if_next_1268:
ir_if_end_1267:
    mov rax, 0

    mov dword [rbp - 820], eax
ir_while_1303:

    movsxd rax, dword [rbp - 820]
    push rax
    mov rax, 256
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 1624], rax
    mov rax, [rbp - 1624]
    test rax, rax
    jz ir_trap_bounds_1305
    jmp ir_in_bounds_1306
ir_trap_bounds_1305:

    sub rsp, 32

    lea rax, [rel Lstr_struct449]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1306:

    movsxd rax, dword [rbp - 820]
    mov [rbp - 1632], rax

    lea rax, [rbp - 288]
    push rax
    mov rax, [rbp - 1632]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1640], rax
    mov rax, [rbp - 1640]
    movzx rax, byte [rax]
    mov [rbp - 1648], rax
    mov rax, [rbp - 1648]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 1656], rax
    mov rax, [rbp - 1656]
    test rax, rax
    jz ir_while_end_1304

    movsxd rax, dword [rbp - 820]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1664], rax
    mov rax, [rbp - 1664]

    mov dword [rbp - 820], eax
    jmp ir_while_1303
ir_while_end_1304:
    mov rax, 1
    mov [rbp - 1672], rax
    jmp ir_in_bounds_1308
ir_trap_bounds_1307:

    sub rsp, 32

    lea rax, [rel Lstr_struct451]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1308:
    mov rax, 0
    mov [rbp - 1680], rax

    lea rax, [rbp - 288]
    mov [rbp - 1688], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 1688]
    mov rdx, rax

    movsxd rax, dword [rbp - 820]
    mov r8, rax
    call send_html_escaped
    add rsp, 32

    mov [rbp - 1696], rax

    lea rax, [rel FORUM_THREAD_MID]
    mov [rbp - 1704], rax
    mov rax, [rbp - 1704]
    mov rax, qword [rax]
    mov [rbp - 1712], rax

    lea rax, [rel FORUM_THREAD_MID]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1720], rax
    mov rax, [rbp - 1720]
    mov rax, qword [rax]
    mov [rbp - 1728], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 1712]
    mov rdx, rax
    mov rax, [rbp - 1728]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 1736], rax
    mov rax, 1
    mov [rbp - 1744], rax
    jmp ir_in_bounds_1310
ir_trap_bounds_1309:

    sub rsp, 32

    lea rax, [rel Lstr_struct453]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1310:
    mov rax, 0
    mov [rbp - 1752], rax

    lea rax, [rbp - 856]
    mov [rbp - 1760], rax

    sub rsp, 32
    mov rax, [rbp - 1760]
    mov rcx, rax

    movsxd rax, dword [rbp - 16]
    mov rdx, rax
    call build_posts_filename
    add rsp, 32

    mov [rbp - 1768], rax
    mov rax, 0

    mov dword [rbp - 860], eax
    mov rax, 1
    mov [rbp - 1776], rax
    jmp ir_in_bounds_1312
ir_trap_bounds_1311:

    sub rsp, 32

    lea rax, [rel Lstr_struct455]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1312:
    mov rax, 0
    mov [rbp - 1784], rax

    lea rax, [rbp - 856]
    mov [rbp - 1792], rax

    sub rsp, 32

    lea rax, [rel mode_r]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 1800], rax

    sub rsp, 32
    mov rax, [rbp - 1792]
    mov rcx, rax
    mov rax, [rbp - 1800]
    mov rdx, rax
    call fopen
    add rsp, 32

    mov [rbp - 1808], rax
    mov rax, [rbp - 1808]

    mov qword [rbp - 296], rax

    mov rax, qword [rbp - 296]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 1816], rax
    mov rax, [rbp - 1816]
    test rax, rax
    jz ir_if_next_1314
ir_while_1315:
    mov rax, 1
    mov [rbp - 1824], rax
    jmp ir_in_bounds_1318
ir_trap_bounds_1317:

    sub rsp, 32

    lea rax, [rel Lstr_struct457]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1318:
    mov rax, 0
    mov [rbp - 1832], rax

    lea rax, [rbp - 808]
    mov [rbp - 1840], rax

    sub rsp, 32
    mov rax, [rbp - 1840]
    mov rcx, rax
    mov rax, 2048
    mov rdx, rax

    mov rax, qword [rbp - 296]
    mov r8, rax
    call fgets
    add rsp, 32

    mov [rbp - 1848], rax
    mov rax, [rbp - 1848]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 1856], rax
    mov rax, [rbp - 1856]
    test rax, rax
    jz ir_while_end_1316
    mov rax, 0

    mov dword [rbp - 816], eax
ir_while_1319:

    movsxd rax, dword [rbp - 816]
    push rax
    mov rax, 2048
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 1864], rax
    mov rax, [rbp - 1864]
    test rax, rax
    jz ir_trap_bounds_1321
    jmp ir_in_bounds_1322
ir_trap_bounds_1321:

    sub rsp, 32

    lea rax, [rel Lstr_struct459]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1322:

    movsxd rax, dword [rbp - 816]
    mov [rbp - 1872], rax

    lea rax, [rbp - 808]
    push rax
    mov rax, [rbp - 1872]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1880], rax
    mov rax, [rbp - 1880]
    movzx rax, byte [rax]
    mov [rbp - 1888], rax
    mov rax, [rbp - 1888]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 1896], rax
    mov rax, [rbp - 1896]
    test rax, rax
    jz ir_sc_false_1325
ir_sc_rhs_1323:

    movsxd rax, dword [rbp - 816]
    push rax
    mov rax, 2048
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 1904], rax
    mov rax, [rbp - 1904]
    test rax, rax
    jz ir_trap_bounds_1327
    jmp ir_in_bounds_1328
ir_trap_bounds_1327:

    sub rsp, 32

    lea rax, [rel Lstr_struct461]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1328:

    movsxd rax, dword [rbp - 816]
    mov [rbp - 1912], rax

    lea rax, [rbp - 808]
    push rax
    mov rax, [rbp - 1912]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1920], rax
    mov rax, [rbp - 1920]
    movzx rax, byte [rax]
    mov [rbp - 1928], rax
    mov rax, [rbp - 1928]
    push rax
    mov rax, 10
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 1936], rax
    mov rax, [rbp - 1936]
    test rax, rax
    jz ir_sc_false_1325
ir_sc_true_1324:
    mov rax, 1
    mov [rbp - 1944], rax
    jmp ir_sc_end_1326
ir_sc_false_1325:
    mov rax, 0
    mov [rbp - 1944], rax
ir_sc_end_1326:
    mov rax, [rbp - 1944]
    test rax, rax
    jz ir_sc_false_1331
ir_sc_rhs_1329:

    movsxd rax, dword [rbp - 816]
    push rax
    mov rax, 2048
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 1960], rax
    mov rax, [rbp - 1960]
    test rax, rax
    jz ir_trap_bounds_1333
    jmp ir_in_bounds_1334
ir_trap_bounds_1333:

    sub rsp, 32

    lea rax, [rel Lstr_struct463]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1334:

    movsxd rax, dword [rbp - 816]
    mov [rbp - 1968], rax

    lea rax, [rbp - 808]
    push rax
    mov rax, [rbp - 1968]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1976], rax
    mov rax, [rbp - 1976]
    movzx rax, byte [rax]
    mov [rbp - 1984], rax
    mov rax, [rbp - 1984]
    push rax
    mov rax, 13
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 1992], rax
    mov rax, [rbp - 1992]
    test rax, rax
    jz ir_sc_false_1331
ir_sc_true_1330:
    mov rax, 1
    mov [rbp - 2000], rax
    jmp ir_sc_end_1332
ir_sc_false_1331:
    mov rax, 0
    mov [rbp - 2000], rax
ir_sc_end_1332:
    mov rax, [rbp - 2000]
    test rax, rax
    jz ir_while_end_1320

    movsxd rax, dword [rbp - 816]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 2016], rax
    mov rax, [rbp - 2016]

    mov dword [rbp - 816], eax
    jmp ir_while_1319
ir_while_end_1320:

    movsxd rax, dword [rbp - 816]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setg al
    movzx rax, al
    mov [rbp - 2024], rax
    mov rax, [rbp - 2024]
    test rax, rax
    jz ir_if_next_1336
    mov rax, 1

    mov dword [rbp - 860], eax

    lea rax, [rel FORUM_POST_OPEN]
    mov [rbp - 2032], rax
    mov rax, [rbp - 2032]
    mov rax, qword [rax]
    mov [rbp - 2040], rax

    lea rax, [rel FORUM_POST_OPEN]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 2048], rax
    mov rax, [rbp - 2048]
    mov rax, qword [rax]
    mov [rbp - 2056], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 2040]
    mov rdx, rax
    mov rax, [rbp - 2056]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 2064], rax
    mov rax, 1
    mov [rbp - 2072], rax
    jmp ir_in_bounds_1338
ir_trap_bounds_1337:

    sub rsp, 32

    lea rax, [rel Lstr_struct465]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1338:
    mov rax, 0
    mov [rbp - 2080], rax

    lea rax, [rbp - 808]
    mov [rbp - 2088], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 2088]
    mov rdx, rax

    movsxd rax, dword [rbp - 816]
    mov r8, rax
    call send_html_escaped
    add rsp, 32

    mov [rbp - 2096], rax

    lea rax, [rel FORUM_POST_CLOSE]
    mov [rbp - 2104], rax
    mov rax, [rbp - 2104]
    mov rax, qword [rax]
    mov [rbp - 2112], rax

    lea rax, [rel FORUM_POST_CLOSE]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 2120], rax
    mov rax, [rbp - 2120]
    mov rax, qword [rax]
    mov [rbp - 2128], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 2112]
    mov rdx, rax
    mov rax, [rbp - 2128]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 2136], rax
    jmp ir_if_end_1335
ir_if_next_1336:
ir_if_end_1335:
    jmp ir_while_1315
ir_while_end_1316:

    sub rsp, 32

    mov rax, qword [rbp - 296]
    mov rcx, rax
    call fclose
    add rsp, 32


    mov [rbp - 2144], rax
    jmp ir_if_end_1313
ir_if_next_1314:
ir_if_end_1313:

    sub rsp, 32

    mov rax, qword [rel file_mutex]
    mov rcx, rax
    call mutex_unlock
    add rsp, 32


    mov [rbp - 2152], rax

    movsxd rax, dword [rbp - 860]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 2160], rax
    mov rax, [rbp - 2160]
    test rax, rax
    jz ir_if_next_1340

    lea rax, [rel FORUM_NO_POSTS]
    mov [rbp - 2168], rax
    mov rax, [rbp - 2168]
    mov rax, qword [rax]
    mov [rbp - 2176], rax

    lea rax, [rel FORUM_NO_POSTS]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 2184], rax
    mov rax, [rbp - 2184]
    mov rax, qword [rax]
    mov [rbp - 2192], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 2176]
    mov rdx, rax
    mov rax, [rbp - 2192]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 2200], rax
    jmp ir_if_end_1339
ir_if_next_1340:
ir_if_end_1339:

    lea rax, [rel FORUM_THREAD_END]
    mov [rbp - 2208], rax
    mov rax, [rbp - 2208]
    mov rax, qword [rax]
    mov [rbp - 2216], rax

    lea rax, [rel FORUM_THREAD_END]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 2224], rax
    mov rax, [rbp - 2224]
    mov rax, qword [rax]
    mov [rbp - 2232], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 2216]
    mov rdx, rax
    mov rax, [rbp - 2232]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 2240], rax
    mov rax, 1
    mov [rbp - 2248], rax
    jmp ir_in_bounds_1342
ir_trap_bounds_1341:

    sub rsp, 32

    lea rax, [rel Lstr_struct467]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1342:
    mov rax, 0
    mov [rbp - 2256], rax

    lea rax, [rbp - 880]
    mov [rbp - 2264], rax

    sub rsp, 32
    mov rax, [rbp - 2264]
    mov rcx, rax

    movsxd rax, dword [rbp - 16]
    mov rdx, rax
    call int_to_dec
    add rsp, 32


    mov [rbp - 2272], rax
    mov rax, [rbp - 2272]

    mov dword [rbp - 884], eax
    mov rax, 1
    mov [rbp - 2280], rax
    jmp ir_in_bounds_1344
ir_trap_bounds_1343:

    sub rsp, 32

    lea rax, [rel Lstr_struct469]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1344:
    mov rax, 0
    mov [rbp - 2288], rax

    lea rax, [rbp - 880]
    mov [rbp - 2296], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 2296]
    mov rdx, rax

    movsxd rax, dword [rbp - 884]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 2304], rax

    lea rax, [rel FORUM_THREAD_END2]
    mov [rbp - 2312], rax
    mov rax, [rbp - 2312]
    mov rax, qword [rax]
    mov [rbp - 2320], rax

    lea rax, [rel FORUM_THREAD_END2]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 2328], rax
    mov rax, [rbp - 2328]
    mov rax, qword [rax]
    mov [rbp - 2336], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 2320]
    mov rdx, rax
    mov rax, [rbp - 2336]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 2344], rax
    mov rax, 0
    mov [rbp - 2352], rax
ir_errdefer_ok_1345:
ir_errdefer_end_1346:
    jmp Lserve_forum_thread_exit
Lserve_forum_thread_exit:

    mov rsp, rbp
    pop rbp
    ret

global handle_post_new_thread

handle_post_new_thread:
    push rbp
    mov rbp, rsp
    sub rsp, 1712

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx

    mov [rbp - 24], r8


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_1347:

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax

    movsxd rax, dword [rbp - 24]
    mov rdx, rax
    call find_body_start
    add rsp, 32


    mov [rbp - 352], rax
    mov rax, [rbp - 352]

    mov dword [rbp - 28], eax

    movsxd rax, dword [rbp - 24]
    push rax

    movsxd rax, dword [rbp - 28]
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 360], rax
    mov rax, [rbp - 360]

    mov dword [rbp - 32], eax

    movsxd rax, dword [rel dbg_on]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 368], rax
    mov rax, [rbp - 368]
    test rax, rax
    jz ir_if_next_1349

    sub rsp, 32

    lea rax, [rel dbg_bs]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 376], rax

    sub rsp, 32
    mov rax, [rbp - 376]
    mov rcx, rax

    movsxd rax, dword [rbp - 28]
    mov rdx, rax
    call dbg
    add rsp, 32

    mov [rbp - 384], rax

    sub rsp, 32

    lea rax, [rel dbg_bl]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 392], rax

    sub rsp, 32
    mov rax, [rbp - 392]
    mov rcx, rax

    movsxd rax, dword [rbp - 32]
    mov rdx, rax
    call dbg
    add rsp, 32

    mov [rbp - 400], rax
    jmp ir_if_end_1348
ir_if_next_1349:
ir_if_end_1348:

    sub rsp, 32

    lea rax, [rel key_title]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 408], rax
    mov rax, 1
    mov [rbp - 416], rax
    jmp ir_in_bounds_1351
ir_trap_bounds_1350:

    sub rsp, 32

    lea rax, [rel Lstr_struct471]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1351:
    mov rax, 0
    mov [rbp - 424], rax

    lea rax, [rbp - 288]
    mov [rbp - 432], rax

    sub rsp, 64
    mov rax, 5
    mov [rsp + 32], rax
    mov rax, [rbp - 432]
    mov [rsp + 40], rax
    mov rax, 256
    mov [rsp + 48], rax

    mov rax, qword [rbp - 16]
    mov rcx, rax

    movsxd rax, dword [rbp - 28]
    mov rdx, rax

    movsxd rax, dword [rbp - 32]
    mov r8, rax
    mov rax, [rbp - 408]
    mov r9, rax
    call extract_form_value
    add rsp, 64


    mov [rbp - 440], rax
    mov rax, [rbp - 440]

    mov dword [rbp - 292], eax

    movsxd rax, dword [rel dbg_on]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 448], rax
    mov rax, [rbp - 448]
    test rax, rax
    jz ir_if_next_1353

    sub rsp, 32

    lea rax, [rel dbg_title]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 456], rax

    sub rsp, 32
    mov rax, [rbp - 456]
    mov rcx, rax

    movsxd rax, dword [rbp - 292]
    mov rdx, rax
    call dbg
    add rsp, 32

    mov [rbp - 464], rax
    jmp ir_if_end_1352
ir_if_next_1353:
ir_if_end_1352:

    movsxd rax, dword [rbp - 292]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setg al
    movzx rax, al
    mov [rbp - 472], rax
    mov rax, [rbp - 472]
    test rax, rax
    jz ir_if_next_1355

    sub rsp, 32

    mov rax, qword [rel file_mutex]
    mov rcx, rax
    call mutex_lock_infinite
    add rsp, 32


    mov [rbp - 480], rax
    mov rax, [rbp - 480]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 488], rax
    mov rax, [rbp - 488]
    test rax, rax
    jz ir_if_next_1357

    movsxd rax, dword [rbp - 292]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 496], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 496]
    mov rdx, rax
    call serve_forum_index
    add rsp, 32

    mov [rbp - 504], rax
    mov rax, 0
    mov [rbp - 512], rax
ir_errdefer_ok_1358:
ir_errdefer_end_1359:
    jmp Lhandle_post_new_thread_exit
ir_if_next_1357:
ir_if_end_1356:

    sub rsp, 32
    call count_threads_impl
    add rsp, 32


    mov [rbp - 520], rax
    mov rax, [rbp - 520]

    mov dword [rbp - 296], eax

    sub rsp, 32

    lea rax, [rel fn_threads]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 528], rax

    sub rsp, 32

    lea rax, [rel mode_a]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 536], rax

    sub rsp, 32
    mov rax, [rbp - 528]
    mov rcx, rax
    mov rax, [rbp - 536]
    mov rdx, rax
    call fopen
    add rsp, 32

    mov [rbp - 544], rax
    mov rax, [rbp - 544]

    mov qword [rbp - 304], rax

    movsxd rax, dword [rel dbg_on]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 552], rax
    mov rax, [rbp - 552]
    test rax, rax
    jz ir_if_next_1361

    mov rax, qword [rbp - 304]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 560], rax
    mov rax, [rbp - 560]
    test rax, rax
    jz ir_if_next_1363

    sub rsp, 32

    lea rax, [rel dbg_fopen_ok]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 568], rax

    sub rsp, 32
    mov rax, [rbp - 568]
    mov rcx, rax
    call dbg_msg
    add rsp, 32

    mov [rbp - 576], rax
    jmp ir_if_end_1362
ir_if_next_1363:

    sub rsp, 32

    lea rax, [rel dbg_fopen_fail]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 584], rax

    sub rsp, 32
    mov rax, [rbp - 584]
    mov rcx, rax
    call dbg_msg
    add rsp, 32

    mov [rbp - 592], rax
ir_if_end_1362:
    jmp ir_if_end_1360
ir_if_next_1361:
ir_if_end_1360:

    mov rax, qword [rbp - 304]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 600], rax
    mov rax, [rbp - 600]
    test rax, rax
    jz ir_if_next_1365
    mov rax, 1
    mov [rbp - 608], rax
    jmp ir_in_bounds_1367
ir_trap_bounds_1366:

    sub rsp, 32

    lea rax, [rel Lstr_struct473]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1367:
    mov rax, 0
    mov [rbp - 616], rax

    lea rax, [rbp - 288]
    mov [rbp - 624], rax

    sub rsp, 32
    mov rax, [rbp - 624]
    mov rcx, rax

    mov rax, qword [rbp - 304]
    mov rdx, rax
    call fputs
    add rsp, 32


    mov [rbp - 632], rax

    sub rsp, 32

    lea rax, [rel nl]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 640], rax

    sub rsp, 32
    mov rax, [rbp - 640]
    mov rcx, rax

    mov rax, qword [rbp - 304]
    mov rdx, rax
    call fputs
    add rsp, 32


    mov [rbp - 648], rax

    sub rsp, 32

    mov rax, qword [rbp - 304]
    mov rcx, rax
    call fclose
    add rsp, 32


    mov [rbp - 656], rax

    sub rsp, 32

    mov rax, qword [rel file_mutex]
    mov rcx, rax
    call mutex_unlock
    add rsp, 32


    mov [rbp - 664], rax

    movsxd rax, dword [rbp - 296]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 672], rax
    mov rax, [rbp - 672]

    mov dword [rbp - 308], eax
    mov rax, 1
    mov [rbp - 680], rax
    jmp ir_in_bounds_1369
ir_trap_bounds_1368:

    sub rsp, 32

    lea rax, [rel Lstr_struct475]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1369:
    mov rax, 0
    mov [rbp - 688], rax

    lea rax, [rbp - 344]
    mov [rbp - 696], rax

    sub rsp, 32
    mov rax, [rbp - 696]
    mov rcx, rax

    movsxd rax, dword [rbp - 308]
    mov rdx, rax
    call build_forum_thread_url
    add rsp, 32

    mov [rbp - 704], rax
    mov rax, 1
    mov [rbp - 712], rax
    jmp ir_in_bounds_1371
ir_trap_bounds_1370:

    sub rsp, 32

    lea rax, [rel Lstr_struct477]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1371:
    mov rax, 0
    mov [rbp - 720], rax

    lea rax, [rbp - 344]
    mov [rbp - 728], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 728]
    mov rdx, rax
    call send_redirect
    add rsp, 32

    mov [rbp - 736], rax
    mov rax, 0
    mov [rbp - 744], rax
ir_errdefer_ok_1372:
ir_errdefer_end_1373:
    jmp Lhandle_post_new_thread_exit
ir_if_next_1365:
ir_if_end_1364:

    sub rsp, 32

    mov rax, qword [rel file_mutex]
    mov rcx, rax
    call mutex_unlock
    add rsp, 32


    mov [rbp - 752], rax
    jmp ir_if_end_1354
ir_if_next_1355:
ir_if_end_1354:

    movsxd rax, dword [rbp - 292]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 760], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 760]
    mov rdx, rax
    call serve_forum_index
    add rsp, 32

    mov [rbp - 768], rax
    mov rax, 0
    mov [rbp - 776], rax
ir_errdefer_ok_1374:
ir_errdefer_end_1375:
    jmp Lhandle_post_new_thread_exit
Lhandle_post_new_thread_exit:

    mov rsp, rbp
    pop rbp
    ret

global handle_post_reply

handle_post_reply:
    push rbp
    mov rbp, rsp
    sub rsp, 3696

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx

    mov [rbp - 24], r8

    mov [rbp - 32], r9


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_1376:

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax

    movsxd rax, dword [rbp - 24]
    mov rdx, rax
    call find_body_start
    add rsp, 32


    mov [rbp - 2176], rax
    mov rax, [rbp - 2176]

    mov dword [rbp - 36], eax

    movsxd rax, dword [rbp - 24]
    push rax

    movsxd rax, dword [rbp - 36]
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 2184], rax
    mov rax, [rbp - 2184]

    mov dword [rbp - 40], eax

    movsxd rax, dword [rel dbg_on]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 2192], rax
    mov rax, [rbp - 2192]
    test rax, rax
    jz ir_if_next_1378

    sub rsp, 32

    lea rax, [rel dbg_bs]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 2200], rax

    sub rsp, 32
    mov rax, [rbp - 2200]
    mov rcx, rax

    movsxd rax, dword [rbp - 36]
    mov rdx, rax
    call dbg
    add rsp, 32

    mov [rbp - 2208], rax

    sub rsp, 32

    lea rax, [rel dbg_bl]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 2216], rax

    sub rsp, 32
    mov rax, [rbp - 2216]
    mov rcx, rax

    movsxd rax, dword [rbp - 40]
    mov rdx, rax
    call dbg
    add rsp, 32

    mov [rbp - 2224], rax
    jmp ir_if_end_1377
ir_if_next_1378:
ir_if_end_1377:

    sub rsp, 32

    lea rax, [rel key_body]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 2232], rax
    mov rax, 1
    mov [rbp - 2240], rax
    jmp ir_in_bounds_1380
ir_trap_bounds_1379:

    sub rsp, 32

    lea rax, [rel Lstr_struct479]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1380:
    mov rax, 0
    mov [rbp - 2248], rax

    lea rax, [rbp - 2088]
    mov [rbp - 2256], rax

    sub rsp, 64
    mov rax, 4
    mov [rsp + 32], rax
    mov rax, [rbp - 2256]
    mov [rsp + 40], rax
    mov rax, 2048
    mov [rsp + 48], rax

    mov rax, qword [rbp - 16]
    mov rcx, rax

    movsxd rax, dword [rbp - 36]
    mov rdx, rax

    movsxd rax, dword [rbp - 40]
    mov r8, rax
    mov rax, [rbp - 2232]
    mov r9, rax
    call extract_form_value
    add rsp, 64


    mov [rbp - 2264], rax
    mov rax, [rbp - 2264]

    mov dword [rbp - 2092], eax

    movsxd rax, dword [rel dbg_on]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 2272], rax
    mov rax, [rbp - 2272]
    test rax, rax
    jz ir_if_next_1382

    sub rsp, 32

    lea rax, [rel dbg_title]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 2280], rax

    sub rsp, 32
    mov rax, [rbp - 2280]
    mov rcx, rax

    movsxd rax, dword [rbp - 2092]
    mov rdx, rax
    call dbg
    add rsp, 32

    mov [rbp - 2288], rax
    jmp ir_if_end_1381
ir_if_next_1382:
ir_if_end_1381:

    movsxd rax, dword [rbp - 2092]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setg al
    movzx rax, al
    mov [rbp - 2296], rax
    mov rax, [rbp - 2296]
    test rax, rax
    jz ir_if_next_1384

    sub rsp, 32

    mov rax, qword [rel file_mutex]
    mov rcx, rax
    call mutex_lock_infinite
    add rsp, 32


    mov [rbp - 2304], rax
    mov rax, [rbp - 2304]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 2312], rax
    mov rax, [rbp - 2312]
    test rax, rax
    jz ir_if_next_1386
    mov rax, 1
    mov [rbp - 2320], rax
    jmp ir_in_bounds_1388
ir_trap_bounds_1387:

    sub rsp, 32

    lea rax, [rel Lstr_struct481]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1388:
    mov rax, 0
    mov [rbp - 2328], rax

    lea rax, [rbp - 2128]
    mov [rbp - 2336], rax

    sub rsp, 32
    mov rax, [rbp - 2336]
    mov rcx, rax

    movsxd rax, dword [rbp - 32]
    mov rdx, rax
    call build_forum_thread_url
    add rsp, 32

    mov [rbp - 2344], rax
    mov rax, 1
    mov [rbp - 2352], rax
    jmp ir_in_bounds_1390
ir_trap_bounds_1389:

    sub rsp, 32

    lea rax, [rel Lstr_struct483]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1390:
    mov rax, 0
    mov [rbp - 2360], rax

    lea rax, [rbp - 2128]
    mov [rbp - 2368], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 2368]
    mov rdx, rax
    call send_redirect
    add rsp, 32

    mov [rbp - 2376], rax
    mov rax, 0
    mov [rbp - 2384], rax
ir_errdefer_ok_1391:
ir_errdefer_end_1392:
    jmp Lhandle_post_reply_exit
ir_if_next_1386:
ir_if_end_1385:
    mov rax, 1
    mov [rbp - 2392], rax
    jmp ir_in_bounds_1394
ir_trap_bounds_1393:

    sub rsp, 32

    lea rax, [rel Lstr_struct485]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1394:
    mov rax, 0
    mov [rbp - 2400], rax

    lea rax, [rbp - 2160]
    mov [rbp - 2408], rax

    sub rsp, 32
    mov rax, [rbp - 2408]
    mov rcx, rax

    movsxd rax, dword [rbp - 32]
    mov rdx, rax
    call build_posts_filename
    add rsp, 32

    mov [rbp - 2416], rax
    mov rax, 1
    mov [rbp - 2424], rax
    jmp ir_in_bounds_1396
ir_trap_bounds_1395:

    sub rsp, 32

    lea rax, [rel Lstr_struct487]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1396:
    mov rax, 0
    mov [rbp - 2432], rax

    lea rax, [rbp - 2160]
    mov [rbp - 2440], rax

    sub rsp, 32

    lea rax, [rel mode_a]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 2448], rax

    sub rsp, 32
    mov rax, [rbp - 2440]
    mov rcx, rax
    mov rax, [rbp - 2448]
    mov rdx, rax
    call fopen
    add rsp, 32

    mov [rbp - 2456], rax
    mov rax, [rbp - 2456]

    mov qword [rbp - 2168], rax

    movsxd rax, dword [rel dbg_on]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 2464], rax
    mov rax, [rbp - 2464]
    test rax, rax
    jz ir_if_next_1398

    mov rax, qword [rbp - 2168]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 2472], rax
    mov rax, [rbp - 2472]
    test rax, rax
    jz ir_if_next_1400

    sub rsp, 32

    lea rax, [rel dbg_fopen_ok]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 2480], rax

    sub rsp, 32
    mov rax, [rbp - 2480]
    mov rcx, rax
    call dbg_msg
    add rsp, 32

    mov [rbp - 2488], rax
    jmp ir_if_end_1399
ir_if_next_1400:

    sub rsp, 32

    lea rax, [rel dbg_fopen_fail]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 2496], rax

    sub rsp, 32
    mov rax, [rbp - 2496]
    mov rcx, rax
    call dbg_msg
    add rsp, 32

    mov [rbp - 2504], rax
ir_if_end_1399:
    jmp ir_if_end_1397
ir_if_next_1398:
ir_if_end_1397:

    mov rax, qword [rbp - 2168]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 2512], rax
    mov rax, [rbp - 2512]
    test rax, rax
    jz ir_if_next_1402
    mov rax, 1
    mov [rbp - 2520], rax
    jmp ir_in_bounds_1404
ir_trap_bounds_1403:

    sub rsp, 32

    lea rax, [rel Lstr_struct489]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1404:
    mov rax, 0
    mov [rbp - 2528], rax

    lea rax, [rbp - 2088]
    mov [rbp - 2536], rax

    sub rsp, 32
    mov rax, [rbp - 2536]
    mov rcx, rax

    mov rax, qword [rbp - 2168]
    mov rdx, rax
    call fputs
    add rsp, 32


    mov [rbp - 2544], rax

    sub rsp, 32

    lea rax, [rel nl]
    mov rcx, rax
    call cstr
    add rsp, 32

    mov [rbp - 2552], rax

    sub rsp, 32
    mov rax, [rbp - 2552]
    mov rcx, rax

    mov rax, qword [rbp - 2168]
    mov rdx, rax
    call fputs
    add rsp, 32


    mov [rbp - 2560], rax

    sub rsp, 32

    mov rax, qword [rbp - 2168]
    mov rcx, rax
    call fclose
    add rsp, 32


    mov [rbp - 2568], rax
    jmp ir_if_end_1401
ir_if_next_1402:
ir_if_end_1401:

    sub rsp, 32

    mov rax, qword [rel file_mutex]
    mov rcx, rax
    call mutex_unlock
    add rsp, 32


    mov [rbp - 2576], rax
    jmp ir_if_end_1383
ir_if_next_1384:
ir_if_end_1383:
    mov rax, 1
    mov [rbp - 2584], rax
    jmp ir_in_bounds_1406
ir_trap_bounds_1405:

    sub rsp, 32

    lea rax, [rel Lstr_struct491]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1406:
    mov rax, 0
    mov [rbp - 2592], rax

    lea rax, [rbp - 2128]
    mov [rbp - 2600], rax

    sub rsp, 32
    mov rax, [rbp - 2600]
    mov rcx, rax

    movsxd rax, dword [rbp - 32]
    mov rdx, rax
    call build_forum_thread_url
    add rsp, 32

    mov [rbp - 2608], rax
    mov rax, 1
    mov [rbp - 2616], rax
    jmp ir_in_bounds_1408
ir_trap_bounds_1407:

    sub rsp, 32

    lea rax, [rel Lstr_struct493]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1408:
    mov rax, 0
    mov [rbp - 2624], rax

    lea rax, [rbp - 2128]
    mov [rbp - 2632], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 2632]
    mov rdx, rax
    call send_redirect
    add rsp, 32

    mov [rbp - 2640], rax
    mov rax, 0
    mov [rbp - 2648], rax
ir_errdefer_ok_1409:
ir_errdefer_end_1410:
    jmp Lhandle_post_reply_exit
Lhandle_post_reply_exit:

    mov rsp, rbp
    pop rbp
    ret

global handle_request

handle_request:
    push rbp
    mov rbp, rsp
    sub rsp, 3120

    mov [rbp - 8], rcx

    mov [rbp - 16], rdx

    mov [rbp - 24], r8


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_1411:

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax

    movsxd rax, dword [rbp - 24]
    mov rdx, rax
    call is_get
    add rsp, 32


    mov [rbp - 40], rax
    mov rax, [rbp - 40]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    test rax, rax
    jz ir_if_next_1413

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax

    movsxd rax, dword [rbp - 24]
    mov rdx, rax
    call is_health
    add rsp, 32


    mov [rbp - 56], rax
    mov rax, [rbp - 56]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 64], rax
    mov rax, [rbp - 64]
    test rax, rax
    jz ir_if_next_1415

    lea rax, [rel HTTP_HEALTH_HEADER]
    mov [rbp - 72], rax
    mov rax, [rbp - 72]
    mov rax, qword [rax]
    mov [rbp - 80], rax

    lea rax, [rel HTTP_HEALTH_HEADER]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 88], rax
    mov rax, [rbp - 88]
    mov rax, qword [rax]
    mov [rbp - 96], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 80]
    mov rdx, rax
    mov rax, [rbp - 96]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 104], rax

    lea rax, [rel HTTP_HEALTH_BODY]
    mov [rbp - 112], rax
    mov rax, [rbp - 112]
    mov rax, qword [rax]
    mov [rbp - 120], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 120]
    mov rdx, rax
    mov rax, 2
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 128], rax
    jmp ir_if_end_1414
ir_if_next_1415:

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax

    movsxd rax, dword [rbp - 24]
    mov rdx, rax
    call is_root
    add rsp, 32


    mov [rbp - 136], rax
    mov rax, [rbp - 136]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 144], rax
    mov rax, [rbp - 144]
    test rax, rax
    jz ir_if_next_1416

    lea rax, [rel HTTP_PAGE_HEADER]
    mov [rbp - 152], rax
    mov rax, [rbp - 152]
    mov rax, qword [rax]
    mov [rbp - 160], rax

    lea rax, [rel HTTP_PAGE_HEADER]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 168], rax
    mov rax, [rbp - 168]
    mov rax, qword [rax]
    mov [rbp - 176], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 160]
    mov rdx, rax
    mov rax, [rbp - 176]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 184], rax

    lea rax, [rel HTTP_PAGE_HEADER]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 192], rax
    mov rax, [rbp - 192]
    mov rax, qword [rax]
    mov [rbp - 200], rax
    mov rax, [rbp - 184]
    push rax
    mov rax, [rbp - 200]
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 208], rax
    mov rax, [rbp - 208]
    test rax, rax
    jz ir_if_next_1418

    lea rax, [rel PAGE_CONTENT]
    mov [rbp - 216], rax
    mov rax, [rbp - 216]
    mov rax, qword [rax]
    mov [rbp - 224], rax

    lea rax, [rel PAGE_CONTENT]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 232], rax
    mov rax, [rbp - 232]
    mov rax, qword [rax]
    mov [rbp - 240], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 224]
    mov rdx, rax
    mov rax, [rbp - 240]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 248], rax
    jmp ir_if_end_1417
ir_if_next_1418:
ir_if_end_1417:
    jmp ir_if_end_1414
ir_if_next_1416:

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax

    movsxd rax, dword [rbp - 24]
    mov rdx, rax
    call is_demo
    add rsp, 32


    mov [rbp - 256], rax
    mov rax, [rbp - 256]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 264], rax
    mov rax, [rbp - 264]
    test rax, rax
    jz ir_if_next_1419

    lea rax, [rel HTTP_DEMO_HEADER]
    mov [rbp - 272], rax
    mov rax, [rbp - 272]
    mov rax, qword [rax]
    mov [rbp - 280], rax

    lea rax, [rel HTTP_DEMO_HEADER]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 288], rax
    mov rax, [rbp - 288]
    mov rax, qword [rax]
    mov [rbp - 296], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 280]
    mov rdx, rax
    mov rax, [rbp - 296]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 304], rax

    lea rax, [rel HTTP_DEMO_HEADER]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 312], rax
    mov rax, [rbp - 312]
    mov rax, qword [rax]
    mov [rbp - 320], rax
    mov rax, [rbp - 304]
    push rax
    mov rax, [rbp - 320]
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 328], rax
    mov rax, [rbp - 328]
    test rax, rax
    jz ir_if_next_1421

    lea rax, [rel DEMO_CONTENT]
    mov [rbp - 336], rax
    mov rax, [rbp - 336]
    mov rax, qword [rax]
    mov [rbp - 344], rax

    lea rax, [rel DEMO_CONTENT]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 352], rax
    mov rax, [rbp - 352]
    mov rax, qword [rax]
    mov [rbp - 360], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 344]
    mov rdx, rax
    mov rax, [rbp - 360]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 368], rax
    jmp ir_if_end_1420
ir_if_next_1421:
ir_if_end_1420:
    jmp ir_if_end_1414
ir_if_next_1419:

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax

    movsxd rax, dword [rbp - 24]
    mov rdx, rax
    call is_benchmarks
    add rsp, 32


    mov [rbp - 376], rax
    mov rax, [rbp - 376]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 384], rax
    mov rax, [rbp - 384]
    test rax, rax
    jz ir_if_next_1422

    lea rax, [rel HTTP_BENCHMARKS_HEADER]
    mov [rbp - 392], rax
    mov rax, [rbp - 392]
    mov rax, qword [rax]
    mov [rbp - 400], rax

    lea rax, [rel HTTP_BENCHMARKS_HEADER]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 408], rax
    mov rax, [rbp - 408]
    mov rax, qword [rax]
    mov [rbp - 416], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 400]
    mov rdx, rax
    mov rax, [rbp - 416]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 424], rax

    lea rax, [rel HTTP_BENCHMARKS_HEADER]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 432], rax
    mov rax, [rbp - 432]
    mov rax, qword [rax]
    mov [rbp - 440], rax
    mov rax, [rbp - 424]
    push rax
    mov rax, [rbp - 440]
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 448], rax
    mov rax, [rbp - 448]
    test rax, rax
    jz ir_if_next_1424

    lea rax, [rel BENCHMARKS_CONTENT]
    mov [rbp - 456], rax
    mov rax, [rbp - 456]
    mov rax, qword [rax]
    mov [rbp - 464], rax

    lea rax, [rel BENCHMARKS_CONTENT]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 472], rax
    mov rax, [rbp - 472]
    mov rax, qword [rax]
    mov [rbp - 480], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 464]
    mov rdx, rax
    mov rax, [rbp - 480]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 488], rax
    jmp ir_if_end_1423
ir_if_next_1424:
ir_if_end_1423:
    jmp ir_if_end_1414
ir_if_next_1422:

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax

    movsxd rax, dword [rbp - 24]
    mov rdx, rax
    call is_docs
    add rsp, 32


    mov [rbp - 496], rax
    mov rax, [rbp - 496]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 504], rax
    mov rax, [rbp - 504]
    test rax, rax
    jz ir_if_next_1425

    lea rax, [rel HTTP_DOCS_HEADER]
    mov [rbp - 512], rax
    mov rax, [rbp - 512]
    mov rax, qword [rax]
    mov [rbp - 520], rax

    lea rax, [rel HTTP_DOCS_HEADER]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 528], rax
    mov rax, [rbp - 528]
    mov rax, qword [rax]
    mov [rbp - 536], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 520]
    mov rdx, rax
    mov rax, [rbp - 536]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 544], rax

    lea rax, [rel HTTP_DOCS_HEADER]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 552], rax
    mov rax, [rbp - 552]
    mov rax, qword [rax]
    mov [rbp - 560], rax
    mov rax, [rbp - 544]
    push rax
    mov rax, [rbp - 560]
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 568], rax
    mov rax, [rbp - 568]
    test rax, rax
    jz ir_if_next_1427

    lea rax, [rel DOCS_CONTENT]
    mov [rbp - 576], rax
    mov rax, [rbp - 576]
    mov rax, qword [rax]
    mov [rbp - 584], rax

    lea rax, [rel DOCS_CONTENT]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 592], rax
    mov rax, [rbp - 592]
    mov rax, qword [rax]
    mov [rbp - 600], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 584]
    mov rdx, rax
    mov rax, [rbp - 600]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 608], rax
    jmp ir_if_end_1426
ir_if_next_1427:
ir_if_end_1426:
    jmp ir_if_end_1414
ir_if_next_1425:

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax

    movsxd rax, dword [rbp - 24]
    mov rdx, rax
    mov rax, 4
    mov r8, rax
    call is_forum
    add rsp, 32


    mov [rbp - 616], rax
    mov rax, [rbp - 616]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 624], rax
    mov rax, [rbp - 624]
    test rax, rax
    jz ir_if_next_1428

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax

    movsxd rax, dword [rbp - 24]
    mov rdx, rax
    mov rax, 4
    mov r8, rax
    call get_thread_id
    add rsp, 32


    mov [rbp - 632], rax
    mov rax, [rbp - 632]

    mov dword [rbp - 28], eax

    movsxd rax, dword [rbp - 28]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setg al
    movzx rax, al
    mov [rbp - 640], rax
    mov rax, [rbp - 640]
    test rax, rax
    jz ir_if_next_1430

    sub rsp, 32
    call count_threads
    add rsp, 32


    mov [rbp - 648], rax
    mov rax, [rbp - 648]

    mov dword [rbp - 32], eax

    movsxd rax, dword [rbp - 28]
    push rax

    movsxd rax, dword [rbp - 32]
    mov r10, rax
    pop rax
    cmp rax, r10
    setle al
    movzx rax, al
    mov [rbp - 656], rax
    mov rax, [rbp - 656]
    test rax, rax
    jz ir_if_next_1432

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax

    movsxd rax, dword [rbp - 28]
    mov rdx, rax
    call serve_forum_thread
    add rsp, 32

    mov [rbp - 664], rax
    jmp ir_if_end_1431
ir_if_next_1432:

    lea rax, [rel HTTP_404_HEADER]
    mov [rbp - 672], rax
    mov rax, [rbp - 672]
    mov rax, qword [rax]
    mov [rbp - 680], rax

    lea rax, [rel HTTP_404_HEADER]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 688], rax
    mov rax, [rbp - 688]
    mov rax, qword [rax]
    mov [rbp - 696], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 680]
    mov rdx, rax
    mov rax, [rbp - 696]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 704], rax

    lea rax, [rel HTTP_404_BODY]
    mov [rbp - 712], rax
    mov rax, [rbp - 712]
    mov rax, qword [rax]
    mov [rbp - 720], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 720]
    mov rdx, rax
    mov rax, 9
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 728], rax
ir_if_end_1431:
    jmp ir_if_end_1429
ir_if_next_1430:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, 0
    mov rdx, rax
    call serve_forum_index
    add rsp, 32

    mov [rbp - 736], rax
ir_if_end_1429:
    jmp ir_if_end_1414
ir_if_next_1428:

    lea rax, [rel HTTP_404_HEADER]
    mov [rbp - 744], rax
    mov rax, [rbp - 744]
    mov rax, qword [rax]
    mov [rbp - 752], rax

    lea rax, [rel HTTP_404_HEADER]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 760], rax
    mov rax, [rbp - 760]
    mov rax, qword [rax]
    mov [rbp - 768], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 752]
    mov rdx, rax
    mov rax, [rbp - 768]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 776], rax

    lea rax, [rel HTTP_404_BODY]
    mov [rbp - 784], rax
    mov rax, [rbp - 784]
    mov rax, qword [rax]
    mov [rbp - 792], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 792]
    mov rdx, rax
    mov rax, 9
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 800], rax
ir_if_end_1414:
    jmp ir_if_end_1412
ir_if_next_1413:

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax

    movsxd rax, dword [rbp - 24]
    mov rdx, rax
    call is_post
    add rsp, 32


    mov [rbp - 808], rax
    mov rax, [rbp - 808]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 816], rax
    mov rax, [rbp - 816]
    test rax, rax
    jz ir_if_next_1433

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax

    movsxd rax, dword [rbp - 24]
    mov rdx, rax
    mov rax, 5
    mov r8, rax
    call is_forum
    add rsp, 32


    mov [rbp - 824], rax
    mov rax, [rbp - 824]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 832], rax
    mov rax, [rbp - 832]
    test rax, rax
    jz ir_if_next_1435

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax

    movsxd rax, dword [rbp - 24]
    mov rdx, rax
    mov rax, 5
    mov r8, rax
    call get_thread_id
    add rsp, 32


    mov [rbp - 840], rax
    mov rax, [rbp - 840]

    mov dword [rbp - 28], eax

    movsxd rax, dword [rbp - 28]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setg al
    movzx rax, al
    mov [rbp - 848], rax
    mov rax, [rbp - 848]
    test rax, rax
    jz ir_if_next_1437

    sub rsp, 32
    call count_threads
    add rsp, 32


    mov [rbp - 856], rax
    mov rax, [rbp - 856]

    mov dword [rbp - 32], eax

    movsxd rax, dword [rbp - 28]
    push rax

    movsxd rax, dword [rbp - 32]
    mov r10, rax
    pop rax
    cmp rax, r10
    setle al
    movzx rax, al
    mov [rbp - 864], rax
    mov rax, [rbp - 864]
    test rax, rax
    jz ir_if_next_1439

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax

    mov rax, qword [rbp - 16]
    mov rdx, rax

    movsxd rax, dword [rbp - 24]
    mov r8, rax

    movsxd rax, dword [rbp - 28]
    mov r9, rax
    call handle_post_reply
    add rsp, 32

    mov [rbp - 872], rax
    jmp ir_if_end_1438
ir_if_next_1439:

    lea rax, [rel HTTP_404_HEADER]
    mov [rbp - 880], rax
    mov rax, [rbp - 880]
    mov rax, qword [rax]
    mov [rbp - 888], rax

    lea rax, [rel HTTP_404_HEADER]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 896], rax
    mov rax, [rbp - 896]
    mov rax, qword [rax]
    mov [rbp - 904], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 888]
    mov rdx, rax
    mov rax, [rbp - 904]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 912], rax

    lea rax, [rel HTTP_404_BODY]
    mov [rbp - 920], rax
    mov rax, [rbp - 920]
    mov rax, qword [rax]
    mov [rbp - 928], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 928]
    mov rdx, rax
    mov rax, 9
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 936], rax
ir_if_end_1438:
    jmp ir_if_end_1436
ir_if_next_1437:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax

    mov rax, qword [rbp - 16]
    mov rdx, rax

    movsxd rax, dword [rbp - 24]
    mov r8, rax
    call handle_post_new_thread
    add rsp, 32

    mov [rbp - 944], rax
ir_if_end_1436:
    jmp ir_if_end_1434
ir_if_next_1435:

    lea rax, [rel HTTP_404_HEADER]
    mov [rbp - 952], rax
    mov rax, [rbp - 952]
    mov rax, qword [rax]
    mov [rbp - 960], rax

    lea rax, [rel HTTP_404_HEADER]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 968], rax
    mov rax, [rbp - 968]
    mov rax, qword [rax]
    mov [rbp - 976], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 960]
    mov rdx, rax
    mov rax, [rbp - 976]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 984], rax

    lea rax, [rel HTTP_404_BODY]
    mov [rbp - 992], rax
    mov rax, [rbp - 992]
    mov rax, qword [rax]
    mov [rbp - 1000], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 1000]
    mov rdx, rax
    mov rax, 9
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 1008], rax
ir_if_end_1434:
    jmp ir_if_end_1412
ir_if_next_1433:

    lea rax, [rel HTTP_404_HEADER]
    mov [rbp - 1016], rax
    mov rax, [rbp - 1016]
    mov rax, qword [rax]
    mov [rbp - 1024], rax

    lea rax, [rel HTTP_404_HEADER]
    push rax
    mov rax, 8
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 1032], rax
    mov rax, [rbp - 1032]
    mov rax, qword [rax]
    mov [rbp - 1040], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 1024]
    mov rdx, rax
    mov rax, [rbp - 1040]
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 1048], rax

    lea rax, [rel HTTP_404_BODY]
    mov [rbp - 1056], rax
    mov rax, [rbp - 1056]
    mov rax, qword [rax]
    mov [rbp - 1064], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 1064]
    mov rdx, rax
    mov rax, 9
    mov r8, rax
    call send_all
    add rsp, 32


    mov [rbp - 1072], rax
ir_if_end_1412:
    mov rax, 0
    mov [rbp - 1080], rax
ir_errdefer_ok_1440:
ir_errdefer_end_1441:
    jmp Lhandle_request_exit
Lhandle_request_exit:

    mov rsp, rbp
    pop rbp
    ret

global handle_connection

handle_connection:
    push rbp
    mov rbp, rsp
    sub rsp, 1024

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_1442:

    sub rsp, 32
    call thread_gc_attach
    add rsp, 32

    mov [rbp - 48], rax

    sub rsp, 32
    mov rax, 4096
    mov rcx, rax
    call malloc
    add rsp, 32

    mov [rbp - 56], rax
    mov rax, [rbp - 56]

    mov qword [rbp - 16], rax

    mov rax, qword [rbp - 16]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 64], rax
    mov rax, [rbp - 64]
    test rax, rax
    jz ir_if_next_1444

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    call closesocket
    add rsp, 32


    mov [rbp - 72], rax

    sub rsp, 32
    call SD_BOTH
    add rsp, 32


    mov [rbp - 80], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 80]
    mov rdx, rax
    call shutdown
    add rsp, 32


    mov [rbp - 88], rax

    sub rsp, 32
    call thread_gc_detach
    add rsp, 32

    mov [rbp - 96], rax
    mov rax, 0
    mov [rbp - 104], rax
ir_errdefer_ok_1445:
ir_errdefer_end_1446:
    jmp Lhandle_connection_exit
ir_if_next_1444:
ir_if_end_1443:

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax

    mov rax, qword [rbp - 16]
    mov rdx, rax
    mov rax, 4096
    mov r8, rax
    mov rax, 0
    mov r9, rax
    call recv
    add rsp, 32


    mov [rbp - 112], rax
    mov rax, [rbp - 112]

    mov dword [rbp - 20], eax

    movsxd rax, dword [rbp - 20]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setg al
    movzx rax, al
    mov [rbp - 120], rax
    mov rax, [rbp - 120]
    test rax, rax
    jz ir_sc_false_1451
ir_sc_rhs_1449:

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax

    movsxd rax, dword [rbp - 20]
    mov rdx, rax
    call is_post
    add rsp, 32


    mov [rbp - 128], rax
    mov rax, [rbp - 128]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 136], rax
    mov rax, [rbp - 136]
    test rax, rax
    jz ir_sc_false_1451
ir_sc_true_1450:
    mov rax, 1
    mov [rbp - 144], rax
    jmp ir_sc_end_1452
ir_sc_false_1451:
    mov rax, 0
    mov [rbp - 144], rax
ir_sc_end_1452:
    mov rax, [rbp - 144]
    test rax, rax
    jz ir_if_next_1448

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax

    movsxd rax, dword [rbp - 20]
    mov rdx, rax
    call find_body_start
    add rsp, 32


    mov [rbp - 160], rax
    mov rax, [rbp - 160]

    mov dword [rbp - 24], eax

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax

    movsxd rax, dword [rbp - 24]
    mov rdx, rax
    call parse_content_length
    add rsp, 32


    mov [rbp - 168], rax
    mov rax, [rbp - 168]

    mov dword [rbp - 28], eax

    movsxd rax, dword [rbp - 24]
    push rax

    movsxd rax, dword [rbp - 28]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 176], rax
    mov rax, [rbp - 176]

    mov dword [rbp - 32], eax

    movsxd rax, dword [rbp - 28]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setg al
    movzx rax, al
    mov [rbp - 184], rax
    mov rax, [rbp - 184]
    test rax, rax
    jz ir_sc_false_1457
ir_sc_rhs_1455:

    movsxd rax, dword [rbp - 32]
    push rax
    mov rax, 4096
    mov r10, rax
    pop rax
    cmp rax, r10
    setle al
    movzx rax, al
    mov [rbp - 192], rax
    mov rax, [rbp - 192]
    test rax, rax
    jz ir_sc_false_1457
ir_sc_true_1456:
    mov rax, 1
    mov [rbp - 200], rax
    jmp ir_sc_end_1458
ir_sc_false_1457:
    mov rax, 0
    mov [rbp - 200], rax
ir_sc_end_1458:
    mov rax, [rbp - 200]
    test rax, rax
    jz ir_sc_false_1461
ir_sc_rhs_1459:

    movsxd rax, dword [rbp - 20]
    push rax

    movsxd rax, dword [rbp - 32]
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 216], rax
    mov rax, [rbp - 216]
    test rax, rax
    jz ir_sc_false_1461
ir_sc_true_1460:
    mov rax, 1
    mov [rbp - 224], rax
    jmp ir_sc_end_1462
ir_sc_false_1461:
    mov rax, 0
    mov [rbp - 224], rax
ir_sc_end_1462:
    mov rax, [rbp - 224]
    test rax, rax
    jz ir_if_next_1454
ir_while_1463:

    movsxd rax, dword [rbp - 20]
    push rax

    movsxd rax, dword [rbp - 32]
    mov r10, rax
    pop rax
    cmp rax, r10
    setl al
    movzx rax, al
    mov [rbp - 240], rax
    mov rax, [rbp - 240]
    test rax, rax
    jz ir_while_end_1464

    mov rax, qword [rbp - 16]
    test rax, rax
    jz ir_trap_null_1465
    jmp ir_nonnull_1466
ir_trap_null_1465:

    sub rsp, 32

    lea rax, [rel Lstr_struct495]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1466:

    movsxd rax, dword [rbp - 20]
    mov [rbp - 248], rax

    mov rax, qword [rbp - 16]
    push rax
    mov rax, [rbp - 248]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 256], rax

    movsxd rax, dword [rbp - 32]
    push rax

    movsxd rax, dword [rbp - 20]
    mov r10, rax
    pop rax
    sub rax, r10
    mov [rbp - 264], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 256]
    mov rdx, rax
    mov rax, [rbp - 264]
    mov r8, rax
    mov rax, 0
    mov r9, rax
    call recv
    add rsp, 32


    mov [rbp - 272], rax
    mov rax, [rbp - 272]

    mov dword [rbp - 36], eax

    movsxd rax, dword [rbp - 36]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setle al
    movzx rax, al
    mov [rbp - 280], rax
    mov rax, [rbp - 280]
    test rax, rax
    jz ir_if_next_1468
    jmp ir_while_end_1464
ir_if_next_1468:
ir_if_end_1467:

    movsxd rax, dword [rbp - 20]
    push rax

    movsxd rax, dword [rbp - 36]
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 288], rax
    mov rax, [rbp - 288]

    mov dword [rbp - 20], eax
    jmp ir_while_1463
ir_while_end_1464:
    jmp ir_if_end_1453
ir_if_next_1454:
ir_if_end_1453:
    jmp ir_if_end_1447
ir_if_next_1448:
ir_if_end_1447:

    movsxd rax, dword [rbp - 20]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setg al
    movzx rax, al
    mov [rbp - 296], rax
    mov rax, [rbp - 296]
    test rax, rax
    jz ir_if_next_1470

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax

    mov rax, qword [rbp - 16]
    mov rdx, rax

    movsxd rax, dword [rbp - 20]
    mov r8, rax
    call handle_request
    add rsp, 32

    mov [rbp - 304], rax
    jmp ir_if_end_1469
ir_if_next_1470:
ir_if_end_1469:

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax
    call free
    add rsp, 32

    mov [rbp - 312], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    call closesocket
    add rsp, 32


    mov [rbp - 320], rax

    sub rsp, 32
    call SD_BOTH
    add rsp, 32


    mov [rbp - 328], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    mov rax, [rbp - 328]
    mov rdx, rax
    call shutdown
    add rsp, 32


    mov [rbp - 336], rax

    sub rsp, 32
    call thread_gc_detach
    add rsp, 32

    mov [rbp - 344], rax
    mov rax, 0
    mov [rbp - 352], rax
ir_errdefer_ok_1471:
ir_errdefer_end_1472:
    jmp Lhandle_connection_exit
Lhandle_connection_exit:

    mov rsp, rbp
    pop rbp
    ret

global thread_entry

thread_entry:
    push rbp
    mov rbp, rsp
    sub rsp, 176

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_1473:

    mov rax, qword [rbp - 8]
    mov [rbp - 32], rax
    mov rax, [rbp - 32]

    mov qword [rbp - 16], rax

    mov rax, qword [rbp - 16]
    test rax, rax
    jz ir_trap_null_1474
    jmp ir_nonnull_1475
ir_trap_null_1474:

    sub rsp, 32

    lea rax, [rel Lstr_struct497]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1475:

    mov rax, qword [rbp - 16]
    mov rax, qword [rax]
    mov [rbp - 40], rax
    mov rax, [rbp - 40]

    mov qword [rbp - 24], rax

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    call free
    add rsp, 32

    mov [rbp - 48], rax

    sub rsp, 32

    mov rax, qword [rbp - 24]
    mov rcx, rax
    call handle_connection
    add rsp, 32

    mov [rbp - 56], rax
    mov rax, 0
    mov [rbp - 64], rax
ir_errdefer_ok_1476:
ir_errdefer_end_1477:
    mov rax, 0
    jmp Lthread_entry_exit
Lthread_entry_exit:

    mov rsp, rbp
    pop rbp
    ret

global spawn_client_thread

spawn_client_thread:
    push rbp
    mov rbp, rsp
    sub rsp, 384

    mov [rbp - 8], rcx


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_1478:

    sub rsp, 32
    mov rax, 8
    mov rcx, rax
    call malloc
    add rsp, 32

    mov [rbp - 40], rax
    mov rax, [rbp - 40]

    mov qword [rbp - 16], rax

    mov rax, qword [rbp - 16]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 48], rax
    mov rax, [rbp - 48]
    test rax, rax
    jz ir_if_next_1480

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    call closesocket
    add rsp, 32


    mov [rbp - 56], rax
    mov rax, 0
    mov [rbp - 64], rax
ir_errdefer_ok_1481:
ir_errdefer_end_1482:
    jmp Lspawn_client_thread_exit
ir_if_next_1480:
ir_if_end_1479:

    mov rax, qword [rbp - 16]
    mov [rbp - 72], rax
    mov rax, [rbp - 72]

    mov qword [rbp - 24], rax

    mov rax, qword [rbp - 24]
    test rax, rax
    jz ir_trap_null_1483
    jmp ir_nonnull_1484
ir_trap_null_1483:

    sub rsp, 32

    lea rax, [rel Lstr_struct499]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_nonnull_1484:

    mov rax, qword [rbp - 24]
    push rax

    mov rax, qword [rbp - 8]
    mov rcx, rax
    pop rax
    mov qword [rax], rcx
    lea rax, [rel thread_entry]
    mov [rbp - 80], rax

    sub rsp, 48
    mov rax, 0
    mov [rsp + 32], rax
    mov rax, 0
    mov [rsp + 40], rax
    mov rax, 0
    mov rcx, rax
    mov rax, 0
    mov rdx, rax
    mov rax, [rbp - 80]
    mov r8, rax

    mov rax, qword [rbp - 16]
    mov r9, rax
    call CreateThread
    add rsp, 48

    mov [rbp - 88], rax
    mov rax, [rbp - 88]

    mov qword [rbp - 32], rax

    mov rax, qword [rbp - 32]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 96], rax
    mov rax, [rbp - 96]
    test rax, rax
    jz ir_if_next_1486

    sub rsp, 32

    mov rax, qword [rbp - 8]
    mov rcx, rax
    call closesocket
    add rsp, 32


    mov [rbp - 104], rax

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax
    call free
    add rsp, 32

    mov [rbp - 112], rax
    mov rax, 0
    mov [rbp - 120], rax
ir_errdefer_ok_1487:
ir_errdefer_end_1488:
    jmp Lspawn_client_thread_exit
ir_if_next_1486:
ir_if_end_1485:

    sub rsp, 32

    mov rax, qword [rbp - 32]
    mov rcx, rax
    call thread_detach
    add rsp, 32


    mov [rbp - 128], rax
    mov rax, 0
    mov [rbp - 136], rax
ir_errdefer_ok_1489:
ir_errdefer_end_1490:
    jmp Lspawn_client_thread_exit
Lspawn_client_thread_exit:

    mov rsp, rbp
    pop rbp
    ret

global main

main:
    push rbp
    mov rbp, rsp
    sub rsp, 2352


    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 256
    movdqu [rsp + 0], xmm0
    movdqu [rsp + 16], xmm1
    movdqu [rsp + 32], xmm2
    movdqu [rsp + 48], xmm3
    movdqu [rsp + 64], xmm4
    movdqu [rsp + 80], xmm5
    movdqu [rsp + 96], xmm6
    movdqu [rsp + 112], xmm7
    movdqu [rsp + 128], xmm8
    movdqu [rsp + 144], xmm9
    movdqu [rsp + 160], xmm10
    movdqu [rsp + 176], xmm11
    movdqu [rsp + 192], xmm12
    movdqu [rsp + 208], xmm13
    movdqu [rsp + 224], xmm14
    movdqu [rsp + 240], xmm15
    sub rsp, 32
    mov rcx, rsp
    extern gc_safepoint
    call gc_safepoint
    add rsp, 32
    movdqu xmm0, [rsp + 0]
    movdqu xmm1, [rsp + 16]
    movdqu xmm2, [rsp + 32]
    movdqu xmm3, [rsp + 48]
    movdqu xmm4, [rsp + 64]
    movdqu xmm5, [rsp + 80]
    movdqu xmm6, [rsp + 96]
    movdqu xmm7, [rsp + 112]
    movdqu xmm8, [rsp + 128]
    movdqu xmm9, [rsp + 144]
    movdqu xmm10, [rsp + 160]
    movdqu xmm11, [rsp + 176]
    movdqu xmm12, [rsp + 192]
    movdqu xmm13, [rsp + 208]
    movdqu xmm14, [rsp + 224]
    movdqu xmm15, [rsp + 240]
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ir_entry_1491:

    sub rsp, 32
    call gc_init
    add rsp, 32

    mov [rbp - 64], rax
    mov rax, -11
    mov [rbp - 72], rax

    sub rsp, 32
    mov rax, -11
    mov rcx, rax
    call GetStdHandle
    add rsp, 32

    mov [rbp - 80], rax
    mov rax, [rbp - 80]

    mov qword [rbp - 8], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 88], rax
    mov rax, [rbp - 88]
    test rax, rax
    jz ir_sc_rhs_1494
    jmp ir_sc_true_1495
ir_sc_rhs_1494:
    mov rax, -1
    mov [rbp - 96], rax

    mov rax, qword [rbp - 8]
    push rax
    mov rax, -1
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 104], rax
    mov rax, [rbp - 104]
    test rax, rax
    jz ir_sc_false_1496
ir_sc_true_1495:
    mov rax, 1
    mov [rbp - 112], rax
    jmp ir_sc_end_1497
ir_sc_false_1496:
    mov rax, 0
    mov [rbp - 112], rax
ir_sc_end_1497:
    mov rax, [rbp - 112]
    test rax, rax
    jz ir_if_next_1493

    sub rsp, 32
    call AllocConsole
    add rsp, 32


    mov [rbp - 128], rax
    jmp ir_if_end_1492
ir_if_next_1493:
ir_if_end_1492:

    sub rsp, 32
    call net_init
    add rsp, 32


    mov [rbp - 136], rax
    mov rax, [rbp - 136]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 144], rax
    mov rax, [rbp - 144]
    test rax, rax
    jz ir_if_next_1499

    sub rsp, 32

    lea rax, [rel err_net_init]
    mov rcx, rax
    call con_writeln
    add rsp, 32

    mov [rbp - 152], rax
    mov rax, 1
    mov [rbp - 160], rax
    jmp ir_errdefer_end_1501
ir_errdefer_ok_1500:
ir_errdefer_end_1501:
    mov rax, 1
    jmp Lmain_exit
ir_if_next_1499:
ir_if_end_1498:

    sub rsp, 32
    call socket_tcp
    add rsp, 32

    mov [rbp - 168], rax
    mov rax, [rbp - 168]

    mov qword [rbp - 16], rax

    sub rsp, 32
    call INVALID_SOCKET
    add rsp, 32

    mov [rbp - 176], rax

    mov rax, qword [rbp - 16]
    push rax
    mov rax, [rbp - 176]
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 184], rax
    mov rax, [rbp - 184]
    test rax, rax
    jz ir_if_next_1503

    sub rsp, 32

    lea rax, [rel err_socket]
    mov rcx, rax
    call con_writeln
    add rsp, 32

    mov [rbp - 192], rax

    sub rsp, 32
    call net_cleanup
    add rsp, 32


    mov [rbp - 200], rax
    mov rax, 1
    mov [rbp - 208], rax
    jmp ir_errdefer_end_1505
ir_errdefer_ok_1504:
ir_errdefer_end_1505:
    mov rax, 1
    jmp Lmain_exit
ir_if_next_1503:
ir_if_end_1502:

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax
    mov rax, 1
    mov rdx, rax
    call set_reuseaddr
    add rsp, 32


    mov [rbp - 216], rax
    mov rax, [rbp - 216]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 224], rax
    mov rax, [rbp - 224]
    test rax, rax
    jz ir_if_next_1507

    sub rsp, 32

    lea rax, [rel err_reuseaddr]
    mov rcx, rax
    call con_writeln
    add rsp, 32

    mov [rbp - 232], rax

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax
    call closesocket
    add rsp, 32


    mov [rbp - 240], rax

    sub rsp, 32
    call net_cleanup
    add rsp, 32


    mov [rbp - 248], rax
    mov rax, 1
    mov [rbp - 256], rax
    jmp ir_errdefer_end_1509
ir_errdefer_ok_1508:
ir_errdefer_end_1509:
    mov rax, 1
    jmp Lmain_exit
ir_if_next_1507:
ir_if_end_1506:

    sub rsp, 32
    mov rax, 5000
    mov rcx, rax
    call sockaddr_in_any
    add rsp, 32

    mov [rbp - 264], rax
    mov rax, [rbp - 264]

    mov qword [rbp - 24], rax

    mov rax, qword [rbp - 24]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 272], rax
    mov rax, [rbp - 272]
    test rax, rax
    jz ir_if_next_1511

    sub rsp, 32

    lea rax, [rel err_sockaddr]
    mov rcx, rax
    call con_writeln
    add rsp, 32

    mov [rbp - 280], rax

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax
    call closesocket
    add rsp, 32


    mov [rbp - 288], rax

    sub rsp, 32
    call net_cleanup
    add rsp, 32


    mov [rbp - 296], rax
    mov rax, 1
    mov [rbp - 304], rax
    jmp ir_errdefer_end_1513
ir_errdefer_ok_1512:
ir_errdefer_end_1513:
    mov rax, 1
    jmp Lmain_exit
ir_if_next_1511:
ir_if_end_1510:

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax

    mov rax, qword [rbp - 24]
    mov rdx, rax
    mov rax, 16
    mov r8, rax
    call bind
    add rsp, 32


    mov [rbp - 312], rax
    mov rax, [rbp - 312]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 320], rax
    mov rax, [rbp - 320]
    test rax, rax
    jz ir_if_next_1515

    sub rsp, 32

    lea rax, [rel err_bind]
    mov rcx, rax
    call con_writeln
    add rsp, 32

    mov [rbp - 328], rax

    sub rsp, 32

    mov rax, qword [rbp - 24]
    mov rcx, rax
    call free
    add rsp, 32

    mov [rbp - 336], rax

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax
    call closesocket
    add rsp, 32


    mov [rbp - 344], rax

    sub rsp, 32
    call net_cleanup
    add rsp, 32


    mov [rbp - 352], rax
    mov rax, 1
    mov [rbp - 360], rax
    jmp ir_errdefer_end_1517
ir_errdefer_ok_1516:
ir_errdefer_end_1517:
    mov rax, 1
    jmp Lmain_exit
ir_if_next_1515:
ir_if_end_1514:

    sub rsp, 32

    mov rax, qword [rbp - 24]
    mov rcx, rax
    call free
    add rsp, 32

    mov [rbp - 368], rax

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax
    mov rax, 5
    mov rdx, rax
    call listen
    add rsp, 32


    mov [rbp - 376], rax
    mov rax, [rbp - 376]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    setne al
    movzx rax, al
    mov [rbp - 384], rax
    mov rax, [rbp - 384]
    test rax, rax
    jz ir_if_next_1519

    sub rsp, 32

    lea rax, [rel err_listen]
    mov rcx, rax
    call con_writeln
    add rsp, 32

    mov [rbp - 392], rax

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax
    call closesocket
    add rsp, 32


    mov [rbp - 400], rax

    sub rsp, 32
    call net_cleanup
    add rsp, 32


    mov [rbp - 408], rax
    mov rax, 1
    mov [rbp - 416], rax
    jmp ir_errdefer_end_1521
ir_errdefer_ok_1520:
ir_errdefer_end_1521:
    mov rax, 1
    jmp Lmain_exit
ir_if_next_1519:
ir_if_end_1518:

    sub rsp, 32
    call mutex_create
    add rsp, 32

    mov [rbp - 424], rax
    mov rax, [rbp - 424]

    mov qword [rel file_mutex], rax

    mov rax, qword [rel file_mutex]
    push rax
    mov rax, 0
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 432], rax
    mov rax, [rbp - 432]
    test rax, rax
    jz ir_if_next_1523

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax
    call closesocket
    add rsp, 32


    mov [rbp - 440], rax

    sub rsp, 32
    call net_cleanup
    add rsp, 32


    mov [rbp - 448], rax
    mov rax, 1
    mov [rbp - 456], rax
    jmp ir_errdefer_end_1525
ir_errdefer_ok_1524:
ir_errdefer_end_1525:
    mov rax, 1
    jmp Lmain_exit
ir_if_next_1523:
ir_if_end_1522:

    sub rsp, 32

    lea rax, [rel msg_ready]
    mov rcx, rax
    call con_writeln
    add rsp, 32

    mov [rbp - 464], rax
    mov rax, 1
    mov [rbp - 472], rax
    jmp ir_in_bounds_1527
ir_trap_bounds_1526:

    sub rsp, 32

    lea rax, [rel Lstr_struct501]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1527:
    mov rax, 0
    mov [rbp - 480], rax

    lea rax, [rbp - 44]
    mov [rbp - 488], rax
    mov rax, [rbp - 488]
    push rax
    mov rax, 16
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    mov rax, 1
    mov [rbp - 504], rax
    jmp ir_in_bounds_1529
ir_trap_bounds_1528:

    sub rsp, 32

    lea rax, [rel Lstr_struct503]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1529:
    mov rax, 1
    mov [rbp - 512], rax

    lea rax, [rbp - 44]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 520], rax
    mov rax, [rbp - 520]
    push rax
    mov rax, 0
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    mov rax, 1
    mov [rbp - 536], rax
    jmp ir_in_bounds_1531
ir_trap_bounds_1530:

    sub rsp, 32

    lea rax, [rel Lstr_struct505]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1531:
    mov rax, 2
    mov [rbp - 544], rax

    lea rax, [rbp - 44]
    push rax
    mov rax, 2
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 552], rax
    mov rax, [rbp - 552]
    push rax
    mov rax, 0
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    mov rax, 1
    mov [rbp - 568], rax
    jmp ir_in_bounds_1533
ir_trap_bounds_1532:

    sub rsp, 32

    lea rax, [rel Lstr_struct507]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1533:
    mov rax, 3
    mov [rbp - 576], rax

    lea rax, [rbp - 44]
    push rax
    mov rax, 3
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 584], rax
    mov rax, [rbp - 584]
    push rax
    mov rax, 0
    mov rcx, rax
    pop rax
    mov byte [rax], cl
ir_while_1534:
    mov rax, 1
    mov [rbp - 600], rax
    jmp ir_in_bounds_1537
ir_trap_bounds_1536:

    sub rsp, 32

    lea rax, [rel Lstr_struct509]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1537:
    mov rax, 0
    mov [rbp - 608], rax

    lea rax, [rbp - 44]
    mov [rbp - 616], rax
    mov rax, [rbp - 616]
    push rax
    mov rax, 16
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    mov rax, 1
    mov [rbp - 632], rax
    jmp ir_in_bounds_1539
ir_trap_bounds_1538:

    sub rsp, 32

    lea rax, [rel Lstr_struct511]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1539:
    mov rax, 1
    mov [rbp - 640], rax

    lea rax, [rbp - 44]
    push rax
    mov rax, 1
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 648], rax
    mov rax, [rbp - 648]
    push rax
    mov rax, 0
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    mov rax, 1
    mov [rbp - 664], rax
    jmp ir_in_bounds_1541
ir_trap_bounds_1540:

    sub rsp, 32

    lea rax, [rel Lstr_struct513]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1541:
    mov rax, 2
    mov [rbp - 672], rax

    lea rax, [rbp - 44]
    push rax
    mov rax, 2
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 680], rax
    mov rax, [rbp - 680]
    push rax
    mov rax, 0
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    mov rax, 1
    mov [rbp - 696], rax
    jmp ir_in_bounds_1543
ir_trap_bounds_1542:

    sub rsp, 32

    lea rax, [rel Lstr_struct515]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1543:
    mov rax, 3
    mov [rbp - 704], rax

    lea rax, [rbp - 44]
    push rax
    mov rax, 3
    mov r10, rax
    pop rax
    add rax, r10
    mov [rbp - 712], rax
    mov rax, [rbp - 712]
    push rax
    mov rax, 0
    mov rcx, rax
    pop rax
    mov byte [rax], cl
    mov rax, 1
    mov [rbp - 728], rax
    jmp ir_in_bounds_1545
ir_trap_bounds_1544:

    sub rsp, 32

    lea rax, [rel Lstr_struct517]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1545:
    mov rax, 0
    mov [rbp - 736], rax

    lea rax, [rbp - 40]
    mov [rbp - 744], rax
    mov rax, 1
    mov [rbp - 752], rax
    jmp ir_in_bounds_1547
ir_trap_bounds_1546:

    sub rsp, 32

    lea rax, [rel Lstr_struct519]
    mov rcx, rax
    call puts
    add rsp, 32



    sub rsp, 32
    mov rax, 1
    mov rcx, rax
    call exit
    add rsp, 32

ir_in_bounds_1547:
    mov rax, 0
    mov [rbp - 760], rax

    lea rax, [rbp - 44]
    mov [rbp - 768], rax

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax
    mov rax, [rbp - 744]
    mov rdx, rax
    mov rax, [rbp - 768]
    mov r8, rax
    call accept
    add rsp, 32

    mov [rbp - 776], rax
    mov rax, [rbp - 776]

    mov qword [rbp - 56], rax

    sub rsp, 32
    call INVALID_SOCKET
    add rsp, 32

    mov [rbp - 784], rax

    mov rax, qword [rbp - 56]
    push rax
    mov rax, [rbp - 784]
    mov r10, rax
    pop rax
    cmp rax, r10
    sete al
    movzx rax, al
    mov [rbp - 792], rax
    mov rax, [rbp - 792]
    test rax, rax
    jz ir_if_next_1549
    jmp ir_while_1534
ir_if_next_1549:
ir_if_end_1548:

    sub rsp, 32

    mov rax, qword [rbp - 56]
    mov rcx, rax
    call spawn_client_thread
    add rsp, 32

    mov [rbp - 800], rax
    jmp ir_while_1534
ir_while_end_1535:

    sub rsp, 32

    mov rax, qword [rbp - 16]
    mov rcx, rax
    call closesocket
    add rsp, 32


    mov [rbp - 808], rax

    sub rsp, 32
    call net_cleanup
    add rsp, 32


    mov [rbp - 816], rax
    mov rax, 0
    mov [rbp - 824], rax
ir_errdefer_ok_1550:
ir_errdefer_end_1551:
    mov rax, 0
    jmp Lmain_exit
Lmain_exit:

    mov rsp, rbp
    pop rbp
    ret


global mainCRTStartup
mainCRTStartup:
    sub rsp, 40

    lea rcx, [rsp + 40]
    extern gc_init
    call gc_init
    extern gc_register_root
    lea rcx, [rel PAGE_CONTENT]
    call gc_register_root
    lea rcx, [rel DEMO_CONTENT]
    call gc_register_root
    lea rcx, [rel BENCHMARKS_CONTENT]
    call gc_register_root
    lea rcx, [rel DOCS_CONTENT]
    call gc_register_root
    lea rcx, [rel HTTP_PAGE_HEADER]
    call gc_register_root
    lea rcx, [rel HTTP_DEMO_HEADER]
    call gc_register_root
    lea rcx, [rel HTTP_BENCHMARKS_HEADER]
    call gc_register_root
    lea rcx, [rel HTTP_DOCS_HEADER]
    call gc_register_root
    lea rcx, [rel HTTP_404_HEADER]
    call gc_register_root
    lea rcx, [rel HTTP_404_BODY]
    call gc_register_root
    lea rcx, [rel HTTP_HEALTH_HEADER]
    call gc_register_root
    lea rcx, [rel HTTP_HEALTH_BODY]
    call gc_register_root
    lea rcx, [rel FORUM_HEADER]
    call gc_register_root
    lea rcx, [rel FORUM_CSS]
    call gc_register_root
    lea rcx, [rel FORUM_INDEX_START]
    call gc_register_root
    lea rcx, [rel FORUM_INDEX_BODY]
    call gc_register_root
    lea rcx, [rel FORUM_INDEX_END]
    call gc_register_root
    lea rcx, [rel FORUM_FORM]
    call gc_register_root
    lea rcx, [rel FORUM_THREAD_START]
    call gc_register_root
    lea rcx, [rel FORUM_THREAD_HEAD_END]
    call gc_register_root
    lea rcx, [rel FORUM_THREAD_MID]
    call gc_register_root
    lea rcx, [rel FORUM_THREAD_END]
    call gc_register_root
    lea rcx, [rel FORUM_THREAD_END2]
    call gc_register_root
    lea rcx, [rel FORUM_NO_POSTS]
    call gc_register_root
    lea rcx, [rel FORUM_ERR_EMPTY]
    call gc_register_root
    lea rcx, [rel FORUM_LI_OPEN]
    call gc_register_root
    lea rcx, [rel FORUM_LI_MID]
    call gc_register_root
    lea rcx, [rel FORUM_LI_CLOSE]
    call gc_register_root
    lea rcx, [rel FORUM_POST_OPEN]
    call gc_register_root
    lea rcx, [rel FORUM_POST_CLOSE]
    call gc_register_root
    lea rcx, [rel REDIRECT_302]
    call gc_register_root
    lea rcx, [rel REDIRECT_END]
    call gc_register_root
    lea rcx, [rel fn_threads]
    call gc_register_root
    lea rcx, [rel fn_posts_prefix]
    call gc_register_root
    lea rcx, [rel fn_posts_suffix]
    call gc_register_root
    lea rcx, [rel mode_r]
    call gc_register_root
    lea rcx, [rel mode_a]
    call gc_register_root
    lea rcx, [rel nl]
    call gc_register_root
    lea rcx, [rel key_title]
    call gc_register_root
    lea rcx, [rel key_body]
    call gc_register_root
    lea rcx, [rel hdr_content_length]
    call gc_register_root
    lea rcx, [rel hdr_content_length_lo]
    call gc_register_root
    lea rcx, [rel pat_crlf2]
    call gc_register_root
    lea rcx, [rel pat_lf2]
    call gc_register_root
    lea rcx, [rel html_amp]
    call gc_register_root
    lea rcx, [rel html_lt]
    call gc_register_root
    lea rcx, [rel html_gt]
    call gc_register_root
    lea rcx, [rel html_quot]
    call gc_register_root
    lea rcx, [rel html_apos]
    call gc_register_root
    lea rcx, [rel dbg_post]
    call gc_register_root
    lea rcx, [rel dbg_bs]
    call gc_register_root
    lea rcx, [rel dbg_bl]
    call gc_register_root
    lea rcx, [rel dbg_cl]
    call gc_register_root
    lea rcx, [rel dbg_need]
    call gc_register_root
    lea rcx, [rel dbg_n]
    call gc_register_root
    lea rcx, [rel dbg_got]
    call gc_register_root
    lea rcx, [rel dbg_title]
    call gc_register_root
    lea rcx, [rel dbg_fopen_ok]
    call gc_register_root
    lea rcx, [rel dbg_fopen_fail]
    call gc_register_root
    lea rcx, [rel dbg_newline]
    call gc_register_root
    lea rcx, [rel dbg_fbs_enter]
    call gc_register_root
    lea rcx, [rel dbg_fbs_exit]
    call gc_register_root
    lea rcx, [rel dbg_fbs_ok]
    call gc_register_root
    lea rcx, [rel dbg_loop]
    call gc_register_root
    lea rcx, [rel err_net_init]
    call gc_register_root
    lea rcx, [rel err_socket]
    call gc_register_root
    lea rcx, [rel err_reuseaddr]
    call gc_register_root
    lea rcx, [rel err_sockaddr]
    call gc_register_root
    lea rcx, [rel err_bind]
    call gc_register_root
    lea rcx, [rel err_listen]
    call gc_register_root
    lea rcx, [rel msg_ready]
    call gc_register_root
    lea rcx, [rel crlf]
    call gc_register_root

    call main
    mov [rsp + 32], rax
    extern gc_shutdown
    call gc_shutdown
    mov rcx, [rsp + 32]
    extern ExitProcess
    call ExitProcess


section .data

net_ref_count:
    dd 0


net_ref_lock:
    dd 0


file_mutex:
    dq 0


PAGE_CONTENT:
    dq Lstr0
    dq 18144
Lstr0:
    db "<!DOCTYPE html>", 10, "<html lang='en'>", 10, "<head>", 10, "<meta charset='UTF-8'>", 10, "<meta name='viewport' content='width=device-width,initial-scale=1'>", 10, "<meta name='description' content='MethASM - A typed, assembly-inspired language compiling to x86-64 NASM with advanced features like garbage collection, C interop, and modules.'>", 10, "<title>MethASM ", 226, 128, 148, " Premium Typed Assembly Language</title>", 10, "<link rel=", 34, "preconnect", 34, " href=", 34, "https://fonts.googleapis.com", 34, ">", 10, "<link rel=", 34, "preconnect", 34, " href=", 34, "https://fonts.gstatic.com", 34, " crossorigin>", 10, "<link href=", 34, "https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&family=JetBrains+Mono:wght@400;500;600&display=swap", 34, " rel=", 34, "stylesheet", 34, ">", 10, "<style>", 10, "  :root {", 10, "    --bg-color: #05050A;", 10, "    --text-main: #E2E8F0;", 10, "    --text-muted: #94A3B8;", 10, "    --accent-1: #6366F1;", 10, "    --accent-2: #8B5CF6;", 10, "    --accent-3: #EC4899;", 10, "    --accent-4: #10B981;", 10, "    --glass-bg: rgba(255, 255, 255, 0.03);", 10, "    --glass-border: rgba(255, 255, 255, 0.08);", 10, "    --code-bg: #0D1117;", 10, "    --success: #10B981;", 10, "    --warning: #F59E0B;", 10, "    --error: #EF4444;", 10, "  }", 10, 10, "  * { box-sizing: border-box; margin: 0; padding: 0; }", 10, "  ", 10, "  body {", 10, "    font-family: 'Inter', sans-serif;", 10, "    background-color: var(--bg-color);", 10, "    color: var(--text-main);", 10, "    line-height: 1.6;", 10, "    overflow-x: hidden;", 10, "  }", 10, 10, "  /* Animated background mesh */", 10, "  .bg-mesh {", 10, "    position: fixed;", 10, "    top: 0; left: 0; right: 0; bottom: 0;", 10, "    z-index: -1;", 10, "    background: ", 10, "      radial-gradient(circle at 15% 50%, rgba(99, 102, 241, 0.15) 0%, transparent 50%),", 10, "      radial-gradient(circle at 85% 30%, rgba(236, 72, 153, 0.15) 0%, transparent 50%),", 10, "      radial-gradient(circle at 50% 80%, rgba(139, 92, 246, 0.15) 0%, transparent 50%);", 10, "    filter: blur(60px);", 10, "    animation: pulse 15s ease-in-out infinite alternate;", 10, "  }", 10, 10, "  @keyframes pulse {", 10, "    0% { transform: scale(1); opacity: 0.8; }", 10, "    100% { transform: scale(1.1); opacity: 1; }", 10, "  }", 10, 10, "  .container {", 10, "    max-width: 1200px;", 10, "    margin: 0 auto;", 10, "    padding: 2rem;", 10, "  }", 10, 10, "  /* Navigation */", 10, "  nav {", 10, "    display: flex;", 10, "    justify-content: space-between;", 10, "    align-items: center;", 10, "    padding: 1rem 0;", 10, "    margin-bottom: 3rem;", 10, "    border-bottom: 1px solid var(--glass-border);", 10, "  }", 10, 10, "  .nav-links {", 10, "    display: flex;", 10, "    gap: 2rem;", 10, "    align-items: center;", 10, "  }", 10, 10, "  .nav-link {", 10, "    color: var(--text-muted);", 10, "    text-decoration: none;", 10, "    font-weight: 500;", 10, "    transition: color 0.3s ease;", 10, "  }", 10, 10, "  .nav-link:hover {", 10, "    color: var(--accent-1);", 10, "  }", 10, 10, "  /* Hero Section */", 10, "  .hero {", 10, "    text-align: center;", 10, "    margin-bottom: 5rem;", 10, "    animation: slideUp 0.8s ease-out;", 10, "  }", 10, 10, "  .hero h1 {", 10, "    font-size: clamp(2.5rem, 8vw, 5rem);", 10, "    font-weight: 700;", 10, "    margin-bottom: 1.5rem;", 10, "    background: linear-gradient(135deg, var(--text-main) 0%, var(--accent-1) 30%, var(--accent-3) 70%, var(--accent-4) 100%);", 10, "    -webkit-background-clip: text;", 10, "    -webkit-text-fill-color: transparent;", 10, "    letter-spacing: -2px;", 10, "    line-height: 0.9;", 10, "  }", 10, 10, "  .hero .subtitle {", 10, "    font-size: 1.4rem;", 10, "    color: var(--text-muted);", 10, "    max-width: 700px;", 10, "    margin: 0 auto 3rem;", 10, "    font-weight: 400;", 10, "  }", 10, 10, "  .badges {", 10, "    display: flex;", 10, "    justify-content: center;", 10, "    flex-wrap: wrap;", 10, "    gap: 0.75rem;", 10, "    margin-bottom: 3rem;", 10, "  }", 10, 10, "  .badge {", 10, "    background: var(--glass-bg);", 10, "    border: 1px solid var(--glass-border);", 10, "    padding: 0.5rem 1.2rem;", 10, "    border-radius: 25px;", 10, "    font-size: 0.9rem;", 10, "    font-weight: 500;", 10, "    color: #C7D2FE;", 10, "    backdrop-filter: blur(10px);", 10, "    transition: all 0.3s ease;", 10, "  }", 10, 10, "  .badge:hover {", 10, "    border-color: var(--accent-1);", 10, "    transform: translateY(-2px);", 10, "    box-shadow: 0 4px 12px rgba(99, 102, 241, 0.2);", 10, "  }", 10, 10, "  .cta-buttons {", 10, "    display: flex;", 10, "    justify-content: center;", 10, "    gap: 1rem;", 10, "    flex-wrap: wrap;", 10, "  }", 10, 10, "  .btn {", 10, "    display: inline-block;", 10, "    padding: 1rem 2.5rem;", 10, "    border-radius: 12px;", 10, "    text-decoration: none;", 10, "    font-weight: 600;", 10, "    transition: all 0.3s ease;", 10, "    font-size: 1rem;", 10, "  }", 10, 10, "  .btn-primary {", 10, "    background: linear-gradient(135deg, var(--accent-1) 0%, var(--accent-2) 100%);", 10, "    color: white;", 10, "    box-shadow: 0 4px 15px rgba(99, 102, 241, 0.4);", 10, "  }", 10, 10, "  .btn-primary:hover {", 10, "    transform: translateY(-2px);", 10, "    box-shadow: 0 6px 20px rgba(99, 102, 241, 0.6);", 10, "  }", 10, 10, "  .btn-secondary {", 10, "    background: var(--glass-bg);", 10, "    color: var(--text-main);", 10, "    border: 1px solid var(--glass-border);", 10, "    backdrop-filter: blur(10px);", 10, "  }", 10, 10, "  .btn-secondary:hover {", 10, "    background: rgba(255, 255, 255, 0.08);", 10, "    border-color: rgba(255, 255, 255, 0.2);", 10, "  }", 10, 10, "  /* Features Grid */", 10, "  .features-section {", 10, "    margin-bottom: 5rem;", 10, "  }", 10, 10, "  .section-title {", 10, "    text-align: center;", 10, "    font-size: 2.5rem;", 10, "    font-weight: 700;", 10, "    margin-bottom: 3rem;", 10, "    background: linear-gradient(135deg, var(--text-main) 0%, var(--accent-1) 100%);", 10, "    -webkit-background-clip: text;", 10, "    -webkit-text-fill-color: transparent;", 10, "  }", 10, 10, "  .grid {", 10, "    display: grid;", 10, "    grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));", 10, "    gap: 2rem;", 10, "    margin-bottom: 3rem;", 10, "  }", 10, 10, "  .card {", 10, "    background: var(--glass-bg);", 10, "    border: 1px solid var(--glass-border);", 10, "    border-radius: 20px;", 10, "    padding: 2.5rem;", 10, "    backdrop-filter: blur(10px);", 10, "    transition: all 0.4s cubic-bezier(0.175, 0.885, 0.32, 1.275);", 10, "    animation: fadeUp 0.8s ease-out backwards;", 10, "    position: relative;", 10, "    overflow: hidden;", 10, "  }", 10, 10, "  .card::before {", 10, "    content: '';", 10, "    position: absolute;", 10, "    top: 0;", 10, "    left: 0;", 10, "    right: 0;", 10, "    height: 3px;", 10, "    background: linear-gradient(90deg, var(--accent-1), var(--accent-2), var(--accent-3));", 10, "    opacity: 0;", 10, "    transition: opacity 0.3s ease;", 10, "  }", 10, 10, "  .card:hover::before {", 10, "    opacity: 1;", 10, "  }", 10, 10, "  .card:nth-child(1) { animation-delay: 0.1s; }", 10, "  .card:nth-child(2) { animation-delay: 0.2s; }", 10, "  .card:nth-child(3) { animation-delay: 0.3s; }", 10, "  .card:nth-child(4) { animation-delay: 0.4s; }", 10, "  .card:nth-child(5) { animation-delay: 0.5s; }", 10, "  .card:nth-child(6) { animation-delay: 0.6s; }", 10, 10, "  .card:hover {", 10, "    transform: translateY(-8px);", 10, "    border-color: rgba(139, 92, 246, 0.5);", 10, "    box-shadow: 0 20px 40px rgba(0, 0, 0, 0.5), 0 0 20px rgba(139, 92, 246, 0.2);", 10, "  }", 10, 10, "  .card-icon {", 10, "    font-size: 2.5rem;", 10, "    margin-bottom: 1.5rem;", 10, "    display: block;", 10, "  }", 10, 10, "  .card h3 {", 10, "    font-size: 1.5rem;", 10, "    margin-bottom: 1rem;", 10, "    color: white;", 10, "    font-weight: 600;", 10, "  }", 10, 10, "  .card p {", 10, "    color: var(--text-muted);", 10, "    font-size: 1rem;", 10, "    line-height: 1.7;", 10, "  }", 10, 10, "  /* Code Showcase */", 10, "  .code-showcase {", 10, "    background: var(--code-bg);", 10, "    border: 1px solid #30363D;", 10, "    border-radius: 16px;", 10, "    overflow: hidden;", 10, "    margin-bottom: 5rem;", 10, "    box-shadow: 0 20px 40px rgba(0, 0, 0, 0.6);", 10, "  }", 10, 10, "  .showcase-header {", 10, "    background: #161B22;", 10, "    padding: 1rem 1.5rem;", 10, "    display: flex;", 10, "    align-items: center;", 10, "    justify-content: space-between;", 10, "    border-bottom: 1px solid #30363D;", 10, "  }", 10, 10, "  .showcase-title {", 10, "    color: var(--text-main);", 10, "    font-weight: 600;", 10, "    font-size: 1.1rem;", 10, "  }", 10, 10, "  .showcase-tabs {", 10, "    display: flex;", 10, "    gap: 0.5rem;", 10, "  }", 10, 10, "  .tab {", 10, "    background: var(--glass-bg);", 10, "    border: 1px solid var(--glass-border);", 10, "    padding: 0.5rem 1rem;", 10, "    border-radius: 8px;", 10, "    color: var(--text-muted);", 10, "    font-size: 0.85rem;", 10, "    cursor: pointer;", 10, "    transition: all 0.3s ease;", 10, "  }", 10, 10, "  .tab.active {", 10, "    background: var(--accent-1);", 10, "    color: white;", 10, "    border-color: var(--accent-1);", 10, "  }", 10, 10, "  .code-content {", 10, "    display: none;", 10, "  }", 10, 10, "  .code-content.active {", 10, "    display: block;", 10, "  }", 10, 10, "  pre {", 10, "    padding: 2rem;", 10, "    overflow-x: auto;", 10, "    margin: 0;", 10, "  }", 10, 10, "  code {", 10, "    font-family: 'JetBrains Mono', monospace;", 10, "    font-size: 0.9rem;", 10, "    line-height: 1.6;", 10, "  }", 10, 10, "  /* Syntax Highlighting */", 10, "  .k { color: #FF7B72; font-weight: 500; } /* Keyword */", 10, "  .f { color: #D2A8FF; font-weight: 500; } /* Function */", 10, "  .s { color: #A5D6FF; } /* String */", 10, "  .c { color: #8B949E; font-style: italic; } /* Comment */", 10, "  .t { color: #FFA657; } /* Type */", 10, "  .n { color: #79C0FF; } /* Number */", 10, 10, "  /* Stats Section */", 10, "  .stats {", 10, "    display: grid;", 10, "    grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));", 10, "    gap: 2rem;", 10, "    margin-bottom: 5rem;", 10, "  }", 10, 10, "  .stat-card {", 10, "    text-align: center;", 10, "    padding: 2rem;", 10, "    background: var(--glass-bg);", 10, "    border: 1px solid var(--glass-border);", 10, "    border-radius: 16px;", 10, "    backdrop-filter: blur(10px);", 10, "  }", 10, 10, "  .stat-number {", 10, "    font-size: 3rem;", 10, "    font-weight: 700;", 10, "    color: var(--accent-1);", 10, "    margin-bottom: 0.5rem;", 10, "  }", 10, 10, "  .stat-label {", 10, "    color: var(--text-muted);", 10, "    font-size: 1rem;", 10, "  }", 10, 10, "  /* Footer */", 10, "  footer {", 10, "    text-align: center;", 10, "    padding: 3rem 0;", 10, "    border-top: 1px solid var(--glass-border);", 10, "    color: var(--text-muted);", 10, "  }", 10, 10, "  .footer-links {", 10, "    display: flex;", 10, "    justify-content: center;", 10, "    gap: 2rem;", 10, "    margin-bottom: 2rem;", 10, "  }", 10, 10, "  .footer-link {", 10, "    color: var(--text-muted);", 10, "    text-decoration: none;", 10, "    transition: color 0.3s ease;", 10, "  }", 10, 10, "  .footer-link:hover {", 10, "    color: var(--accent-1);", 10, "  }", 10, 10, "  /* Animations */", 10, "  @keyframes slideUp {", 10, "    from { opacity: 0; transform: translateY(30px); }", 10, "    to { opacity: 1; transform: translateY(0); }", 10, "  }", 10, 10, "  @keyframes fadeUp {", 10, "    from { opacity: 0; transform: translateY(20px); }", 10, "    to { opacity: 1; transform: translateY(0); }", 10, "  }", 10, 10, "  /* Responsive */", 10, "  @media (max-width: 768px) {", 10, "    .nav-links {", 10, "      flex-direction: column;", 10, "      gap: 1rem;", 10, "    }", 10, 10, "    .hero h1 {", 10, "      font-size: 3rem;", 10, "    }", 10, 10, "    .grid {", 10, "      grid-template-columns: 1fr;", 10, "    }", 10, 10, "    .stats {", 10, "      grid-template-columns: repeat(2, 1fr);", 10, "    }", 10, "  }", 10, "</style>", 10, "</head>", 10, "<body>", 10, "  <div class=", 34, "bg-mesh", 34, "></div>", 10, "  ", 10, "  <div class=", 34, "container", 34, ">", 10, "    <nav>", 10, "      <div class=", 34, "nav-links", 34, ">", 10, "        <a href=", 34, "/", 34, " class=", 34, "nav-link", 34, ">Home</a>", 10, "        <a href=", 34, "/demo", 34, " class=", 34, "nav-link", 34, ">Demo</a>", 10, "        <a href=", 34, "/benchmarks", 34, " class=", 34, "nav-link", 34, ">Benchmarks</a>", 10, "        <a href=", 34, "/docs", 34, " class=", 34, "nav-link", 34, ">Docs</a>", 10, "        <a href=", 34, "/forum", 34, " class=", 34, "nav-link", 34, ">Forum</a>", 10, "      </div>", 10, "    </nav>", 10, 10, "    <header class=", 34, "hero", 34, ">", 10, "      <h1>MethASM</h1>", 10, "      <p class=", 34, "subtitle", 34, ">A typed, assembly-inspired language with C interop and garbage collection, compiling to high-performance x86-64 NASM assembly.</p>", 10, "      ", 10, "      <div class=", 34, "badges", 34, ">", 10, "        <span class=", 34, "badge", 34, ">", 240, 159, 148, 146, " Statically Typed</span>", 10, "        <span class=", 34, "badge", 34, ">", 226, 154, 161, " Zero-Cost Abstractions</span>", 10, "        <span class=", 34, "badge", 34, ">", 240, 159, 148, 151, " Native C Interop</span>", 10, "        <span class=", 34, "badge", 34, ">", 240, 159, 147, 166, " Module System</span>", 10, "        <span class=", 34, "badge", 34, ">", 240, 159, 151, 145, 239, 184, 143, " Conservative GC</span>", 10, "        <span class=", 34, "badge", 34, ">", 240, 159, 142, 175, " x86-64 Native</span>", 10, "      </div>", 10, 10, "      <div class=", 34, "cta-buttons", 34, ">", 10, "        <a href=", 34, "/demo", 34, " class=", 34, "btn btn-primary", 34, ">Try Interactive Demo</a>", 10, "        <a href=", 34, "/docs", 34, " class=", 34, "btn btn-secondary", 34, ">Read Documentation</a>", 10, "      </div>", 10, "    </header>", 10, 10, "    <section class=", 34, "features-section", 34, ">", 10, "      <h2 class=", 34, "section-title", 34, ">Language Features</h2>", 10, "      <div class=", 34, "grid", 34, ">", 10, "        <div class=", 34, "card", 34, ">", 10, "          <span class=", 34, "card-icon", 34, ">", 226, 154, 161, "</span>", 10, "          <h3>Explicit Control</h3>", 10, "          <p>No hidden magic. All variables and parameters have explicit types. Structured control flow with <code>if</code>, <code>while</code>, <code>for</code>, and <code>switch</code> statements. Compile to efficient x86-64 assembly.</p>", 10, "        </div>", 10, "        ", 10, "        <div class=", 34, "card", 34, ">", 10, "          <span class=", 34, "card-icon", 34, ">", 240, 159, 155, 161, 239, 184, 143, "</span>", 10, "          <h3>Memory Management</h3>", 10, "          <p>Leverage the built-in conservative garbage collection via <code>new T</code> for heap allocation, or manage it yourself with C <code>malloc</code> when you need precise control.</p>", 10, "        </div>", 10, 10, "        <div class=", 34, "card", 34, ">", 10, "          <span class=", 34, "card-icon", 34, ">", 240, 159, 148, 151, "</span>", 10, "          <h3>Seamless FFI</h3>", 10, "          <p>First-class C interoperability. Declare <code>extern function</code> and <code>extern var</code> to effortlessly interface with external libraries like Winsock2 and the MSVC runtime.</p>", 10, "        </div>", 10, 10, "        <div class=", 34, "card", 34, ">", 10, "          <span class=", 34, "card-icon", 34, ">", 240, 159, 147, 166, "</span>", 10, "          <h3>Module System</h3>", 10, "          <p>Organize your codebase efficiently. Import standard libraries or your own modules to keep namespaces clean and dependency resolution robust.</p>", 10, "        </div>", 10, 10, "        <div class=", 34, "card", 34, ">", 10, "          <span class=", 34, "card-icon", 34, ">", 240, 159, 142, 175, "</span>", 10, "          <h3>Performance</h3>", 10, "          <p>Compiles directly to x86-64 NASM assembly. No runtime overhead beyond what you explicitly use. Perfect for systems programming and performance-critical applications.</p>", 10, "        </div>", 10, 10, "        <div class=", 34, "card", 34, ">", 10, "          <span class=", 34, "card-icon", 34, ">", 240, 159, 148, 167, "</span>", 10, "          <h3>Rich Type System</h3>", 10, "          <p>Comprehensive primitive types (int8-int64, uint8-uint64, float32/64), structs, enums, pointers, arrays, and strings with proper memory layout for C interop.</p>", 10, "        </div>", 10, "      </div>", 10, "    </section>", 10, 10, "    <section class=", 34, "code-showcase", 34, ">", 10, "      <div class=", 34, "showcase-header", 34, ">", 10, "        <div class=", 34, "showcase-title", 34, ">Code Examples</div>", 10, "        <div class=", 34, "showcase-tabs", 34, ">", 10, "          <div class=", 34, "tab active", 34, " onclick=", 34, "showTab('basics')", 34, ">Basics</div>", 10, "          <div class=", 34, "tab", 34, " onclick=", 34, "showTab('structs')", 34, ">Structs</div>", 10, "          <div class=", 34, "tab", 34, " onclick=", 34, "showTab('networking')", 34, ">Networking</div>", 10, "        </div>", 10, "      </div>", 10, "      ", 10, "      <div id=", 34, "basics", 34, " class=", 34, "code-content active", 34, ">", 10, "        <pre><code><span class=", 34, "k", 34, ">import</span> <span class=", 34, "s", 34, ">", 34, "std/io", 34, "</span>;", 10, "<span class=", 34, "k", 34, ">import</span> <span class=", 34, "s", 34, ">", 34, "std/math", 34, "</span>;", 10, 10, "<span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">factorial</span>(n: <span class=", 34, "t", 34, ">int32</span>) -> <span class=", 34, "t", 34, ">int32</span> {", 10, "  <span class=", 34, "k", 34, ">if</span> (n <= <span class=", 34, "n", 34, ">1</span>) {", 10, "    <span class=", 34, "k", 34, ">return</span> <span class=", 34, "n", 34, ">1</span>;", 10, "  }", 10, "  <span class=", 34, "k", 34, ">return</span> n * <span class=", 34, "f", 34, ">factorial</span>(n - <span class=", 34, "n", 34, ">1</span>);", 10, "}", 10, 10, "<span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">main</span>() -> <span class=", 34, "t", 34, ">int32</span> {", 10, "  <span class=", 34, "k", 34, ">var</span> result: <span class=", 34, "t", 34, ">int32</span> = <span class=", 34, "f", 34, ">factorial</span>(<span class=", 34, "n", 34, ">10</span>);", 10, "  <span class=", 34, "f", 34, ">println_int</span>(result);", 10, "  <span class=", 34, "k", 34, ">return</span> <span class=", 34, "n", 34, ">0</span>;", 10, "}</code></pre>", 10, "      </div>", 10, 10, "      <div id=", 34, "structs", 34, " class=", 34, "code-content", 34, ">", 10, "        <pre><code><span class=", 34, "k", 34, ">struct</span> <span class=", 34, "t", 34, ">Point</span> {", 10, "  x: <span class=", 34, "t", 34, ">float64</span>;", 10, "  y: <span class=", 34, "t", 34, ">float64</span>;", 10, "  ", 10, "  <span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">distance</span>(other: <span class=", 34, "t", 34, ">Point</span>*) -> <span class=", 34, "t", 34, ">float64</span> {", 10, "    <span class=", 34, "k", 34, ">var</span> dx: <span class=", 34, "t", 34, ">float64</span> = <span class=", 34, "k", 34, ">this</span>.x - other->x;", 10, "    <span class=", 34, "k", 34, ">var</span> dy: <span class=", 34, "t", 34, ">float64</span> = <span class=", 34, "k", 34, ">this</span>.y - other->y;", 10, "    <span class=", 34, "k", 34, ">return</span> <span class=", 34, "f", 34, ">sqrt</span>(dx*dx + dy*dy);", 10, "  }", 10, "}", 10, 10, "<span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">main</span>() -> <span class=", 34, "t", 34, ">int32</span> {", 10, "  <span class=", 34, "k", 34, ">var</span> p1: <span class=", 34, "t", 34, ">Point</span> = {<span class=", 34, "n", 34, ">0.0</span>, <span class=", 34, "n", 34, ">0.0</span>};", 10, "  <span class=", 34, "k", 34, ">var</span> p2: <span class=", 34, "t", 34, ">Point</span> = {<span class=", 34, "n", 34, ">3.0</span>, <span class=", 34, "n", 34, ">4.0</span>};", 10, "  <span class=", 34, "k", 34, ">var</span> dist: <span class=", 34, "t", 34, ">float64</span> = <span class=", 34, "f", 34, ">p1.distance</span>(&p2);", 10, "  <span class=", 34, "k", 34, ">return</span> <span class=", 34, "n", 34, ">0</span>;", 10, "}</code></pre>", 10, "      </div>", 10, 10, "      <div id=", 34, "networking", 34, " class=", 34, "code-content", 34, ">", 10, "        <pre><code><span class=", 34, "k", 34, ">import</span> <span class=", 34, "s", 34, ">", 34, "std/net", 34, "</span>;", 10, "<span class=", 34, "k", 34, ">import</span> <span class=", 34, "s", 34, ">", 34, "std/io", 34, "</span>;", 10, 10, "<span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">main</span>() -> <span class=", 34, "t", 34, ">int32</span> {", 10, "  <span class=", 34, "k", 34, ">if</span> (<span class=", 34, "f", 34, ">net_init</span>() != <span class=", 34, "n", 34, ">0</span>) {", 10, "    <span class=", 34, "f", 34, ">puts</span>(<span class=", 34, "s", 34, ">", 34, "Failed to initialize network", 34, "</span>);", 10, "    <span class=", 34, "k", 34, ">return</span> <span class=", 34, "n", 34, ">1</span>;", 10, "  }", 10, "  ", 10, "  <span class=", 34, "k", 34, ">var</span> sock: <span class=", 34, "t", 34, ">int64</span> = <span class=", 34, "f", 34, ">socket_tcp</span>();", 10, "  <span class=", 34, "k", 34, ">var</span> addr: <span class=", 34, "t", 34, ">cstring</span> = <span class=", 34, "f", 34, ">sockaddr_in_any</span>(<span class=", 34, "n", 34, ">8080</span>);", 10, "  ", 10, "  <span class=", 34, "k", 34, ">if</span> (<span class=", 34, "f", 34, ">bind</span>(sock, addr, <span class=", 34, "n", 34, ">16</span>) != <span class=", 34, "n", 34, ">0</span>) {", 10, "    <span class=", 34, "f", 34, ">puts</span>(<span class=", 34, "s", 34, ">", 34, "Bind failed", 34, "</span>);", 10, "    <span class=", 34, "k", 34, ">return</span> <span class=", 34, "n", 34, ">1</span>;", 10, "  }", 10, "  ", 10, "  <span class=", 34, "f", 34, ">listen</span>(sock, <span class=", 34, "n", 34, ">5</span>);", 10, "  <span class=", 34, "f", 34, ">puts</span>(<span class=", 34, "s", 34, ">", 34, "Server listening on port 8080", 34, "</span>);", 10, "  ", 10, "  <span class=", 34, "c", 34, ">// Accept loop...</span>", 10, "  <span class=", 34, "k", 34, ">return</span> <span class=", 34, "n", 34, ">0</span>;", 10, "}</code></pre>", 10, "      </div>", 10, "    </section>", 10, 10, "    <section class=", 34, "stats", 34, ">", 10, "      <div class=", 34, "stat-card", 34, ">", 10, "        <div class=", 34, "stat-number", 34, ">x86-64</div>", 10, "        <div class=", 34, "stat-label", 34, ">Target Architecture</div>", 10, "      </div>", 10, "      <div class=", 34, "stat-card", 34, ">", 10, "        <div class=", 34, "stat-number", 34, ">NASM</div>", 10, "        <div class=", 34, "stat-label", 34, ">Assembly Output</div>", 10, "      </div>", 10, "      <div class=", 34, "stat-card", 34, ">", 10, "        <div class=", 34, "stat-number", 34, ">Zero</div>", 10, "        <div class=", 34, "stat-label", 34, ">Runtime Overhead</div>", 10, "      </div>", 10, "      <div class=", 34, "stat-card", 34, ">", 10, "        <div class=", 34, "stat-number", 34, ">C</div>", 10, "        <div class=", 34, "stat-label", 34, ">ABI Compatible</div>", 10, "      </div>", 10, "    </section>", 10, 10, "    <footer>", 10, "      <div class=", 34, "footer-links", 34, ">", 10, "        <a href=", 34, "/demo", 34, " class=", 34, "footer-link", 34, ">Interactive Demo</a>", 10, "        <a href=", 34, "/benchmarks", 34, " class=", 34, "footer-link", 34, ">Benchmarks</a>", 10, "        <a href=", 34, "/docs", 34, " class=", 34, "footer-link", 34, ">Documentation</a>", 10, "        <a href=", 34, "/forum", 34, " class=", 34, "footer-link", 34, ">Community Forum</a>", 10, "      </div>", 10, "      <p>&copy; 2024 MethASM. Compiles to high-performance x86-64 assembly.</p>", 10, "    </footer>", 10, "  </div>", 10, 10, "  <script>", 10, "    function showTab(tabName) {", 10, "      // Hide all content", 10, "      document.querySelectorAll('.code-content').forEach(content => {", 10, "        content.classList.remove('active');", 10, "      });", 10, "      ", 10, "      // Remove active from all tabs", 10, "      document.querySelectorAll('.tab').forEach(tab => {", 10, "        tab.classList.remove('active');", 10, "      });", 10, "      ", 10, "      // Show selected content", 10, "      document.getElementById(tabName).classList.add('active');", 10, "      ", 10, "      // Add active to clicked tab", 10, "      event.target.classList.add('active');", 10, "    }", 10, "  </script>", 10, "</body>", 10, "</html>", 10, 0


DEMO_CONTENT:
    dq Lstr1
    dq 19546
Lstr1:
    db "<!DOCTYPE html>", 10, "<html lang='en'>", 10, "<head>", 10, "<meta charset='UTF-8'>", 10, "<meta name='viewport' content='width=device-width,initial-scale=1'>", 10, "<meta name='description' content='Interactive MethASM code demonstrations and examples. Try the language features in your browser.'>", 10, "<title>MethASM ", 226, 128, 148, " Interactive Demo</title>", 10, "<link rel=", 34, "preconnect", 34, " href=", 34, "https://fonts.googleapis.com", 34, ">", 10, "<link rel=", 34, "preconnect", 34, " href=", 34, "https://fonts.gstatic.com", 34, " crossorigin>", 10, "<link href=", 34, "https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&family=JetBrains+Mono:wght@400;500;600&display=swap", 34, " rel=", 34, "stylesheet", 34, ">", 10, "<style>", 10, "  :root {", 10, "    --bg-color: #05050A;", 10, "    --text-main: #E2E8F0;", 10, "    --text-muted: #94A3B8;", 10, "    --accent-1: #6366F1;", 10, "    --accent-2: #8B5CF6;", 10, "    --accent-3: #EC4899;", 10, "    --accent-4: #10B981;", 10, "    --glass-bg: rgba(255, 255, 255, 0.03);", 10, "    --glass-border: rgba(255, 255, 255, 0.08);", 10, "    --code-bg: #0D1117;", 10, "    --success: #10B981;", 10, "    --warning: #F59E0B;", 10, "    --error: #EF4444;", 10, "  }", 10, 10, "  * { box-sizing: border-box; margin: 0; padding: 0; }", 10, "  ", 10, "  body {", 10, "    font-family: 'Inter', sans-serif;", 10, "    background-color: var(--bg-color);", 10, "    color: var(--text-main);", 10, "    line-height: 1.6;", 10, "    overflow-x: hidden;", 10, "  }", 10, 10, "  /* Animated background mesh */", 10, "  .bg-mesh {", 10, "    position: fixed;", 10, "    top: 0; left: 0; right: 0; bottom: 0;", 10, "    z-index: -1;", 10, "    background: ", 10, "      radial-gradient(circle at 15% 50%, rgba(99, 102, 241, 0.15) 0%, transparent 50%),", 10, "      radial-gradient(circle at 85% 30%, rgba(236, 72, 153, 0.15) 0%, transparent 50%),", 10, "      radial-gradient(circle at 50% 80%, rgba(139, 92, 246, 0.15) 0%, transparent 50%);", 10, "    filter: blur(60px);", 10, "    animation: pulse 15s ease-in-out infinite alternate;", 10, "  }", 10, 10, "  @keyframes pulse {", 10, "    0% { transform: scale(1); opacity: 0.8; }", 10, "    100% { transform: scale(1.1); opacity: 1; }", 10, "  }", 10, 10, "  .container {", 10, "    max-width: 1200px;", 10, "    margin: 0 auto;", 10, "    padding: 2rem;", 10, "  }", 10, 10, "  /* Navigation */", 10, "  nav {", 10, "    display: flex;", 10, "    justify-content: space-between;", 10, "    align-items: center;", 10, "    padding: 1rem 0;", 10, "    margin-bottom: 3rem;", 10, "    border-bottom: 1px solid var(--glass-border);", 10, "  }", 10, 10, "  .nav-links {", 10, "    display: flex;", 10, "    gap: 2rem;", 10, "    align-items: center;", 10, "  }", 10, 10, "  .nav-link {", 10, "    color: var(--text-muted);", 10, "    text-decoration: none;", 10, "    font-weight: 500;", 10, "    transition: color 0.3s ease;", 10, "  }", 10, 10, "  .nav-link:hover {", 10, "    color: var(--accent-1);", 10, "  }", 10, 10, "  /* Header */", 10, "  .header {", 10, "    text-align: center;", 10, "    margin-bottom: 4rem;", 10, "    animation: slideUp 0.8s ease-out;", 10, "  }", 10, 10, "  .header h1 {", 10, "    font-size: clamp(2.5rem, 6vw, 4rem);", 10, "    font-weight: 700;", 10, "    margin-bottom: 1.5rem;", 10, "    background: linear-gradient(135deg, var(--text-main) 0%, var(--accent-1) 50%, var(--accent-3) 100%);", 10, "    -webkit-background-clip: text;", 10, "    background-clip: text;", 10, "    -webkit-text-fill-color: transparent;", 10, "    letter-spacing: -1px;", 10, "  }", 10, 10, "  .header p {", 10, "    font-size: 1.2rem;", 10, "    color: var(--text-muted);", 10, "    max-width: 600px;", 10, "    margin: 0 auto;", 10, "  }", 10, 10, "  /* Demo Sections */", 10, "  .demo-section {", 10, "    margin-bottom: 4rem;", 10, "    animation: fadeUp 0.8s ease-out backwards;", 10, "  }", 10, 10, "  .demo-section:nth-child(2) { animation-delay: 0.1s; }", 10, "  .demo-section:nth-child(3) { animation-delay: 0.2s; }", 10, "  .demo-section:nth-child(4) { animation-delay: 0.3s; }", 10, 10, "  .demo-header {", 10, "    display: flex;", 10, "    justify-content: space-between;", 10, "    align-items: center;", 10, "    margin-bottom: 2rem;", 10, "    padding: 1.5rem;", 10, "    background: var(--glass-bg);", 10, "    border: 1px solid var(--glass-border);", 10, "    border-radius: 16px;", 10, "    backdrop-filter: blur(10px);", 10, "  }", 10, 10, "  .demo-title {", 10, "    font-size: 1.5rem;", 10, "    font-weight: 600;", 10, "    color: white;", 10, "  }", 10, 10, "  .demo-description {", 10, "    color: var(--text-muted);", 10, "    margin-bottom: 1rem;", 10, "  }", 10, 10, "  .demo-controls {", 10, "    display: flex;", 10, "    gap: 1rem;", 10, "  }", 10, 10, "  .btn {", 10, "    padding: 0.6rem 1.5rem;", 10, "    border-radius: 8px;", 10, "    text-decoration: none;", 10, "    font-weight: 500;", 10, "    transition: all 0.3s ease;", 10, "    font-size: 0.9rem;", 10, "    border: none;", 10, "    cursor: pointer;", 10, "  }", 10, 10, "  .btn-primary {", 10, "    background: linear-gradient(135deg, var(--accent-1) 0%, var(--accent-2) 100%);", 10, "    color: white;", 10, "  }", 10, 10, "  .btn-primary:hover {", 10, "    transform: translateY(-1px);", 10, "    box-shadow: 0 4px 12px rgba(99, 102, 241, 0.4);", 10, "  }", 10, 10, "  .btn-secondary {", 10, "    background: var(--glass-bg);", 10, "    color: var(--text-main);", 10, "    border: 1px solid var(--glass-border);", 10, "  }", 10, 10, "  .btn-secondary:hover {", 10, "    background: rgba(255, 255, 255, 0.08);", 10, "  }", 10, 10, "  /* Code Editor */", 10, "  .editor-container {", 10, "    display: grid;", 10, "    grid-template-columns: 1fr 1fr;", 10, "    gap: 1rem;", 10, "    margin-bottom: 2rem;", 10, "  }", 10, 10, "  .editor-panel {", 10, "    background: var(--code-bg);", 10, "    border: 1px solid #30363D;", 10, "    border-radius: 12px;", 10, "    overflow: hidden;", 10, "  }", 10, 10, "  .panel-header {", 10, "    background: #161B22;", 10, "    padding: 0.75rem 1rem;", 10, "    border-bottom: 1px solid #30363D;", 10, "    display: flex;", 10, "    align-items: center;", 10, "    gap: 0.5rem;", 10, "  }", 10, 10, "  .panel-title {", 10, "    color: var(--text-main);", 10, "    font-weight: 500;", 10, "    font-size: 0.9rem;", 10, "  }", 10, 10, "  .panel-content {", 10, "    padding: 1rem;", 10, "  }", 10, 10, "  .code-editor {", 10, "    width: 100%;", 10, "    min-height: 300px;", 10, "    background: transparent;", 10, "    border: none;", 10, "    color: #E2E8F0;", 10, "    font-family: 'JetBrains Mono', monospace;", 10, "    font-size: 0.9rem;", 10, "    line-height: 1.6;", 10, "    resize: vertical;", 10, "    outline: none;", 10, "  }", 10, 10, "  .output-display {", 10, "    min-height: 300px;", 10, "    background: transparent;", 10, "    border: none;", 10, "    color: #E2E8F0;", 10, "    font-family: 'JetBrains Mono', monospace;", 10, "    font-size: 0.9rem;", 10, "    line-height: 1.6;", 10, "    padding: 1rem;", 10, "    white-space: pre-wrap;", 10, "    overflow-y: auto;", 10, "  }", 10, 10, "  /* Feature Cards */", 10, "  .feature-grid {", 10, "    display: grid;", 10, "    grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));", 10, "    gap: 2rem;", 10, "    margin-bottom: 4rem;", 10, "  }", 10, 10, "  .feature-card {", 10, "    background: var(--glass-bg);", 10, "    border: 1px solid var(--glass-border);", 10, "    border-radius: 16px;", 10, "    padding: 2rem;", 10, "    backdrop-filter: blur(10px);", 10, "    transition: all 0.3s ease;", 10, "    cursor: pointer;", 10, "  }", 10, 10, "  .feature-card:hover {", 10, "    transform: translateY(-4px);", 10, "    border-color: var(--accent-1);", 10, "    box-shadow: 0 10px 30px rgba(0, 0, 0, 0.3);", 10, "  }", 10, 10, "  .feature-icon {", 10, "    font-size: 2rem;", 10, "    margin-bottom: 1rem;", 10, "    display: block;", 10, "  }", 10, 10, "  .feature-title {", 10, "    font-size: 1.2rem;", 10, "    font-weight: 600;", 10, "    color: white;", 10, "    margin-bottom: 0.5rem;", 10, "  }", 10, 10, "  .feature-desc {", 10, "    color: var(--text-muted);", 10, "    font-size: 0.9rem;", 10, "  }", 10, 10, "  /* Status Messages */", 10, "  .status {", 10, "    padding: 1rem;", 10, "    border-radius: 8px;", 10, "    margin-bottom: 1rem;", 10, "    font-weight: 500;", 10, "  }", 10, 10, "  .status.success {", 10, "    background: rgba(16, 185, 129, 0.1);", 10, "    border: 1px solid var(--success);", 10, "    color: var(--success);", 10, "  }", 10, 10, "  .status.error {", 10, "    background: rgba(239, 68, 68, 0.1);", 10, "    border: 1px solid var(--error);", 10, "    color: var(--error);", 10, "  }", 10, 10, "  .status.info {", 10, "    background: rgba(99, 102, 241, 0.1);", 10, "    border: 1px solid var(--accent-1);", 10, "    color: var(--accent-1);", 10, "  }", 10, 10, "  /* Animations */", 10, "  @keyframes slideUp {", 10, "    from { opacity: 0; transform: translateY(30px); }", 10, "    to { opacity: 1; transform: translateY(0); }", 10, "  }", 10, 10, "  @keyframes fadeUp {", 10, "    from { opacity: 0; transform: translateY(20px); }", 10, "    to { opacity: 1; transform: translateY(0); }", 10, "  }", 10, 10, "  /* Responsive */", 10, "  @media (max-width: 768px) {", 10, "    .nav-links {", 10, "      flex-direction: column;", 10, "      gap: 1rem;", 10, "    }", 10, 10, "    .editor-container {", 10, "      grid-template-columns: 1fr;", 10, "    }", 10, 10, "    .demo-header {", 10, "      flex-direction: column;", 10, "      align-items: flex-start;", 10, "      gap: 1rem;", 10, "    }", 10, 10, "    .feature-grid {", 10, "      grid-template-columns: 1fr;", 10, "    }", 10, "  }", 10, "</style>", 10, "</head>", 10, "<body>", 10, "  <div class=", 34, "bg-mesh", 34, "></div>", 10, "  ", 10, "  <div class=", 34, "container", 34, ">", 10, "    <nav>", 10, "      <div class=", 34, "nav-links", 34, ">", 10, "        <a href=", 34, "/", 34, " class=", 34, "nav-link", 34, ">Home</a>", 10, "        <a href=", 34, "/demo", 34, " class=", 34, "nav-link", 34, ">Demo</a>", 10, "        <a href=", 34, "/benchmarks", 34, " class=", 34, "nav-link", 34, ">Benchmarks</a>", 10, "        <a href=", 34, "/docs", 34, " class=", 34, "nav-link", 34, ">Docs</a>", 10, "        <a href=", 34, "/forum", 34, " class=", 34, "nav-link", 34, ">Forum</a>", 10, "      </div>", 10, "    </nav>", 10, 10, "    <header class=", 34, "header", 34, ">", 10, "      <h1>Interactive Demo</h1>", 10, "      <p>Explore MethASM's features through live, interactive code examples. Try modifying the code and see the results!</p>", 10, "    </header>", 10, 10, "    <!-- Feature Selection -->", 10, "    <div class=", 34, "feature-grid", 34, ">", 10, "      <div class=", 34, "feature-card", 34, " onclick=", 34, "loadDemo('basics')", 34, ">", 10, "        <span class=", 34, "feature-icon", 34, ">", 240, 159, 154, 128, "</span>", 10, "        <div class=", 34, "feature-title", 34, ">Language Basics</div>", 10, "        <div class=", 34, "feature-desc", 34, ">Variables, functions, and control flow fundamentals</div>", 10, "      </div>", 10, "      ", 10, "      <div class=", 34, "feature-card", 34, " onclick=", 34, "loadDemo('structs')", 34, ">", 10, "        <span class=", 34, "feature-icon", 34, ">", 240, 159, 143, 151, 239, 184, 143, "</span>", 10, "        <div class=", 34, "feature-title", 34, ">Structs & Methods</div>", 10, "        <div class=", 34, "feature-desc", 34, ">Data structures and their associated methods</div>", 10, "      </div>", 10, "      ", 10, "      <div class=", 34, "feature-card", 34, " onclick=", 34, "loadDemo('memory')", 34, ">", 10, "        <span class=", 34, "feature-icon", 34, ">", 240, 159, 167, 160, "</span>", 10, "        <div class=", 34, "feature-title", 34, ">Memory Management</div>", 10, "        <div class=", 34, "feature-desc", 34, ">Stack allocation vs garbage collection</div>", 10, "      </div>", 10, "      ", 10, "      <div class=", 34, "feature-card", 34, " onclick=", 34, "loadDemo('interop')", 34, ">", 10, "        <span class=", 34, "feature-icon", 34, ">", 240, 159, 148, 151, "</span>", 10, "        <div class=", 34, "feature-title", 34, ">C Interop</div>", 10, "        <div class=", 34, "feature-desc", 34, ">Calling external C functions</div>", 10, "      </div>", 10, "      ", 10, "      <div class=", 34, "feature-card", 34, " onclick=", 34, "loadDemo('networking')", 34, ">", 10, "        <span class=", 34, "feature-icon", 34, ">", 240, 159, 140, 144, "</span>", 10, "        <div class=", 34, "feature-title", 34, ">Networking</div>", 10, "        <div class=", 34, "feature-desc", 34, ">Socket programming and network I/O</div>", 10, "      </div>", 10, "      ", 10, "      <div class=", 34, "feature-card", 34, " onclick=", 34, "loadDemo('advanced')", 34, ">", 10, "        <span class=", 34, "feature-icon", 34, ">", 226, 154, 161, "</span>", 10, "        <div class=", 34, "feature-title", 34, ">Advanced Features</div>", 10, "        <div class=", 34, "feature-desc", 34, ">Pointers, arrays, and optimization</div>", 10, "      </div>", 10, "    </div>", 10, 10, "    <!-- Demo Editor -->", 10, "    <div class=", 34, "demo-section", 34, ">", 10, "      <div class=", 34, "demo-header", 34, ">", 10, "        <div>", 10, "          <div class=", 34, "demo-title", 34, " id=", 34, "demoTitle", 34, ">Language Basics</div>", 10, "          <div class=", 34, "demo-description", 34, " id=", 34, "demoDescription", 34, ">Try basic MethAML syntax and concepts</div>", 10, "        </div>", 10, "        <div class=", 34, "demo-controls", 34, ">", 10, "          <button class=", 34, "btn btn-secondary", 34, " onclick=", 34, "resetCode()", 34, ">Reset</button>", 10, "          <button class=", 34, "btn btn-primary", 34, " onclick=", 34, "runCode()", 34, ">Run Code</button>", 10, "        </div>", 10, "      </div>", 10, 10, "      <div id=", 34, "statusMessage", 34, "></div>", 10, 10, "      <div class=", 34, "editor-container", 34, ">", 10, "        <div class=", 34, "editor-panel", 34, ">", 10, "          <div class=", 34, "panel-header", 34, ">", 10, "            <div class=", 34, "panel-title", 34, ">", 240, 159, 147, 157, " MethASM Code</div>", 10, "          </div>", 10, "          <div class=", 34, "panel-content", 34, ">", 10, "            <textarea id=", 34, "codeEditor", 34, " class=", 34, "code-editor", 34, " spellcheck=", 34, "false", 34, ">// MethASM Interactive Demo", 10, "// Try modifying this code and click ", 34, "Run Code", 34, 10, 10, "import ", 34, "std/io", 34, ";", 10, 10, "function factorial(n: int32) -> int32 {", 10, "  if (n <= 1) {", 10, "    return 1;", 10, "  }", 10, "  return n * factorial(n - 1);", 10, "}", 10, 10, "function main() -> int32 {", 10, "  var i: int32 = 5;", 10, "  var result: int32 = factorial(i);", 10, "  ", 10, "  puts(", 34, "Factorial of ", 34, ");", 10, "  println_int(i);", 10, "  puts(", 34, " is ", 34, ");", 10, "  println_int(result);", 10, "  ", 10, "  return 0;", 10, "}</textarea>", 10, "          </div>", 10, "        </div>", 10, 10, "        <div class=", 34, "editor-panel", 34, ">", 10, "          <div class=", 34, "panel-header", 34, ">", 10, "            <div class=", 34, "panel-title", 34, ">", 240, 159, 147, 164, " Output</div>", 10, "          </div>", 10, "          <div class=", 34, "panel-content", 34, ">", 10, "            <div id=", 34, "outputDisplay", 34, " class=", 34, "output-display", 34, ">// Click ", 34, "Run Code", 34, " to see the output", 10, "// This simulates what the MethASM compiler would produce</div>", 10, "          </div>", 10, "        </div>", 10, "      </div>", 10, "    </div>", 10, "  </div>", 10, 10, "  <script>", 10, "    const demos = {", 10, "      basics: {", 10, "        title: ", 34, "Language Basics", 34, ",", 10, "        description: ", 34, "Try basic MethASM syntax and concepts", 34, ",", 10, "        code: `// MethASM Language Basics", 10, "// Variables, functions, and control flow", 10, 10, "import ", 34, "std/io", 34, ";", 10, 10, "function fibonacci(n: int32) -> int32 {", 10, "  if (n <= 1) {", 10, "    return n;", 10, "  }", 10, "  return fibonacci(n - 1) + fibonacci(n - 2);", 10, "}", 10, 10, "function main() -> int32 {", 10, "  var count: int32 = 10;", 10, "  var i: int32 = 0;", 10, "  ", 10, "  puts(", 34, "Fibonacci sequence: ", 34, ");", 10, "  while (i < count) {", 10, "    println_int(fibonacci(i));", 10, "    i = i + 1;", 10, "  }", 10, "  ", 10, "  return 0;", 10, "}`,", 10, "        output: `Fibonacci sequence: ", 10, "0", 10, "1", 10, "1", 10, "2", 10, "3", 10, "5", 10, "8", 10, "13", 10, "21", 10, "34`", 10, "      },", 10, "      ", 10, "      structs: {", 10, "        title: ", 34, "Structs & Methods", 34, ",", 10, "        description: ", 34, "Data structures and their associated methods", 34, ",", 10, "        code: `// Structs and Methods in MethASM", 10, "// Define data structures with behavior", 10, 10, "import ", 34, "std/io", 34, ";", 10, 10, "struct Vector2D {", 10, "  x: float64;", 10, "  y: float64;", 10, "  ", 10, "  function magnitude() -> float64 {", 10, "    return (this.x * this.x + this.y * this.y) * 0.5; // Simplified sqrt", 10, "  }", 10, "  ", 10, "  function add(other: Vector2D*) -> Vector2D {", 10, "    var result: Vector2D;", 10, "    result.x = this.x + other->x;", 10, "    result.y = this.y + other->y;", 10, "    return result;", 10, "  }", 10, "}", 10, 10, "function main() -> int32 {", 10, "  var v1: Vector2D = {3.0, 4.0};", 10, "  var v2: Vector2D = {1.0, 2.0};", 10, "  ", 10, "  var sum: Vector2D = v1.add(&v2);", 10, "  ", 10, "  puts(", 34, "Vector sum: (", 34, ");", 10, "  print_float(sum.x);", 10, "  puts(", 34, ", ", 34, ");", 10, "  print_float(sum.y);", 10, "  puts(", 34, ")", 34, ");", 10, "  ", 10, "  return 0;", 10, "}`,", 10, "        output: `Vector sum: (4.0, 6.0)`", 10, "      },", 10, "      ", 10, "      memory: {", 10, "        title: ", 34, "Memory Management", 34, ",", 10, "        description: ", 34, "Stack allocation vs garbage collection", 34, ",", 10, "        code: `// Memory Management in MethASM", 10, "// Stack allocation vs garbage collection", 10, 10, "import ", 34, "std/io", 34, ";", 10, "import ", 34, "std/mem", 34, ";", 10, 10, "struct Node {", 10, "  value: int32;", 10, "  next: Node*;", 10, "}", 10, 10, "function main() -> int32 {", 10, "  // Stack allocation (automatic cleanup)", 10, "  var stack_var: int32 = 42;", 10, "  puts(", 34, "Stack variable: ", 34, ");", 10, "  println_int(stack_var);", 10, "  ", 10, "  // Manual memory management with C malloc", 10, "  var buffer: cstring = malloc(1024);", 10, "  if (buffer != 0) {", 10, "    puts(", 34, "Allocated 1024 bytes with malloc", 34, ");", 10, "    free(buffer);", 10, "    puts(", 34, "Freed memory", 34, ");", 10, "  }", 10, "  ", 10, "  // Garbage collection (if GC runtime linked)", 10, "  // var gc_node: Node* = new Node;", 10, "  // gc_node->value = 100;", 10, "  // puts(", 34, "Created node with GC", 34, ");", 10, "  ", 10, "  return 0;", 10, "}`,", 10, "        output: `Stack variable: 42", 10, "Allocated 1024 bytes with malloc", 10, "Freed memory`", 10, "      },", 10, "      ", 10, "      interop: {", 10, "        title: ", 34, "C Interop", 34, ",", 10, "        description: ", 34, "Calling external C functions", 34, ",", 10, "        code: `// C Interoperability in MethASM", 10, "// Calling external C functions", 10, 10, "import ", 34, "std/io", 34, ";", 10, 10, "// Declare external C functions", 10, "extern function strlen(s: cstring) -> int32 = ", 34, "strlen", 34, ";", 10, "extern function strcpy(dest: cstring, src: cstring) -> cstring = ", 34, "strcpy", 34, ";", 10, "extern function atoi(s: cstring) -> int32 = ", 34, "atoi", 34, ";", 10, 10, "function main() -> int32 {", 10, "  var message: uint8[256];", 10, "  var number_str: uint8[32] = ", 34, "42", 34, ";", 10, "  ", 10, "  // Use C string functions", 10, "  strcpy(&message[0], ", 34, "Hello from C interop!", 34, ");", 10, "  var len: int32 = strlen(&message[0]);", 10, "  ", 10, "  puts(", 34, "Message: ", 34, ");", 10, "  println(&message[0]);", 10, "  puts(", 34, "Length: ", 34, ");", 10, "  println_int(len);", 10, "  ", 10, "  // Convert string to integer", 10, "  var number: int32 = atoi(&number_str[0]);", 10, "  puts(", 34, "Parsed number: ", 34, ");", 10, "  println_int(number);", 10, "  ", 10, "  return 0;", 10, "}`,", 10, "        output: `Message: Hello from C interop!", 10, "Length: 19", 10, "Parsed number: 42`", 10, "      },", 10, "      ", 10, "      networking: {", 10, "        title: ", 34, "Networking", 34, ",", 10, "        description: ", 34, "Socket programming and network I/O", 34, ",", 10, "        code: `// Networking with MethASM", 10, "// Basic TCP socket operations", 10, 10, "import ", 34, "std/net", 34, ";", 10, "import ", 34, "std/io", 34, ";", 10, 10, "function main() -> int32 {", 10, "  if (net_init() != 0) {", 10, "    puts(", 34, "Failed to initialize network", 34, ");", 10, "    return 1;", 10, "  }", 10, "  ", 10, "  var sock: int64 = socket_tcp();", 10, "  if (sock == INVALID_SOCKET()) {", 10, "    puts(", 34, "Failed to create socket", 34, ");", 10, "    net_cleanup();", 10, "    return 1;", 10, "  }", 10, "  ", 10, "  var addr: cstring = sockaddr_in_any(8080);", 10, "  if (bind(sock, addr, 16) != 0) {", 10, "    puts(", 34, "Failed to bind to port 8080", 34, ");", 10, "    closesocket(sock);", 10, "    net_cleanup();", 10, "    return 1;", 10, "  }", 10, "  ", 10, "  if (listen(sock, 5) != 0) {", 10, "    puts(", 34, "Failed to listen", 34, ");", 10, "    closesocket(sock);", 10, "    net_cleanup();", 10, "    return 1;", 10, "  }", 10, "  ", 10, "  puts(", 34, "Server listening on port 8080", 34, ");", 10, "  puts(", 34, "Press Ctrl+C to stop", 34, ");", 10, "  ", 10, "  // In a real server, you'd have an accept loop here", 10, "  closesocket(sock);", 10, "  net_cleanup();", 10, "  return 0;", 10, "}`,", 10, "        output: `Server listening on port 8080", 10, "Press Ctrl+C to stop`", 10, "      },", 10, "      ", 10, "      advanced: {", 10, "        title: ", 34, "Advanced Features", 34, ",", 10, "        description: ", 34, "Pointers, arrays, and optimization", 34, ",", 10, "        code: `// Advanced MethASM Features", 10, "// Pointers, arrays, and performance", 10, 10, "import ", 34, "std/io", 34, ";", 10, "import ", 34, "std/mem", 34, ";", 10, 10, "struct Matrix {", 10, "  rows: int32;", 10, "  cols: int32;", 10, "  data: float64*;", 10, "}", 10, 10, "function matrix_create(rows: int32, cols: int32) -> Matrix* {", 10, "  var m: Matrix* = malloc(sizeof(Matrix));", 10, "  m->rows = rows;", 10, "  m->cols = cols;", 10, "  m->data = malloc(rows * cols * sizeof(float64));", 10, "  return m;", 10, "}", 10, 10, "function matrix_multiply(a: Matrix*, b: Matrix*) -> Matrix* {", 10, "  if (a->cols != b->rows) {", 10, "    puts(", 34, "Matrix dimensions incompatible", 34, ");", 10, "    return 0;", 10, "  }", 10, "  ", 10, "  var result: Matrix* = matrix_create(a->rows, b->cols);", 10, "  var i: int32 = 0;", 10, "  var j: int32 = 0;", 10, "  var k: int32 = 0;", 10, "  ", 10, "  while (i < a->rows) {", 10, "    j = 0;", 10, "    while (j < b->cols) {", 10, "      result->data[i * result->cols + j] = 0.0;", 10, "      k = 0;", 10, "      while (k < a->cols) {", 10, "        var a_val: float64 = a->data[i * a->cols + k];", 10, "        var b_val: float64 = b->data[k * b->cols + j];", 10, "        result->data[i * result->cols + j] = result->data[i * result->cols + j] + a_val * b_val;", 10, "        k = k + 1;", 10, "      }", 10, "      j = j + 1;", 10, "    }", 10, "    i = i + 1;", 10, "  }", 10, "  ", 10, "  return result;", 10, "}", 10, 10, "function main() -> int32 {", 10, "  var m1: Matrix* = matrix_create(2, 2);", 10, "  var m2: Matrix* = matrix_create(2, 2);", 10, "  ", 10, "  // Initialize matrices", 10, "  m1->data[0] = 1.0; m1->data[1] = 2.0;", 10, "  m1->data[2] = 3.0; m1->data[3] = 4.0;", 10, "  ", 10, "  m2->data[0] = 5.0; m2->data[1] = 6.0;", 10, "  m2->data[2] = 7.0; m2->data[3] = 8.0;", 10, "  ", 10, "  var result: Matrix* = matrix_multiply(m1, m2);", 10, "  if (result != 0) {", 10, "    puts(", 34, "Matrix multiplication completed", 34, ");", 10, "    puts(", 34, "Result dimensions: ", 34, ");", 10, "    print_int(result->rows);", 10, "    puts(", 34, "x", 34, ");", 10, "    println_int(result->cols);", 10, "  }", 10, "  ", 10, "  return 0;", 10, "}`,", 10, "        output: `Matrix multiplication completed", 10, "Result dimensions: 2x2`", 10, "      }", 10, "    };", 10, 10, "    let currentDemo = 'basics';", 10, 10, "    function loadDemo(demoName) {", 10, "      const demo = demos[demoName];", 10, "      if (demo) {", 10, "        currentDemo = demoName;", 10, "        document.getElementById('demoTitle').textContent = demo.title;", 10, "        document.getElementById('demoDescription').textContent = demo.description;", 10, "        document.getElementById('codeEditor').value = demo.code;", 10, "        document.getElementById('outputDisplay').textContent = '// Click ", 34, "Run Code", 34, " to see the output';", 10, "        clearStatus();", 10, "      }", 10, "    }", 10, 10, "    function runCode() {", 10, "      const code = document.getElementById('codeEditor').value;", 10, "      const output = document.getElementById('outputDisplay');", 10, "      ", 10, "      showStatus('Compiling and running MethASM code...', 'info');", 10, "      ", 10, "      // Simulate compilation and execution", 10, "      setTimeout(() => {", 10, "        const demo = demos[currentDemo];", 10, "        if (demo) {", 10, "          output.textContent = demo.output;", 10, "          showStatus('Code executed successfully!', 'success');", 10, "        } else {", 10, "          output.textContent = '// Demo output not available';", 10, "          showStatus('Demo not found', 'error');", 10, "        }", 10, "      }, 1000);", 10, "    }", 10, 10, "    function resetCode() {", 10, "      loadDemo(currentDemo);", 10, "    }", 10, 10, "    function showStatus(message, type) {", 10, "      const statusDiv = document.getElementById('statusMessage');", 10, "      statusDiv.className = `status ${type}`;", 10, "      statusDiv.textContent = message;", 10, "      statusDiv.style.display = 'block';", 10, "      ", 10, "      if (type === 'success') {", 10, "        setTimeout(clearStatus, 3000);", 10, "      }", 10, "    }", 10, 10, "    function clearStatus() {", 10, "      const statusDiv = document.getElementById('statusMessage');", 10, "      statusDiv.style.display = 'none';", 10, "    }", 10, 10, "    // Initialize with basics demo", 10, "    window.onload = function() {", 10, "      loadDemo('basics');", 10, "    };", 10, "  </script>", 10, "</body>", 10, "</html>", 10, 0


BENCHMARKS_CONTENT:
    dq Lstr2
    dq 19074
Lstr2:
    db "<!DOCTYPE html>", 10, "<html lang='en'>", 10, "<head>", 10, "<meta charset='UTF-8'>", 10, "<meta name='viewport' content='width=device-width,initial-scale=1'>", 10, "<meta name='description' content='MethASM performance benchmarks and comparisons with other languages.'>", 10, "<title>MethASM ", 226, 128, 148, " Performance Benchmarks</title>", 10, "<link rel=", 34, "preconnect", 34, " href=", 34, "https://fonts.googleapis.com", 34, ">", 10, "<link rel=", 34, "preconnect", 34, " href=", 34, "https://fonts.gstatic.com", 34, " crossorigin>", 10, "<link href=", 34, "https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&family=JetBrains+Mono:wght@400;500;600&display=swap", 34, " rel=", 34, "stylesheet", 34, ">", 10, "<style>", 10, "  :root {", 10, "    --bg-color: #05050A;", 10, "    --text-main: #E2E8F0;", 10, "    --text-muted: #94A3B8;", 10, "    --accent-1: #6366F1;", 10, "    --accent-2: #8B5CF6;", 10, "    --accent-3: #EC4899;", 10, "    --accent-4: #10B981;", 10, "    --glass-bg: rgba(255, 255, 255, 0.03);", 10, "    --glass-border: rgba(255, 255, 255, 0.08);", 10, "    --code-bg: #0D1117;", 10, "    --success: #10B981;", 10, "    --warning: #F59E0B;", 10, "    --error: #EF4444;", 10, "  }", 10, 10, "  * { box-sizing: border-box; margin: 0; padding: 0; }", 10, "  ", 10, "  body {", 10, "    font-family: 'Inter', sans-serif;", 10, "    background-color: var(--bg-color);", 10, "    color: var(--text-main);", 10, "    line-height: 1.6;", 10, "    overflow-x: hidden;", 10, "  }", 10, 10, "  /* Animated background mesh */", 10, "  .bg-mesh {", 10, "    position: fixed;", 10, "    top: 0; left: 0; right: 0; bottom: 0;", 10, "    z-index: -1;", 10, "    background: ", 10, "      radial-gradient(circle at 15% 50%, rgba(99, 102, 241, 0.15) 0%, transparent 50%),", 10, "      radial-gradient(circle at 85% 30%, rgba(236, 72, 153, 0.15) 0%, transparent 50%),", 10, "      radial-gradient(circle at 50% 80%, rgba(139, 92, 246, 0.15) 0%, transparent 50%);", 10, "    filter: blur(60px);", 10, "    animation: pulse 15s ease-in-out infinite alternate;", 10, "  }", 10, 10, "  @keyframes pulse {", 10, "    0% { transform: scale(1); opacity: 0.8; }", 10, "    100% { transform: scale(1.1); opacity: 1; }", 10, "  }", 10, 10, "  .container {", 10, "    max-width: 1200px;", 10, "    margin: 0 auto;", 10, "    padding: 2rem;", 10, "  }", 10, 10, "  /* Navigation */", 10, "  nav {", 10, "    display: flex;", 10, "    justify-content: space-between;", 10, "    align-items: center;", 10, "    padding: 1rem 0;", 10, "    margin-bottom: 3rem;", 10, "    border-bottom: 1px solid var(--glass-border);", 10, "  }", 10, 10, "  .nav-links {", 10, "    display: flex;", 10, "    gap: 2rem;", 10, "    align-items: center;", 10, "  }", 10, 10, "  .nav-link {", 10, "    color: var(--text-muted);", 10, "    text-decoration: none;", 10, "    font-weight: 500;", 10, "    transition: color 0.3s ease;", 10, "  }", 10, 10, "  .nav-link:hover {", 10, "    color: var(--accent-1);", 10, "  }", 10, 10, "  /* Header */", 10, "  .header {", 10, "    text-align: center;", 10, "    margin-bottom: 4rem;", 10, "    animation: slideUp 0.8s ease-out;", 10, "  }", 10, 10, "  .header h1 {", 10, "    font-size: clamp(2.5rem, 6vw, 4rem);", 10, "    font-weight: 700;", 10, "    margin-bottom: 1.5rem;", 10, "    background: linear-gradient(135deg, var(--text-main) 0%, var(--accent-1) 50%, var(--accent-3) 100%);", 10, "    -webkit-background-clip: text;", 10, "    background-clip: text;", 10, "    -webkit-text-fill-color: transparent;", 10, "    letter-spacing: -1px;", 10, "  }", 10, 10, "  .header p {", 10, "    font-size: 1.2rem;", 10, "    color: var(--text-muted);", 10, "    max-width: 600px;", 10, "    margin: 0 auto;", 10, "  }", 10, 10, "  /* Benchmark Sections */", 10, "  .benchmark-section {", 10, "    margin-bottom: 4rem;", 10, "    animation: fadeUp 0.8s ease-out backwards;", 10, "  }", 10, 10, "  .benchmark-section:nth-child(2) { animation-delay: 0.1s; }", 10, "  .benchmark-section:nth-child(3) { animation-delay: 0.2s; }", 10, "  .benchmark-section:nth-child(4) { animation-delay: 0.3s; }", 10, 10, "  .section-title {", 10, "    font-size: 2rem;", 10, "    font-weight: 600;", 10, "    margin-bottom: 2rem;", 10, "    color: white;", 10, "  }", 10, 10, "  /* Benchmark Cards */", 10, "  .benchmark-grid {", 10, "    display: grid;", 10, "    grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));", 10, "    gap: 2rem;", 10, "    margin-bottom: 3rem;", 10, "  }", 10, 10, "  .benchmark-card {", 10, "    background: var(--glass-bg);", 10, "    border: 1px solid var(--glass-border);", 10, "    border-radius: 16px;", 10, "    padding: 2rem;", 10, "    backdrop-filter: blur(10px);", 10, "    transition: all 0.3s ease;", 10, "  }", 10, 10, "  .benchmark-card:hover {", 10, "    transform: translateY(-4px);", 10, "    border-color: var(--accent-1);", 10, "    box-shadow: 0 10px 30px rgba(0, 0, 0, 0.3);", 10, "  }", 10, 10, "  .benchmark-title {", 10, "    font-size: 1.3rem;", 10, "    font-weight: 600;", 10, "    color: white;", 10, "    margin-bottom: 1rem;", 10, "  }", 10, 10, "  .benchmark-description {", 10, "    color: var(--text-muted);", 10, "    margin-bottom: 1.5rem;", 10, "  }", 10, 10, "  /* Performance Table */", 10, "  .performance-table {", 10, "    width: 100%;", 10, "    border-collapse: collapse;", 10, "    margin-bottom: 2rem;", 10, "  }", 10, 10, "  .performance-table th,", 10, "  .performance-table td {", 10, "    padding: 1rem;", 10, "    text-align: left;", 10, "    border-bottom: 1px solid var(--glass-border);", 10, "  }", 10, 10, "  .performance-table th {", 10, "    background: rgba(99, 102, 241, 0.1);", 10, "    color: white;", 10, "    font-weight: 600;", 10, "  }", 10, 10, "  .performance-table tr:hover {", 10, "    background: rgba(255, 255, 255, 0.02);", 10, "  }", 10, 10, "  .performance-value {", 10, "    font-weight: 600;", 10, "    color: var(--accent-1);", 10, "  }", 10, 10, "  .performance-better {", 10, "    color: var(--success);", 10, "    font-weight: 600;", 10, "  }", 10, 10, "  .performance-worse {", 10, "    color: var(--warning);", 10, "    font-weight: 600;", 10, "  }", 10, 10, "  /* Charts */", 10, "  .chart-container {", 10, "    background: var(--glass-bg);", 10, "    border: 1px solid var(--glass-border);", 10, "    border-radius: 16px;", 10, "    padding: 2rem;", 10, "    margin-bottom: 3rem;", 10, "    backdrop-filter: blur(10px);", 10, "  }", 10, 10, "  .chart-title {", 10, "    font-size: 1.2rem;", 10, "    font-weight: 600;", 10, "    color: white;", 10, "    margin-bottom: 1.5rem;", 10, "  }", 10, 10, "  .bar-chart {", 10, "    display: flex;", 10, "    align-items: end;", 10, "    height: 200px;", 10, "    gap: 1rem;", 10, "    margin-bottom: 1rem;", 10, "  }", 10, 10, "  .bar {", 10, "    flex: 1;", 10, "    background: linear-gradient(135deg, var(--accent-1) 0%, var(--accent-2) 100%);", 10, "    border-radius: 8px 8px 0 0;", 10, "    position: relative;", 10, "    transition: all 0.3s ease;", 10, "    cursor: pointer;", 10, "  }", 10, 10, "  .bar:hover {", 10, "    transform: translateY(-5px);", 10, "    box-shadow: 0 5px 15px rgba(99, 102, 241, 0.4);", 10, "  }", 10, 10, "  .bar-label {", 10, "    position: absolute;", 10, "    bottom: -25px;", 10, "    left: 50%;", 10, "    transform: translateX(-50%);", 10, "    font-size: 0.8rem;", 10, "    color: var(--text-muted);", 10, "    white-space: nowrap;", 10, "  }", 10, 10, "  .bar-value {", 10, "    position: absolute;", 10, "    top: -25px;", 10, "    left: 50%;", 10, "    transform: translateX(-50%);", 10, "    font-size: 0.8rem;", 10, "    font-weight: 600;", 10, "    color: white;", 10, "  }", 10, 10, "  /* Stats Grid */", 10, "  .stats-grid {", 10, "    display: grid;", 10, "    grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));", 10, "    gap: 1.5rem;", 10, "    margin-bottom: 3rem;", 10, "  }", 10, 10, "  .stat-card {", 10, "    background: var(--glass-bg);", 10, "    border: 1px solid var(--glass-border);", 10, "    border-radius: 12px;", 10, "    padding: 1.5rem;", 10, "    text-align: center;", 10, "    backdrop-filter: blur(10px);", 10, "  }", 10, 10, "  .stat-value {", 10, "    font-size: 2rem;", 10, "    font-weight: 700;", 10, "    color: var(--accent-1);", 10, "    margin-bottom: 0.5rem;", 10, "  }", 10, 10, "  .stat-label {", 10, "    color: var(--text-muted);", 10, "    font-size: 0.9rem;", 10, "  }", 10, 10, "  /* Compilation Times */", 10, "  .compilation-stats {", 10, "    background: var(--code-bg);", 10, "    border: 1px solid #30363D;", 10, "    border-radius: 12px;", 10, "    padding: 2rem;", 10, "    margin-bottom: 3rem;", 10, "  }", 10, 10, "  .stat-row {", 10, "    display: flex;", 10, "    justify-content: space-between;", 10, "    align-items: center;", 10, "    padding: 1rem 0;", 10, "    border-bottom: 1px solid #30363D;", 10, "  }", 10, 10, "  .stat-row:last-child {", 10, "    border-bottom: none;", 10, "  }", 10, 10, "  .stat-name {", 10, "    color: var(--text-main);", 10, "    font-weight: 500;", 10, "  }", 10, 10, "  .stat-time {", 10, "    color: var(--accent-1);", 10, "    font-weight: 600;", 10, "    font-family: 'JetBrains Mono', monospace;", 10, "  }", 10, 10, "  /* Animations */", 10, "  @keyframes slideUp {", 10, "    from { opacity: 0; transform: translateY(30px); }", 10, "    to { opacity: 1; transform: translateY(0); }", 10, "  }", 10, 10, "  @keyframes fadeUp {", 10, "    from { opacity: 0; transform: translateY(20px); }", 10, "    to { opacity: 1; transform: translateY(0); }", 10, "  }", 10, 10, "  /* Responsive */", 10, "  @media (max-width: 768px) {", 10, "    .nav-links {", 10, "      flex-direction: column;", 10, "      gap: 1rem;", 10, "    }", 10, 10, "    .benchmark-grid {", 10, "      grid-template-columns: 1fr;", 10, "    }", 10, 10, "    .stats-grid {", 10, "      grid-template-columns: repeat(2, 1fr);", 10, "    }", 10, 10, "    .performance-table {", 10, "      font-size: 0.9rem;", 10, "    }", 10, 10, "    .bar-chart {", 10, "      height: 150px;", 10, "    }", 10, "  }", 10, "</style>", 10, "</head>", 10, "<body>", 10, "  <div class=", 34, "bg-mesh", 34, "></div>", 10, "  ", 10, "  <div class=", 34, "container", 34, ">", 10, "    <nav>", 10, "      <div class=", 34, "nav-links", 34, ">", 10, "        <a href=", 34, "/", 34, " class=", 34, "nav-link", 34, ">Home</a>", 10, "        <a href=", 34, "/demo", 34, " class=", 34, "nav-link", 34, ">Demo</a>", 10, "        <a href=", 34, "/benchmarks", 34, " class=", 34, "nav-link", 34, ">Benchmarks</a>", 10, "        <a href=", 34, "/docs", 34, " class=", 34, "nav-link", 34, ">Docs</a>", 10, "        <a href=", 34, "/forum", 34, " class=", 34, "nav-link", 34, ">Forum</a>", 10, "      </div>", 10, "    </nav>", 10, 10, "    <header class=", 34, "header", 34, ">", 10, "      <h1>Performance Benchmarks</h1>", 10, "      <p>See how MethASM performs compared to other systems programming languages. All benchmarks compiled with optimizations enabled.</p>", 10, "    </header>", 10, 10, "    <!-- Overview Stats -->", 10, "    <div class=", 34, "benchmark-section", 34, ">", 10, "      <h2 class=", 34, "section-title", 34, ">Performance Overview</h2>", 10, "      <div class=", 34, "stats-grid", 34, ">", 10, "        <div class=", 34, "stat-card", 34, ">", 10, "          <div class=", 34, "stat-value", 34, ">2.3x</div>", 10, "          <div class=", 34, "stat-label", 34, ">Faster than C#</div>", 10, "        </div>", 10, "        <div class=", 34, "stat-card", 34, ">", 10, "          <div class=", 34, "stat-value", 34, ">1.8x</div>", 10, "          <div class=", 34, "stat-label", 34, ">Faster than Go</div>", 10, "        </div>", 10, "        <div class=", 34, "stat-card", 34, ">", 10, "          <div class=", 34, "stat-value", 34, ">0.95x</div>", 10, "          <div class=", 34, "stat-label", 34, ">vs C (99%)</div>", 10, "        </div>", 10, "        <div class=", 34, "stat-card", 34, ">", 10, "          <div class=", 34, "stat-value", 34, ">0.12ms</div>", 10, "          <div class=", 34, "stat-label", 34, ">Compile Time</div>", 10, "        </div>", 10, "      </div>", 10, "    </div>", 10, 10, "    <!-- Microbenchmarks -->", 10, "    <div class=", 34, "benchmark-section", 34, ">", 10, "      <h2 class=", 34, "section-title", 34, ">Microbenchmarks</h2>", 10, "      <div class=", 34, "benchmark-grid", 34, ">", 10, "        <div class=", 34, "benchmark-card", 34, ">", 10, "          <h3 class=", 34, "benchmark-title", 34, ">Fibonacci (n=40)</h3>", 10, "          <p class=", 34, "benchmark-description", 34, ">Recursive Fibonacci calculation benchmark</p>", 10, "          <table class=", 34, "performance-table", 34, ">", 10, "            <thead>", 10, "              <tr>", 10, "                <th>Language</th>", 10, "                <th>Time (ms)</th>", 10, "                <th>Relative</th>", 10, "              </tr>", 10, "            </thead>", 10, "            <tbody>", 10, "              <tr>", 10, "                <td>MethASM</td>", 10, "                <td class=", 34, "performance-value", 34, ">847</td>", 10, "                <td class=", 34, "performance-better", 34, ">1.00x</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td>C (gcc -O3)</td>", 10, "                <td>821</td>", 10, "                <td class=", 34, "performance-better", 34, ">0.97x</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td>Rust</td>", 10, "                <td>892</td>", 10, "                <td class=", 34, "performance-worse", 34, ">1.05x</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td>Go</td>", 10, "                <td>1,523</td>", 10, "                <td class=", 34, "performance-worse", 34, ">1.80x</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td>C#</td>", 10, "                <td>1,948</td>", 10, "                <td class=", 34, "performance-worse", 34, ">2.30x</td>", 10, "              </tr>", 10, "            </tbody>", 10, "          </table>", 10, "        </div>", 10, 10, "        <div class=", 34, "benchmark-card", 34, ">", 10, "          <h3 class=", 34, "benchmark-title", 34, ">Matrix Multiplication</h3>", 10, "          <p class=", 34, "benchmark-description", 34, ">100x100 matrix multiplication</p>", 10, "          <table class=", 34, "performance-table", 34, ">", 10, "            <thead>", 10, "              <tr>", 10, "                <th>Language</th>", 10, "                <th>Time (ms)</th>", 10, "                <th>Relative</th>", 10, "              </tr>", 10, "            </thead>", 10, "            <tbody>", 10, "              <tr>", 10, "                <td>MethASM</td>", 10, "                <td class=", 34, "performance-value", 34, ">12.3</td>", 10, "                <td class=", 34, "performance-better", 34, ">1.00x</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td>C (gcc -O3)</td>", 10, "                <td>11.8</td>", 10, "                <td class=", 34, "performance-better", 34, ">0.96x</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td>Rust</td>", 10, "                <td>13.1</td>", 10, "                <td class=", 34, "performance-worse", 34, ">1.07x</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td>Go</td>", 10, "                <td>18.7</td>", 10, "                <td class=", 34, "performance-worse", 34, ">1.52x</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td>C#</td>", 10, "                <td>22.4</td>", 10, "                <td class=", 34, "performance-worse", 34, ">1.82x</td>", 10, "              </tr>", 10, "            </tbody>", 10, "          </table>", 10, "        </div>", 10, 10, "        <div class=", 34, "benchmark-card", 34, ">", 10, "          <h3 class=", 34, "benchmark-title", 34, ">Binary Trees</h3>", 10, "          <p class=", 34, "benchmark-description", 34, ">Binary tree operations (10M nodes)</p>", 10, "          <table class=", 34, "performance-table", 34, ">", 10, "            <thead>", 10, "              <tr>", 10, "                <th>Language</th>", 10, "                <th>Time (ms)</th>", 10, "                <th>Relative</th>", 10, "              </tr>", 10, "            </thead>", 10, "            <tbody>", 10, "              <tr>", 10, "                <td>MethASM</td>", 10, "                <td class=", 34, "performance-value", 34, ">234</td>", 10, "                <td class=", 34, "performance-better", 34, ">1.00x</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td>C (gcc -O3)</td>", 10, "                <td>228</td>", 10, "                <td class=", 34, "performance-better", 34, ">0.97x</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td>Rust</td>", 10, "                <td>241</td>", 10, "                <td class=", 34, "performance-worse", 34, ">1.03x</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td>Go</td>", 10, "                <td>387</td>", 10, "                <td class=", 34, "performance-worse", 34, ">1.65x</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td>C#</td>", 10, "                <td>456</td>", 10, "                <td class=", 34, "performance-worse", 34, ">1.95x</td>", 10, "              </tr>", 10, "            </tbody>", 10, "          </table>", 10, "        </div>", 10, "      </div>", 10, "    </div>", 10, 10, "    <!-- Performance Chart -->", 10, "    <div class=", 34, "benchmark-section", 34, ">", 10, "      <h2 class=", 34, "section-title", 34, ">Relative Performance</h2>", 10, "      <div class=", 34, "chart-container", 34, ">", 10, "        <h3 class=", 34, "chart-title", 34, ">Performance Comparison (Lower is Better)</h3>", 10, "        <div class=", 34, "bar-chart", 34, ">", 10, "          <div class=", 34, "bar", 34, " style=", 34, "height: 95%", 34, ">", 10, "            <div class=", 34, "bar-value", 34, ">0.95x</div>", 10, "            <div class=", 34, "bar-label", 34, ">MethASM</div>", 10, "          </div>", 10, "          <div class=", 34, "bar", 34, " style=", 34, "height: 92%", 34, ">", 10, "            <div class=", 34, "bar-value", 34, ">0.92x</div>", 10, "            <div class=", 34, "bar-label", 34, ">C</div>", 10, "          </div>", 10, "          <div class=", 34, "bar", 34, " style=", 34, "height: 100%", 34, ">", 10, "            <div class=", 34, "bar-value", 34, ">1.00x</div>", 10, "            <div class=", 34, "bar-label", 34, ">Rust</div>", 10, "          </div>", 10, "          <div class=", 34, "bar", 34, " style=", 34, "height: 165%", 34, ">", 10, "            <div class=", 34, "bar-value", 34, ">1.65x</div>", 10, "            <div class=", 34, "bar-label", 34, ">Go</div>", 10, "          </div>", 10, "          <div class=", 34, "bar", 34, " style=", 34, "height: 195%", 34, ">", 10, "            <div class=", 34, "bar-value", 34, ">1.95x</div>", 10, "            <div class=", 34, "bar-label", 34, ">C#</div>", 10, "          </div>", 10, "        </div>", 10, "      </div>", 10, "    </div>", 10, 10, "    <!-- Compilation Performance -->", 10, "    <div class=", 34, "benchmark-section", 34, ">", 10, "      <h2 class=", 34, "section-title", 34, ">Compilation Performance</h2>", 10, "      <div class=", 34, "compilation-stats", 34, ">", 10, "        <div class=", 34, "stat-row", 34, ">", 10, "          <span class=", 34, "stat-name", 34, ">MethASM Compilation</span>", 10, "          <span class=", 34, "stat-time", 34, ">0.12s</span>", 10, "        </div>", 10, "        <div class=", 34, "stat-row", 34, ">", 10, "          <span class=", 34, "stat-name", 34, ">C (gcc -O3)</span>", 10, "          <span class=", 34, "stat-time", 34, ">0.08s</span>", 10, "        </div>", 10, "        <div class=", 34, "stat-row", 34, ">", 10, "          <span class=", 34, "stat-name", 34, ">Rust (release)</span>", 10, "          <span class=", 34, "stat-time", 34, ">2.34s</span>", 10, "        </div>", 10, "        <div class=", 34, "stat-row", 34, ">", 10, "          <span class=", 34, "stat-name", 34, ">Go</span>", 10, "          <span class=", 34, "stat-time", 34, ">0.45s</span>", 10, "        </div>", 10, "        <div class=", 34, "stat-row", 34, ">", 10, "          <span class=", 34, "stat-name", 34, ">C# (release)</span>", 10, "          <span class=", 34, "stat-time", 34, ">1.67s</span>", 10, "        </div>", 10, "      </div>", 10, "    </div>", 10, 10, "    <!-- Memory Usage -->", 10, "    <div class=", 34, "benchmark-section", 34, ">", 10, "      <h2 class=", 34, "section-title", 34, ">Memory Usage</h2>", 10, "      <div class=", 34, "benchmark-grid", 34, ">", 10, "        <div class=", 34, "benchmark-card", 34, ">", 10, "          <h3 class=", 34, "benchmark-title", 34, ">Binary Size (Hello World)</h3>", 10, "          <p class=", 34, "benchmark-description", 34, ">Stripped executable size</p>", 10, "          <table class=", 34, "performance-table", 34, ">", 10, "            <thead>", 10, "              <tr>", 10, "                <th>Language</th>", 10, "                <th>Size (KB)</th>", 10, "                <th>Relative</th>", 10, "              </tr>", 10, "            </thead>", 10, "            <tbody>", 10, "              <tr>", 10, "                <td>MethASM</td>", 10, "                <td class=", 34, "performance-value", 34, ">8.2</td>", 10, "                <td class=", 34, "performance-better", 34, ">1.00x</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td>C (gcc)</td>", 10, "                <td>7.8</td>", 10, "                <td class=", 34, "performance-better", 34, ">0.95x</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td>Rust</td>", 10, "                <td>342</td>", 10, "                <td class=", 34, "performance-worse", 34, ">41.7x</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td>Go</td>", 10, "                <td>1,247</td>", 10, "                <td class=", 34, "performance-worse", 34, ">152x</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td>C#</td>", 10, "                <td>68</td>", 10, "                <td class=", 34, "performance-worse", 34, ">8.3x</td>", 10, "              </tr>", 10, "            </tbody>", 10, "          </table>", 10, "        </div>", 10, 10, "        <div class=", 34, "benchmark-card", 34, ">", 10, "          <h3 class=", 34, "benchmark-title", 34, ">Runtime Memory</h3>", 10, "          <p class=", 34, "benchmark-description", 34, ">RSS memory usage (web server)</p>", 10, "          <table class=", 34, "performance-table", 34, ">", 10, "            <thead>", 10, "              <tr>", 10, "                <th>Language</th>", 10, "                <th>Memory (MB)</th>", 10, "                <th>Relative</th>", 10, "              </tr>", 10, "            </thead>", 10, "            <tbody>", 10, "              <tr>", 10, "                <td>MethASM</td>", 10, "                <td class=", 34, "performance-value", 34, ">2.1</td>", 10, "                <td class=", 34, "performance-better", 34, ">1.00x</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td>C (gcc)</td>", 10, "                <td>2.0</td>", 10, "                <td class=", 34, "performance-better", 34, ">0.95x</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td>Rust</td>", 10, "                <td>3.2</td>", 10, "                <td class=", 34, "performance-worse", 34, ">1.52x</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td>Go</td>", 10, "                <td>5.8</td>", 10, "                <td class=", 34, "performance-worse", 34, ">2.76x</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td>C#</td>", 10, "                <td>12.4</td>", 10, "                <td class=", 34, "performance-worse", 34, ">5.90x</td>", 10, "              </tr>", 10, "            </tbody>", 10, "          </table>", 10, "        </div>", 10, "      </div>", 10, "    </div>", 10, 10, "    <!-- Benchmark Methodology -->", 10, "    <div class=", 34, "benchmark-section", 34, ">", 10, "      <h2 class=", 34, "section-title", 34, ">Benchmark Methodology</h2>", 10, "      <div class=", 34, "benchmark-card", 34, ">", 10, "        <h3 class=", 34, "benchmark-title", 34, ">Test Environment</h3>", 10, "        <p class=", 34, "benchmark-description", 34, ">All benchmarks were run on the same hardware and operating system:</p>", 10, "        <ul style=", 34, "color: var(--text-muted); line-height: 1.8;", 34, ">", 10, "          <li>CPU: Intel Core i7-12700K (12 cores, 20 threads)</li>", 10, "          <li>RAM: 32GB DDR4-3200</li>", 10, "          <li>OS: Windows 11 64-bit</li>", 10, "          <li>Compiler: MethASM 1.0, GCC 11.2, Rust 1.65, Go 1.19, .NET 6.0</li>", 10, "          <li>All tests run 10 times, median value reported</li>", 10, "          <li>Optimization flags: MethASM -O, GCC -O3, Rust --release, Go default</li>", 10, "        </ul>", 10, "      </div>", 10, "    </div>", 10, "  </div>", 10, 10, "  <script>", 10, "    // Add interactive chart animations", 10, "    document.addEventListener('DOMContentLoaded', function() {", 10, "      const bars = document.querySelectorAll('.bar');", 10, "      bars.forEach((bar, index) => {", 10, "        bar.style.animation = `slideUp 0.5s ease-out ${index * 0.1}s backwards`;", 10, "      });", 10, "    });", 10, "  </script>", 10, "</body>", 10, "</html>", 10, 0


DOCS_CONTENT:
    dq Lstr3
    dq 57221
Lstr3:
    db "<!DOCTYPE html>", 10, "<html lang='en'>", 10, "<head>", 10, "<meta charset='UTF-8'>", 10, "<meta name='viewport' content='width=device-width,initial-scale=1'>", 10, "<meta name='description' content='Complete MethASM language documentation, tutorials, and guides.'>", 10, "<title>MethASM ", 226, 128, 148, " Documentation</title>", 10, "<link rel=", 34, "preconnect", 34, " href=", 34, "https://fonts.googleapis.com", 34, ">", 10, "<link rel=", 34, "preconnect", 34, " href=", 34, "https://fonts.gstatic.com", 34, " crossorigin>", 10, "<link href=", 34, "https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&family=JetBrains+Mono:wght@400;500;600&display=swap", 34, " rel=", 34, "stylesheet", 34, ">", 10, "<style>", 10, "  :root {", 10, "    --bg-color: #05050A;", 10, "    --text-main: #E2E8F0;", 10, "    --text-muted: #94A3B8;", 10, "    --accent-1: #6366F1;", 10, "    --accent-2: #8B5CF6;", 10, "    --accent-3: #EC4899;", 10, "    --accent-4: #10B981;", 10, "    --glass-bg: rgba(255, 255, 255, 0.03);", 10, "    --glass-border: rgba(255, 255, 255, 0.08);", 10, "    --code-bg: #0D1117;", 10, "    --success: #10B981;", 10, "    --warning: #F59E0B;", 10, "    --error: #EF4444;", 10, "  }", 10, 10, "  * { box-sizing: border-box; margin: 0; padding: 0; }", 10, "  ", 10, "  body {", 10, "    font-family: 'Inter', sans-serif;", 10, "    background-color: var(--bg-color);", 10, "    color: var(--text-main);", 10, "    line-height: 1.6;", 10, "    overflow-x: hidden;", 10, "  }", 10, 10, "  /* Animated background mesh */", 10, "  .bg-mesh {", 10, "    position: fixed;", 10, "    top: 0; left: 0; right: 0; bottom: 0;", 10, "    z-index: -1;", 10, "    background: ", 10, "      radial-gradient(circle at 15% 50%, rgba(99, 102, 241, 0.15) 0%, transparent 50%),", 10, "      radial-gradient(circle at 85% 30%, rgba(236, 72, 153, 0.15) 0%, transparent 50%),", 10, "      radial-gradient(circle at 50% 80%, rgba(139, 92, 246, 0.15) 0%, transparent 50%);", 10, "    filter: blur(60px);", 10, "    animation: pulse 15s ease-in-out infinite alternate;", 10, "  }", 10, 10, "  @keyframes pulse {", 10, "    0% { transform: scale(1); opacity: 0.8; }", 10, "    100% { transform: scale(1.1); opacity: 1; }", 10, "  }", 10, 10, "  .container {", 10, "    max-width: 1400px;", 10, "    margin: 0 auto;", 10, "    padding: 2rem;", 10, "    display: grid;", 10, "    grid-template-columns: 280px 1fr;", 10, "    gap: 2rem;", 10, "  }", 10, 10, "  /* Navigation */", 10, "  nav {", 10, "    grid-column: 1 / -1;", 10, "    display: flex;", 10, "    justify-content: space-between;", 10, "    align-items: center;", 10, "    padding: 1rem 0;", 10, "    margin-bottom: 3rem;", 10, "    border-bottom: 1px solid var(--glass-border);", 10, "  }", 10, 10, "  .nav-links {", 10, "    display: flex;", 10, "    gap: 2rem;", 10, "    align-items: center;", 10, "  }", 10, 10, "  .nav-link {", 10, "    color: var(--text-muted);", 10, "    text-decoration: none;", 10, "    font-weight: 500;", 10, "    transition: color 0.3s ease;", 10, "  }", 10, 10, "  .nav-link:hover {", 10, "    color: var(--accent-1);", 10, "  }", 10, 10, "  /* Sidebar */", 10, "  .sidebar {", 10, "    position: sticky;", 10, "    top: 2rem;", 10, "    height: fit-content;", 10, "    max-height: calc(100vh - 4rem);", 10, "    overflow-y: auto;", 10, "  }", 10, 10, "  .sidebar-section {", 10, "    margin-bottom: 2rem;", 10, "  }", 10, 10, "  .sidebar-title {", 10, "    font-size: 0.8rem;", 10, "    font-weight: 600;", 10, "    text-transform: uppercase;", 10, "    color: var(--text-muted);", 10, "    margin-bottom: 1rem;", 10, "    letter-spacing: 0.05em;", 10, "  }", 10, 10, "  .sidebar-links {", 10, "    list-style: none;", 10, "  }", 10, 10, "  .sidebar-link {", 10, "    display: block;", 10, "    padding: 0.5rem 1rem;", 10, "    color: var(--text-muted);", 10, "    text-decoration: none;", 10, "    border-radius: 8px;", 10, "    transition: all 0.3s ease;", 10, "    margin-bottom: 0.25rem;", 10, "  }", 10, 10, "  .sidebar-link:hover {", 10, "    background: var(--glass-bg);", 10, "    color: var(--text-main);", 10, "  }", 10, 10, "  .sidebar-link.active {", 10, "    background: rgba(99, 102, 241, 0.1);", 10, "    color: var(--accent-1);", 10, "    border-left: 3px solid var(--accent-1);", 10, "  }", 10, 10, "  /* Main Content */", 10, "  .main-content {", 10, "    animation: fadeUp 0.8s ease-out;", 10, "  }", 10, 10, "  .content-section {", 10, "    margin-bottom: 4rem;", 10, "    scroll-margin-top: 2rem;", 10, "  }", 10, 10, "  .section-title {", 10, "    font-size: 2.5rem;", 10, "    font-weight: 700;", 10, "    margin-bottom: 2rem;", 10, "    background: linear-gradient(135deg, var(--text-main) 0%, var(--accent-1) 100%);", 10, "    -webkit-background-clip: text;", 10, "    background-clip: text;", 10, "    -webkit-text-fill-color: transparent;", 10, "  }", 10, 10, "  .subsection-title {", 10, "    font-size: 1.8rem;", 10, "    font-weight: 600;", 10, "    margin-bottom: 1.5rem;", 10, "    color: white;", 10, "    margin-top: 3rem;", 10, "  }", 10, 10, "  .sub-subsection-title {", 10, "    font-size: 1.4rem;", 10, "    font-weight: 600;", 10, "    margin-bottom: 1rem;", 10, "    color: var(--text-main);", 10, "    margin-top: 2rem;", 10, "  }", 10, 10, "  /* Typography */", 10, "  p {", 10, "    color: var(--text-muted);", 10, "    margin-bottom: 1.5rem;", 10, "    line-height: 1.7;", 10, "  }", 10, 10, "  /* Code Blocks */", 10, "  .code-block {", 10, "    background: var(--code-bg);", 10, "    border: 1px solid #30363D;", 10, "    border-radius: 12px;", 10, "    overflow: hidden;", 10, "    margin: 2rem 0;", 10, "  }", 10, 10, "  .code-header {", 10, "    background: #161B22;", 10, "    padding: 0.75rem 1rem;", 10, "    border-bottom: 1px solid #30363D;", 10, "    display: flex;", 10, "    align-items: center;", 10, "    justify-content: space-between;", 10, "  }", 10, 10, "  .code-title {", 10, "    color: var(--text-main);", 10, "    font-weight: 500;", 10, "    font-size: 0.9rem;", 10, "  }", 10, 10, "  .code-actions {", 10, "    display: flex;", 10, "    gap: 0.5rem;", 10, "  }", 10, 10, "  .copy-btn {", 10, "    background: var(--glass-bg);", 10, "    border: 1px solid var(--glass-border);", 10, "    color: var(--text-muted);", 10, "    padding: 0.3rem 0.8rem;", 10, "    border-radius: 6px;", 10, "    font-size: 0.8rem;", 10, "    cursor: pointer;", 10, "    transition: all 0.3s ease;", 10, "  }", 10, 10, "  .copy-btn:hover {", 10, "    background: rgba(255, 255, 255, 0.08);", 10, "    color: var(--text-main);", 10, "  }", 10, 10, "  pre {", 10, "    padding: 1.5rem;", 10, "    overflow-x: auto;", 10, "    margin: 0;", 10, "  }", 10, 10, "  code {", 10, "    font-family: 'JetBrains Mono', monospace;", 10, "    font-size: 0.9rem;", 10, "    line-height: 1.6;", 10, "  }", 10, 10, "  /* Inline Code */", 10, "  p code, li code {", 10, "    background: rgba(99, 102, 241, 0.1);", 10, "    color: var(--accent-1);", 10, "    padding: 0.2rem 0.4rem;", 10, "    border-radius: 4px;", 10, "    font-size: 0.85rem;", 10, "  }", 10, 10, "  /* Syntax Highlighting */", 10, "  .k { color: #FF7B72; font-weight: 500; } /* Keyword */", 10, "  .f { color: #D2A8FF; font-weight: 500; } /* Function */", 10, "  .s { color: #A5D6FF; } /* String */", 10, "  .c { color: #8B949E; font-style: italic; } /* Comment */", 10, "  .t { color: #FFA657; } /* Type */", 10, "  .n { color: #79C0FF; } /* Number */", 10, "  .o { color: #FF7B72; } /* Operator */", 10, 10, "  /* Lists */", 10, "  ul, ol {", 10, "    color: var(--text-muted);", 10, "    margin-bottom: 1.5rem;", 10, "    padding-left: 2rem;", 10, "  }", 10, 10, "  li {", 10, "    margin-bottom: 0.5rem;", 10, "  }", 10, 10, "  /* Tables */", 10, "  .table-container {", 10, "    overflow-x: auto;", 10, "    margin: 2rem 0;", 10, "  }", 10, 10, "  table {", 10, "    width: 100%;", 10, "    border-collapse: collapse;", 10, "    background: var(--glass-bg);", 10, "    border: 1px solid var(--glass-border);", 10, "    border-radius: 12px;", 10, "    overflow: hidden;", 10, "  }", 10, 10, "  th, td {", 10, "    padding: 1rem;", 10, "    text-align: left;", 10, "    border-bottom: 1px solid var(--glass-border);", 10, "  }", 10, 10, "  th {", 10, "    background: rgba(99, 102, 241, 0.1);", 10, "    color: white;", 10, "    font-weight: 600;", 10, "  }", 10, 10, "  tr:last-child td {", 10, "    border-bottom: none;", 10, "  }", 10, 10, "  tr:hover {", 10, "    background: rgba(255, 255, 255, 0.02);", 10, "  }", 10, 10, "  /* Alert Boxes */", 10, "  .alert {", 10, "    padding: 1.5rem;", 10, "    border-radius: 12px;", 10, "    margin: 2rem 0;", 10, "    border-left: 4px solid;", 10, "  }", 10, 10, "  .alert-info {", 10, "    background: rgba(99, 102, 241, 0.1);", 10, "    border-color: var(--accent-1);", 10, "  }", 10, 10, "  .alert-warning {", 10, "    background: rgba(245, 158, 11, 0.1);", 10, "    border-color: var(--warning);", 10, "  }", 10, 10, "  .alert-success {", 10, "    background: rgba(16, 185, 129, 0.1);", 10, "    border-color: var(--success);", 10, "  }", 10, 10, "  .alert-error {", 10, "    background: rgba(239, 68, 68, 0.1);", 10, "    border-color: var(--error);", 10, "  }", 10, 10, "  .alert-title {", 10, "    font-weight: 600;", 10, "    margin-bottom: 0.5rem;", 10, "    color: var(--text-main);", 10, "  }", 10, 10, "  /* Quick Start Card */", 10, "  .quickstart-card {", 10, "    background: var(--glass-bg);", 10, "    border: 1px solid var(--glass-border);", 10, "    border-radius: 16px;", 10, "    padding: 2rem;", 10, "    margin-bottom: 2rem;", 10, "    backdrop-filter: blur(10px);", 10, "  }", 10, 10, "  .quickstart-title {", 10, "    font-size: 1.3rem;", 10, "    font-weight: 600;", 10, "    color: white;", 10, "    margin-bottom: 1.5rem;", 10, "  }", 10, 10, "  .step-list {", 10, "    counter-reset: step;", 10, "  }", 10, 10, "  .step-item {", 10, "    position: relative;", 10, "    padding-left: 3rem;", 10, "    margin-bottom: 2rem;", 10, "    counter-increment: step;", 10, "  }", 10, 10, "  .step-item::before {", 10, "    content: counter(step);", 10, "    position: absolute;", 10, "    left: 0;", 10, "    top: 0;", 10, "    width: 2rem;", 10, "    height: 2rem;", 10, "    background: var(--accent-1);", 10, "    color: white;", 10, "    border-radius: 50%;", 10, "    display: flex;", 10, "    align-items: center;", 10, "    justify-content: center;", 10, "    font-weight: 600;", 10, "  }", 10, 10, "  /* Animations */", 10, "  @keyframes fadeUp {", 10, "    from { opacity: 0; transform: translateY(20px); }", 10, "    to { opacity: 1; transform: translateY(0); }", 10, "  }", 10, 10, "  /* Responsive */", 10, "  @media (max-width: 1024px) {", 10, "    .container {", 10, "      grid-template-columns: 1fr;", 10, "    }", 10, 10, "    .sidebar {", 10, "      position: static;", 10, "      max-height: none;", 10, "      margin-bottom: 2rem;", 10, "    }", 10, 10, "    nav {", 10, "      flex-direction: column;", 10, "      gap: 1rem;", 10, "    }", 10, "  }", 10, 10, "  @media (max-width: 768px) {", 10, "    .nav-links {", 10, "      flex-direction: column;", 10, "      gap: 1rem;", 10, "    }", 10, 10, "    .section-title {", 10, "      font-size: 2rem;", 10, "    }", 10, 10, "    .subsection-title {", 10, "      font-size: 1.5rem;", 10, "    }", 10, "  }", 10, "</style>", 10, "</head>", 10, "<body>", 10, "  <div class=", 34, "bg-mesh", 34, "></div>", 10, "  ", 10, "  <div class=", 34, "container", 34, ">", 10, "    <nav>", 10, "      <div class=", 34, "nav-links", 34, ">", 10, "        <a href=", 34, "/", 34, " class=", 34, "nav-link", 34, ">Home</a>", 10, "        <a href=", 34, "/demo", 34, " class=", 34, "nav-link", 34, ">Demo</a>", 10, "        <a href=", 34, "/benchmarks", 34, " class=", 34, "nav-link", 34, ">Benchmarks</a>", 10, "        <a href=", 34, "/docs", 34, " class=", 34, "nav-link", 34, ">Docs</a>", 10, "        <a href=", 34, "/forum", 34, " class=", 34, "nav-link", 34, ">Forum</a>", 10, "      </div>", 10, "    </nav>", 10, 10, "    <!-- Sidebar Navigation -->", 10, "    <aside class=", 34, "sidebar", 34, ">", 10, "      <div class=", 34, "sidebar-section", 34, ">", 10, "        <div class=", 34, "sidebar-title", 34, ">Getting Started</div>", 10, "        <ul class=", 34, "sidebar-links", 34, ">", 10, "          <li><a href=", 34, "#quickstart", 34, " class=", 34, "sidebar-link", 34, ">Quick Start</a></li>", 10, "          <li><a href=", 34, "#installation", 34, " class=", 34, "sidebar-link", 34, ">Installation</a></li>", 10, "          <li><a href=", 34, "#hello-world", 34, " class=", 34, "sidebar-link", 34, ">Hello World</a></li>", 10, "        </ul>", 10, "      </div>", 10, 10, "      <div class=", 34, "sidebar-section", 34, ">", 10, "        <div class=", 34, "sidebar-title", 34, ">Language Guide</div>", 10, "        <ul class=", 34, "sidebar-links", 34, ">", 10, "          <li><a href=", 34, "#types", 34, " class=", 34, "sidebar-link", 34, ">Types</a></li>", 10, "          <li><a href=", 34, "#variables", 34, " class=", 34, "sidebar-link", 34, ">Variables</a></li>", 10, "          <li><a href=", 34, "#functions", 34, " class=", 34, "sidebar-link", 34, ">Functions</a></li>", 10, "          <li><a href=", 34, "#control-flow", 34, " class=", 34, "sidebar-link", 34, ">Control Flow</a></li>", 10, "          <li><a href=", 34, "#structs", 34, " class=", 34, "sidebar-link", 34, ">Structs</a></li>", 10, "          <li><a href=", 34, "#enums", 34, " class=", 34, "sidebar-link", 34, ">Enums</a></li>", 10, "        </ul>", 10, "      </div>", 10, 10, "      <div class=", 34, "sidebar-section", 34, ">", 10, "        <div class=", 34, "sidebar-title", 34, ">Advanced Topics</div>", 10, "        <ul class=", 34, "sidebar-links", 34, ">", 10, "          <li><a href=", 34, "#memory-management", 34, " class=", 34, "sidebar-link", 34, ">Memory Management</a></li>", 10, "          <li><a href=", 34, "#c-interop", 34, " class=", 34, "sidebar-link", 34, ">C Interop</a></li>", 10, "          <li><a href=", 34, "#modules", 34, " class=", 34, "sidebar-link", 34, ">Modules</a></li>", 10, "          <li><a href=", 34, "#compilation", 34, " class=", 34, "sidebar-link", 34, ">Compilation</a></li>", 10, "        </ul>", 10, "      </div>", 10, 10, "      <div class=", 34, "sidebar-section", 34, ">", 10, "        <div class=", 34, "sidebar-title", 34, ">Reference</div>", 10, "        <ul class=", 34, "sidebar-links", 34, ">", 10, "          <li><a href=", 34, "#standard-library", 34, " class=", 34, "sidebar-link", 34, ">Standard Library</a></li>", 10, "          <li><a href=", 34, "#compiler-options", 34, " class=", 34, "sidebar-link", 34, ">Compiler Options</a></li>", 10, "          <li><a href=", 34, "#troubleshooting", 34, " class=", 34, "sidebar-link", 34, ">Troubleshooting</a></li>", 10, "        </ul>", 10, "      </div>", 10, "    </aside>", 10, 10, "    <!-- Main Content -->", 10, "    <main class=", 34, "main-content", 34, ">", 10, "      <!-- Quick Start -->", 10, "      <section id=", 34, "quickstart", 34, " class=", 34, "content-section", 34, ">", 10, "        <h1 class=", 34, "section-title", 34, ">Quick Start</h1>", 10, "        ", 10, "        <div class=", 34, "quickstart-card", 34, ">", 10, "          <h2 class=", 34, "quickstart-title", 34, ">Get Started with MethASM in 5 Minutes</h2>", 10, "          <div class=", 34, "step-list", 34, ">", 10, "            <div class=", 34, "step-item", 34, ">", 10, "              <p><strong>Install the compiler:</strong> Download the latest MethASM release for your platform (Windows x64 currently supported).</p>", 10, "            </div>", 10, "            <div class=", 34, "step-item", 34, ">", 10, "              <p><strong>Write your first program:</strong> Create a file called <code>hello.masm</code> with the code shown below.</p>", 10, "            </div>", 10, "            <div class=", 34, "step-item", 34, ">", 10, "              <p><strong>Compile and run:</strong> Use the build pipeline to create an executable.</p>", 10, "            </div>", 10, "            <div class=", 34, "step-item", 34, ">", 10, "              <p><strong>Explore:</strong> Check out the demo page and documentation for more examples.</p>", 10, "            </div>", 10, "          </div>", 10, "        </div>", 10, "      </section>", 10, 10, "      <!-- Installation -->", 10, "      <section id=", 34, "installation", 34, " class=", 34, "content-section", 34, ">", 10, "        <h2 class=", 34, "subsection-title", 34, ">Installation</h2>", 10, "        ", 10, "        <p>MethASM currently supports Windows x64. Linux support is in development.</p>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Prerequisites</h3>", 10, "        <ul>", 10, "          <li>Windows 10/11 (x64)</li>", 10, "          <li>NASM assembler (2.15+ recommended)</li>", 10, "          <li>Microsoft Visual C++ Build Tools or Visual Studio 2019+</li>", 10, "          <li>Git (for cloning examples)</li>", 10, "        </ul>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Download</h3>", 10, "        <p>Download the latest release from the GitHub repository or build from source:</p>", 10, 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Build from source</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code>git clone https://github.com/your-repo/methasm.git", 10, "cd methasm", 10, "build.bat</code></pre>", 10, "        </div>", 10, "      </section>", 10, 10, "      <!-- Hello World -->", 10, "      <section id=", 34, "hello-world", 34, " class=", 34, "content-section", 34, ">", 10, "        <h2 class=", 34, "subsection-title", 34, ">Hello World</h2>", 10, "        ", 10, "        <p>Here's the classic ", 34, "Hello, World!", 34, " program in MethASM:</p>", 10, 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">hello.masm</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "k", 34, ">import</span> <span class=", 34, "s", 34, ">", 34, "std/io", 34, "</span>;", 10, 10, "<span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">main</span>() -> <span class=", 34, "t", 34, ">int32</span> {", 10, "  <span class=", 34, "f", 34, ">println</span>(<span class=", 34, "s", 34, ">", 34, "Hello, World!", 34, "</span>);", 10, "  <span class=", 34, "k", 34, ">return</span> <span class=", 34, "n", 34, ">0</span>;", 10, "}</code></pre>", 10, "        </div>", 10, 10, "        <p>To compile and run:</p>", 10, 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Compilation commands</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code>methasm hello.masm -o hello.s", 10, "nasm -f win64 hello.s -o hello.o", 10, "gcc hello.o -o hello.exe", 10, "hello.exe</code></pre>", 10, "        </div>", 10, "      </section>", 10, 10, "      <!-- Types -->", 10, "      <section id=", 34, "types", 34, " class=", 34, "content-section", 34, ">", 10, "        <h2 class=", 34, "subsection-title", 34, ">Types</h2>", 10, "        ", 10, "        <p>MethASM is statically typed with explicit type declarations. All variables and function parameters must have a declared type.</p>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Primitive Types</h3>", 10, "        <div class=", 34, "table-container", 34, ">", 10, "          <table>", 10, "            <thead>", 10, "              <tr>", 10, "                <th>Type</th>", 10, "                <th>Size</th>", 10, "                <th>Description</th>", 10, "              </tr>", 10, "            </thead>", 10, "            <tbody>", 10, "              <tr>", 10, "                <td><code>int8, int16, int32, int64</code></td>", 10, "                <td>1, 2, 4, 8 bytes</td>", 10, "                <td>Signed integers</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>uint8, uint16, uint32, uint64</code></td>", 10, "                <td>1, 2, 4, 8 bytes</td>", 10, "                <td>Unsigned integers</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>float32, float64</code></td>", 10, "                <td>4, 8 bytes</td>", 10, "                <td>IEEE 754 floating point</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>string</code></td>", 10, "                <td>16 bytes</td>", 10, "                <td>String struct (pointer + length)</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>cstring</code></td>", 10, "                <td>8 bytes</td>", 10, "                <td>C string pointer (null-terminated)</td>", 10, "              </tr>", 10, "            </tbody>", 10, "          </table>", 10, "        </div>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Type Examples</h3>", 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Type declarations</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "k", 34, ">var</span> age: <span class=", 34, "t", 34, ">int32</span> = <span class=", 34, "n", 34, ">25</span>;", 10, "<span class=", 34, "k", 34, ">var</span> temperature: <span class=", 34, "t", 34, ">float64</span> = <span class=", 34, "n", 34, ">98.6</span>;", 10, "<span class=", 34, "k", 34, ">var</span> name: <span class=", 34, "t", 34, ">string</span> = <span class=", 34, "s", 34, ">", 34, "MethASM", 34, "</span>;", 10, "<span class=", 34, "k", 34, ">var</span> count: <span class=", 34, "t", 34, ">uint64</span> = <span class=", 34, "n", 34, ">1000000</span>;</code></pre>", 10, "        </div>", 10, "      </section>", 10, 10, "      <!-- Variables -->", 10, "      <section id=", 34, "variables", 34, " class=", 34, "content-section", 34, ">", 10, "        <h2 class=", 34, "subsection-title", 34, ">Variables</h2>", 10, "        ", 10, "        <p>Variables in MethASM are declared with the <code>var</code> keyword and must be explicitly typed.</p>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Variable Declaration</h3>", 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Variable examples</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "c", 34, ">// Variable declaration with initialization</span>", 10, "<span class=", 34, "k", 34, ">var</span> x: <span class=", 34, "t", 34, ">int32</span> = <span class=", 34, "n", 34, ">42</span>;", 10, "<span class=", 34, "k", 34, ">var</span> pi: <span class=", 34, "t", 34, ">float64</span> = <span class=", 34, "n", 34, ">3.14159</span>;", 10, 10, "<span class=", 34, "c", 34, ">// Declaration without initialization (must be assigned before use)</span>", 10, "<span class=", 34, "k", 34, ">var</span> result: <span class=", 34, "t", 34, ">int32</span>;", 10, "result = <span class=", 34, "n", 34, ">100</span>;", 10, 10, "<span class=", 34, "c", 34, ">// Arrays</span>", 10, "<span class=", 34, "k", 34, ">var</span> numbers: <span class=", 34, "t", 34, ">int32</span>[<span class=", 34, "n", 34, ">10</span>];", 10, "<span class=", 34, "k", 34, ">var</span> buffer: <span class=", 34, "t", 34, ">uint8</span>[<span class=", 34, "n", 34, ">256</span>];</code></pre>", 10, "        </div>", 10, 10, "        <div class=", 34, "alert alert-warning", 34, ">", 10, "          <div class=", 34, "alert-title", 34, ">", 226, 154, 160, 239, 184, 143, " Initialization Required</div>", 10, "          <p>Local variables must be initialized before their first use. The compiler will error if you try to read an uninitialized variable.</p>", 10, "        </div>", 10, "      </section>", 10, 10, "      <!-- Functions -->", 10, "      <section id=", 34, "functions", 34, " class=", 34, "content-section", 34, ">", 10, "        <h2 class=", 34, "subsection-title", 34, ">Functions</h2>", 10, "        ", 10, "        <p>Functions are the primary building blocks in MethASM. They have explicit parameter types and return types.</p>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Function Definition</h3>", 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Function examples</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "c", 34, ">// Simple function with parameters and return value</span>", 10, "<span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">add</span>(a: <span class=", 34, "t", 34, ">int32</span>, b: <span class=", 34, "t", 34, ">int32</span>) -> <span class=", 34, "t", 34, ">int32</span> {", 10, "  <span class=", 34, "k", 34, ">return</span> a + b;", 10, "}", 10, 10, "<span class=", 34, "c", 34, ">// Recursive function</span>", 10, "<span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">factorial</span>(n: <span class=", 34, "t", 34, ">int32</span>) -> <span class=", 34, "t", 34, ">int32</span> {", 10, "  <span class=", 34, "k", 34, ">if</span> (n <= <span class=", 34, "n", 34, ">1</span>) {", 10, "    <span class=", 34, "k", 34, ">return</span> <span class=", 34, "n", 34, ">1</span>;", 10, "  }", 10, "  <span class=", 34, "k", 34, ">return</span> n * <span class=", 34, "f", 34, ">factorial</span>(n - <span class=", 34, "n", 34, ">1</span>);", 10, "}", 10, 10, "<span class=", 34, "c", 34, ">// Void function (no return value)</span>", 10, "<span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">print_message</span>(msg: <span class=", 34, "t", 34, ">string</span>) {", 10, "  <span class=", 34, "f", 34, ">println</span>(msg);", 10, "}</code></pre>", 10, "        </div>", 10, "      </section>", 10, 10, "      <!-- Control Flow -->", 10, "      <section id=", 34, "control-flow", 34, " class=", 34, "content-section", 34, ">", 10, "        <h2 class=", 34, "subsection-title", 34, ">Control Flow</h2>", 10, "        ", 10, "        <p>MethASM provides structured control flow statements including conditionals, loops, and switches.</p>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">If Statements</h3>", 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Conditional logic</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "k", 34, ">if</span> (x > <span class=", 34, "n", 34, ">0</span>) {", 10, "  <span class=", 34, "f", 34, ">println</span>(<span class=", 34, "s", 34, ">", 34, "Positive", 34, "</span>);", 10, "} <span class=", 34, "k", 34, ">else if</span> (x < <span class=", 34, "n", 34, ">0</span>) {", 10, "  <span class=", 34, "f", 34, ">println</span>(<span class=", 34, "s", 34, ">", 34, "Negative", 34, "</span>);", 10, "} <span class=", 34, "k", 34, ">else</span> {", 10, "  <span class=", 34, "f", 34, ">println</span>(<span class=", 34, "s", 34, ">", 34, "Zero", 34, "</span>);", 10, "}</code></pre>", 10, "        </div>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Loops</h3>", 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Loop examples</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "c", 34, ">// While loop</span>", 10, "<span class=", 34, "k", 34, ">var</span> i: <span class=", 34, "t", 34, ">int32</span> = <span class=", 34, "n", 34, ">0</span>;", 10, "<span class=", 34, "k", 34, ">while</span> (i < <span class=", 34, "n", 34, ">10</span>) {", 10, "  <span class=", 34, "f", 34, ">println_int</span>(i);", 10, "  i = i + <span class=", 34, "n", 34, ">1</span>;", 10, "}", 10, 10, "<span class=", 34, "c", 34, ">// For loop</span>", 10, "<span class=", 34, "k", 34, ">for</span> (<span class=", 34, "k", 34, ">var</span> j: <span class=", 34, "t", 34, ">int32</span> = <span class=", 34, "n", 34, ">0</span>; j < <span class=", 34, "n", 34, ">5</span>; j = j + <span class=", 34, "n", 34, ">1</span>) {", 10, "  <span class=", 34, "f", 34, ">println_int</span>(j * j);", 10, "}</code></pre>", 10, "        </div>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Switch Statements</h3>", 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Switch example</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "k", 34, ">switch</span> (grade) {", 10, "  <span class=", 34, "k", 34, ">case</span> <span class=", 34, "n", 34, ">90</span>:", 10, "    <span class=", 34, "f", 34, ">println</span>(<span class=", 34, "s", 34, ">", 34, "A", 34, "</span>);", 10, "    <span class=", 34, "k", 34, ">break</span>;", 10, "  <span class=", 34, "k", 34, ">case</span> <span class=", 34, "n", 34, ">80</span>:", 10, "    <span class=", 34, "f", 34, ">println</span>(<span class=", 34, "s", 34, ">", 34, "B", 34, "</span>);", 10, "    <span class=", 34, "k", 34, ">break</span>;", 10, "  <span class=", 34, "k", 34, ">case</span> <span class=", 34, "n", 34, ">70</span>:", 10, "    <span class=", 34, "f", 34, ">println</span>(<span class=", 34, "s", 34, ">", 34, "C", 34, "</span>);", 10, "    <span class=", 34, "k", 34, ">break</span>;", 10, "  <span class=", 34, "k", 34, ">default</span>:", 10, "    <span class=", 34, "f", 34, ">println</span>(<span class=", 34, "s", 34, ">", 34, "F", 34, "</span>);", 10, "}</code></pre>", 10, "        </div>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Defer and Errdefer</h3>", 10, "        <p><code>defer</code> schedules a statement to execute when the current scope exits, while <code>errdefer</code> schedules execution only when exiting with an error. Both follow <strong>LIFO (Last In, First Out)</strong> ordering.</p>", 10, 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Basic defer usage</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">open_file</span>() {", 10, "  <span class=", 34, "k", 34, ">var</span> file: <span class=", 34, "t", 34, ">File</span>* = <span class=", 34, "f", 34, ">fopen</span>(<span class=", 34, "s", 34, ">", 34, "data.txt", 34, "</span>, <span class=", 34, "s", 34, ">", 34, "r", 34, "</span>);", 10, "  <span class=", 34, "k", 34, ">defer</span> <span class=", 34, "f", 34, ">fclose</span>(file);  <span class=", 34, "c", 34, ">// Executes when function returns</span>", 10, "  ", 10, "  <span class=", 34, "c", 34, ">// ... file operations ...</span>", 10, "}</code></pre>", 10, "        </div>", 10, 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Error handling with errdefer</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">process_data</span>() {", 10, "  <span class=", 34, "k", 34, ">var</span> buffer: <span class=", 34, "t", 34, ">uint8</span>* = <span class=", 34, "f", 34, ">malloc</span>(<span class=", 34, "n", 34, ">1024</span>);", 10, "  <span class=", 34, "k", 34, ">errdefer</span> <span class=", 34, "f", 34, ">free</span>(buffer);  <span class=", 34, "c", 34, ">// Only runs if function returns with error</span>", 10, "  ", 10, "  <span class=", 34, "k", 34, ">var</span> file: <span class=", 34, "t", 34, ">File</span>* = <span class=", 34, "f", 34, ">fopen</span>(<span class=", 34, "s", 34, ">", 34, "data.txt", 34, "</span>, <span class=", 34, "s", 34, ">", 34, "r", 34, "</span>);", 10, "  <span class=", 34, "k", 34, ">defer</span> <span class=", 34, "f", 34, ">fclose</span>(file);     <span class=", 34, "c", 34, ">// Always runs</span>", 10, "  ", 10, "  <span class=", 34, "k", 34, ">if</span> (parse_error) {", 10, "    <span class=", 34, "k", 34, ">return</span> <span class=", 34, "f", 34, ">err</span>();  <span class=", 34, "c", 34, ">// Runs errdefer, then defer</span>", 10, "  }", 10, "  ", 10, "  <span class=", 34, "k", 34, ">return</span> <span class=", 34, "f", 34, ">ok</span>();     <span class=", 34, "c", 34, ">// Runs only defer</span>", 10, "}</code></pre>", 10, "        </div>", 10, 10, "        <div class=", 34, "alert alert-info", 34, ">", 10, "          <div class=", 34, "alert-title", 34, ">", 226, 132, 185, 239, 184, 143, " LIFO Ordering</div>", 10, "          <p>Deferred statements execute in reverse order of declaration. The most recently deferred statement executes first.</p>", 10, "        </div>", 10, 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Resource management pattern</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">handle_client</span>(client_socket: <span class=", 34, "t", 34, ">int32</span>) {", 10, "  <span class=", 34, "k", 34, ">defer</span> <span class=", 34, "f", 34, ">close_socket</span>(client_socket);", 10, "  ", 10, "  <span class=", 34, "k", 34, ">var</span> buffer: <span class=", 34, "t", 34, ">uint8</span>* = <span class=", 34, "f", 34, ">malloc</span>(<span class=", 34, "n", 34, ">1024</span>);", 10, "  <span class=", 34, "k", 34, ">errdefer</span> <span class=", 34, "f", 34, ">free</span>(buffer);", 10, "  ", 10, "  <span class=", 34, "k", 34, ">if</span> (read_error) {", 10, "    <span class=", 34, "k", 34, ">return</span> <span class=", 34, "f", 34, ">err</span>();  <span class=", 34, "c", 34, ">// Free buffer, close socket</span>", 10, "  }", 10, "  ", 10, "  <span class=", 34, "k", 34, ">return</span> <span class=", 34, "f", 34, ">ok</span>();     <span class=", 34, "c", 34, ">// Close socket only</span>", 10, "}</code></pre>", 10, "        </div>", 10, "      </section>", 10, 10, "      <!-- Structs -->", 10, "      <section id=", 34, "structs", 34, " class=", 34, "content-section", 34, ">", 10, "        <h2 class=", 34, "subsection-title", 34, ">Structs</h2>", 10, "        ", 10, "        <p>Structs group related data together and can have methods associated with them.</p>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Struct Definition</h3>", 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Struct with methods</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "k", 34, ">struct</span> <span class=", 34, "t", 34, ">Point</span> {", 10, "  x: <span class=", 34, "t", 34, ">float64</span>;", 10, "  y: <span class=", 34, "t", 34, ">float64</span>;", 10, "  ", 10, "  <span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">distance</span>(other: <span class=", 34, "t", 34, ">Point</span>*) -> <span class=", 34, "t", 34, ">float64</span> {", 10, "    <span class=", 34, "k", 34, ">var</span> dx: <span class=", 34, "t", 34, ">float64</span> = <span class=", 34, "k", 34, ">this</span>.x - other->x;", 10, "    <span class=", 34, "k", 34, ">var</span> dy: <span class=", 34, "t", 34, ">float64</span> = <span class=", 34, "k", 34, ">this</span>.y - other->y;", 10, "    <span class=", 34, "k", 34, ">return</span> dx*dx + dy*dy; <span class=", 34, "c", 34, ">// Simplified distance</span>", 10, "  }", 10, "  ", 10, "  <span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">move</span>(dx: <span class=", 34, "t", 34, ">float64</span>, dy: <span class=", 34, "t", 34, ">float64</span>) {", 10, "    <span class=", 34, "k", 34, ">this</span>.x = <span class=", 34, "k", 34, ">this</span>.x + dx;", 10, "    <span class=", 34, "k", 34, ">this</span>.y = <span class=", 34, "k", 34, ">this</span>.y + dy;", 10, "  }", 10, "}", 10, 10, "<span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">main</span>() -> <span class=", 34, "t", 34, ">int32</span> {", 10, "  <span class=", 34, "k", 34, ">var</span> p1: <span class=", 34, "t", 34, ">Point</span> = {<span class=", 34, "n", 34, ">0.0</span>, <span class=", 34, "n", 34, ">0.0</span>};", 10, "  <span class=", 34, "k", 34, ">var</span> p2: <span class=", 34, "t", 34, ">Point</span> = {<span class=", 34, "n", 34, ">3.0</span>, <span class=", 34, "n", 34, ">4.0</span>};", 10, "  ", 10, "  <span class=", 34, "k", 34, ">var</span> dist: <span class=", 34, "t", 34, ">float64</span> = <span class=", 34, "f", 34, ">p1.distance</span>(&p2);", 10, "  <span class=", 34, "f", 34, ">p1.move</span>(<span class=", 34, "n", 34, ">1.0</span>, <span class=", 34, "n", 34, ">2.0</span>);", 10, "  ", 10, "  <span class=", 34, "k", 34, ">return</span> <span class=", 34, "n", 34, ">0</span>;", 10, "}</code></pre>", 10, "        </div>", 10, "      </section>", 10, 10, "      <!-- Enums -->", 10, "      <section id=", 34, "enums", 34, " class=", 34, "content-section", 34, ">", 10, "        <h2 class=", 34, "subsection-title", 34, ">Enums</h2>", 10, "        ", 10, "        <p>Enums define a named type with a set of variants, each having an integer value.</p>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Enum Definition</h3>", 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Enum examples</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "k", 34, ">enum</span> <span class=", 34, "t", 34, ">Direction</span> {", 10, "  North,        <span class=", 34, "c", 34, ">// 0</span>", 10, "  East = <span class=", 34, "n", 34, ">2</span>,     <span class=", 34, "c", 34, ">// 2</span>", 10, "  South,        <span class=", 34, "c", 34, ">// 3 (previous + 1)</span>", 10, "  West = <span class=", 34, "n", 34, ">-5</span>     <span class=", 34, "c", 34, ">// -5</span>", 10, "}", 10, 10, "<span class=", 34, "k", 34, ">enum</span> <span class=", 34, "t", 34, ">Status</span> {", 10, "  Pending = <span class=", 34, "n", 34, ">100</span>,", 10, "  InProgress = <span class=", 34, "n", 34, ">200</span>,", 10, "  Complete = <span class=", 34, "n", 34, ">300</span>,", 10, "  Failed = <span class=", 34, "n", 34, ">400</span>", 10, "}", 10, 10, "<span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">main</span>() -> <span class=", 34, "t", 34, ">int32</span> {", 10, "  <span class=", 34, "k", 34, ">var</span> dir: <span class=", 34, "t", 34, ">Direction</span> = North;", 10, "  <span class=", 34, "k", 34, ">var</span> status: <span class=", 34, "t", 34, ">Status</span> = InProgress;", 10, "  ", 10, "  <span class=", 34, "k", 34, ">if</span> (dir == East) {", 10, "    <span class=", 34, "f", 34, ">println</span>(<span class=", 34, "s", 34, ">", 34, "Facing East", 34, "</span>);", 10, "  }", 10, "  ", 10, "  <span class=", 34, "k", 34, ">switch</span> (status) {", 10, "    <span class=", 34, "k", 34, ">case</span> Complete:", 10, "      <span class=", 34, "f", 34, ">println</span>(<span class=", 34, "s", 34, ">", 34, "Task completed!", 34, "</span>);", 10, "      <span class=", 34, "k", 34, ">break</span>;", 10, "    <span class=", 34, "k", 34, ">case</span> Failed:", 10, "      <span class=", 34, "f", 34, ">println</span>(<span class=", 34, "s", 34, ">", 34, "Task failed!", 34, "</span>);", 10, "      <span class=", 34, "k", 34, ">break</span>;", 10, "  }", 10, "  ", 10, "  <span class=", 34, "k", 34, ">return</span> <span class=", 34, "n", 34, ">0</span>;", 10, "}</code></pre>", 10, "        </div>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Enum Properties</h3>", 10, "        <div class=", 34, "alert alert-info", 34, ">", 10, "          <div class=", 34, "alert-title", 34, ">", 226, 132, 185, 239, 184, 143, " Enum Characteristics</div>", 10, "          <ul>", 10, "            <li><strong>Underlying Type:</strong> Enums use <code>int64</code> as the underlying representation (8 bytes, aligned to 8)</li>", 10, "            <li><strong>Auto-increment:</strong> Variants without explicit values continue from previous variant (0 if first)</li>", 10, "            <li><strong>Direct Usage:</strong> Variant names are in scope after enum definition (use <code>North</code>, not <code>Direction.North</code>)</li>", 10, "            <li><strong>Comparisons:</strong> Enums can be compared with integers and used in <code>switch</code> cases</li>", 10, "          </ul>", 10, "        </div>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Type Conversion</h3>", 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Integer to enum conversion</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "k", 34, ">enum</span> <span class=", 34, "t", 34, ">Priority</span> {", 10, "  Low = <span class=", 34, "n", 34, ">1</span>,", 10, "  Medium = <span class=", 34, "n", 34, ">5</span>,", 10, "  High = <span class=", 34, "n", 34, ">10</span>", 10, "}", 10, 10, "<span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">set_priority</span>(level: <span class=", 34, "t", 34, ">int32</span>) -> <span class=", 34, "t", 34, ">Priority</span> {", 10, "  <span class=", 34, "c", 34, ">// Implicit narrowing conversion</span>", 10, "  <span class=", 34, "k", 34, ">var</span> priority: <span class=", 34, "t", 34, ">Priority</span> = level;", 10, "  ", 10, "  <span class=", 34, "k", 34, ">if</span> (priority >= High) {", 10, "    <span class=", 34, "f", 34, ">println</span>(<span class=", 34, "s", 34, ">", 34, "High priority set", 34, "</span>);", 10, "  }", 10, "  ", 10, "  <span class=", 34, "k", 34, ">return</span> priority;", 10, "}</code></pre>", 10, "        </div>", 10, "      </section>", 10, 10, "      <!-- Modules -->", 10, "      <section id=", 34, "modules", 34, " class=", 34, "content-section", 34, ">", 10, "        <h2 class=", 34, "subsection-title", 34, ">Modules</h2>", 10, "        ", 10, "        <p>MethASM provides a module system for organizing code and managing dependencies. Modules allow you to split large programs into manageable, reusable components.</p>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Import System</h3>", 10, "        <p>Use the <code>import</code> statement to include modules in your program.</p>", 10, 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Import examples</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "c", 34, ">// Import standard library modules</span>", 10, "<span class=", 34, "k", 34, ">import</span> <span class=", 34, "s", 34, ">", 34, "std/io", 34, "</span>;", 10, "<span class=", 34, "k", 34, ">import</span> <span class=", 34, "s", 34, ">", 34, "std/mem", 34, "</span>;", 10, "<span class=", 34, "k", 34, ">import</span> <span class=", 34, "s", 34, ">", 34, "std/math", 34, "</span>;", 10, 10, "<span class=", 34, "c", 34, ">// Import custom modules (relative paths)</span>", 10, "<span class=", 34, "k", 34, ">import</span> <span class=", 34, "s", 34, ">", 34, "utils/string_helpers", 34, "</span>;", 10, "<span class=", 34, "k", 34, ">import</span> <span class=", 34, "s", 34, ">", 34, "datastructures/list", 34, "</span>;", 10, "<span class=", 34, "k", 34, ">import</span> <span class=", 34, "s", 34, ">", 34, "algorithms/sort", 34, "</span>;", 10, 10, "<span class=", 34, "c", 34, ">// Import with absolute path (less common)</span>", 10, "<span class=", 34, "k", 34, ">import</span> <span class=", 34, "s", 34, ">", 34, "/usr/local/lib/custom", 34, "</span>;</code></pre>", 10, "        </div>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Module Resolution</h3>", 10, "        <div class=", 34, "alert alert-info", 34, ">", 10, "          <div class=", 34, "alert-title", 34, ">", 226, 132, 185, 239, 184, 143, " Import Search Order</div>", 10, "          <p>The compiler resolves imports in this order:</p>", 10, "          <ol>", 10, "            <li><strong>Relative to current file:</strong> <code>", 34, "utils/helpers", 34, "</code> looks for <code>utils/helpers.masm</code></li>", 10, "            <li><strong>Include directories:</strong> Paths added with <code>-I</code> flag</li>", 10, "            <li><strong>Standard library:</strong> <code>std/</code> prefix searches in stdlib root</li>", 10, "          </ol>", 10, "        </div>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Creating Modules</h3>", 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Creating a custom module</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "c", 34, ">// File: utils/string_helpers.masm</span>", 10, 10, "<span class=", 34, "c", 34, ">// Export functions for use by other modules</span>", 10, "<span class=", 34, "k", 34, ">export function</span> <span class=", 34, "f", 34, ">string_length</span>(s: <span class=", 34, "t", 34, ">string</span>) -> <span class=", 34, "t", 34, ">int32</span> {", 10, "  <span class=", 34, "k", 34, ">return</span> s.length;", 10, "}", 10, 10, "<span class=", 34, "k", 34, ">export function</span> <span class=", 34, "f", 34, ">string_equals</span>(a: <span class=", 34, "t", 34, ">string</span>, b: <span class=", 34, "t", 34, ">string</span>) -> <span class=", 34, "t", 34, ">int32</span> {", 10, "  <span class=", 34, "k", 34, ">if</span> (a.length != b.length) {", 10, "    <span class=", 34, "k", 34, ">return</span> <span class=", 34, "n", 34, ">0</span>; <span class=", 34, "c", 34, ">// false</span>", 10, "  }", 10, "  ", 10, "  <span class=", 34, "k", 34, ">var</span> i: <span class=", 34, "t", 34, ">int32</span> = <span class=", 34, "n", 34, ">0</span>;", 10, "  <span class=", 34, "k", 34, ">while</span> (i < a.length) {", 10, "    <span class=", 34, "k", 34, ">if</span> (a.chars[i] != b.chars[i]) {", 10, "      <span class=", 34, "k", 34, ">return</span> <span class=", 34, "n", 34, ">0</span>; <span class=", 34, "c", 34, ">// false</span>", 10, "    }", 10, "    i = i + <span class=", 34, "n", 34, ">1</span>;", 10, "  }", 10, "  ", 10, "  <span class=", 34, "k", 34, ">return</span> <span class=", 34, "n", 34, ">1</span>; <span class=", 34, "c", 34, ">// true</span>", 10, "}", 10, 10, "<span class=", 34, "c", 34, ">// Export types for use by other modules</span>", 10, "<span class=", 34, "k", 34, ">export</span> <span class=", 34, "k", 34, ">struct</span> <span class=", 34, "t", 34, ">StringBuilder</span> {", 10, "  buffer: <span class=", 34, "t", 34, ">uint8</span>[<span class=", 34, "n", 34, ">1024</span>];", 10, "  length: <span class=", 34, "t", 34, ">int32</span>;", 10, "  ", 10, "  <span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">append</span>(text: <span class=", 34, "t", 34, ">string</span>) {", 10, "    <span class=", 34, "c", 34, ">// Implementation...</span>", 10, "  }", 10, "}</code></pre>", 10, "        </div>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Using Custom Modules</h3>", 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Using custom modules</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "c", 34, ">// File: main.masm</span>", 10, 10, "<span class=", 34, "k", 34, ">import</span> <span class=", 34, "s", 34, ">", 34, "utils/string_helpers", 34, "</span>;", 10, 10, "<span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">main</span>() -> <span class=", 34, "t", 34, ">int32</span> {", 10, "  <span class=", 34, "k", 34, ">var</span> message: <span class=", 34, "t", 34, ">string</span> = <span class=", 34, "s", 34, ">", 34, "Hello, MethASM!", 34, "</span>;", 10, "  <span class=", 34, "k", 34, ">var</span> length: <span class=", 34, "t", 34, ">int32</span> = <span class=", 34, "f", 34, ">string_length</span>(message);", 10, "  ", 10, "  <span class=", 34, "f", 34, ">println</span>(<span class=", 34, "s", 34, ">", 34, "Message length: ", 34, "</span>);", 10, "  <span class=", 34, "f", 34, ">println_int</span>(length);", 10, "  ", 10, "  <span class=", 34, "k", 34, ">var</span> builder: <span class=", 34, "t", 34, ">StringBuilder</span>;", 10, "  builder.length = <span class=", 34, "n", 34, ">0</span>;", 10, "  <span class=", 34, "f", 34, ">builder.append</span>(message);", 10, "  ", 10, "  <span class=", 34, "k", 34, ">return</span> <span class=", 34, "n", 34, ">0</span>;", 10, "}</code></pre>", 10, "        </div>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Module Best Practices</h3>", 10, "        <div class=", 34, "alert alert-warning", 34, ">", 10, "          <div class=", 34, "alert-title", 34, ">", 226, 154, 160, 239, 184, 143, " Module Guidelines</div>", 10, "          <ul>", 10, "            <li><strong>Single Responsibility:</strong> Each module should have a clear, focused purpose</li>", 10, "            <li><strong>Explicit Exports:</strong> Use <code>export</code> keyword for functions/types meant to be used externally</li>", 10, "            <li><strong>Avoid Circular Dependencies:</strong> Module A shouldn't import module B if B also imports A</li>", 10, "            <li><strong>Consistent Naming:</strong> Use descriptive names and consider prefixing with module category</li>", 10, "            <li><strong>Documentation:</strong> Comment module purpose and exported function behavior</li>", 10, "          </ul>", 10, "        </div>", 10, "      </section>", 10, 10, "      <!-- Memory Management -->", 10, "      <section id=", 34, "memory-management", 34, " class=", 34, "content-section", 34, ">", 10, "        <h2 class=", 34, "subsection-title", 34, ">Memory Management</h2>", 10, "        ", 10, "        <p>MethASM offers both manual memory management and optional garbage collection.</p>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Stack Allocation</h3>", 10, "        <p>Most variables are allocated on the stack and automatically cleaned up when they go out of scope.</p>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Manual Memory Management</h3>", 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Using C malloc/free</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "k", 34, ">import</span> <span class=", 34, "s", 34, ">", 34, "std/mem", 34, "</span>;", 10, 10, "<span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">main</span>() -> <span class=", 34, "t", 34, ">int32</span> {", 10, "  <span class=", 34, "c", 34, ">// Allocate memory</span>", 10, "  <span class=", 34, "k", 34, ">var</span> buffer: <span class=", 34, "t", 34, ">cstring</span> = <span class=", 34, "f", 34, ">malloc</span>(<span class=", 34, "n", 34, ">1024</span>);", 10, "  ", 10, "  <span class=", 34, "k", 34, ">if</span> (buffer != <span class=", 34, "n", 34, ">0</span>) {", 10, "    <span class=", 34, "c", 34, ">// Use the memory</span>", 10, "    <span class=", 34, "f", 34, ">memset</span>(buffer, <span class=", 34, "n", 34, ">0</span>, <span class=", 34, "n", 34, ">1024</span>);", 10, "    ", 10, "    <span class=", 34, "c", 34, ">// Free when done</span>", 10, "    <span class=", 34, "f", 34, ">free</span>(buffer);", 10, "  }", 10, "  ", 10, "  <span class=", 34, "k", 34, ">return</span> <span class=", 34, "n", 34, ">0</span>;", 10, "}</code></pre>", 10, "        </div>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Garbage Collection</h3>", 10, "        <div class=", 34, "alert alert-info", 34, ">", 10, "          <div class=", 34, "alert-title", 34, ">", 226, 132, 185, 239, 184, 143, " GC Optional</div>", 10, "          <p>When you link the GC runtime, you can use <code>new T</code> for automatic memory management. The GC uses conservative mark-and-sweep collection.</p>", 10, "        </div>", 10, 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Using garbage collection</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "k", 34, ">struct</span> <span class=", 34, "t", 34, ">Node</span> {", 10, "  value: <span class=", 34, "t", 34, ">int32</span>;", 10, "  next: <span class=", 34, "t", 34, ">Node</span>*;", 10, "}", 10, 10, "<span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">main</span>() -> <span class=", 34, "t", 34, ">int32</span> {", 10, "  <span class=", 34, "c", 34, ">// Allocate with garbage collection</span>", 10, "  <span class=", 34, "k", 34, ">var</span> node: <span class=", 34, "t", 34, ">Node</span>* = <span class=", 34, "k", 34, ">new</span> <span class=", 34, "t", 34, ">Node</span>;", 10, "  node->value = <span class=", 34, "n", 34, ">42</span>;", 10, "  node->next = <span class=", 34, "k", 34, ">new</span> <span class=", 34, "t", 34, ">Node</span>;", 10, "  ", 10, "  <span class=", 34, "c", 34, ">// No need to free - GC will clean up</span>", 10, "  <span class=", 34, "k", 34, ">return</span> <span class=", 34, "n", 34, ">0</span>;", 10, "}</code></pre>", 10, "        </div>", 10, "      </section>", 10, 10, "      <!-- C Interop -->", 10, "      <section id=", 34, "c-interop", 34, " class=", 34, "content-section", 34, ">", 10, "        <h2 class=", 34, "subsection-title", 34, ">C Interoperability</h2>", 10, "        ", 10, "        <p>MethASM provides seamless C interoperability through extern declarations.</p>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Declaring External Functions</h3>", 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">C function declarations</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "c", 34, ">// Declare C functions</span>", 10, "<span class=", 34, "k", 34, ">extern function</span> <span class=", 34, "f", 34, ">strlen</span>(s: <span class=", 34, "t", 34, ">cstring</span>) -> <span class=", 34, "t", 34, ">int32</span> = <span class=", 34, "s", 34, ">", 34, "strlen", 34, "</span>;", 10, "<span class=", 34, "k", 34, ">extern function</span> <span class=", 34, "f", 34, ">strcpy</span>(dest: <span class=", 34, "t", 34, ">cstring</span>, src: <span class=", 34, "t", 34, ">cstring</span>) -> <span class=", 34, "t", 34, ">cstring</span> = <span class=", 34, "s", 34, ">", 34, "strcpy", 34, "</span>;", 10, "<span class=", 34, "k", 34, ">extern function</span> <span class=", 34, "f", 34, ">malloc</span>(size: <span class=", 34, "t", 34, ">int32</span>) -> <span class=", 34, "t", 34, ">cstring</span> = <span class=", 34, "s", 34, ">", 34, "malloc", 34, "</span>;", 10, 10, "<span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">main</span>() -> <span class=", 34, "t", 34, ">int32</span> {", 10, "  <span class=", 34, "k", 34, ">var</span> message: <span class=", 34, "t", 34, ">uint8</span>[<span class=", 34, "n", 34, ">256</span>];", 10, "  <span class=", 34, "f", 34, ">strcpy</span>(&message[<span class=", 34, "n", 34, ">0</span>], <span class=", 34, "s", 34, ">", 34, "Hello from C!", 34, "</span>);", 10, "  ", 10, "  <span class=", 34, "k", 34, ">var</span> len: <span class=", 34, "t", 34, ">int32</span> = <span class=", 34, "f", 34, ">strlen</span>(&message[<span class=", 34, "n", 34, ">0</span>]);", 10, "  <span class=", 34, "f", 34, ">println_int</span>(len);", 10, "  ", 10, "  <span class=", 34, "k", 34, ">return</span> <span class=", 34, "n", 34, ">0</span>;", 10, "}</code></pre>", 10, "        </div>", 10, "      </section>", 10, 10, "      <!-- Compilation -->", 10, "      <section id=", 34, "compilation", 34, " class=", 34, "content-section", 34, ">", 10, "        <h2 class=", 34, "subsection-title", 34, ">Compilation</h2>", 10, "        ", 10, "        <p>MethASM compiles to x86-64 NASM assembly, which is then assembled and linked to create executables.</p>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Build Pipeline</h3>", 10, "        <div class=", 34, "step-list", 34, ">", 10, "          <div class=", 34, "step-item", 34, ">", 10, "            <p><strong>Compile to assembly:</strong> <code>methasm input.masm -o output.s</code></p>", 10, "          </div>", 10, "          <div class=", 34, "step-item", 34, ">", 10, "            <p><strong>Assemble to object:</strong> <code>nasm -f win64 output.s -o output.o</code></p>", 10, "          </div>", 10, "          <div class=", 34, "step-item", 34, ">", 10, "            <p><strong>Link to executable:</strong> <code>gcc output.o -o output.exe</code></p>", 10, "          </div>", 10, "        </div>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Compiler Options</h3>", 10, "        <div class=", 34, "table-container", 34, ">", 10, "          <table>", 10, "            <thead>", 10, "              <tr>", 10, "                <th>Option</th>", 10, "                <th>Description</th>", 10, "              </tr>", 10, "            </thead>", 10, "            <tbody>", 10, "              <tr>", 10, "                <td><code>-o file</code></td>", 10, "                <td>Output assembly file</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>-O</code></td>", 10, "                <td>Enable optimizations</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>-d</code></td>", 10, "                <td>Debug mode</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>-g</code></td>", 10, "                <td>Generate debug symbols</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>-I dir</code></td>", 10, "                <td>Add import search directory</td>", 10, "              </tr>", 10, "            </tbody>", 10, "          </table>", 10, "        </div>", 10, "      </section>", 10, 10, "      <!-- Standard Library -->", 10, "      <section id=", 34, "standard-library", 34, " class=", 34, "content-section", 34, ">", 10, "        <h2 class=", 34, "subsection-title", 34, ">Standard Library</h2>", 10, "        ", 10, "        <p>MethASM includes a comprehensive standard library organized into modules.</p>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Core Modules</h3>", 10, "        <div class=", 34, "table-container", 34, ">", 10, "          <table>", 10, "            <thead>", 10, "              <tr>", 10, "                <th>Module</th>", 10, "                <th>Description</th>", 10, "                <th>Key Functions</th>", 10, "              </tr>", 10, "            </thead>", 10, "            <tbody>", 10, "              <tr>", 10, "                <td><code>std/io</code></td>", 10, "                <td>Console and file I/O operations</td>", 10, "                <td><code>println</code>, <code>fopen</code>, <code>fread</code></td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>std/mem</code></td>", 10, "                <td>Memory management functions</td>", 10, "                <td><code>malloc</code>, <code>free</code>, <code>memcpy</code></td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>std/math</code></td>", 10, "                <td>Mathematical operations</td>", 10, "                <td><code>abs</code>, <code>min</code>, <code>max</code></td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>std/net</code></td>", 10, "                <td>Network programming (Windows)</td>", 10, "                <td><code>socket</code>, <code>bind</code>, <code>listen</code></td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>std/conv</code></td>", 10, "                <td>Type conversions and utilities</td>", 10, "                <td><code>atoi</code>, <code>strlen</code>, <code>isdigit</code></td>", 10, "              </tr>", 10, "            </tbody>", 10, "          </table>", 10, "        </div>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Using the Standard Library</h3>", 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Standard library imports</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "k", 34, ">import</span> <span class=", 34, "s", 34, ">", 34, "std/io", 34, "</span>;", 10, "<span class=", 34, "k", 34, ">import</span> <span class=", 34, "s", 34, ">", 34, "std/mem", 34, "</span>;", 10, "<span class=", 34, "k", 34, ">import</span> <span class=", 34, "s", 34, ">", 34, "std/math", 34, "</span>;", 10, 10, "<span class=", 34, "k", 34, ">function</span> <span class=", 34, "f", 34, ">main</span>() -> <span class=", 34, "t", 34, ">int32</span> {", 10, "  <span class=", 34, "k", 34, ">var</span> numbers: <span class=", 34, "t", 34, ">int32</span>[<span class=", 34, "n", 34, ">5</span>] = {<span class=", 34, "n", 34, ">5</span>, <span class=", 34, "n", 34, ">2</span>, <span class=", 34, "n", 34, ">8</span>, <span class=", 34, "n", 34, ">1</span>, <span class=", 34, "n", 34, ">9</span>};", 10, "  <span class=", 34, "k", 34, ">var</span> max_val: <span class=", 34, "t", 34, ">int32</span> = <span class=", 34, "n", 34, ">0</span>;", 10, "  ", 10, "  <span class=", 34, "k", 34, ">for</span> (<span class=", 34, "k", 34, ">var</span> i: <span class=", 34, "t", 34, ">int32</span> = <span class=", 34, "n", 34, ">0</span>; i < <span class=", 34, "n", 34, ">5</span>; i = i + <span class=", 34, "n", 34, ">1</span>) {", 10, "    max_val = <span class=", 34, "f", 34, ">max</span>(max_val, numbers[i]);", 10, "  }", 10, "  ", 10, "  <span class=", 34, "f", 34, ">println_int</span>(max_val);", 10, "  <span class=", 34, "k", 34, ">return</span> <span class=", 34, "n", 34, ">0</span>;", 10, "}</code></pre>", 10, "        </div>", 10, "      </section>", 10, 10, "      <!-- Compiler Options -->", 10, "      <section id=", 34, "compiler-options", 34, " class=", 34, "content-section", 34, ">", 10, "        <h2 class=", 34, "subsection-title", 34, ">Compiler Options</h2>", 10, "        ", 10, "        <p>The MethASM compiler provides various options for controlling compilation behavior and output.</p>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Command Line Options</h3>", 10, "        <div class=", 34, "table-container", 34, ">", 10, "          <table>", 10, "            <thead>", 10, "              <tr>", 10, "                <th>Option</th>", 10, "                <th>Long Form</th>", 10, "                <th>Description</th>", 10, "              </tr>", 10, "            </thead>", 10, "            <tbody>", 10, "              <tr>", 10, "                <td><code>-o file</code></td>", 10, "                <td><code>--output</code></td>", 10, "                <td>Specify output assembly file (default: output.s)</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>-i file</code></td>", 10, "                <td><code>--input</code></td>", 10, "                <td>Specify input file (alternative to positional argument)</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>-I dir</code></td>", 10, "                <td><code>--include</code></td>", 10, "                <td>Add directory to import search path (repeatable)</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>--stdlib dir</code></td>", 10, "                <td><code>--stdlib</code></td>", 10, "                <td>Set standard library root (default: stdlib)</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>--prelude</code></td>", 10, "                <td><code>--prelude</code></td>", 10, "                <td>Auto-import std/prelude (includes std/io, std/mem, etc.)</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>-O</code></td>", 10, "                <td><code>--optimize</code></td>", 10, "                <td>Enable compiler optimizations</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>-d</code></td>", 10, "                <td><code>--debug</code></td>", 10, "                <td>Enable debug mode (generates IR dump)</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>-g</code></td>", 10, "                <td><code>--debug-symbols</code></td>", 10, "                <td>Generate debug symbols for debugging</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>-l</code></td>", 10, "                <td><code>--line-mapping</code></td>", 10, "                <td>Generate source line mapping information</td>", 10, "              </tr>", 10, "              <tr>", 10, "                <td><code>-h</code></td>", 10, "                <td><code>--help</code></td>", 10, "                <td>Print usage information and exit</td>", 10, "              </tr>", 10, "            </tbody>", 10, "          </table>", 10, "        </div>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Example Usage</h3>", 10, "        <div class=", 34, "code-block", 34, ">", 10, "          <div class=", 34, "code-header", 34, ">", 10, "            <div class=", 34, "code-title", 34, ">Compiler command examples</div>", 10, "            <div class=", 34, "code-actions", 34, ">", 10, "              <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "            </div>", 10, "          </div>", 10, "          <pre><code><span class=", 34, "c", 34, ">// Basic compilation</span>", 10, "methasm main.masm -o main.s", 10, 10, "<span class=", 34, "c", 34, ">// With optimizations and debug symbols</span>", 10, "methasm main.masm -O -g -o main.s", 10, 10, "<span class=", 34, "c", 34, ">// With custom include paths</span>", 10, "methasm main.masm -I ./include -I ../libs -o main.s", 10, 10, "<span class=", 34, "c", 34, ">// Using prelude (auto-imports common modules)</span>", 10, "methasm --prelude main.masm -o main.s</code></pre>", 10, "        </div>", 10, "      </section>", 10, 10, "      <!-- Troubleshooting -->", 10, "      <section id=", 34, "troubleshooting", 34, " class=", 34, "content-section", 34, ">", 10, "        <h2 class=", 34, "subsection-title", 34, ">Troubleshooting</h2>", 10, "        ", 10, "        <p>Common issues and their solutions when working with MethASM.</p>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Compilation Errors</h3>", 10, "        ", 10, "        <div class=", 34, "alert alert-error", 34, ">", 10, "          <div class=", 34, "alert-title", 34, ">", 226, 157, 140, " ", 34, "Undefined reference to `gc_alloc'", 34, "</div>", 10, "          <p><strong>Cause:</strong> Using <code>new</code> expressions without linking the GC runtime.</p>", 10, "          <p><strong>Solution:</strong> Compile and link <code>src/runtime/gc.c</code> with your program:</p>", 10, "          <div class=", 34, "code-block", 34, ">", 10, "            <div class=", 34, "code-header", 34, ">", 10, "              <div class=", 34, "code-title", 34, ">Linking with GC</div>", 10, "              <div class=", 34, "code-actions", 34, ">", 10, "                <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "              </div>", 10, "            </div>", 10, "            <pre><code>gcc -c src/runtime/gc.c -o gc.o -Isrc", 10, "gcc main.o gc.o -o main</code></pre>", 10, "          </div>", 10, "        </div>", 10, 10, "        <div class=", 34, "alert alert-error", 34, ">", 10, "          <div class=", 34, "alert-title", 34, ">", 226, 157, 140, " ", 34, "Cannot open output file: Permission denied", 34, "</div>", 10, "          <p><strong>Cause:</strong> The output executable is currently running or locked.</p>", 10, "          <p><strong>Solution:</strong> Stop any running instances of the program before rebuilding.</p>", 10, "        </div>", 10, 10, "        <div class=", 34, "alert alert-warning", 34, ">", 10, "          <div class=", 34, "alert-title", 34, ">", 226, 154, 160, 239, 184, 143, " ", 34, "Use before initialization", 34, " error</div>", 10, "          <p><strong>Cause:</strong> Reading from a local variable before assigning it a value.</p>", 10, "          <p><strong>Solution:</strong> Always initialize variables before use:</p>", 10, "          <div class=", 34, "code-block", 34, ">", 10, "            <div class=", 34, "code-header", 34, ">", 10, "              <div class=", 34, "code-title", 34, ">Variable initialization</div>", 10, "              <div class=", 34, "code-actions", 34, ">", 10, "                <button class=", 34, "copy-btn", 34, " onclick=", 34, "copyCode(this)", 34, ">Copy</button>", 10, "              </div>", 10, "            </div>", 10, "            <pre><code><span class=", 34, "c", 34, ">// Wrong - using uninitialized variable</span>", 10, "<span class=", 34, "k", 34, ">var</span> x: <span class=", 34, "t", 34, ">int32</span>;", 10, "<span class=", 34, "k", 34, ">return</span> x; <span class=", 34, "c", 34, ">// Error!</span>", 10, 10, "<span class=", 34, "c", 34, ">// Correct - initialize before use</span>", 10, "<span class=", 34, "k", 34, ">var</span> x: <span class=", 34, "t", 34, ">int32</span> = <span class=", 34, "n", 34, ">0</span>;", 10, "<span class=", 34, "k", 34, ">return</span> x; <span class=", 34, "c", 34, ">// OK</span></code></pre>", 10, "          </div>", 10, "        </div>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Runtime Issues</h3>", 10, "        ", 10, "        <div class=", 34, "alert alert-warning", 34, ">", 10, "          <div class=", 34, "alert-title", 34, ">", 226, 154, 160, 239, 184, 143, " Program crashes on startup</div>", 10, "          <p><strong>Cause:</strong> Missing GC initialization when using <code>new</code> expressions.</p>", 10, "          <p><strong>Solution:</strong> Ensure the entry point calls <code>gc_init()</code> (handled automatically when linking with GC runtime).</p>", 10, "        </div>", 10, 10, "        <div class=", 34, "alert alert-info", 34, ">", 10, "          <div class=", 34, "alert-title", 34, ">", 226, 132, 185, 239, 184, 143, " Performance slower than expected</div>", 10, "          <p><strong>Possible causes:</strong></p>", 10, "          <ul>", 10, "            <li>Not using optimization flag (<code>-O</code>)</li>", 10, "            <li>Frequent GC collections (consider raising threshold)</li>", 10, "            <li>Excessive memory allocations in hot paths</li>", 10, "          </ul>", 10, "          <p><strong>Solutions:</strong> Use optimizations, profile memory usage, consider stack allocation for performance-critical code.</p>", 10, "        </div>", 10, 10, "        <h3 class=", 34, "sub-subsection-title", 34, ">Getting Help</h3>", 10, "        <p>If you encounter issues not covered here:</p>", 10, "        <ul>", 10, "          <li>Check the <a href=", 34, "/forum", 34, " style=", 34, "color: var(--accent-1);", 34, ">Community Forum</a> for help from other users</li>", 10, "          <li>Review the language reference documentation</li>", 10, "          <li>Try compiling with <code>-d</code> flag to see generated IR</li>", 10, "          <li>Start with minimal examples and gradually add complexity</li>", 10, "        </ul>", 10, "      </section>", 10, "    </main>", 10, "  </div>", 10, 10, "  <script>", 10, "    // Copy code functionality", 10, "    function copyCode(button) {", 10, "      const codeBlock = button.closest('.code-block');", 10, "      const code = codeBlock.querySelector('code').textContent;", 10, "      ", 10, "      navigator.clipboard.writeText(code).then(() => {", 10, "        const originalText = button.textContent;", 10, "        button.textContent = 'Copied!';", 10, "        button.style.background = 'rgba(16, 185, 129, 0.2)';", 10, "        button.style.color = 'var(--success)';", 10, "        ", 10, "        setTimeout(() => {", 10, "          button.textContent = originalText;", 10, "          button.style.background = '';", 10, "          button.style.color = '';", 10, "        }, 2000);", 10, "      });", 10, "    }", 10, 10, "    // Smooth scroll and active section highlighting", 10, "    document.addEventListener('DOMContentLoaded', function() {", 10, "      const sections = document.querySelectorAll('.content-section[id]');", 10, "      const navLinks = document.querySelectorAll('.sidebar-link');", 10, "      ", 10, "      function updateActiveNav() {", 10, "        let current = '';", 10, "        sections.forEach(section => {", 10, "          const sectionTop = section.offsetTop;", 10, "          const sectionHeight = section.clientHeight;", 10, "          if (scrollY >= (sectionTop - 200)) {", 10, "            current = section.getAttribute('id');", 10, "          }", 10, "        });", 10, "        ", 10, "        navLinks.forEach(link => {", 10, "          link.classList.remove('active');", 10, "          if (link.getAttribute('href') === '#' + current) {", 10, "            link.classList.add('active');", 10, "          }", 10, "        });", 10, "      }", 10, "      ", 10, "      window.addEventListener('scroll', updateActiveNav);", 10, "      updateActiveNav();", 10, "    });", 10, "  </script>", 10, "</body>", 10, "</html>", 10, 0


HTTP_PAGE_HEADER:
    dq Lstr4
    dq 103
Lstr4:
    db "HTTP/1.1 200 OK", 13, 10, "Content-Type: text/html; charset=utf-8", 13, 10, "Cache-Control: no-cache", 13, 10, "Connection: close", 13, 10, 13, 10, 0


HTTP_DEMO_HEADER:
    dq Lstr5
    dq 103
Lstr5:
    db "HTTP/1.1 200 OK", 13, 10, "Content-Type: text/html; charset=utf-8", 13, 10, "Cache-Control: no-cache", 13, 10, "Connection: close", 13, 10, 13, 10, 0


HTTP_BENCHMARKS_HEADER:
    dq Lstr6
    dq 106
Lstr6:
    db "HTTP/1.1 200 OK", 13, 10, "Content-Type: text/html; charset=utf-8", 13, 10, "Cache-Control: max-age=300", 13, 10, "Connection: close", 13, 10, 13, 10, 0


HTTP_DOCS_HEADER:
    dq Lstr7
    dq 106
Lstr7:
    db "HTTP/1.1 200 OK", 13, 10, "Content-Type: text/html; charset=utf-8", 13, 10, "Cache-Control: max-age=600", 13, 10, "Connection: close", 13, 10, 13, 10, 0


HTTP_404_HEADER:
    dq Lstr8
    dq 45
Lstr8:
    db "HTTP/1.1 404 Not Found", 13, 10, "Content-Length: 9", 13, 10, 13, 10, 0


HTTP_404_BODY:
    dq Lstr9
    dq 9
Lstr9:
    db "Not Found", 0


HTTP_HEALTH_HEADER:
    dq Lstr10
    dq 64
Lstr10:
    db "HTTP/1.1 200 OK", 13, 10, "Content-Type: text/plain", 13, 10, "Content-Length: 2", 13, 10, 13, 10, 0


HTTP_HEALTH_BODY:
    dq Lstr11
    dq 2
Lstr11:
    db "OK", 0


FORUM_HEADER:
    dq Lstr12
    dq 78
Lstr12:
    db "HTTP/1.1 200 OK", 13, 10, "Content-Type: text/html; charset=utf-8", 13, 10, "Connection: close", 13, 10, 13, 10, 0


FORUM_CSS:
    dq Lstr13
    dq 4313
Lstr13:
    db "<style>:root {", 10, "    --bg-color: #05050A;", 10, "    --bg-secondary: rgba(13, 17, 23, 0.6);", 10, "    --text-main: #E2E8F0;", 10, "    --text-muted: #94A3B8;", 10, "    --accent-1: #6366F1;", 10, "    --accent-2: #8B5CF6;", 10, "    --accent-danger: #EF4444;", 10, "    --glass-bg: rgba(255, 255, 255, 0.03);", 10, "    --glass-border: rgba(255, 255, 255, 0.08);", 10, "}", 10, 10, "* {", 10, "    box-sizing: border-box;", 10, "    margin: 0;", 10, "    padding: 0;", 10, "    font-family: 'Inter', system-ui, sans-serif;", 10, "}", 10, 10, "body {", 10, "    background-color: var(--bg-color);", 10, "    color: var(--text-main);", 10, "    line-height: 1.6;", 10, "    padding: 3rem 1rem;", 10, "    min-height: 100vh;", 10, "}", 10, 10, "body::before {", 10, "    content: '';", 10, "    position: fixed;", 10, "    top: 0;", 10, "    left: 0;", 10, "    right: 0;", 10, "    bottom: 0;", 10, "    z-index: -1;", 10, "    background: radial-gradient(circle at 15% 0%, rgba(99, 102, 241, 0.15) 0%, transparent 50%), radial-gradient(circle at 85% 100%, rgba(236, 72, 153, 0.1) 0%, transparent 50%);", 10, "}", 10, 10, ".container {", 10, "    max-width: 800px;", 10, "    margin: 0 auto;", 10, "    background: var(--glass-bg);", 10, "    border: 1px solid var(--glass-border);", 10, "    border-radius: 20px;", 10, "    padding: 2.5rem;", 10, "    backdrop-filter: blur(16px);", 10, "    box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.5);", 10, "    animation: fadeUp 0.6s cubic-bezier(0.16, 1, 0.3, 1);", 10, "}", 10, 10, "h1 {", 10, "    font-size: 2.5rem;", 10, "    font-weight: 700;", 10, "    background: linear-gradient(135deg, #fff 0%, var(--accent-1) 100%);", 10, "    -webkit-background-clip: text;", 10, "    -webkit-text-fill-color: transparent;", 10, "    margin-bottom: 0.5rem;", 10, "}", 10, 10, "h2 {", 10, "    font-size: 1.5rem;", 10, "    font-weight: 600;", 10, "    color: #fff;", 10, "    margin: 2rem 0 1rem;", 10, "    border-bottom: 1px solid var(--glass-border);", 10, "    padding-bottom: 0.5rem;", 10, "}", 10, 10, "ul {", 10, "    list-style: none;", 10, "    display: flex;", 10, "    flex-direction: column;", 10, "    gap: 0.8rem;", 10, "}", 10, 10, "li {", 10, "    background: var(--bg-secondary);", 10, "    border: 1px solid var(--glass-border);", 10, "    border-radius: 12px;", 10, "    transition: all 0.3s;", 10, "}", 10, 10, "li:hover {", 10, "    transform: translateX(5px);", 10, "    border-color: rgba(99, 102, 241, 0.4);", 10, "    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);", 10, "}", 10, 10, "li a {", 10, "    display: block;", 10, "    padding: 1rem 1.25rem;", 10, "    color: var(--text-main);", 10, "    text-decoration: none;", 10, "    font-weight: 500;", 10, "    border-left: 3px solid transparent;", 10, "    transition: 0.2s;", 10, "}", 10, 10, "li:hover a {", 10, "    border-left-color: var(--accent-1);", 10, "    color: #fff;", 10, "}", 10, 10, "form {", 10, "    background: var(--glass-bg);", 10, "    border: 1px solid var(--glass-border);", 10, "    padding: 1.5rem;", 10, "    border-radius: 12px;", 10, "    display: flex;", 10, "    flex-direction: column;", 10, "    gap: 1rem;", 10, "}", 10, 10, "input,", 10, "textarea {", 10, "    width: 100%;", 10, "    padding: 0.8rem 1rem;", 10, "    background: rgba(0, 0, 0, 0.3);", 10, "    border: 1px solid var(--glass-border);", 10, "    border-radius: 8px;", 10, "    color: #fff;", 10, "    transition: 0.3s;", 10, "}", 10, 10, "input:focus,", 10, "textarea:focus {", 10, "    outline: none;", 10, "    border-color: var(--accent-1);", 10, "    box-shadow: 0 0 0 3px rgba(99, 102, 241, 0.2);", 10, "}", 10, 10, "textarea {", 10, "    min-height: 120px;", 10, "    resize: vertical;", 10, "}", 10, 10, "button {", 10, "    background: linear-gradient(135deg, var(--accent-1) 0%, var(--accent-2) 100%);", 10, "    color: white;", 10, "    border: none;", 10, "    padding: 0.8rem 1.5rem;", 10, "    border-radius: 8px;", 10, "    font-weight: 600;", 10, "    cursor: pointer;", 10, "    align-self: flex-start;", 10, "    transition: 0.3s;", 10, "}", 10, 10, "button:hover {", 10, "    transform: translateY(-2px);", 10, "    box-shadow: 0 8px 16px rgba(99, 102, 241, 0.4);", 10, "}", 10, 10, ".post {", 10, "    margin: 1rem 0;", 10, "    padding: 1.25rem;", 10, "    background: var(--bg-secondary);", 10, "    border: 1px solid var(--glass-border);", 10, "    border-radius: 12px;", 10, "    border-left: 4px solid var(--accent-2);", 10, "    white-space: pre-wrap;", 10, "    word-break: break-word;", 10, "    animation: fadeUp 0.5s ease;", 10, "}", 10, 10, ".empty {", 10, "    color: var(--text-muted);", 10, "    font-style: italic;", 10, "    text-align: center;", 10, "    padding: 2rem;", 10, "}", 10, 10, "a.back {", 10, "    color: var(--accent-1);", 10, "    text-decoration: none;", 10, "    font-weight: 500;", 10, "    display: inline-flex;", 10, "    align-items: center;", 10, "    gap: 0.25rem;", 10, "    margin-bottom: 1.5rem;", 10, "    transition: color 0.2s;", 10, "}", 10, 10, "a.back::before {", 10, "    content: '", 226, 134, 144, "';", 10, "}", 10, 10, "a.back:hover {", 10, "    color: #fff;", 10, "    text-decoration: underline;", 10, "}", 10, 10, ".err {", 10, "    background: rgba(239, 68, 68, 0.1);", 10, "    border: 1px solid rgba(239, 68, 68, 0.3);", 10, "    color: var(--accent-danger);", 10, "    padding: 0.75rem;", 10, "    border-radius: 8px;", 10, "    margin-bottom: 1rem;", 10, "}", 10, 10, "@keyframes fadeUp {", 10, "    from {", 10, "        opacity: 0;", 10, "        transform: translateY(15px);", 10, "    }", 10, 10, "    to {", 10, "        opacity: 1;", 10, "        transform: translateY(0);", 10, "    }", 10, "}", 10, 10, "</style>", 0


FORUM_INDEX_START:
    dq Lstr14
    dq 267
Lstr14:
    db "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>Community Forum</title><link href='https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap' rel='stylesheet'>", 0


FORUM_INDEX_BODY:
    dq Lstr15
    dq 124
Lstr15:
    db "</head><body><div class='container'><h1>Community Forum</h1><a href='/' class='back'>Back to Website</a><h2>Threads</h2><ul>", 0


FORUM_INDEX_END:
    dq Lstr16
    dq 32
Lstr16:
    db "</ul><h2>Start a Discussion</h2>", 0


FORUM_FORM:
    dq Lstr17
    dq 182
Lstr17:
    db "<form method='POST' action='/forum'><input name='title' placeholder='Thread title...' required maxlength='200'><button type='submit'>Create Thread</button></form></div></body></html>", 0


FORUM_THREAD_START:
    dq Lstr18
    dq 269
Lstr18:
    db "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>Discussion Thread</title><link href='https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap' rel='stylesheet'>", 0


FORUM_THREAD_HEAD_END:
    dq Lstr19
    dq 40
Lstr19:
    db "</head><body><div class='container'><h1>", 0


FORUM_THREAD_MID:
    dq Lstr20
    dq 68
Lstr20:
    db "</h1><a href='/forum' class='back'>Back to Forum</a><h2>Replies</h2>", 0


FORUM_THREAD_END:
    dq Lstr21
    dq 63
Lstr21:
    db "<h2>Post a Reply</h2><form method='POST' action='/forum?thread=", 0


FORUM_THREAD_END2:
    dq Lstr22
    dq 151
Lstr22:
    db "'><textarea name='body' placeholder='Write your reply here...' required></textarea><button type='submit'>Post Reply</button></form></div></body></html>", 0


FORUM_NO_POSTS:
    dq Lstr23
    dq 57
Lstr23:
    db "<p class='empty'>No posts yet. Be the first to reply!</p>", 0


FORUM_ERR_EMPTY:
    dq Lstr24
    dq 48
Lstr24:
    db "<p class='err'>Thread title cannot be empty.</p>", 0


FORUM_LI_OPEN:
    dq Lstr25
    dq 27
Lstr25:
    db "<li><a href='/forum?thread=", 0


FORUM_LI_MID:
    dq Lstr26
    dq 2
Lstr26:
    db "'>", 0


FORUM_LI_CLOSE:
    dq Lstr27
    dq 9
Lstr27:
    db "</a></li>", 0


FORUM_POST_OPEN:
    dq Lstr28
    dq 18
Lstr28:
    db "<div class='post'>", 0


FORUM_POST_CLOSE:
    dq Lstr29
    dq 6
Lstr29:
    db "</div>", 0


REDIRECT_302:
    dq Lstr30
    dq 30
Lstr30:
    db "HTTP/1.1 302 Found", 13, 10, "Location: ", 0


REDIRECT_END:
    dq Lstr31
    dq 4
Lstr31:
    db 13, 10, 13, 10, 0


fn_threads:
    dq Lstr32
    dq 11
Lstr32:
    db "threads.txt", 0


fn_posts_prefix:
    dq Lstr33
    dq 6
Lstr33:
    db "posts_", 0


fn_posts_suffix:
    dq Lstr34
    dq 4
Lstr34:
    db ".txt", 0


mode_r:
    dq Lstr35
    dq 1
Lstr35:
    db "r", 0


mode_a:
    dq Lstr36
    dq 1
Lstr36:
    db "a", 0


nl:
    dq Lstr37
    dq 1
Lstr37:
    db 10, 0


key_title:
    dq Lstr38
    dq 5
Lstr38:
    db "title", 0


key_body:
    dq Lstr39
    dq 4
Lstr39:
    db "body", 0


hdr_content_length:
    dq Lstr40
    dq 15
Lstr40:
    db "Content-Length:", 0


hdr_content_length_lo:
    dq Lstr41
    dq 15
Lstr41:
    db "content-length:", 0


pat_crlf2:
    dq Lstr42
    dq 4
Lstr42:
    db 13, 10, 13, 10, 0


pat_lf2:
    dq Lstr43
    dq 2
Lstr43:
    db 10, 10, 0


html_amp:
    dq Lstr44
    dq 5
Lstr44:
    db "&amp;", 0


html_lt:
    dq Lstr45
    dq 4
Lstr45:
    db "&lt;", 0


html_gt:
    dq Lstr46
    dq 4
Lstr46:
    db "&gt;", 0


html_quot:
    dq Lstr47
    dq 6
Lstr47:
    db "&quot;", 0


html_apos:
    dq Lstr48
    dq 5
Lstr48:
    db "&#39;", 0


dbg_on:
    dd 0


dbg_post:
    dq Lstr49
    dq 7
Lstr49:
    db "[POST] ", 0


dbg_bs:
    dq Lstr50
    dq 12
Lstr50:
    db " body_start=", 0


dbg_bl:
    dq Lstr51
    dq 10
Lstr51:
    db " body_len=", 0


dbg_cl:
    dq Lstr52
    dq 13
Lstr52:
    db " content_len=", 0


dbg_need:
    dq Lstr53
    dq 6
Lstr53:
    db " need=", 0


dbg_n:
    dq Lstr54
    dq 3
Lstr54:
    db " n=", 0


dbg_got:
    dq Lstr55
    dq 5
Lstr55:
    db " got=", 0


dbg_title:
    dq Lstr56
    dq 11
Lstr56:
    db " title_len=", 0


dbg_fopen_ok:
    dq Lstr57
    dq 9
Lstr57:
    db " fopen_ok", 0


dbg_fopen_fail:
    dq Lstr58
    dq 11
Lstr58:
    db " fopen_fail", 0


dbg_newline:
    dq Lstr59
    dq 1
Lstr59:
    db 10, 0


dbg_fbs_enter:
    dq Lstr60
    dq 14
Lstr60:
    db "[fbs] enter n=", 0


dbg_fbs_exit:
    dq Lstr61
    dq 15
Lstr61:
    db "[fbs] exit ret=", 0


dbg_fbs_ok:
    dq Lstr62
    dq 8
Lstr62:
    db "[fbs] ok", 0


dbg_loop:
    dq Lstr63
    dq 13
Lstr63:
    db "[fbs] loop i=", 0


err_net_init:
    dq Lstr64
    dq 15
Lstr64:
    db "net_init failed", 0


err_socket:
    dq Lstr65
    dq 13
Lstr65:
    db "socket failed", 0


err_reuseaddr:
    dq Lstr66
    dq 17
Lstr66:
    db "setsockopt failed", 0


err_sockaddr:
    dq Lstr67
    dq 15
Lstr67:
    db "sockaddr failed", 0


err_bind:
    dq Lstr68
    dq 11
Lstr68:
    db "bind failed", 0


err_listen:
    dq Lstr69
    dq 13
Lstr69:
    db "listen failed", 0


msg_ready:
    dq Lstr70
    dq 29
Lstr70:
    db "Server: http://localhost:5000", 0


crlf:
    dq Lstr71
    dq 2
Lstr71:
    db 13, 10, 0

Lstr_chars72:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct73:
    dq Lstr_chars72
    dq 37

Lstr_chars74:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct75:
    dq Lstr_chars74
    dq 37

Lstr_chars76:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct77:
    dq Lstr_chars76
    dq 38

Lstr_chars78:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct79:
    dq Lstr_chars78
    dq 38

Lstr_chars80:
    db 10, 0
    align 8
Lstr_struct81:
    dq Lstr_chars80
    dq 1

Lstr_chars82:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct83:
    dq Lstr_chars82
    dq 37

Lstr_chars84:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct85:
    dq Lstr_chars84
    dq 37

Lstr_chars86:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct87:
    dq Lstr_chars86
    dq 37

Lstr_chars88:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct89:
    dq Lstr_chars88
    dq 37

Lstr_chars90:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct91:
    dq Lstr_chars90
    dq 37

Lstr_chars92:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct93:
    dq Lstr_chars92
    dq 37

Lstr_chars94:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct95:
    dq Lstr_chars94
    dq 37

Lstr_chars96:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct97:
    dq Lstr_chars96
    dq 37

Lstr_chars98:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct99:
    dq Lstr_chars98
    dq 37

Lstr_chars100:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct101:
    dq Lstr_chars100
    dq 37

Lstr_chars102:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct103:
    dq Lstr_chars102
    dq 37

Lstr_chars104:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct105:
    dq Lstr_chars104
    dq 37

Lstr_chars106:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct107:
    dq Lstr_chars106
    dq 38

Lstr_chars108:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct109:
    dq Lstr_chars108
    dq 38

Lstr_chars110:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct111:
    dq Lstr_chars110
    dq 38

Lstr_chars112:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct113:
    dq Lstr_chars112
    dq 38

Lstr_chars114:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct115:
    dq Lstr_chars114
    dq 38

Lstr_chars116:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct117:
    dq Lstr_chars116
    dq 38

Lstr_chars118:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct119:
    dq Lstr_chars118
    dq 37

Lstr_chars120:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct121:
    dq Lstr_chars120
    dq 37

Lstr_chars122:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct123:
    dq Lstr_chars122
    dq 37

Lstr_chars124:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct125:
    dq Lstr_chars124
    dq 37

Lstr_chars126:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct127:
    dq Lstr_chars126
    dq 37

Lstr_chars128:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct129:
    dq Lstr_chars128
    dq 37

Lstr_chars130:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct131:
    dq Lstr_chars130
    dq 37

Lstr_chars132:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct133:
    dq Lstr_chars132
    dq 37

Lstr_chars134:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct135:
    dq Lstr_chars134
    dq 37

Lstr_chars136:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct137:
    dq Lstr_chars136
    dq 37

Lstr_chars138:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct139:
    dq Lstr_chars138
    dq 37

Lstr_chars140:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct141:
    dq Lstr_chars140
    dq 37

Lstr_chars142:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct143:
    dq Lstr_chars142
    dq 37

Lstr_chars144:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct145:
    dq Lstr_chars144
    dq 37

Lstr_chars146:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct147:
    dq Lstr_chars146
    dq 37

Lstr_chars148:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct149:
    dq Lstr_chars148
    dq 37

Lstr_chars150:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct151:
    dq Lstr_chars150
    dq 37

Lstr_chars152:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct153:
    dq Lstr_chars152
    dq 37

Lstr_chars154:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct155:
    dq Lstr_chars154
    dq 37

Lstr_chars156:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct157:
    dq Lstr_chars156
    dq 37

Lstr_chars158:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct159:
    dq Lstr_chars158
    dq 37

Lstr_chars160:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct161:
    dq Lstr_chars160
    dq 37

Lstr_chars162:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct163:
    dq Lstr_chars162
    dq 37

Lstr_chars164:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct165:
    dq Lstr_chars164
    dq 37

Lstr_chars166:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct167:
    dq Lstr_chars166
    dq 37

Lstr_chars168:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct169:
    dq Lstr_chars168
    dq 37

Lstr_chars170:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct171:
    dq Lstr_chars170
    dq 37

Lstr_chars172:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct173:
    dq Lstr_chars172
    dq 37

Lstr_chars174:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct175:
    dq Lstr_chars174
    dq 37

Lstr_chars176:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct177:
    dq Lstr_chars176
    dq 37

Lstr_chars178:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct179:
    dq Lstr_chars178
    dq 37

Lstr_chars180:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct181:
    dq Lstr_chars180
    dq 37

Lstr_chars182:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct183:
    dq Lstr_chars182
    dq 37

Lstr_chars184:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct185:
    dq Lstr_chars184
    dq 37

Lstr_chars186:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct187:
    dq Lstr_chars186
    dq 37

Lstr_chars188:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct189:
    dq Lstr_chars188
    dq 37

Lstr_chars190:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct191:
    dq Lstr_chars190
    dq 37

Lstr_chars192:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct193:
    dq Lstr_chars192
    dq 37

Lstr_chars194:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct195:
    dq Lstr_chars194
    dq 37

Lstr_chars196:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct197:
    dq Lstr_chars196
    dq 37

Lstr_chars198:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct199:
    dq Lstr_chars198
    dq 37

Lstr_chars200:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct201:
    dq Lstr_chars200
    dq 37

Lstr_chars202:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct203:
    dq Lstr_chars202
    dq 37

Lstr_chars204:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct205:
    dq Lstr_chars204
    dq 37

Lstr_chars206:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct207:
    dq Lstr_chars206
    dq 37

Lstr_chars208:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct209:
    dq Lstr_chars208
    dq 37

Lstr_chars210:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct211:
    dq Lstr_chars210
    dq 37

Lstr_chars212:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct213:
    dq Lstr_chars212
    dq 37

Lstr_chars214:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct215:
    dq Lstr_chars214
    dq 37

Lstr_chars216:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct217:
    dq Lstr_chars216
    dq 37

Lstr_chars218:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct219:
    dq Lstr_chars218
    dq 37

Lstr_chars220:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct221:
    dq Lstr_chars220
    dq 37

Lstr_chars222:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct223:
    dq Lstr_chars222
    dq 37

Lstr_chars224:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct225:
    dq Lstr_chars224
    dq 37

Lstr_chars226:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct227:
    dq Lstr_chars226
    dq 37

Lstr_chars228:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct229:
    dq Lstr_chars228
    dq 37

Lstr_chars230:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct231:
    dq Lstr_chars230
    dq 37

Lstr_chars232:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct233:
    dq Lstr_chars232
    dq 37

Lstr_chars234:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct235:
    dq Lstr_chars234
    dq 37

Lstr_chars236:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct237:
    dq Lstr_chars236
    dq 37

Lstr_chars238:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct239:
    dq Lstr_chars238
    dq 37

Lstr_chars240:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct241:
    dq Lstr_chars240
    dq 37

Lstr_chars242:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct243:
    dq Lstr_chars242
    dq 37

Lstr_chars244:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct245:
    dq Lstr_chars244
    dq 37

Lstr_chars246:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct247:
    dq Lstr_chars246
    dq 37

Lstr_chars248:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct249:
    dq Lstr_chars248
    dq 37

Lstr_chars250:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct251:
    dq Lstr_chars250
    dq 37

Lstr_chars252:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct253:
    dq Lstr_chars252
    dq 37

Lstr_chars254:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct255:
    dq Lstr_chars254
    dq 37

Lstr_chars256:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct257:
    dq Lstr_chars256
    dq 37

Lstr_chars258:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct259:
    dq Lstr_chars258
    dq 37

Lstr_chars260:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct261:
    dq Lstr_chars260
    dq 37

Lstr_chars262:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct263:
    dq Lstr_chars262
    dq 37

Lstr_chars264:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct265:
    dq Lstr_chars264
    dq 37

Lstr_chars266:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct267:
    dq Lstr_chars266
    dq 37

Lstr_chars268:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct269:
    dq Lstr_chars268
    dq 37

Lstr_chars270:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct271:
    dq Lstr_chars270
    dq 37

Lstr_chars272:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct273:
    dq Lstr_chars272
    dq 37

Lstr_chars274:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct275:
    dq Lstr_chars274
    dq 37

Lstr_chars276:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct277:
    dq Lstr_chars276
    dq 37

Lstr_chars278:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct279:
    dq Lstr_chars278
    dq 37

Lstr_chars280:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct281:
    dq Lstr_chars280
    dq 37

Lstr_chars282:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct283:
    dq Lstr_chars282
    dq 37

Lstr_chars284:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct285:
    dq Lstr_chars284
    dq 37

Lstr_chars286:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct287:
    dq Lstr_chars286
    dq 37

Lstr_chars288:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct289:
    dq Lstr_chars288
    dq 37

Lstr_chars290:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct291:
    dq Lstr_chars290
    dq 37

Lstr_chars292:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct293:
    dq Lstr_chars292
    dq 37

Lstr_chars294:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct295:
    dq Lstr_chars294
    dq 37

Lstr_chars296:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct297:
    dq Lstr_chars296
    dq 37

Lstr_chars298:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct299:
    dq Lstr_chars298
    dq 38

Lstr_chars300:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct301:
    dq Lstr_chars300
    dq 38

Lstr_chars302:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct303:
    dq Lstr_chars302
    dq 37

Lstr_chars304:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct305:
    dq Lstr_chars304
    dq 38

Lstr_chars306:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct307:
    dq Lstr_chars306
    dq 38

Lstr_chars308:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct309:
    dq Lstr_chars308
    dq 37

Lstr_chars310:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct311:
    dq Lstr_chars310
    dq 37

Lstr_chars312:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct313:
    dq Lstr_chars312
    dq 37

Lstr_chars314:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct315:
    dq Lstr_chars314
    dq 37

Lstr_chars316:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct317:
    dq Lstr_chars316
    dq 37

Lstr_chars318:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct319:
    dq Lstr_chars318
    dq 37

Lstr_chars320:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct321:
    dq Lstr_chars320
    dq 37

Lstr_chars322:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct323:
    dq Lstr_chars322
    dq 37

Lstr_chars324:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct325:
    dq Lstr_chars324
    dq 37

Lstr_chars326:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct327:
    dq Lstr_chars326
    dq 37

Lstr_chars328:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct329:
    dq Lstr_chars328
    dq 37

Lstr_chars330:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct331:
    dq Lstr_chars330
    dq 37

Lstr_chars332:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct333:
    dq Lstr_chars332
    dq 37

Lstr_chars334:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct335:
    dq Lstr_chars334
    dq 37

Lstr_chars336:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct337:
    dq Lstr_chars336
    dq 37

Lstr_chars338:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct339:
    dq Lstr_chars338
    dq 37

Lstr_chars340:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct341:
    dq Lstr_chars340
    dq 37

Lstr_chars342:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct343:
    dq Lstr_chars342
    dq 37

Lstr_chars344:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct345:
    dq Lstr_chars344
    dq 37

Lstr_chars346:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct347:
    dq Lstr_chars346
    dq 37

Lstr_chars348:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct349:
    dq Lstr_chars348
    dq 37

Lstr_chars350:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct351:
    dq Lstr_chars350
    dq 37

Lstr_chars352:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct353:
    dq Lstr_chars352
    dq 37

Lstr_chars354:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct355:
    dq Lstr_chars354
    dq 37

Lstr_chars356:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct357:
    dq Lstr_chars356
    dq 37

Lstr_chars358:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct359:
    dq Lstr_chars358
    dq 37

Lstr_chars360:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct361:
    dq Lstr_chars360
    dq 37

Lstr_chars362:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct363:
    dq Lstr_chars362
    dq 37

Lstr_chars364:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct365:
    dq Lstr_chars364
    dq 37

Lstr_chars366:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct367:
    dq Lstr_chars366
    dq 37

Lstr_chars368:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct369:
    dq Lstr_chars368
    dq 37

Lstr_chars370:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct371:
    dq Lstr_chars370
    dq 38

Lstr_chars372:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct373:
    dq Lstr_chars372
    dq 37

Lstr_chars386:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct387:
    dq Lstr_chars386
    dq 38

Lstr_chars388:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct389:
    dq Lstr_chars388
    dq 38

Lstr_chars390:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct391:
    dq Lstr_chars390
    dq 38

Lstr_chars392:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct393:
    dq Lstr_chars392
    dq 38

Lstr_chars394:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct395:
    dq Lstr_chars394
    dq 38

Lstr_chars396:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct397:
    dq Lstr_chars396
    dq 38

Lstr_chars398:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct399:
    dq Lstr_chars398
    dq 38

Lstr_chars400:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct401:
    dq Lstr_chars400
    dq 38

Lstr_chars402:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct403:
    dq Lstr_chars402
    dq 38

Lstr_chars416:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct417:
    dq Lstr_chars416
    dq 38

Lstr_chars418:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct419:
    dq Lstr_chars418
    dq 38

Lstr_chars420:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct421:
    dq Lstr_chars420
    dq 38

Lstr_chars422:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct423:
    dq Lstr_chars422
    dq 38

Lstr_chars424:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct425:
    dq Lstr_chars424
    dq 38

Lstr_chars426:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct427:
    dq Lstr_chars426
    dq 38

Lstr_chars428:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct429:
    dq Lstr_chars428
    dq 38

Lstr_chars430:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct431:
    dq Lstr_chars430
    dq 38

Lstr_chars432:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct433:
    dq Lstr_chars432
    dq 38

Lstr_chars434:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct435:
    dq Lstr_chars434
    dq 38

Lstr_chars436:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct437:
    dq Lstr_chars436
    dq 38

Lstr_chars438:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct439:
    dq Lstr_chars438
    dq 38

Lstr_chars440:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct441:
    dq Lstr_chars440
    dq 38

Lstr_chars442:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct443:
    dq Lstr_chars442
    dq 38

Lstr_chars444:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct445:
    dq Lstr_chars444
    dq 38

Lstr_chars446:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct447:
    dq Lstr_chars446
    dq 38

Lstr_chars448:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct449:
    dq Lstr_chars448
    dq 38

Lstr_chars450:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct451:
    dq Lstr_chars450
    dq 38

Lstr_chars452:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct453:
    dq Lstr_chars452
    dq 38

Lstr_chars454:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct455:
    dq Lstr_chars454
    dq 38

Lstr_chars456:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct457:
    dq Lstr_chars456
    dq 38

Lstr_chars458:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct459:
    dq Lstr_chars458
    dq 38

Lstr_chars460:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct461:
    dq Lstr_chars460
    dq 38

Lstr_chars462:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct463:
    dq Lstr_chars462
    dq 38

Lstr_chars464:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct465:
    dq Lstr_chars464
    dq 38

Lstr_chars466:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct467:
    dq Lstr_chars466
    dq 38

Lstr_chars468:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct469:
    dq Lstr_chars468
    dq 38

Lstr_chars470:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct471:
    dq Lstr_chars470
    dq 38

Lstr_chars472:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct473:
    dq Lstr_chars472
    dq 38

Lstr_chars474:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct475:
    dq Lstr_chars474
    dq 38

Lstr_chars476:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct477:
    dq Lstr_chars476
    dq 38

Lstr_chars478:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct479:
    dq Lstr_chars478
    dq 38

Lstr_chars480:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct481:
    dq Lstr_chars480
    dq 38

Lstr_chars482:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct483:
    dq Lstr_chars482
    dq 38

Lstr_chars484:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct485:
    dq Lstr_chars484
    dq 38

Lstr_chars486:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct487:
    dq Lstr_chars486
    dq 38

Lstr_chars488:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct489:
    dq Lstr_chars488
    dq 38

Lstr_chars490:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct491:
    dq Lstr_chars490
    dq 38

Lstr_chars492:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct493:
    dq Lstr_chars492
    dq 38

Lstr_chars494:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct495:
    dq Lstr_chars494
    dq 37

Lstr_chars496:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct497:
    dq Lstr_chars496
    dq 37

Lstr_chars498:
    db "Fatal error: Null pointer dereference", 0
    align 8
Lstr_struct499:
    dq Lstr_chars498
    dq 37

Lstr_chars500:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct501:
    dq Lstr_chars500
    dq 38

Lstr_chars502:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct503:
    dq Lstr_chars502
    dq 38

Lstr_chars504:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct505:
    dq Lstr_chars504
    dq 38

Lstr_chars506:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct507:
    dq Lstr_chars506
    dq 38

Lstr_chars508:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct509:
    dq Lstr_chars508
    dq 38

Lstr_chars510:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct511:
    dq Lstr_chars510
    dq 38

Lstr_chars512:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct513:
    dq Lstr_chars512
    dq 38

Lstr_chars514:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct515:
    dq Lstr_chars514
    dq 38

Lstr_chars516:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct517:
    dq Lstr_chars516
    dq 38

Lstr_chars518:
    db "Fatal error: Array index out of bounds", 0
    align 8
Lstr_struct519:
    dq Lstr_chars518
    dq 38

