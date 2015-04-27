org  0100h	
	jmp	LABEL_START		; Start
; 下面是 FAT12 磁盘的头, 之所以包含它是因为下面用到了磁盘的一些信息
%include	"fat12hdr.inc" ;fat12 磁盘头
%include	"pm.inc"
%include	"load.inc"

;GDT
;GDT 被加载到90000+LABEL_GDT
LABEL_GDT:	Descriptor 0,		0,	0	;
;										段基址		段界限	     属性
;用户代码段：0G--1G ，界限1G,    ring 0 , 粒度4k
LABEL_DESC_CODE_USER:		Descriptor 0,			03ffffh,	DA_CR|DA_32|DA_DPL0|DA_LIMIT_4K		
;用户数据段：0G--1G ，界限1G，   ring 0   粒度4k
LABEL_DESC_DATA_USER:		Descriptor 0,			03ffffh,	DA_DRW|DA_32|DA_DPL0|DA_LIMIT_4K	
;显存首址
LABEL_DESC_VIDEO:			Descriptor 0B8000h,		0ffffh,		DA_DRW|DA_DPL3	; 
;核心代码段：3G--4G，界限1G
LABEL_DESC_CODE_KERNEL:		Descriptor 0c0000000h,	 03ffffh,	DA_CR|DA_32|DA_DPL0|DA_LIMIT_4K	
;核心数据段，3G--4G
LABEL_DESC_DATA_KERNEL:		Descriptor 0c0000000h,	 03ffffh,	DA_DRW|DA_32|DA_DPL0|DA_LIMIT_4K


;初始化gdtr的常量
GdtLen			equ			$ - LABEL_GDT
GdtPtr			dw			GdtLen - 1
				dd			BaseOfLoaderPhyAddr + LABEL_GDT

;GDT 选择子
SelectorCodeUser	equ		LABEL_DESC_CODE_USER - LABEL_GDT
SelectorDataUser	equ		LABEL_DESC_DATA_USER - LABEL_GDT
SelectorVideo		equ		LABEL_DESC_VIDEO	 - LABEL_GDT + SA_RPL3 ;选择子的RPL为3，需要和显存段属性一致
SelectorCodeKernel	equ		LABEL_DESC_CODE_KERNEL - LABEL_GDT
SelectorDataKernel	equ		LABEL_DESC_DATA_KERNEL - LABEL_GDT

BaseOfStack	equ	0100h
PageDirBase	equ 500000h	;页目录开始地址：1M 5M
PageTblBase equ 501000h ;页表开始地址： 1M + 4K 5M+4K
;BaseOfKernelFile	equ	 08000h	; KERNEL.BIN 被加载到的位置 ----  段地址
;OffsetOfKernelFile	equ	     0h	; KERNEL.BIN 被加载到的位置 ---- 偏移地址

LABEL_START:			; <--- 从这里开始 *************
	mov	ax, cs
	mov	ds, ax
	mov	es, ax
	mov	ss, ax
	mov	sp, BaseOfStack
	mov	dh, 0			; "Loading  "
	call	DispStr			; 显示字符串

	; 下面在 A 盘的根目录寻找 KERNEL.BIN
	mov	word [wSectorNo], SectorNoOfRootDirectory	
	xor	ah, ah	; `.
	xor	dl, dl	;  | 软驱复位
	int	13h	; /
LABEL_SEARCH_IN_ROOT_DIR_BEGIN:
	cmp	word [wRootDirSizeForLoop], 0	; `.
	jz	LABEL_NO_KERNELBIN		;  | 判断根目录区是不是已经读完,
	dec	word [wRootDirSizeForLoop]	; /  读完表示没有找到 KERNEL.BIN
	mov	ax, BaseOfKernelFile
	mov	es, ax			; es <- BaseOfKernelFile
	mov	bx, OffsetOfKernelFile	; bx <- OffsetOfKernelFile
	mov	ax, [wSectorNo]		; ax <- Root Directory 中的某 Sector 号
	mov	cl, 1
	call	ReadSector
	mov	si, KernelFileName	; ds:si -> "KERNEL  BIN"
	mov	di, OffsetOfKernelFile
	cld
	mov	dx, 10h
