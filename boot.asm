MBALIGN     equ  1 << 0            
MEMINFO     equ  1 << 1            
VIDEO_MODE  equ  1 << 2            
FLAGS       equ  MBALIGN | MEMINFO | VIDEO_MODE 
MAGIC       equ  0x1BADB002        
CHECKSUM    equ -(MAGIC + FLAGS)   

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM


    dd 0                     
    dd 640                    
    dd 480                    
    dd 16                     

section .bss
align 16
stack_bottom:
    resb 16384                  
stack_top:

section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top          
    push ebx                   
    push eax                    
    call kernel_main            
    cli
.hang:  hlt
    jmp .hang

