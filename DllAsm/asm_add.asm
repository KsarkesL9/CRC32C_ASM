.data
align 16
kAkB_3way QWORD 08A074012h, 093E106A4h

.CODE

AsmCrc32cHardwareScalar PROC
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
AsmCrc32cHardwareScalar ENDP

AsmCrc32cHardwarePipelining PROC
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
AsmCrc32cHardwarePipelining ENDP

END