LABEL_SEARCH_FOR_KERNELBIN:
	cmp	dx, 0				  ; `.
	jz	LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR;  | 循环次数控制, 如果已经读完
	dec	dx				  ; /  了一个 Sector, 就跳到下一个
	mov	cx, 11
LABEL_CMP_FILENAME:
	cmp	cx, 0			; `.
	jz	LABEL_FILENAME_FOUND	;  | 循环次数控制, 如果比较了 11 个字符都
	dec	cx			; /  相等, 表示找到
	lodsb				; ds:si -> al
	cmp	al, byte [es:di]	; if al == es:di
	jz	LABEL_GO_ON
	jmp	LABEL_DIFFERENT
LABEL_GO_ON:
	inc	di
	jmp	LABEL_CMP_FILENAME	;	继续循环
LABEL_DIFFERENT:
	and	di, 0FFE0h		; else`. 让 di 是 20h 的倍数
	add	di, 20h			;      |
	mov	si, KernelFileName	;      | di += 20h  下一个目录条目
	jmp	LABEL_SEARCH_FOR_KERNELBIN;   /
LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR:
	add	word [wSectorNo], 1
	jmp	LABEL_SEARCH_IN_ROOT_DIR_BEGIN
LABEL_NO_KERNELBIN:
	mov	dh, 2			; "No KERNEL."
	call	DispStr			; 显示字符串
%ifdef	_LOADER_DEBUG_
	mov	ax, 4c00h		; `.
	int	21h			; / 没有找到 KERNEL.BIN, 回到 DOS
%else
	jmp	$			; 没有找到 KERNEL.BIN, 死循环在这里
%endif

LABEL_FILENAME_FOUND:			; 找到 KERNEL.BIN 后便来到这里继续
	mov	ax, RootDirSectors
	and	di, 0FFF0h		; di -> 当前条目的开始
	push	eax
	mov	eax, [es : di + 01Ch]		; `.
	mov	dword [dwKernelSize], eax	; / 保存 KERNEL.BIN 文件大小
	pop	eax
	add	di, 01Ah		; di -> 首 Sector
	mov	cx, word [es:di]
	push	cx			; 保存此 Sector 在 FAT 中的序号
	add	cx, ax
	add	cx, DeltaSectorNo	; cl <- LOADER.BIN 的起始扇区号(0-based)
	mov	ax, BaseOfKernelFile
	mov	es, ax			; es <- BaseOfKernelFile
	mov	bx, OffsetOfKernelFile	; bx <- OffsetOfKernelFile
	mov	ax, cx			; ax <- Sector 号
LABEL_GOON_LOADING_FILE:
	push	ax			; `.
	push	bx			;  |
	mov	ah, 0Eh			;  | 每读一个扇区就在 "Loading  " 后面
	mov	al, '.'			;  | 打一个点, 形成这样的效果:
	mov	bl, 0Fh			;  | Loading ......
	int	10h			;  |
	pop	bx			;  |
	pop	ax			; /
	mov	cl, 1
	call	ReadSector
	pop	ax			; 取出此 Sector 在 FAT 中的序号
	call	GetFATEntry
	cmp	ax, 0FFFh
	jz	LABEL_FILE_LOADED
	push	ax			; 保存 Sector 在 FAT 中的序号
	mov	dx, RootDirSectors
	add	ax, dx
	add	ax, DeltaSectorNo
	add	bx, [BPB_BytsPerSec]
	jmp	LABEL_GOON_LOADING_FILE
LABEL_FILE_LOADED:
	call	KillMotor		; 关闭软驱马达
	mov	dh, 1			; "Ready."
	call	DispStr			; 显示字符串

;进入保护模式
	; 加载gdtr
	lgdt	[GdtPtr]
	;关中断
	cli
	;打开地址A20
	in al,92h
	or al,10b
	out 92h,al

	;准备切换到保护模式
	mov eax, cr0
	or	eax, 1
	mov	cr0, eax
	;进入保护模式
	;jmp SelectorCodeUser:(90000h+LABEL_PM_START)
	jmp	dword SelectorCodeUser:(BaseOfLoaderPhyAddr+LABEL_PM_START)
