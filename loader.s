bits 32

; === Symbols ===
global loader
extern kmain

; === Constants ===
MAGIC_NUMBER      equ 0x1BADB002
FLAGS             equ 0x0
CHECKSUM          equ -(MAGIC_NUMBER + FLAGS)
KERNEL_STACK_SIZE equ 4096
VGA_ADDR          equ 0xB8000

; === Multiboot header (must appear early in the binary) ===
section .multiboot
    align 4
    dd MAGIC_NUMBER
    dd FLAGS
    dd CHECKSUM

; === BSS (uninitialized) ===
section .bss
    align 4
kernel_stack:
    resb KERNEL_STACK_SIZE
pos:
    resd 1           ; pozice pro VGA (index znaků, 0..2000)

; === Code ===
section .text
    align 4

; Entry point called by GRUB / bootloader
loader:
    ; nastavíme stack na náš kernel_stack
    mov  esp, kernel_stack + KERNEL_STACK_SIZE
    mov  ebp, esp

    ; vytiskneme úvodní zprávu (print_string používá ESI)
    mov  esi, message
    call print_string

    ; zavoláme C entrypoint kernelu
    call kmain

    ; pokud se kmain vrátí, zastavíme jádro
.halt:
    hlt
    jmp .halt

; -------------------------
; Print routines (VGA text mode 80x25)
; Vstup: AL = znak (print_char), nebo ESI ukazuje na NUL-terminated string (print_string)
; -------------------------
print_char:
    pusha
    mov  ebx, [pos]         ; index znaku
    mov  edi, VGA_ADDR
    lea  edi, [edi + ebx*2] ; každá buňka = 2 bajty (char + attr)
    mov  ah, 0x0F           ; atribut (bílé písmo)
    stosw                   ; uloží AX (AL=znak, AH=atribut)
    inc  dword [pos]
    popa
    ret

print_string:
    pusha
.print_next:
    lodsb                   ; načte byte z [ESI] do AL a inkrementuje ESI
    test al, al
    jz .print_done
    call print_char
    jmp .print_next
.print_done:
    popa
    ret

; -------------------------
; Data
; -------------------------
section .data
    align 4
message:
    db "System start OK. Btw Bro Welcome to Crusader OS v01.", 0