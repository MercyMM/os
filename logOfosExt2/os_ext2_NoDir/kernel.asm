; 编译链接方法
; $ nasm -f elf kernel.asm -o kernel.o
; $ ld -s kernel.o -o kernel.bin    #‘-s’选项意为“strip all”




global _start

extern gdt_ptr
extern cstart
extern disp_pos

SELECTOR_KERNEL_CS	equ	20
[section .bss]

StackSpace		resb	2 * 1024
StackTop:		



[section .text]	; 代码在此
ALIGN 32
[BITS 32]

_start:
	;mov ax,SelectorVideo
	;mov gs,ax

	mov ah,0Fh
	mov al,'K'
	mov [gs:((80*0+42)*2)],ax
	
	mov esp,StackTop

	sgdt	[gdt_ptr]	; cstart() 中将会用到 gdt_ptr
	call	cstart		; 在此函数中改变了gdt_ptr，让它指向新的GDT
	lgdt	[gdt_ptr]	; 使用新的GDT

	jmp	SELECTOR_KERNEL_CS:csinit
csinit:		; 这个跳转指令强制使用刚刚初始化的结构
	jmp $
	

