本文档用来介绍实现本工程所需的所有技术。
目录：
1.工程文件介绍
2.运行代码流程
3.Inode，Entry_Dir结构
4.调试技术
5.代码细节


1.工程文件介绍：

代码部分：boot_ext2.asm loader_ext2.asm kernel.asm
头文件	: load.inc	pm.inc
脚本程序：boot.run	load.run	kernel.run
配置文件：bochsrc
软盘	：a.img

2.运行代码流程
2.1.执行./boot.run，具体代码如下：
	nasm boot_ext2.asm -o boot_ext2.bin
	sleep 1
	dd if=boot_ext2.bin of=a.img bs=512 count=1 conv=notrunc
	他负责把boot_ext2.asm汇编成二进制文件，然后将其写入a.img的第一个扇区中。
这样，a.img就可以作为启动盘了。
2.2.执行./kernel.run
	nasm -f elf kernel.asm -o kernel.o
	ld -s -Ttext 0x30400 kernel.o -o kernel.bin
	他负责把kernel.asm汇编成elf格式的文件，然后将其链接成入口地址为0x30400
的elf文件。（这个具体过程未深究，只用知道kernel.bin是elf格式的文件）
2.3.执行./load.run，具体代码如下：
	nasm loader_ext2.asm -o loader.bin
	sudo mount -o loop a.img /mnt/floppy/
	sudo cp loader.bin /mnt/floppy/ -v
	sudo cp kernel.bin /mnt/floppy/ -v
	sleep 1
	sudo umount /mnt/floppy/
	他负责把loader_ext2.asm汇编成二进制文件loader.bin。再将a.img挂载到
/mnt/floop目录下（这样才能将文件复制进软盘a.img）。随后便将loader.bin和
kernel.bin复制进/mnt/floop中，即a.img中。最后将a.img卸载下来。
2.4.执行bochs命令，运行bochs虚拟机。


3.Inode，Entry_Dir结构
在叙述代码功能，并详细解释之前我们还需要了解一些基本知识。
首先我们必须知道ext2文件系统中的inode和目录项的结构（此处只叙述
我们需要知道的成员，与本实验无关的暂不考虑）。
inode结构
struct inode{
	_u32 unknow			;unknow代表我也不清楚此成员的含义
	_u32 size;			;
	_u32 Creat_T
	_u32 Modif_T
	_u32 Access_T
	_u32 Del_T
	_u16 unknow
	_u16 link_count
	_u32 Block_count	;记载了block×2数
	_u32 unknow
	_u32 unknow
	_u32 Block[15]		;15个索引项，包括12个一级索引，2个二级，1个三级
	_u32 Generation
	_u32 unkown[6]			;代表有6个未知项，不是数组的意思
}
	这是这是存放在ext2磁盘上的inode结构，位置也是一一对应。也就是说索引项
是从偏移量为40的地方开始，block_count距头部偏移为28。
	我们需要的成员是：Block_count和Block[15]。	
	最后我们可以看出一个inode占128字节。
目录项的结构就比较简单了
struct Entry_Dir{
	_u32 inode		;inode节点号
	_u16 rec_len	;本目录项的长度
	_u8  name_len	;目录项名的长度
	_u8  file_type  ;文件类型，常规文件或目录或链接等
	char name[]		;长度不固定。
}
	从中可以看出目录项的长度是不固定的，所以需要两个字节来存放本目录项的长度
信息。
	最后我们需要清楚ext2文件系统的格式




	有了这些知识我们就可以加载存放在ext2类型的软盘中的文件loader.bin和
kernel.bin了。

4.调试技术
	我们可以用的调试技术很简单

bochs基本调试命令：
	b 0x7c00	打断点
	blist		显示所有断点信息
	info b		同上
	del n		删除第n个断点
	c			执行到断点处
	n			单步执行，不跳入函数
	s	N		单步执行，跳入函数。N可选，表示执行N条指令
	x /12xcb addr	按字符显示地址addr处的12个字节内存信息，
	u /100		显示100行反汇编代码

