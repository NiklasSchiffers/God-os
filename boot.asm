; boot.asm - Multiboot Header und Kernel-Einstiegspunkt
MBALIGN     equ  1 << 0            ; Ausrichtung auf 4KB-Grenzen
MEMINFO     equ  1 << 1            ; Speicherinformationen bereitstellen
VIDEO_MODE  equ  1 << 2            ; Grafikmodus von GRUB anfordern
FLAGS       equ  MBALIGN | MEMINFO | VIDEO_MODE ; Multiboot Flags erweitert
MAGIC       equ  0x1BADB002        ; Magische Zahl für GRUB
CHECKSUM    equ -(MAGIC + FLAGS)   ; Prüfsumme

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    
    ; Grafik-Felder für Multiboot (wird benötigt, wenn VIDEO_MODE Flag gesetzt ist)
    dd 0                      ; mode_type (0 = linearer Framebuffer)
    dd 640                    ; width (Bildschirmbreite)
    dd 480                    ; height (Bildschirmhöhe)
    dd 16                     ; depth (16-Bit Farbtiefe)

section .bss
align 16
stack_bottom:
    resb 16384                  ; 16 KB Stack reservieren
stack_top:

section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top          ; Stack-Pointer setzen
    push ebx                    ; Multiboot-Informationsstruktur übergeben
    push eax                    ; Magische Zahl übergeben
    call kernel_main            ; C-Kernel aufrufen
    cli
.hang:  hlt
    jmp .hang

