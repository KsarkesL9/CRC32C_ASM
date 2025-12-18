.data
align 16
CONST_K1K2      DQ 00000000ccaa009eh, 00000001751997d0h
CONST_K3K4      DQ 0163cd6124h, 00000001f7011640h
CONST_K5K6      DQ 0579665f3h, 047b04d9bh
CONST_BARRETT   DQ 0dea713f1h, 000000000h
CONST_POLY      DQ 082f63b78h, 000000000h
CONST_MASK      DD 000000000h, 000000000h, 0ffffffffh, 0ffffffffh
kAkB QWORD 0155AD968h, 02E7D11A7h
kAkB_3way QWORD 08A074012h, 093E106A4h
kAkB_1024 QWORD 0F20C0DFEh, 0493C7D27h


align 16
Constants_K1K2      DQ 00000000ccaa009eh, 00000001751997d0h
Constants_K3K4      DQ 00000001f7860f71h, 0000000163cd6124h
Constants_MuPoly    DQ 0000000dea713f23h, 0000000105ec76f0h

.CODE

AsmAdd PROC
    mov eax, ecx    
    add eax, edx    
    ret             
AsmAdd ENDP

AsmCrc32cUpdate PROC
    mov eax, ecx
    test r8, r8
    jz done
    mov r9, r8          
    shr r9, 3          
    jz process_bytes    
    align 16
qword_loop:
    crc32 rax, qword ptr [rdx]
    add rdx, 8          
    dec r9              
    jnz qword_loop
process_bytes:
    and r8, 7           
    jz done             
byte_loop:
    crc32 eax, byte ptr [rdx]
    inc rdx
    dec r8
    jnz byte_loop
done:
    ret
AsmCrc32cUpdate ENDP

AsmCrc32cUpdate3Way PROC
    push rbx
    push r12
    mov eax, ecx        
    cmp r8, 4096
    jb tail_processing
    movdqa xmm2, XMMWORD PTR [kAkB_3way]
align 16
block_4k_loop:
    xor r9d, r9d        
    xor r10d, r10d      
    mov r11, 170        
align 16
interleave_loop:
    crc32 rax, qword ptr [rdx]
    crc32 r9, qword ptr [rdx + 1360]
    crc32 r10, qword ptr [rdx + 2720]
    add rdx, 8
    dec r11
    jnz interleave_loop
    movd xmm0, eax      
    movd xmm1, r9d      
    pclmulqdq xmm0, xmm2, 00h
    pclmulqdq xmm1, xmm2, 10h
    pxor xmm0, xmm1
    movq r9, xmm0
    crc32 r10, qword ptr [rdx + 2720]
    mov r11, qword ptr [rdx + 2728] 
    xor r9, r11                     
    crc32 r10, r9
    mov eax, r10d
    add rdx, 2736
    sub r8, 4096
    cmp r8, 4096
    jae block_4k_loop
tail_processing:
    mov r9, r8
    shr r9, 3           
    jz tail_bytes       
align 16
tail_qword_loop:
    crc32 rax, qword ptr [rdx]
    add rdx, 8
    dec r9
    jnz tail_qword_loop
tail_bytes:
    and r8, 7
    jz exit_func
tail_byte_loop:
    crc32 eax, byte ptr [rdx]
    inc rdx
    dec r8
    jnz tail_byte_loop
exit_func:
    pop r12
    pop rbx
    ret
AsmCrc32cUpdate3Way ENDP




AsmCrc32cUpdateFusion PROC FRAME
    push rbx
    .pushreg rbx
    push rdi
    .pushreg rdi
    push rsi
    .pushreg rsi
    .endprolog

    mov eax, ecx
    mov rdi, rdx       
    mov rsi, r8         

    jmp scalar_fallback

    test rdi, 15
    jz vector_init

align_loop:
    crc32 eax, byte ptr [rdi]
    inc rdi
    dec rsi
    test rdi, 15
    jnz align_loop

vector_init:
    movdqa xmm10, xmmword ptr [CONST_K1K2]
    movdqa xmm11, xmmword ptr [CONST_K3K4]

    movd xmm0, eax
    pxor xmm0, xmmword ptr [rdi]
    movdqa xmm1, xmmword ptr [rdi + 16]
    movdqa xmm2, xmmword ptr [rdi + 32]
    movdqa xmm3, xmmword ptr [rdi + 48]
    add rdi, 64
    sub rsi, 64

