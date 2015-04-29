#ifndef _GRAMMAR_PHRASE_H_
#define _GRAMMAR_PHRASE_H_

#define VT_NR	77
#define VN_NR   68
//#define VT_N_NR VT_NR + VN_NR
#define BNF_NR 213
#define FUNC_NR 4

#define FIRST_DATA_SIZE 687
#define FOLLOW_DATA_SIZE 1105 
#define STACK_SIZE 128

//操作符
#define IS_OPERATOR(tk) (((tk) >= 26 && (tk) <= 50) ||\
		(tk) == NTS_ASSIGNMENT_OPTOR || (tk) == NTS_UNARY_OPTOR)

//16进制数
#define ishex(ch)	((ch) >= '0' && (ch) <= '9' ||\
		(ch) >= 'a' && (ch) <= 'f' ||\
		(ch) >= 'A' && (ch) <= 'F')

//8进制数
#define isoctal(ch) ((ch) >= '0' && (ch) <= '7')

#define TRUE 1
#define FALSE 0

int fun_flg = FALSE;

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
	int code; //编码
	int map_idx; //在map表首个包含此code的元素的索引
	char strname[32]; //vt/n的串名
};
//终结符/非终结符表
extern struct vtn vtn_tbl[VT_N_NR + 2];

struct bnf
{
	int right[11]; //right[0]左部，right[1-10]右部
	void (*func[FUNC_NR])(void); //规则函数
};
//规则表（产生式集合）
struct bnf bnf_set[BNF_NR] = {0};

struct first_follow
{
	int nr;
	int idx;
};
//VT_N_NR 只初始化了非终结符VN_NR个为[0, VN_NR]
//follow集
struct first_follow follow[VT_N_NR] = {0};

int follow_data[FOLLOW_DATA_SIZE] = {0};

struct first_follow first[VT_N_NR] = {0};

int first_data[FIRST_DATA_SIZE] = {0};

#define STRING_HASH_SPACE 2357
unsigned char *str_hash_tbl;

//语法树节点
struct tree_node{
	int code;
	struct tree_node *parent;
	struct tree_node *sibling;
	struct tree_node *child;

	int flag; //place的标志，0未用，1临时变量，2表示常量，3表示普通变量
	unsigned int place;

	int begin;  //标号=表索引：循环起始指令位置
	int next;   //标号=表索引：循环结束后下条指令位置
	int true;   //标号=表索引：条件判断真出口的指令位置
	int false;  //标号=表索引：条件判断假出口的指令位置

	char *name; 	//id名
	void *type; 		//类型链表头指针
	void *param_list;	//参数列表头指针
	void *member_list;	//结构体成员列表头指针
	void *enumer_list;	//枚举常量列表头指针
};

//归约栈
struct tree_node *ll_stack[STACK_SIZE] = {0};

int top = -1; //归约栈栈顶指针

int next_tk; //预取token的code值

extern int no; //行号
extern char *string;
extern int value;

struct seek{
	char ch;
	int scan;
	int next;
	long int pos;
};

int pre_scan = FALSE;
int assign_flg_stk[64];
int *af_stk_top = assign_flg_stk + 64;

//API
extern const char *get_vtn_string(int code);
extern struct tree_node *alloc_tree_node(void);
extern void init_tree_node(struct tree_node *node, int code);
extern void release_tree_node(struct tree_node *node);
extern int insert_tree_node(struct tree_node *parent,
		struct tree_node *new);
extern void del_tree_node(struct tree_node *node);
extern void del_tree(struct tree_node *root);
extern struct tree_node *get_nth_child(struct tree_node *parent, int nth);
extern void traversal_tree_node(struct tree_node *start);
#endif
