BaseOfLoader	    equ	 09000h	; LOADER.BIN 被加载到的位置 ----  段地址
OffsetOfLoader	    equ	  0100h	; LOADER.BIN 被加载到的位置 ---- 偏移地址

BaseOfLoaderPhyAddr equ	BaseOfLoader*10h ; LOADER.BIN 被加载到的位置 ---- 物理地址

BaseOfKernel    equ	 08000h	; KERNEL.BIN 被加载到的位置 ----  段地址
OffsetOfKernel  equ	     0h	; KERNEL.BIN 被加载到的位置 ---- 偏移地址

BaseOfKernelPhyAddr equ  080000h;	;kernel.bin 

KernelEntryPointPhyAddr equ 0100000h ;需与ld命令参数一致

