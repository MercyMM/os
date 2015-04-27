/*************************************************************************
    > File Name: start.c
    > Author: mercy

*/
#include "type.h"
#include "protect.h"
#include "const.h"
#include "global.h"
#include "string.h"




void cstart(){

	disp_str("\n\n\n\n\n\n\n\n\n\n\n\n"
			"-----\"cstart\" start-----");
	MemCpy(&gdt,						//dst
			(void*)(*(u32*)(&gdt_ptr[2])),	//src
			*((u16*)(&gdt_ptr[0]))+1				//size
			);
	u16* p_gdt_limit = (u16*)(gdt_ptr);
	u32* p_gdt_base	 = (u32*)(&gdt_ptr[2]);
	*p_gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;
	*p_gdt_base  = (u32)(&gdt);
	

	disp_str("\n-----\"cstart\" ends-----\n");

//	return 0;
}
