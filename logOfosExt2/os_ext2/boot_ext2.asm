

org  07c00h			; Boot 状态, Bios 将把 Boot Sector 加载到 0:7C00 处并开始执行

BaseOfStack		equ	07c00h	; Boot状态下堆栈基地址(栈底, 从这个位置向低地址生长)

BaseOfLoader		equ	09000h	; LOADER.BIN 被加载到的位置 ----  段地址
OffsetOfLoader		equ	0100h	; LOADER.BIN 被加载到的位置 ---- 偏移地址

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

;================================================================================================
;================================================================================================

LABEL_START:	
	mov	ax, cs
	mov	ds, ax
	mov	es, ax
	mov	ss, ax
	mov	sp, BaseOfStack

	; 清屏
	mov	ax, 0600h		; AH = 6,  AL = 0h
	mov	bx, 0700h		; 黑底白字(BL = 07h)
	mov	cx, 0			; 左上角: (0, 0)
	mov	dx, 0184fh		; 右下角: (80, 50)
	int	10h			; int 10h

	mov	dh, 0			; "Booting  "
	call	DispStr			; 显示字符串
	
	xor	ah, ah	; ┓
	xor	dl, dl	; ┣ 软驱复位
	int	13h	; ┛
	
; 在根目录下寻找loader.bin
;	根目录的block在8400处，即第33块block。

;过程：
; 1.将根目录的block读入内存
; 2.比较目录项：先比较NameLen是否为10,如果是，继续比较文件名，如果否，继续下一个目录项的比较
; 3.如果比较了32个目录下都没有loader.bin，则输出:No Loader

	mov	word [wSectorNo], SectorNoOfRootDirectory
LABEL_SEARCH_IN_ROOT_DIR_BEGIN:

	mov	ax, BaseOfLoader
	mov	es, ax			; es <- BaseOfLoader
	mov	bx, OffsetOfLoader	; bx <- OffsetOfLoader	于是, es:bx = BaseOfLoader:OffsetOfLoader
	mov	ax, [wSectorNo]	; ax <- Root Directory 中的某 Sector 号
	mov	cl, 2
	call	ReadSector  ;需要四个参数:ax，cl，es:bx（es:bx需要512=200h对齐）。从第ax个扇区开始，将cl个扇区读入es:bx指向的内存

	mov	si, LoaderFileName	; ds:si -> "LOADER  BIN"
	mov	di, OffsetOfLoader	; es:di -> BaseOfLoader:OffsetOfLoader = 9000h*10h+100h
	cld
	
	mov dx,di	;每次开始循环，dx指向目录项头，di指向目录项名
	add di,DirEntryNameOffset

LABEL_SEARCH_FOR_LOADERBIN:	
	cmp word [EntryNumInRoot],0
	jz	LABEL_NO_LOADERBIN
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
	dec	cx
	jz	LABEL_FILENAME_FOUND	; 如果比较了 11 个字符都相等, 表示找到
	lodsb				; ds:si -> al加载字符串常量“loader.bin”
	cmp	al, byte [es:di];与目录项名字进行比较
	jz	LABEL_GO_ON
	jmp	LABEL_DIFFERENT		; 只要发现不一样的字符就表明本 DirectoryEntry 不是
; 我们要找的 LOADER.BINctxt
LABEL_GO_ON:
	inc	di
	jmp	LABEL_CMP_FILENAME	;	继续循环

LABEL_DIFFERENT:
	mov	si, LoaderFileName          ;si指向“loader.bin”字符串常量
	mov bx,dx
	add bx,DirEntryRecLenOffset
	add dx, [es:bx]			;dx 指向下一个目录项开头，di=dx+8指向目录项的name开头
	mov di,dx
	add di,DirEntryNameOffset
	jmp	LABEL_SEARCH_FOR_LOADERBIN	;    


LABEL_NO_LOADERBIN:
	mov	dh, 2			; "No LOADER."
	call	DispStr			; 显示字符串

;执行到此处后es:di指向loader.bin。es:dx指向其目录项开头

;找到loader.bin的目录项后，加载其Inode节点，进而加载其Block
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;加载loader.bin过程
	;1,计算loader.bin的inode的地址
	;2,将inode所在扇区读入8f00:0000处内存
	;3,循环读入block数组，即loader.bin的内容。

