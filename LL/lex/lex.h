#ifndef _LEX_H_
#define _LEX_H_

#define END_OF_FILE -1
#define MAP_NR 706

enum {
	ZERO = 0,
#define TSYM(tk, s)	tk,
#include "../tsym.h"
#undef TSYM
#define NONTSYM(tk, s) tk,
#include "../nontsym.h"
#undef NONTSYM
	VT_N_NR
};

struct vtn
{
	int code; 				//编码
	int map_idx; 			//map表首个包含此code的元素的索引
	char strname[32]; 		//vt/n的串名
};

/*
	终结符/非终结符表
 */
struct vtn vtn_tbl[VT_N_NR + 2] = {
	{0, 0, "NULL"},
#define TSYM(tk, s) {tk, 0, s},
#include "../tsym.h"
#undef TSYM
#define NONTSYM(tk, s) {tk, 0, s},
#include "../nontsym.h"
#undef NONTSYM
	{VT_N_NR, MAP_NR, "map_nr"}
};

/*
	记录输入文件的信息
 */
struct input
{
	char ch;			//当前扫描的字符
    char *filename;		//文件名
    FILE *fp;
};
struct input input;

int value; 					//词法解析出的数值值
char *string = NULL; 		//词法解析的id字符串
int no = 1; 				//行号

/*
	字符串hash节点，hash表中一项
	存储程序的字符串中的信息
 */
struct string_hash_node {
	char *name;			//存储的字符串指针
	struct string_hash_node *next;
};

/*
	词法字符串堆结构
 */
struct string_heap{
	char *base;
	int size;			//存储的字符串个数
	struct string_heap *next;
};

#endif
