/*************************************************************************
    > File Name: global.h
    > Author: mercy
	*/
#ifdef GLOBAL_VAR_HERE
#undef	EXTERN
#define EXTERN
#endif
//上面的定义使得这些变量定义在global.c中，而在其他.c中都是extern声明

EXTERN int disp_pos;

EXTERN u8	gdt_ptr[6];	//0~15  limiit ; 16~47 Base
EXTERN DESCRIPTOR	gdt[GDT_SIZE];		//new GDT

EXTERN u8	idt_ptr[6];
EXTERN GATE idt[IDT_SIZE];