LABEL_FILENAME_FOUND:
	;1,计算loader.bin的inode的地址
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
	div bx				;ax存商，dx存余数
	push dx

	;将第ax号扇区读入，并将ax设置成指向loader.bin的inode处
	mov bx,es
	sub bx,100h			;int 13h 需要512对齐 
	mov es,bx
	
	mov bx,0
	mov cl,1

	call ReadSector	;将inode所在扇区读到(BaseOfLoader-100):0000处,避免加载loader时将其覆盖
	
	pop dx												
	add bx,dx			;es:bx指向inode
	
	
	;3,循环读入block数组，即loader.bin的内容。
	add bx,28
	mov ax,[es:bx]		;block_count读入cx	
	mov cx,ax		

	add bx,12
	mov ax,bx

	; 由于变量的访问是使用ds:off，而我们下面要使用ds来寻址inode结构，所以
	; 在一下代码中不能使用变量。
	mov bp ,sp                 ;action:push= sub+mov
	sub sp,2
	mov word [bp-2],ax   ;[bp-2]用来寻址block数组

	;use ds:si to find inode.because es:bx should be used to readsector
	mov ax,es
	mov ds,ax

	mov ax,BaseOfLoader
	mov es,ax

	mov bx,OffsetOfLoader
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
	jmp BaseOfLoader:OffsetOfLoader 
	




;============================================================================
;变量
;-------------------------------7ca0: (                    ): ---------------------------------------------
EntryNumInRoot	dw	32		;目前支持根目下只能有32个目录项
wSectorNo		dw	0		; 要读取的扇区号
bOdd			db	0		; 奇数还是偶数

Inode_Block		dw  0		;在加载loader.bin时 用于inode中block数组的定位
;============================================================================
;字符串
;----------------------------------------------------------------------------
LoaderFileName		db	"loader.bin", 0	; LOADER.BIN 之文件名
; 为简化代码, 下面每个字符串的长度均为 MessageLength
MessageLength		equ	9
BootMessage:		db	"Booting  "; 9字节, 不够则用空格补齐. 序号 0
Message1		db	"Ready.   "; 9字节, 不够则用空格补齐. 序号 1
Message2		db	"No LOADER"; 9字节, 不够则用空格补齐. 序号 2
;============================================================================


;----------------------------------------------------------------------------
; 函数名: DispStr
;----------------------------------------------------------------------------
; 作用:
;	显示一个字符串, 函数开始时 dh 中应该是字符串序号(0-based)
DispStr:
	mov	ax, MessageLength
	mul	dh
	add	ax, BootMessage
	mov	bp, ax			; ┓
	mov	ax, ds			; ┣ ES:BP = 串地址
	mov	es, ax			; ┛
	mov	cx, MessageLength	; CX = 串长度
	mov	ax, 01301h		; AH = 13,  AL = 01h
	mov	bx, 0007h		; 页号为0(BH = 0) 黑底白字(BL = 07h)
	mov	dl, 0
	int	10h			; int 10h
	ret


;----------------------------------------------------------------------------
; 函数名: ReadSector
;----------------------------------------------------------------------------
; 作用:
;	从第 ax 个 Sector 开始, 将 cl 个 Sector 读入 es:bx 中
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
;	mov	bl, [BPB_SecPerTrk]	; bl: 除数
	mov	bl,18 	; bl: 除数
	div	bl			; y 在 al 中, z 在 ah 中
	inc	ah			; z ++
	mov	cl, ah			; cl <- 起始扇区号
	mov	dh, al			; dh <- y
	shr	al, 1			; y >> 1 (其实是 y/BPB_NumHeads, 这里BPB_NumHeads=2)
	mov	ch, al			; ch <- 柱面号
	and	dh, 1			; dh & 1 = 磁头号
	pop	bx			; 恢复 bx
	; 至此, "柱面号, 起始扇区, 磁头号" 全部得到 ^^^^^^^^^^^^^^^^^^^^^^^^
	;mov	dl, [BS_DrvNum]		; 驱动器号 (0 表示 A 盘)
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

times 	510-($-$$)	db	0	; 填充剩下的空间，使生成的二进制代码恰好为512字节
dw 	0xaa55				; 结束标志
