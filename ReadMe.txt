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

boot_ext2.asm代码详解：
	boot的代码逻辑很简单，主要分为两部分：第一部分加载根目录文件，并在其中找到loader目录项。
找loader的过程逻辑上可分为三部：
1.将根目录的block读入内存
2.比较目录项：先比较NameLen是否为10,如果是，继续比较文件名，如果否，继续下一个目录项的比较
3.如果比较了32个目录下都没有loader.bin，则输出:No Loader
	在第二步中如果比较文件名后确定是loader.bin，则跳到LABEL_FILENAME_FOUND,执行加载过程。加载过程
也分为三步：
1,计算loader.bin的inode的地址
2,将inode所在扇区读入8f00:0000处内存
3,循环读入block数组，即loader.bin的内容。
	加载完loader.bin后便跳入loader的代码中执行loader程序。
	至此boot结束
boot_ext2.asm详解：

	
	




首先是boot_ext2.asm
这是加载到启动扇区的代码，他的作用就仅仅是加载loader.bin,
