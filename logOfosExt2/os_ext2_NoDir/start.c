/*************************************************************************
    > File Name: stat.c
    > Author: mercy
    > Mail:  
    > Created Time: 2015年04月22日 星期三 10时17分14秒
 ************************************************************************/
#define GDT_SIZE	128

typedef unsigned int	u32;
typedef unsigned short	u16;
typedef unsigned char	u8;


typedef struct s_descriptor{
	u16	limit_low;
	u16	base_low;
	u8	base_mid;
	u8	attr1;
	u8	limit_high_attr2;
	u8	base_high;
}DESCRIPTOR;

u8 	gdt_ptr[6];
DESCRIPTOR	gdt[GDT_SIZE];
int disp_pos;


void *Memcpy(void* p_dst,void* p_src,int size);

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



}