vector_loop:
    cmp rsi, 64
    jb vector_fold

    movdqa xmm4, xmm0
    pclmulqdq xmm4, xmm10, 11h
    pclmulqdq xmm0, xmm10, 00h
    pxor xmm0, xmm4
    pxor xmm0, xmmword ptr [rdi]

    movdqa xmm5, xmm1
    pclmulqdq xmm5, xmm10, 11h
    pclmulqdq xmm1, xmm10, 00h
    pxor xmm1, xmm5
    pxor xmm1, xmmword ptr [rdi + 16]

    movdqa xmm4, xmm2
    pclmulqdq xmm4, xmm10, 11h
    pclmulqdq xmm2, xmm10, 00h
    pxor xmm2, xmm4
    pxor xmm2, xmmword ptr [rdi + 32]

    movdqa xmm5, xmm3
    pclmulqdq xmm5, xmm10, 11h
    pclmulqdq xmm3, xmm10, 00h
    pxor xmm3, xmm5
    pxor xmm3, xmmword ptr [rdi + 48]

    add rdi, 64
    sub rsi, 64
    jmp vector_loop

vector_fold:
    movdqa xmm4, xmm0
    pclmulqdq xmm4, xmm11, 11h
    pclmulqdq xmm0, xmm11, 00h
    pxor xmm0, xmm4
    pxor xmm0, xmm1

    movdqa xmm4, xmm0
    pclmulqdq xmm4, xmm11, 11h
    pclmulqdq xmm0, xmm11, 00h
    pxor xmm0, xmm4
    pxor xmm0, xmm2

    movdqa xmm4, xmm0
    pclmulqdq xmm4, xmm11, 11h
    pclmulqdq xmm0, xmm11, 00h
    pxor xmm0, xmm4
    pxor xmm0, xmm3

    sub rsp, 16
    movdqa [rsp], xmm0
    
    xor eax, eax
    crc32 rax, qword ptr [rsp]
    crc32 rax, qword ptr [rsp + 8]
    add rsp, 16
    
    test rsi, rsi
    jz done
    
remaining_bytes:
    crc32 eax, byte ptr [rdi]
    inc rdi
    dec rsi
    jnz remaining_bytes
    jmp done

scalar_fallback:
    mov r9, rsi
    shr r9, 3           
    jz scalar_bytes     
    
    align 16
scalar_qword_loop:
    crc32 rax, qword ptr [rdi]
    add rdi, 8
    dec r9
    jnz scalar_qword_loop

scalar_bytes:
    and rsi, 7          
    jz done
scalar_byte_loop:
    crc32 eax, byte ptr [rdi]
    inc rdi
    dec rsi
    jnz scalar_byte_loop

done:
    pop rsi
    pop rdi
    pop rbx
    ret
AsmCrc32cUpdateFusion ENDP

AsmCrc32cUpdateReport PROC FRAME
    push rdi
    .pushreg rdi
    push rsi
    .pushreg rsi
    .endprolog
    
    mov eax, ecx
    mov rdi, rdx
    mov rsi, r8
    
    cmp rsi, 128
    jb L_scalar_fallback
    
    mov r9, rsi
    shr r9, 7
    
    align 16
L_main_loop:
    crc32 rax, qword ptr [rdi]
    crc32 rax, qword ptr [rdi + 8]
    crc32 rax, qword ptr [rdi + 16]
    crc32 rax, qword ptr [rdi + 24]
    crc32 rax, qword ptr [rdi + 32]
    crc32 rax, qword ptr [rdi + 40]
    crc32 rax, qword ptr [rdi + 48]
    crc32 rax, qword ptr [rdi + 56]
    crc32 rax, qword ptr [rdi + 64]
    crc32 rax, qword ptr [rdi + 72]
    crc32 rax, qword ptr [rdi + 80]
    crc32 rax, qword ptr [rdi + 88]
    crc32 rax, qword ptr [rdi + 96]
    crc32 rax, qword ptr [rdi + 104]
    crc32 rax, qword ptr [rdi + 112]
    crc32 rax, qword ptr [rdi + 120]
    
    add rdi, 128
    dec r9
    jnz L_main_loop
    
    and rsi, 127
    
L_scalar_fallback:
    mov r9, rsi
    shr r9, 3
    jz L_bytes
    
L_qword_loop:
    crc32 rax, qword ptr [rdi]
    add rdi, 8
    dec r9
    jnz L_qword_loop
    
L_bytes:
    and rsi, 7
    jz L_done
    
L_byte_loop:
    crc32 eax, byte ptr [rdi]
    inc rdi
    dec rsi
    jnz L_byte_loop
    
L_done:
    pop rsi
    pop rdi
    ret
AsmCrc32cUpdateReport ENDP

END