;============================================================================
;变量
;----------------------------------------------------------------------------
wRootDirSizeForLoop	dw	RootDirSectors	; Root Directory 占用的扇区数
wSectorNo		dw	0		; 要读取的扇区号
bOdd			db	0		; 奇数还是偶数
dwKernelSize		dd	0		; KERNEL.BIN 文件大小
;============================================================================
;字符串
;----------------------------------------------------------------------------
KernelFileName		db	"KERNEL  BIN", 0	; KERNEL.BIN 之文件名
; 为简化代码, 下面每个字符串的长度均为 MessageLength
MessageLength		equ	9
LoadMessage:		db	"Loading  "
Message1		db	"Ready.   "
Message2		db	"No KERNEL"
;============================================================================
;----------------------------------------------------------------------------
; 函数名: DispStr
;----------------------------------------------------------------------------
; 作用:
;	显示一个字符串, 函数开始时 dh 中应该是字符串序号(0-based)
DispStr:
	mov	ax, MessageLength
	mul	dh
	add	ax, LoadMessage
	mov	bp, ax			; ┓
	mov	ax, ds			; ┣ ES:BP = 串地址
	mov	es, ax			; ┛
	mov	cx, MessageLength	; CX = 串长度
	mov	ax, 01301h		; AH = 13,  AL = 01h
	mov	bx, 0007h		; 页号为0(BH = 0) 黑底白字(BL = 07h)
	mov	dl, 0
	add	dh, 3			; 从第 3 行往下显示
	int	10h			; int 10h
	ret
;----------------------------------------------------------------------------
; 函数名: ReadSector
;----------------------------------------------------------------------------
; 作用:
;	从序号(Directory Entry 中的 Sector 号)为 ax 的的 Sector 开始, 将 cl 个 Sector 读入 es:bx 中
ReadSector:
	; -----------------------------------------------------------------------
	; 怎样由扇区号求扇区在磁盘中的位置 (扇区号 -> 柱面号, 起始扇区, 磁头号)
	; -----------------------------------------------------------------------
	; 设扇区号为 x
	;                           ┌ 柱面号 = y >> 1
	;       x           ┌ 商 y ┤
	; -------------- => ┤      └ 磁头号 = y & 1
	;  每磁道扇区数     │
	;                   └ 余 z => 起始扇区号 = z + 1
	push	bp
	mov	bp, sp
	sub	esp, 2			; 辟出两个字节的堆栈区域保存要读的扇区数: byte [bp-2]
	mov	byte [bp-2], cl
	push	bx			; 保存 bx
	mov	bl, [BPB_SecPerTrk]	; bl: 除数
	div	bl			; y 在 al 中, z 在 ah 中
	inc	ah			; z ++
	mov	cl, ah			; cl <- 起始扇区号
	mov	dh, al			; dh <- y
	shr	al, 1			; y >> 1 (其实是 y/BPB_NumHeads, 这里BPB_NumHeads=2)
	mov	ch, al			; ch <- 柱面号
	and	dh, 1			; dh & 1 = 磁头号
	pop	bx			; 恢复 bx
	; 至此, "柱面号, 起始扇区, 磁头号" 全部得到 ^^^^^^^^^^^^^^^^^^^^^^^^
	mov	dl, [BS_DrvNum]		; 驱动器号 (0 表示 A 盘)
.GoOnReading:
	mov	ah, 2			; 读
	mov	al, byte [bp-2]		; 读 al 个扇区
	int	13h
	jc	.GoOnReading		; 如果读取错误 CF 会被置为 1, 这时就不停地读, 直到正确为止
	add	esp, 2
	pop	bp
	ret
;----------------------------------------------------------------------------
; 函数名: GetFATEntry
;----------------------------------------------------------------------------
; 作用:
;	找到序号为 ax 的 Sector 在 FAT 中的条目, 结果放在 ax 中
;	需要注意的是, 中间需要读 FAT 的扇区到 es:bx 处, 所以函数一开始保存了 es 和 bx
GetFATEntry:
	push	es
	push	bx
	push	ax
	mov	ax, BaseOfKernelFile	; ┓
	sub	ax, 0100h		; ┣ 在 BaseOfKernelFile 后面留出 4K 空间用于存放 FAT
	mov	es, ax			; ┛
	pop	ax
	mov	byte [bOdd], 0
	mov	bx, 3
	mul	bx			; dx:ax = ax * 3
	mov	bx, 2
	div	bx			; dx:ax / 2  ==>  ax <- 商, dx <- 余数
	cmp	dx, 0
	jz	LABEL_EVEN
	mov	byte [bOdd], 1
