; 编译链接方法
; $ nasm -f elf kernel.asm -o kernel.o
; $ ld -s kernel.o -o kernel.bin    #‘-s’选项意为“strip all”

[section .text]	; 代码在此
ALIGN 32
[BITS 32]

global _start

_start:
	;mov ax,SelectorVideo
	;mov gs,ax

	mov ah,0Fh
	mov al,'K'
	mov [gs:((80*0+42)*2)],ax

	jmp $
	