dubugfs调试：
将ext2文件系统挂载上后，使用df查看挂载点，在使用debugfs+挂载点 打开此文件
系统。
	ls显示文件及其inode节点信息。 
	stat <12> 显示第12个inode节点信息
	mi <12>  同上
别忘记sudo。
最后一个需要记住的命令是：
	xxd -u -a -g l -c 16 -s +0x2800 -l 512 a.img 
这个命令可以用来查看镜像内容



5.代码细节
	最后我们来看看代码实现

	
保护模式下的栈？？

ext2文件系统格式

代码详解
首先我们先整体看一下代码流程，再进入函数仔细分析代码。（以下boot就指示boot_ext2.asm
或其二进制文件。loader，kernel同理）
代码流程：
	boot负责把loader加载到内存9000h:100h处，然后jmp到loader处
执行loader的代码。
	loader将kernel加载到8000h:0000处。然后进入保护模式，在
保护模式下设置段寄存器，启动分页机制（call SetupPaging），装载8000h:0000处ELF格式
的kernel（call InitKernel）（由于kernel是elf格式的，所以不能直接jmp到
8000h:0000处执行，要按照elf格式将其装载到正确地址处，再jmp过去
才能正确执行我们这里将其装载到物理内存的30400h处）。
	好了现在我们已经把内核加载到了正确位置，我们可以将控制权交给kernle了：
	jmp SelectorCodeKernel:KernelEntryPointPhyAddr
	这个jmp跳到了3G+30400处。为什么执行kernel代码要跳到这里
呢？我们明明只把它装载到30400h了，为什么加上3G也能正确运行呢？
这就有劳我们loader.bin中的分页机制了。看了SetupPaging自会理解。
	最后kernel仅输出一个K字符便进入死循环。



boot_ext2.asm代码详解：
	boot的代码逻辑很简单，主要分为两部分：第一部分加载根目录文件，并在其中找到loader目录项。找到目录项后就是第二步，加载其文件内容。
找loader的过程逻辑上可分为三部：
1.将根目录的block读入内存
2.比较目录项：先比较NameLen是否为10,如果是，继续比较文件名，如果否，继续下一个目录项的比较
3.如果比较了32个目录下都没有loader.bin，则输出:No Loader
	在第二步中如果比较文件名后确定是loader.bin，则跳到LABEL_FILENAME_FOUND,执行加载文件内容。加载过程
也分为三步：
1,计算loader.bin的inode的地址
2,将inode所在扇区读入8f00:0000处内存
3,循环读入block数组，即loader.bin的内容。
	加载完loader.bin后便跳入loader的代码中执行loader程序。
	至此boot结束
loader_ext2.asm详解：
	loader的过程上文已经叙述，现在来详细说明。
1.首先是加载kernel。此过程与boot加载loader过程基本一样，再次不再叙述。
2.然后就可以进入保护模式了。进入的过程比较简单，此处略过。
3.进入保护模式后由于寻址方式不同，所以要从新设置段寄存器。
4.启动分页机制。我们把页目录表放在1M--（1M+4k-1）处，页表紧随其后。我们需要
把0-1G和3G-4G的线性地址空间映射到0-1G的物理内存（我们使用的内存就是1G大小）。
由于一个页目录表便可对应1G的空间，所以我们使前256个页目录项和后256个页
目录项都指向代表前1G物理内存的页表，这样便实现了我们上面的目标。读者可以
对照代码自己分析一遍。
5.装载ELF格式的kernel。按照ELF的ProgramHeaderTable来加载各个段的内容，
ProgramHeader会指定某段应该加载到什么位置的内存。有关这部分详细信息可以
参考《一个操作系统的实现》第5.3节和5.4.3节。
6.通过jmp我们便可以跳入kernel了。看到这里读者应该能够理解loader最后jmp
到3G+30400h处为何能正确执行了，因为我们线性地址空间的0-1G和3G-4G对应的
是相同的物理空间，也就是这两部分的线性地址会转化成相同的物理地址。

	




	
	
		