LABEL_EVEN:;偶数
	xor	dx, dx			; 现在 ax 中是 FATEntry 在 FAT 中的偏移量. 下面来计算 FATEntry 在哪个扇区中(FAT占用不止一个扇区)
	mov	bx, [BPB_BytsPerSec]
	div	bx			; dx:ax / BPB_BytsPerSec  ==>	ax <- 商   (FATEntry 所在的扇区相对于 FAT 来说的扇区号)
					;				dx <- 余数 (FATEntry 在扇区内的偏移)。
	push	dx
	mov	bx, 0			; bx <- 0	于是, es:bx = (BaseOfKernelFile - 100):00 = (BaseOfKernelFile - 100) * 10h
	add	ax, SectorNoOfFAT1	; 此句执行之后的 ax 就是 FATEntry 所在的扇区号
	mov	cl, 2
	call	ReadSector		; 读取 FATEntry 所在的扇区, 一次读两个, 避免在边界发生错误, 因为一个 FATEntry 可能跨越两个扇区
	pop	dx
	add	bx, dx
	mov	ax, [es:bx]
	cmp	byte [bOdd], 1
	jnz	LABEL_EVEN_2
	shr	ax, 4
LABEL_EVEN_2:
	and	ax, 0FFFh
LABEL_GET_FAT_ENRY_OK:
	pop	bx
	pop	es
	ret
;----------------------------------------------------------------------------
;----------------------------------------------------------------------------
; 函数名: KillMotor
;----------------------------------------------------------------------------
; 作用:
;	关闭软驱马达
KillMotor:
	push	dx
	mov	dx, 03F2h
	mov	al, 0
	out	dx, al
	pop	dx
	ret
;----------------------------------------------------------------------------
[SECTION .s32]; 段命为.s32
ALIGN	32
[BITS 32]		;编译成32位指令
LABEL_PM_START:
	mov ax,SelectorDataUser
	mov ds,ax
	mov es,ax
	mov fs,ax
	mov ss,ax
	mov esp,TopOfStack
	;显示`
	mov ax,SelectorVideo
	mov gs,ax
	
	;mov edi,(80*11+79)*2
	mov ah,0Fh	; black background  red word
	mov al,'P'
	mov [gs:((80 * 0 + 39) * 2)],ax
	call SetupPaging

	; jmp to  kernel(3G-4G) to test paging is or not open
	jmp SelectorCodeKernel:BaseOfKernelPhyAddr

						   
;---------------------------------------------------------------------
;
;	启动分页机制
;
;	功能：线性地址的0-1G和3G到4G 映射到物理地址的0-1G
;---------------------------------------------------------------------
SetupPaging:
	;初始化页目录
	 mov ecx,1024	;页目录项个数1024个,表示4G的线性空间。（1024×1024×4096）
	 mov ax,SelectorDataUser
	 mov es,ax
	 mov edi,PageDirBase ;es:edi指向页目录表
	 xor eax,eax
	 mov eax,PageTblBase | PG_P|PG_USU|PG_RWW
.1:
	stosd	;写4字节
	add eax,4096
	loop .1
	mov ecx,256
	mov edi,PageDirBase
	add edi,3072
	xor eax,eax
	mov eax,PageTblBase|PG_P|PG_USU|PG_RWW
.3G:
	stosd
	add eax,4096
	loop .3G
	;初始化页表
	; 只初始化指向 0-1G 和 3G-4G 的页表	
	;初始化0-1G页表
	mov edi,PageTblBase ;edi指向页表基址
	mov eax,256		;256个页表代表1G空间
	mov ebx,1024
	mul ebx			;eax 页表项个数
	mov ecx,eax		;循环次数
	xor eax,eax
	mov eax,PG_P|PG_USU|PG_RWW
.2:
	stosd
	add eax,4096
	loop .2
	
	;init cr3
	mov eax,PageDirBase
	mov cr3,eax

	
	mov eax,cr0
	or  eax,80000000h
	mov cr0,eax
;????????????????????????????????????????????????????
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	jmp short .4
.4:
	nop
	ret
;_______________________________________________________________
;		分页机制启动完毕
;
;
;
;———————————————————————————————————————————————————————————————
[SECTION .data1]
ALIGN	32
LABEL_DATA:
	StackSpace: times 1024	db 0
	TopOfStack	equ	BaseOfLoaderPhyAddr +$ ; 栈向下增长
