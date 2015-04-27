/*************************************************************************
    > File Name: init_idt.h
    > Author: mercy
    > Mail:  
    > Created Time: 2015年04月18日 星期六 18时29分09秒
 ************************************************************************/


/* klib.asm */
PUBLIC void	out_byte(u16 port, u8 value);
PUBLIC u8	in_byte(u16 port);
PUBLIC void	disp_str(char * info);
PUBLIC void	disp_color_str(char * info, int color);
PUBLIC void	init_prot();
PUBLIC void	init_8259A();


