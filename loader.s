; minimal loader.s pro ELF32 / GRUB (NASM)
bits 32

global loader
extern kmain

MAGIC_NUMBER      equ 0x1BADB002
FLAGS             equ 0x0
CHECKSUM          equ -(MAGIC_NUMBER + FLAGS)
KERNEL_STACK_SIZE equ 4096

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
    ; nastavíme stack na vrchol rezervovaného bufferu
    ; lea je jasné a bezpečné (adresní vzorec)
    lea esp, [kernel_stack + KERNEL_STACK_SIZE]
    mov ebp, esp

    ; voláme C entrypoint jádra
    call kmain

    ; pokud se kmain vrátí, zablokujeme CPU
.hang:
    cli
    hlt
    jmp .hang

section .note.GNU-stack

