.DATA
align 16
k1k2 QWORD 0740EEF02h, 09E4ADDF8h
k3k4 QWORD 0F20C0DFEh, 0493C7D27h
k5k6 QWORD 03DA6D0CBh, 0BA4FC28Eh
kCk0 QWORD 0F48642E9h, 000000000h
kAkB QWORD 0155AD968h, 02E7D11A7h
kAkB_3way QWORD 08A074012h, 093E106A4h
k_pct QWORD 0DDA0442Bh, 0DEBB642Fh
kPoly QWORD 011EDC6F41h, 011EDC6F41h 

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

AsmCrc32cUpdateFusion PROC
    push rbx
    push rdi
    push rsi
    sub rsp, 16 + 16 

    mov eax, ecx        
    mov rdi, rdx     
    mov rsi, r8        

  
    cmp rsi, 256
    jb use_legacy

  
    mov r8, rdi
    and r8, 15
    jz aligned_start
    mov rcx, 16
    sub rcx, r8        
    sub rsi, rcx       
    
align_loop:
    crc32 eax, byte ptr [rdi]
    inc rdi
    dec rcx
    jnz align_loop

aligned_start:
   
    movd xmm0, eax      
    
 
    movdqa xmm10, XMMWORD PTR [k1k2]
    movdqa xmm11, XMMWORD PTR [k3k4]
    
   
    movdqa xmm1, [rdi]
    movdqa xmm2, [rdi+16]
    movdqa xmm3, [rdi+32]
    movdqa xmm4, [rdi+48]
    
   
    pxor xmm1, xmm0
    
    add rdi, 64
    sub rsi, 64

   
    cmp rsi, 64
    jb fold_final_64

align 16
fold_loop:
   
    prefetcht0 [rdi + 256]

   
    
  
    movdqa xmm5, xmm1
    movdqa xmm6, xmm2
    
    pclmulqdq xmm1, xmm10, 11h 
    pclmulqdq xmm5, xmm10, 00h 
    
    pclmulqdq xmm2, xmm10, 11h
    pclmulqdq xmm6, xmm10, 00h
    
    pxor xmm1, xmm5
    pxor xmm2, xmm6
    
    pxor xmm1, [rdi]      
    pxor xmm2, [rdi+16]

   
   
    movdqa xmm7, xmm3
    movdqa xmm8, xmm4
    
    pclmulqdq xmm3, xmm10, 11h
    pclmulqdq xmm7, xmm10, 00h
    
    pclmulqdq xmm4, xmm10, 11h
    pclmulqdq xmm8, xmm10, 00h
    
    pxor xmm3, xmm7
    pxor xmm4, xmm8
    
    pxor xmm3, [rdi+32]
    pxor xmm4, [rdi+48]
    
    add rdi, 64
    sub rsi, 64
    cmp rsi, 64
    jae fold_loop

fold_final_64:
    
    
   
    movdqa xmm5, xmm1
    pclmulqdq xmm1, xmm11, 11h ; High * K3
    pclmulqdq xmm5, xmm11, 00h ; Low * K4
    pxor xmm1, xmm5
    pxor xmm1, xmm2
    
   
    movdqa xmm5, xmm1
    pclmulqdq xmm1, xmm11, 11h
    pclmulqdq xmm5, xmm11, 00h
    pxor xmm1, xmm5
    pxor xmm1, xmm3

  
    movdqa xmm5, xmm1
    pclmulqdq xmm1, xmm11, 11h
    pclmulqdq xmm5, xmm11, 00h
    pxor xmm1, xmm5
    pxor xmm1, xmm4

   
    
    movdqa xmm12, XMMWORD PTR [k5k6]
    
   
    movdqa xmm2, xmm1
    pclmulqdq xmm1, xmm12, 11h 
    pclmulqdq xmm2, xmm12, 00h 
    pxor xmm1, xmm2
    
   
    
    jmp use_legacy

   

use_legacy:
   
    mov r8, rsi      
    mov ecx, eax     
    mov rdx, rdi     
    
    cmp r8, 0
    jz fusion_ret
    
   
    mov r9, r8
    shr r9, 3
    jz fusion_tail_bytes
align 16
fusion_qword_loop:
    crc32 rax, qword ptr [rdx]
    add rdx, 8
    dec r9
    jnz fusion_qword_loop
fusion_tail_bytes:
    and r8, 7
    jz fusion_ret
fusion_tail_byte_loop:
    crc32 eax, byte ptr [rdx]
    inc rdx
    dec r8
    jnz fusion_tail_byte_loop

fusion_ret:
    add rsp, 32
    pop rsi
    pop rdi
    pop rbx
    ret

AsmCrc32cUpdateFusion ENDP

END