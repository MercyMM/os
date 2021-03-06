org  0100h	
	jmp	LABEL_START		; Start
%include	"pm.inc"
%include	"load.inc"


SectorNoOfRootDirectory	equ	66	; Root Directory 的第一个扇区号,扇区的大小是512B，block的大小是1024B.

BaseOfInodeTable	equ 2800h
InodeSize			equ 128			;128 =  80h

DirEntryNameOffset	     equ 8
DirEntryNameLenOffset	 equ 6
DirEntryRecLenOffset	 equ 4
;ext_dir{
;	_u32 inode_No;	目录项对应的inode号
;	_u16 rec_len;	目录项的长度
;	_u8	 name_len;	目录下名字的长度
;	_u8	 file_type;	目录项类型：文件or目录or链接
;	char name[];	目录项的名字
;}


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




; 在根目录下寻找loader.bin
;	根目录的block在8400处，即第33块block。

;过程：
; 1.将根目录的block读入内存
; 2.比较目录项：先比较NameLen是否为10,如果是，继续比较文件名，如果否，继续下一个目录项的比较
; 3.如果比较了32个目录下都没有loader.bin，则输出:No Loader

	mov	word [wSectorNo], SectorNoOfRootDirectory
LABEL_SEARCH_IN_ROOT_DIR_BEGIN:

	mov	ax, BaseOfKernel
	mov	es, ax			; es <- BaseOfKernel
	mov	bx, OffsetOfKernel	; bx <- OffsetOfKernel	于是, es:bx = BaseOfKernel:OffsetOfKernel
	mov	ax, [wSectorNo]	; ax <- Root Directory 中的某 Sector 号
	mov	cl, 2
	call	ReadSector  ;需要四个参数:ax，cl，es:bx（es:bx需要512=200h对齐）。从第ax个扇区开始，将cl个扇区读入es:bx指向的内存

	mov	si, KernelFileName	; ds:si -> "kernel.bin"
	mov	di, OffsetOfKernel	; es:di -> BaseOfKernel:OffsetOfKernel = 8000h*10h+100h
	cld
	
	mov dx,di	;每次开始循环，dx指向目录项头，di指向目录项名
	add di,DirEntryNameOffset

LABEL_SEARCH_FOR_LOADERBIN:	
	cmp word [EntryNumInRoot],0
	jz	LABEL_NO_KERNELBIN
	mov ax,word [EntryNumInRoot]
	dec ax
	mov word [EntryNumInRoot],ax

	mov	cx, 10
	
	mov bx,dx
	add bx,DirEntryNameLenOffset
	cmp cl, byte [es:bx]  ;;比较文件名和10的值，相等才继续比较
	jz	LABEL_CMP_FILENAME	;相等，继续比较文件名
	jmp LABEL_DIFFERENT		;不等，比较下一个文件名
LABEL_CMP_FILENAME:	
	cmp	cx, 0
	jz	LABEL_FILENAME_FOUND	; 如果比较了 11 个字符都相等, 表示找到
	dec	cx
	lodsb				; ds:si -> al加载字符串常量“loader.bin”
	cmp	al, byte [es:di];与目录项名字进行比较
	jz	LABEL_GO_ON
	jmp	LABEL_DIFFERENT		; 只要发现不一样的字符就表明本 DirectoryEntry 不是
; 我们要找的 LOADER.BINctxt
LABEL_GO_ON:
	inc	di
	jmp	LABEL_CMP_FILENAME	;	继续循环

LABEL_DIFFERENT:
	mov	si, KernelFileName          ;si指向“Kernel.bin”字符串常量
	mov bx,dx
	add bx,DirEntryRecLenOffset
	add dx, [es:bx]			;dx 指向下一个目录项开头，di=dx+8指向目录项的name开头
	mov di,dx
	add di,DirEntryNameOffset
	jmp LABEL_SEARCH_FOR_LOADERBIN	;    


