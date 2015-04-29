#ifndef _CC_H_
#define _CC_H_

struct vtn
{
	int code; //编码
	int map_idx; //在map表首个包含此code的元素的索引
	char strname[32]; //vt/n的串名
};

#define VT_NR	76
#define VN_NR   68
#define VT_N_NR VT_NR + VN_NR
extern struct vtn vtn_tbl[VT_N_NR + 2];

extern const char *get_vtn_string(int code);
extern int get_next_token(void);
extern void init_lex(char *file);
extern void release_lex(void);
extern void init_gen(char *file);
extern void release_gen(void);
extern int grammar(void);
#endif
