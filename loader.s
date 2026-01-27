bits 32
global loader
extern kmain

; VARIABLES:

MAGIC_NUMBER      equ 0x1BADB002
FLAGS             equ 0x0
CHECKSUM          equ -(MAGIC_NUMBER + FLAGS)
KERNEL_STACK_SIZE equ 4096

;MAIN:

section .multiboot
    align 4
    dd MAGIC_NUMBER
    dd FLAGS
    dd CHECKSUM

section .bss
    align 16
kernel_stack:
    resb KERNEL_STACK_SIZE

section .text
    align 4

loader:
    lea esp, [kernel_stack + KERNEL_STACK_SIZE]
    mov ebp, esp

    ; calling C file part kmain
    call kmain

    ;kills cpu if kmain comes back lol

.hang:
    cli
    hlt
    jmp .hang

section .note.GNU-stack

