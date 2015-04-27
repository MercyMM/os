; 编译链接方法
; $ nasm -f elf kernel.asm -o kernel.o
; $ ld -s kernel.o -o kernel.bin    #‘-s’选项意为“strip all”

[section .text]	; 代码在此
ALIGN 32
[BITS 32]


extern choose

global _start
global myprint

_start:

;	mov ah,0Fh
;	mov al,'K'
;	mov [gs:((80*0+42)*2)],ax

;	jmp $


	push 3
	push 4
	call choose
	add esp,8

	jmp $

; void myprint(char* msg,int len)
myprint:
	mov	edx,[esp+8]
	mov ecx,[esp+4]
	mov ebx,1
	mov eax,4
	int 0x80
	ret