LABEL_NO_KERNELBIN:
	mov	dh, 2			; "No LOADER."
	call	DispStr			; 显示字符串

;执行到此处后es:di指向kernel.bin。es:dx指向其目录项开头

;找到kernel.bin的目录项后，加载其Inode节点，进而加载其Block
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;加载kernel.bin过程
	;1,计算kernel.bin的inode的地址
	;2,将inode所在扇区读入8f00:0000处内存(此处与loader.bin的inode共用内存）
	;3,循环读入block数组，即kernel.bin的内容。

LABEL_FILENAME_FOUND:
	;1,计算kernel.bin的inode的地址
	;ax = BaseOfInodeTable +(eax-1)*InodeSize = 2800h+(eax-1)*80h，
	mov bx,dx
	mov ax,word [es:bx]		;inode号送入ax,虽然只读入16位，但此处不影响
	sub ax,1
	mov bx,InodeSize
	mul bx		;dx:ax = ax * InodeSize
	add ax,BaseOfInodeTable	;ax->inode节点
	
	;2,将inode所在扇区读入内存
	mov dx,0
	mov bx,512
	div bx				;ax存商，dx存余数;ax为将要读的扇区号，dx为此inode在本扇区的偏移量
	push dx				;将余数存起来，

	;将第ax号扇区读入es:bx=7f00:0000处
	mov bx,es
	sub bx,100h			;int 13h 需要512对齐 	pop	b
	mov es,bx
	
	mov bx,0
	mov cl,1

	call ReadSector	;将inode所在扇区读到(BaseOfkernel-100):0000处,避免加载kernel时将其覆盖
	
	pop dx												
	add bx,dx			;es:bx指向inode
	
	
	;3,循环读入block数组，即loader.bin的内容。
	add bx,28
	mov ax,[es:bx]		;block_count读入cx	
	mov cx,ax		

	add bx,12
	mov ax,bx		;ax存放block[]数组的偏移
	; 由于变量的访问是使用ds:off，而我们下面要使用ds来寻址inode结构，所以
	; 在一下代码中不能使用变量。
	mov bp ,sp                 ;action:push= sub+mov
	sub sp,2
	mov word [bp-2],ax   ;[bp-2]用来寻址block数组

	;因为es:bx用来ReadSector，所以用ds:si操作inode，由此导致变量不能使用。
	mov ax,es
	mov ds,ax

	mov ax,BaseOfKernel
	mov es,ax

	mov bx,OffsetOfKernel
LOAD:
	;cx-=2,because read two sector one loop
	cmp cx,0
	je	LOAD_OVER
	dec cx
	dec cx
	
	mov	si,word [bp-2]
	mov ax,[ds:si]				;ax the index of this block
	mov dx,2					;bx (block) = 2*sector
	mul dx
	
	push cx
	mov cl,2
	call ReadSector	
	pop cx

	mov ax,word [bp-2]
	add ax,4
	mov word [bp-2],ax
	add bx,1024
	jmp LOAD






LOAD_OVER:
	call	KillMotor		; 关闭软驱马达
	mov	dh, 1			; "Ready."
	call	DispStr			; 显示字符串

	mov ax,cs
	mov ds,ax
	;mov es,ax

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
EntryNumInRoot	dw	32		;目前支持根目下只能有32个目录项

wSectorNo		dw	0		; 要读取的扇区号
bOdd			db	0		; 奇数还是偶数
dwKernelSize		dd	0		; KERNEL.BIN 文件大小
;============================================================================
;字符串
;----------------------------------------------------------------------------
KernelFileName		db	"kernel.bin", 0	; KERNEL.BIN 之文件名
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
	;mov	bl, [BPB_SecPerTrk]	; bl: 除数
	mov	bl,18	
	div	bl			; y 在 al 中, z 在 ah 中
	inc	ah			; z ++
	mov	cl, ah			; cl <- 起始扇区号
	mov	dh, al			; dh <- y
	shr	al, 1			; y >> 1 (其实是 y/BPB_NumHeads, 这里BPB_NumHeads=2)
	mov	ch, al			; ch <- 柱面号
	and	dh, 1			; dh & 1 = 磁头号
	pop	bx			; 恢复 bx
	; 至此, "柱面号, 起始扇区, 磁头号" 全部得到 ^^^^^^^^^^^^^^^^^^^^^^^^
;	mov	dl, [BS_DrvNum]		; 驱动器号 (0 表示 A 盘)
	mov	dl, 0		; 驱动器号 (0 表示 A 盘)
.GoOnReading:
	mov	ah, 2			; 读
	mov	al, byte [bp-2]		; 读 al 个扇区
	int	13h
	jc	.GoOnReading		; 如果读取错误 CF 会被置为 1, 这时就不停地读, 直到正确为止
	add	esp, 2
	pop	bp
	ret




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

	;load ELF OSkernel
	call InitKernel


	; jmp to  kernel(3G-4G) to test paging is or not open
	;	jmp SelectorCodeKernel:BaseOfKernelPhyAddr
	jmp SelectorCodeKernel:KernelEntryPointPhyAddr
						   
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


; InitKernel ---------------------------------------------------------------------------------
; 将 KERNEL.BIN 的内容经过整理对齐后放到新的位置
; 遍历每一个 Program Header，根据 Program Header 中的信息来确定把什么放进内存，放到什么位置，以及放多少。
; --------------------------------------------------------------------------------------------
InitKernel:
    xor   esi, esi
	mov   cx, word [BaseOfKernelPhyAddr+2Ch];`. ecx <- pELFHdr->e_phnum
	movzx ecx, cx                               ;/
	mov   esi, [BaseOfKernelPhyAddr + 1Ch]  ; esi <- pELFHdr->e_phoff
	add   esi, BaseOfKernelPhyAddr;esi<-OffsetOfKernel+pELFHdr->e_phoff
.Begin:
    mov   eax, [esi + 0]
    cmp   eax, 0                      ; PT_NULL
    jz    .NoAction
    push  dword [esi + 010h]    ;size ;`.
    mov   eax, [esi + 04h]            ; |
    add   eax, BaseOfKernelPhyAddr; | memcpy((void*)(pPHdr->p_vaddr),
    push  eax		    ;src  ; |      uchCode + pPHdr->p_offset,
    push  dword [esi + 08h]     ;dst  ; |      pPHdr->p_filesz;
    call  MemCpy                      ; |
    add   esp, 12                     ;/
.NoAction:
    add   esi, 020h                   ; esi += pELFHdr->e_phentsize
    dec   ecx
    jnz   .Begin
																																					
	 ret
; InitKernel ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	

; ------------------------------------------------------------------------
; 内存拷贝，仿 memcpy
; ------------------------------------------------------------------------
; void* MemCpy(void* es:pDest, void* ds:pSrc, int iSize);
; ------------------------------------------------------------------------
MemCpy:
	push	ebp
	mov	ebp, esp

	push	esi
	push	edi
	push	ecx

	mov	edi, [ebp + 8]	; Destination
	mov	esi, [ebp + 12]	; Source
	mov	ecx, [ebp + 16]	; Counter
.1:
	cmp	ecx, 0		; 判断计数器
	jz	.2		; 计数器为零时跳出

	mov	al, [ds:esi]		; ┓
	inc	esi					; ┃
							; ┣ 逐字节移动
	mov	byte [es:edi], al	; ┃
	inc	edi					; ┛

	dec	ecx		; 计数器减一
	jmp	.1		; 循环
.2:
	mov	eax, [ebp + 8]	; 返回值

	pop	ecx
	pop	edi
	pop	esi
	mov	esp, ebp
	pop	ebp

	ret			; 函数结束，返回
; MemCpy 结束-------------------------------------------------------------




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
