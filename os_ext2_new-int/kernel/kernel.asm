; 编译链接方法
; $ nasm -f elf kernel.asm -o kernel.o
; $ ld -s kernel.o -o kernel.bin    #‘-s’选项意为“strip all”



SELECTOR_KERNEL_CS	equ	0x20 ;第五个描述符

; 导入函数
extern	cstart

; 导入全局变量
extern	gdt_ptr
extern	disp_pos



global _start	; 导出 _start



	
[section .bss]

StackSpace		resb	2 * 1024
StackTop:		



[section .text]	; 代码在此
ALIGN 32
[BITS 32]

_start:
	;打印字符'K'
	mov ah,0Fh
	mov al,'K'
	mov [gs:((80*0+42)*2)],ax
	;设置新堆栈	 
	mov esp,StackTop
	;初始化字符串打印函数的全局变量
	mov	dword [disp_pos], 0
	
	;移动原GDT至内核区 
	sgdt	[gdt_ptr]	; cstart() 中将会用到 gdt_ptr
	call	cstart		; 在此函数中改变了gdt_ptr，让它指向新的GDT
	lgdt	[gdt_ptr]	; 使用新的GDT

	jmp	SELECTOR_KERNEL_CS:csinit
csinit:		; 这个跳转指令强制使用刚刚初始化的结构
	jmp $

