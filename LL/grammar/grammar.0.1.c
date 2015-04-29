#include <stdio.h> 
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include "grammar.h"
#include "ll.h"

#define TEST_OUTPUT() printf("%s\n", __func__)
#define ENTER() printf("\n")

#define TRUE 1
#define FALSE 0

int fun_flg = FALSE;

//void output_product(struct bnf *set);
//查询符号表，目前只是测试使用
void *search_tb(void *name)
{
	return name;
}

//测试输出，将code转换成可读字符串
void test(int code)
{
	switch(code) {
		case TS_STR:
		case TS_ID:
			if(string) {
				printf("%s  ", string);
			}
			else
				printf("%s  ", vtn_tbl[code].strname);
			break;
		case TS_CONSTANT:
			printf("%d  ", value); 
			break;
		default:
			printf("%s  ", vtn_tbl[code].strname);

	}
}

//即‘匹配’和‘跳过’token
void skip(int tk)
{
	if(scan != tk) 
		output_error("token is not match\n");
	scan = next;
	if(scan == 0)
		exit(0);
	next = get_next_token();
	if(next == -1)
		next = 0;
	printf("\n#: ");
	test(scan);
	test(next);
	printf("\n");

}

static unsigned int JSHash(char *str)
{
	unsigned int hash = 1315423911;
	while (*str) {
		hash ^= ((hash << 5) + (*str++) + (hash >> 2));
	}
	return (hash & 0x7FFFFFFF);
}

unsigned char *create_str_hash_tbl(void)
{
	int i;
	unsigned char *tbl;
	tbl = malloc(STRING_HASH_SPACE);
	if (tbl == NULL) {
		fprintf(stderr, "Error: malloc fail!\n");
		return NULL;
	}
	for (i = 1; i <= VT_N_NR; ++i)
		tbl[JSHash(vtn_tbl[i].strname) % STRING_HASH_SPACE] =
			vtn_tbl[i].code;
	return tbl;
}

void destroy_str_hash_tbl(void *addr)
{
	free(addr);
}

//初始化follow基数据数组
int init_follow_data(char *file)
{
	int fd;
	int i = 0;
	int tk_idx = 0;
	int f_idx = 0;
	off_t length;
	unsigned char *addr;
	char token[32] = { 0 };
	fd = open(file, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Error:Open file %s fail\n", file);
		return -1;
	}
	length = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	addr = mmap(NULL, length, PROT_READ, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED) {
		fprintf(stderr, "Error:mmap file %s fail\n", file);
		close(fd);
		return -2;
	}
	while (i < length) {
		while (isspace(addr[i]) && i < length) {
			++i;
		}
		if (i >= length)
			break;
		if (addr[i] == '$') {
			++i;
			memset(token, 0, 32);
			tk_idx = 0;
			continue;
		}
		while (!isspace(addr[i]) && i < length) {
			token[tk_idx++] = addr[i++];
		}
		if (strncmp(token, "#", 1) == 0)
			follow_data[f_idx++] = 0;
		else
			follow_data[f_idx++] = str_hash_tbl[JSHash(token)
												% STRING_HASH_SPACE];
	}
	munmap(addr, length);
	close(fd);
	follow_data[f_idx] = VT_N_NR;
	return 0;
}

//初始化first基数据数组
int init_first_data(char *file)
{
	int fd;
	int i = 0;
	int tk_idx = 0;
	int f_idx = 0;
	off_t length;
	unsigned char *addr;
	char token[32] = { 0 };
	fd = open(file, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Error:Open file %s fail\n", file);
		return -1;
	}
	length = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	addr = mmap(NULL, length, PROT_READ, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED) {
		fprintf(stderr, "Error:mmap file %s fail\n", file);
		close(fd);
		return -2;
	}
	while (i < length) {
		while (isspace(addr[i]) && i < length) {
			++i;
		}
		if (i >= length)
			break;
		if (addr[i] == '$') {
			++i;
			memset(token, 0, 32);
			tk_idx = 0;
			continue;
		}
		while (!isspace(addr[i]) && i < length) {
			token[tk_idx++] = addr[i++];
		}
		if (strncmp(token, "#", 1) == 0)
			first_data[f_idx++] = 0;
		else
			first_data[f_idx++] = str_hash_tbl[JSHash(token)
												% STRING_HASH_SPACE];
	}
	munmap(addr, length);
	close(fd);
	first_data[f_idx] = VT_N_NR;
	return 0;
}

void swap_int(int *a, int *b)
{
	int tmp;
	if (a == b)
		return;
	tmp = *a;
	*a = *b;
	*b = tmp;
}

int partition_int(int *data, int low, int high)
{
	int privotKey = data[low];
	while (low < high) {
		while (low < high && data[high] >= privotKey)
			--high;
		swap_int(data + low, data + high);
		while (low < high && data[low] <= privotKey)
			++low;
		swap_int(data + low, data + high);
	}
	return low;
}

//快速排序，用于排序follow集和first集
void sort(int *data, int low, int high)
{
	int privot;
	if (low < high) {
		privot = partition_int(data, low, high);
		sort(data, low, privot - 1);
		sort(data, privot + 1, high);
	}
}

//对非终结符的follow集依照编码大小进行排序，排序目的就是为了后文
//is_follow()能够使用折半查找
void deal_follow_data(void)
{
	int curr;
	int i;
	int nr;
	curr = 0;
	nr = 0;
	for (i = 0; i <= FOLLOW_DATA_SIZE; ++i) {
		if (follow_data[i] > VT_NR) {
			follow_data[curr] = nr;
			sort(follow_data, curr + 1, i - 1);
			curr = i;
			nr = 0;
		} else
			++nr;
	}
}

//对非终结符的first集依照编码大小进行排序，排序目的就是为了后文
//is_first()能够使用折半查找
void deal_first_data(void)
{
	int curr;
	int i;
	int nr;
	curr = 0;
	nr = 0;
	for (i = 0; i <= FIRST_DATA_SIZE; ++i) {
		if (first_data[i] > VT_NR) {
			first_data[curr] = nr;
			sort(first_data, curr + 1, i - 1);
			curr = i;
			nr = 0;
		} else
			++nr;
	}
}

void init_follow(void)
{
	int i, j;
	int code;
	for (i = j = 0, code = 0; code < VN_NR; ++code, ++i, ++j) {
		follow[code].nr = follow_data[i];
		follow[code].idx = i + 1;
		i += follow_data[i];
	}
}

void init_first(void)
{
	int i, j;
	int code;
	for (i = j = 0, code = 0; code < VN_NR; ++code, ++i, ++j) {
		first[code].nr = first_data[i];
		first[code].idx = i + 1;
		i += first_data[i];
	}
}

static inline int is_follow(int vn_code, int vt_code)
{
	int *f;
	int start, end, mid;
	int nr;
	f = follow_data + follow[vn_code - VT_NR - 1].idx;
	nr = follow[vn_code - VT_NR - 1].nr;
	if (nr > 5) {
		start = 0;
		end = nr - 1;
		while (end >= start) {
			mid = (start + end) >> 1;
			if (f[mid] == vt_code)
				return 1;
			if (f[mid] > vt_code)
				end = mid - 1;
			else
				start = mid + 1;
		}
		if (end < start)
			return 0;
	} else {
		for (end = nr; end; --end, ++f) {
			if (*f == vt_code)
				return 1;
		}
		return 0;
	}
}

static inline int is_first(int vn_code, int vt_code)
{
	int *f;
	int start, end, mid;
	int nr;
	f = first_data + first[vn_code - VT_NR - 1].idx;
	nr = first[vn_code - VT_NR - 1].nr;
	if (nr > 5) {
		start = 0;
		end = nr - 1;
		while (end >= start) {
			mid = (start + end) >> 1;
			if (f[mid] == vt_code)
				return 1;
			if (f[mid] > vt_code)
				end = mid - 1;
			else
				start = mid + 1;
		}
		if (end < start)
			return 0;
	} else {
		for (end = nr; end; --end, ++f) {
			if (*f == vt_code)
				return 1;
		}
		return 0;
	}
}


/*expression:
	 assignment_expression                  
	| expression ,  assignment_expression*/

void expression()
{
	TEST_OUTPUT();
	assignment_expression();
	while(scan == TS_COMMA) {
		skip(TS_COMMA);
		assignment_expression();
	}

	if(is_follow(NTS_EXP, scan)) 
		return;
	else
		output_error("在expression中出错");
}

//用于assignment_expression()中的分支选择，遇到如下终结符
//要选择走conditional_expression()
int is_roll_back(int op)
{
	switch(op) {
		case TS_QUESTION:
		case TS_OR:
		case TS_AND:
		case TS_BITOR:
		case TS_BITXOR:
		case TS_BITAND:
		case TS_EQ:
		case TS_NOT_EQ:
		case TS_LESS:
		case TS_LESS_EQ:
		case TS_GREATER:
		case TS_GREATER_EQ:
		case TS_LSHIFT:
		case TS_RSHIFT:
		case TS_ADD:
		case TS_SUB:
		case TS_MUL:
		case TS_MOD:
		case TS_DIV:
		case TS_LPAREN:
			return TRUE;
		default:
			return FALSE;
	}
}

#define is_condition_exp(scan)	is_roll_back(scan)

/*assignment_expression:
	conditional_expression  //1  conditional_expression的部分语言以unary_expression开头
	| unary_expression   assignment_operator   assignment_expression  //2
	*/
void assignment_expression()
{
	struct seek old;

	TEST_OUTPUT();
	mark(&old, scan, next); //打标志：使用局部变量保存预扫描起点

	if(is_first(NTS_UNARY_EXP, scan)) { //二者first基一样
		if(is_condition_exp(next) == TRUE || scan == TS_LPAREN)
			//预扫描下一个不是赋值运算符，或者当前扫描是‘(’，则执行conditional_expression()
			//只所以加上'('是因为解决函数调用和其他运算符的结合，比如fun(i) + 10
			conditional_expression();
		else if(unary_expression() == TRUE && assignment_operator() == TRUE){
			//unary_expression() assignment_operator()
			//执行成功，则继续执行，否则执行下面if语句
			assignment_expression( );
		}
		else if(is_roll_back(scan) == TRUE){
			roll_back(&old, &scan, &next);
			//回滚：恢复预扫描的起点，走另外一条产生式
			conditional_expression();
		}
		//运行到这里，则不一定是语法错误，比如 a[i] = 10;
		//使用产生式 direct_declarator [ assignment_expression ] 
		//其中assignment_expression()调用，scan是']'就运行到这里
	}
	else
		output_error("在assignment_expression中出错");

	if(is_follow(NTS_ASSIGNMENT_EXP, scan)) 
		return;
	else
		output_error("在assignment_expression中出错");
}

//判断运算符op是不是赋值运算符
//用于assignment_operator()和cast_expression()中
static int is_assignment_op(int op)
{
	switch(op) {
		case TS_ASSIGN:
		case TS_ADD_ASSIGN:    	
		case TS_SUB_ASSIGN:    	
		case TS_MUL_ASSIGN:    	
		case TS_DIV_ASSIGN:    	
		case TS_MOD_ASSIGN:    	
		case TS_LSHIFT_ASSIGN:
		case TS_RSHIFT_ASSIGN: 	
		case TS_BITAND_ASSIGN: 	
		case TS_BITXOR_ASSIGN:  
		case TS_BITOR_ASSIGN: 	
			return TRUE;
		default:
			return FALSE;
	}
}

/*
assignment_operator:  
	=  *=  /=  %=  +=  _=  <<=  >>=  &=  ^=  |=
*/

int assignment_operator()
{
	TEST_OUTPUT();
	if(is_assignment_op(scan) == TRUE) {
		skip(scan);
		return TRUE;
	}
	else
		return FALSE;
}

/*constant_expression: 
		conditional_expression*/

void constant_expression()
{
	TEST_OUTPUT();
	conditional_expression();

	//这里加上TS_COMMA即','的原因是：使用产生式
	// enumeration_constant  = constant_expression
	//展开时，调用constant_expression()执行到这里，此时scan是','，而constant_expression
	//的follow基中没有','，无法退出此函数。因此或上','使得enum常量能正常运行
	if(scan == TS_COMMA || is_follow(NTS_CONST_EXP, scan)) 
		return;
	else
		output_error("在constant_expression中出错");
}

/*conditional_expression: 
	logical_OR_expression  //1
	| logical_OR_expression  ?  expression  :  conditional_expression  //2
	*/

void conditional_expression()
{
	TEST_OUTPUT();
	logical_OR_expression();
	if(is_follow(NTS_COND_EXP, scan)) 
		return;

	if(scan == TS_QUESTION) {
		skip(TS_QUESTION);
		expression();
	}
	else
		output_error("在conditional_expression中缺失'？'");

	if(scan == TS_COLON) {
		skip(TS_COLON);
		while(is_first(NTS_COND_EXP, scan)) {
			logical_OR_expression();
			if(is_follow(NTS_COND_EXP, scan)) 
				break;//2展开结束

			if(scan == TS_QUESTION) {
				skip(TS_QUESTION);
				expression();
			}
			else
				output_error("在conditional_expression中缺失'？'");
		}
	}
	else
		output_error("在conditional_expression中缺失'：'");

	if(is_follow(NTS_COND_EXP, scan)) 
		return;
	else
		output_error("在conditional_expression中出错");
}

/*logical_OR_expression: 
		logical_AND_expression
	| logical_OR_expression  ||  logical_AND_expression */

void logical_OR_expression()
{
	TEST_OUTPUT();
	logical_AND_expression();

	while(scan == TS_OR) {
		skip(TS_OR);
		logical_AND_expression();
	}

	if(is_follow(NTS_OR_EXP, scan)) 
		return;
	else
		output_error("在logical_OR_expression中出错");
}

/*logical_AND_expression: 
		inclusive_OR_expression
	| logical_AND_expression  &&  inclusive_OR_expression  //2*/

void logical_AND_expression()
{
	TEST_OUTPUT();
	inclusive_OR_expression();

	while(scan == TS_AND) {
		skip(TS_AND);
		inclusive_OR_expression();
	}

	if(is_follow(NTS_AND_EXP, scan)) 
		return;		
	else
		output_error("在logical_AND_expression中出错");
}

/*inclusive_OR_expression: 
	exclusive_OR_expression
	| inclusive_OR_expression  |  exclusive_OR_expression  //2*/

void inclusive_OR_expression()
{
	TEST_OUTPUT();
	exclusive_OR_expression();

	while(scan == TS_BITOR) {
		skip(TS_BITOR);
		exclusive_OR_expression();
	}

	if(is_follow(NTS_BITOR_EXP, scan)) 
		return;		
	else
		output_error("在inclusive_OR_expression中出错");
}

/*	exclusive_OR_expression: 
		AND_expression 
		| exclusive_OR_expression  ^  AND_expression  //2*/

void exclusive_OR_expression()
{
	TEST_OUTPUT();
	AND_expression();

	while(scan == TS_BITXOR) {
		skip(TS_BITXOR);
		AND_expression();
	}

	if(is_follow(NTS_XOR_EXP, scan)) 
		return;		
	else
		output_error("在exclusive_OR_expression中出错");
}

/*AND_expression:
		 equality_expression 
		| AND_expression  &  equality_expression  //2*/

void AND_expression()
{
	TEST_OUTPUT();
	equality_expression();

	while(scan == TS_BITAND) {
		skip(TS_BITAND);
		equality_expression();
	}

	if(is_follow(NTS_BITAND_EXP, scan)) 
		return;		
	else
		output_error("在AND_expression中出错");
}

/*equality_expression: 
	relational_expression
	| equality_expression  = =  relational_expression  //2
	| equality_expression  !=  relational_expression  //3*/

void equality_expression()
{
	TEST_OUTPUT();
	relational_expression();

	while(scan == TS_EQ || scan == TS_NOT_EQ) {
		skip(scan);
		relational_expression();
	}

	if(is_follow(NTS_EQ_EXP, scan)) 
		return;		
	else
		output_error("在equality_expression中出错");
}

/*relational_expression: 
	shift_expression
	| relational_expression  <  shift_expression  //2
	| relational_expression  >  shift_expression  //3
	| relational_expression  <=  shift_expression  //4
	| relational_expression  >=  shift_expression  //5*/

void relational_expression()
{
	TEST_OUTPUT();
	shift_expression();

	while(scan == TS_LESS || scan == TS_GREATER || 
			scan == TS_LESS_EQ || scan == TS_GREATER_EQ) {
		skip(scan);
		shift_expression();
	}

	if(is_follow(NTS_RELAT_EXP, scan)) 
		return;		
	else
		output_error("在relational_expression中出错");
}

/*shift_expression: 
	additive_expression
	| shift_expression  <<  additive_expression  //2
	| shift_expression  >>  additive_expression  //3*/

void shift_expression()
{
	TEST_OUTPUT();
	additive_expression();

	while(scan == TS_LSHIFT || scan == TS_RSHIFT) {
		skip(scan);
		additive_expression();
	}

	if(is_follow(NTS_SHIFT_EXP, scan)) 
		return;		
	else
		output_error("在shift_expression中出错");
}

/*additive_expression: 
	multiplicative_expression
	| additive_expression  +  multiplicative_expression  //2
	| additive_expression  _  multiplicative_expression  //3*/

void additive_expression()
{
	TEST_OUTPUT();
	multiplicative_expression();		
	while(scan == TS_ADD || scan == TS_SUB) {
		skip(scan);
		multiplicative_expression();

	}

	if(is_follow(NTS_ADD_EXP, scan)) 
		return;		
	else
		output_error("在additive _expression中出错");
}

/*multiplicative_expression: 
	cast_expression
	| multiplicative_expression  *  cast_expression  //2
	| multiplicative_expression  /  cast_expression  //3
	| multiplicative_expression  %  cast_expression  //4*/

void multiplicative_expression()
{
	TEST_OUTPUT();
	cast_expression();

	while(scan == TS_MUL || scan == TS_DIV || scan == TS_MOD) {
		skip(scan);
		cast_expression();
	}

	if(is_follow(NTS_MUL_EXP, scan)) 
		return;		
	else
		output_error("在multiplicative _expression中出错");
}


/*cast_expression: 
		unary_expression 
		| (  type_name  )  cast_expression  //强制类型转换*/
	
void cast_expression()
{
	TEST_OUTPUT();
	while(is_first(NTS_UNARY_EXP, scan) || scan == TS_LPAREN) {
		//first(unary_exp)中包含'(', 解决办法：使用next判断展开分支
		//此外，first(type-name)和first(unary_exp)都包含ID，即typedef的ID
		//需要查询符号表解决
		if(is_first(NTS_UNARY_EXP, scan) && scan != TS_LPAREN) {
			unary_expression();
		}

		if(scan == TS_LPAREN) {
			if(is_first(NTS_TP_NAME, next) && next != TS_ID) {
				//这里为了测试不查询符号表，当ID存在括号内，直接转到unary_exp
				skip(TS_LPAREN);
				type_name();
				skip(TS_RPAREN);
			}
			else
				unary_expression();
		}
	}
	
	//这里或上is_assignment_op的原因是cast_exp的follow基中没有‘=’等赋值符 导致
	// 类似*a = *b;这样的赋值语句无法继续；*a使用产生式
	//unary_operator cast_expression 因此在退出cast_expression时候，scan为‘=’
	//导致判断follow基失败而出错。
	if(is_follow(NTS_CST_EXP, scan) || is_assignment_op(scan) == TRUE) 
		return;
	else
		output_error("在cast_expression中出错");

}

//判断终结符是不是类型
//用于unary_expression()中选择type_name分支
static int is_type(int scan)
{
	switch(scan) {
		case TS_VOID:
		case TS_SIGNED:
		case TS_UNSIGNED:
		case TS_CHAR:
		case TS_SHORT:
		case TS_INT:
		case TS_LONG:
		case TS_FLOAT:
		case TS_DOUBLE:
		case TS_STRUCT:
		case TS_UNION:
		case TS_ENUM:
			return TRUE;
		default:
			return FALSE;
	}
}

/*unary_expression:                     //一元运算
	postfix_expression//1
	| ++  unary_expression//2
	| --  unary_expression //3
	| unary_operator  cast_expression//4
	| sizeof  unary_expression//5
	| sizeof  (  type_name  )//6*/
int unary_expression()
{
	TEST_OUTPUT();
	struct seek old;

	if(is_first(NTS_POSTFIX_EXP, scan)) {
		postfix_expression();
	}
	else if(scan == TS_BITAND || scan == TS_MUL ||
			scan == TS_ADD || scan == TS_SUB ||
			scan == TS_COMPL || scan == TS_NOT) {
		skip(scan);
		cast_expression();
	}
	else if(scan == TS_SIZEOF && next == TS_LPAREN) {
		skip(TS_SIZEOF); 
		//此时scan = '('
		if(next == TS_ID || is_type(next) == FALSE) { //这里还需要查符号表，决定ID是不是typedef
			unary_expression();
		}
		else {
			skip(TS_LPAREN);
			type_name();
			skip(TS_RPAREN); 
		}
	}

	if(is_follow(NTS_UNARY_EXP, scan)) 
		return TRUE; //这里告知assignment_expression()执行成功
	
	while (scan == TS_INC || scan == TS_DEC || scan == TS_SIZEOF) {
		skip(scan); 
		if(is_first(NTS_POSTFIX_EXP, scan)) {
			postfix_expression();
		}
		else if(scan == TS_BITAND || scan == TS_MUL ||
				scan == TS_ADD || scan == TS_SUB ||
				scan == TS_COMPL) {
			skip(scan);
			cast_expression();
		}
		else if(scan == TS_SIZEOF && next == TS_LPAREN) {
			skip(TS_SIZEOF);
			//此时scan = '('
			if(next == TS_ID) { //这里还需要查符号表，决定ID是不是typedef
				unary_expression();
			}
			else {
				skip(TS_LPAREN);
				type_name();
				skip(TS_RPAREN); 
			}
		}
	}

	if(is_follow(NTS_UNARY_EXP, scan)) 
		return TRUE;
	else
		printf("unary_expression之后的字符出错");
	return FALSE;//这里告知assignment_expression()执行失败
}

/*postfix_expression:
	 primary_expression 
	| postfix_expression  [  expression  ]  //2
	| postfix_expression  (  )  //3
	| postfix_expression  (  argument_expression_list  )  //4
	| postfix_expression  .  identifier  //5
	| postfix_expression  _>  identifier //6
	| postfix_expression  ++ //7
	| postfix_expression  __ //8*/

void postfix_expression()
{
	TEST_OUTPUT();
	primary_expression();

	while(scan == TS_LBRAKET || scan == TS_LPAREN ||
			scan == TS_DOT || scan == TS_POINTER ||
			scan == TS_INC || scan == TS_DEC) {
		switch(scan){
			case TS_LBRAKET: //'[': //展开2
				skip(scan);
				expression();
				skip(TS_RBRAKET);
				break;

			case TS_LPAREN: //'('：
				skip(scan);
				if(is_first(NTS_ARGUMENT_EXP_L, scan))
					argument_expression_list();
				skip(TS_RPAREN);
					break; 
			case  TS_DOT: //'.': //展开5
				skip(scan);
				if(scan == TS_ID) {
					skip(TS_ID);
					break;
				}
				else
					output_error("在postfix_expression中_>.期望ID");
				break;
			case  TS_POINTER://'->': //展开6
				skip(scan);
				if(scan == TS_ID) {
					skip(TS_ID);
					break;
				}
				else
					output_error("在postfix_expression中_>.期望ID");
				break;
			case TS_INC: //'++'：//展开7
				skip(scan);
				break;
			case TS_DEC://'--'：//展开8
				skip(scan);
				break;
		}//switch
	}//while

	if(is_follow(NTS_POSTFIX_EXP, scan)) 
		return;
	else
		output_error("在postfix_expression中出错");
}

/*argument_expression_list:          
	assignment_expression
	| argument_expression_list  ,  assignment_expression  //参数表达式表*/

void argument_expression_list()
{
	TEST_OUTPUT();
	assignment_expression();
	while(scan == TS_COMMA) {
		skip(TS_COMMA);
		assignment_expression();
	}
	if(is_follow(NTS_ARGUMENT_EXP_L, scan)) 
		return;
	else
		output_error("在argument_expression_list中出错");
}
	
/*primary_expression: 
	identifier  
	| constant
	| string_literal 
	|  (  expression  )*/

void primary_expression()
{
	TEST_OUTPUT();
	if(scan == TS_LPAREN) {
		skip(TS_LPAREN);
		expression();
		skip(TS_RPAREN);
	}
	else if(scan == TS_ID || scan == TS_CONSTANT || scan == TS_STR) 
		skip(scan);

	if(is_follow(NTS_PRIMARY_EXP, scan)) 
		return;
	else
		output_error("在primary_expression中出错");
}


/* declaration_list:
	declaration 
	| declaration_list  declaration*/

void declaration_list(void)
{
	TEST_OUTPUT();
	if(scan == TS_ASSIGN) {
		skip(TS_ASSIGN);
		initializer();
	} 
	else if(scan == TS_COMMA) {
		skip(TS_COMMA); //
		init_declarator_list();
	}
	else if(scan == TS_SEMICOLON)
		skip(TS_SEMICOLON);
	else
		output_error("error: in declaration_list wrong");

	while(is_first(NTS_DECLON, scan) && fun_flg == FALSE)
		declaration();

	//skip(TS_SEMICOLON);

	if(is_follow(NTS_DECLON_L, scan))
		return;
	else  
		output_error("error: in declaration_list结束时出错");
}

/* declaration_specifiers:
	storage_class_specifier 
	| storage_class_specifier  declaration_specifiers
	| type_specifier
	| type_specifier  declaration_specifiers*/

void declaration_specifiers()
{
	TEST_OUTPUT();

	if(is_first(NTS_STORE_CLASS_SPF, scan)) 
		storage_class_specifier();
	else if(is_first(NTS_TP_SPF,scan)) 
		type_specifier();

	while(is_first(NTS_TP_SPF, scan) || 
			is_first(NTS_STORE_CLASS_SPF, scan)) {
		if(is_first(NTS_TP_SPF, scan)) {
			if(scan == TS_ID) //这里还需要查询符号表，以确定它是不是typedef
				break;
			type_specifier();
		}
		if(is_first(NTS_STORE_CLASS_SPF, scan)) 
			storage_class_specifier( );
	}

	if(is_follow(NTS_DECLON_SPF, scan)) 
		return;
	else 
		output_error("类型定义说明declaration_specifiers的开始符号错");
}

/* storage_class_specifier 
   typedef  |  extern  |  static*/

void storage_class_specifier( ) 
{
	TEST_OUTPUT();
	if ((scan == TS_TYPEDEF || scan == TS_EXTERN || scan == TS_STATIC)
			&& is_follow(NTS_STORE_CLASS_SPF, scan)) {
		skip(scan);
		return;
	}
	else
		output_error("error: in storage_class_specifier程序出错");
}

/*declaration:                                  //声明  
  declaration_specifiers ; 
  | declaration_specifiers init_declarator_list ;*/

void declaration()
{
	TEST_OUTPUT();
	struct seek old;

	fun_flg = FALSE;
	mark(&old, scan, next);

	declaration_specifiers();
	if(scan == TS_SEMICOLON && is_follow(NTS_DECLON, next)) {
		skip(TS_SEMICOLON);
		return;
	}
	else if(is_first(NTS_INIT_DECLOR_L,scan)) {
		init_declarator_list();
		if(scan == TS_SEMICOLON && is_follow(NTS_DECLON, next)) {
			skip(TS_SEMICOLON);
			return;
		}
		else if(scan == TS_LBRACE) {
			roll_back(&old, &scan, &next);
			fun_flg = TRUE;
			return;
		}
			output_error("init_declarator_list没有以结束';',出错");	
	}
	else
		output_error("declaration声明没有以结束';'出错");
}

/*type_specifier:
  void  |  char  |  short  |  int  |  long  |  float  |  double  |
  signed  |  unsigned                  //情形0
  | struct_specifier                    //情形1
  | union_specifier                    //情形2
  | enum_specifier                    //情形3
  | typedef_name                     //情形4 */

void type_specifier()
{
	TEST_OUTPUT();
	switch (scan) {
		case TS_VOID: 
		case TS_CHAR:
		case TS_SHORT: 
		case TS_INT:
		case TS_LONG:
		case TS_SIGNED:             //情形0
		case TS_UNSIGNED:             //情形0
		case TS_FLOAT:
		case TS_DOUBLE:
			skip(scan);
			break;
		case TS_STRUCT:        //情形1
			struct_specifier();
			break;
		case TS_UNION:        //情形2
			union_specifier();
			break;
		case TS_ENUM:        //情形3
			enum_specifier();
			break;
		case TS_ID:       //情形4
			//skip(scan);
			typedef_name();
			break;
		default:
			output_error("error: in type_specifier程序出错");
	}
	if(is_follow(NTS_TP_SPF, scan)) 
		return;
	else
		output_error("error: in type_specifier程序出错");
}

/*struct_specifier:                                //结构体声明
  struct  identifier  {  struct_declaration_list  }  //产生式0
  | struct  {  struct_declaration_list  }          //产生式1
  | struct  identifier                          //产生式2*/
void struct_specifier()
{
	TEST_OUTPUT();
	skip(TS_STRUCT);	
	if (scan == TS_ID || scan == TS_LBRACE) { 
		if(scan == TS_ID)
			skip(TS_ID);
		if (scan == TS_LBRACE) {
			skip(TS_LBRACE);
			struct_declaration_list();
			skip(TS_RBRACE);
		} 
	}
	else
		output_error("error in struct_specifier");

	if(is_follow(NTS_STRUCT_SPF, scan)) 
		return;
	else
		output_error("error: in struct_specifier程序出错");
}

/*union_specifier:                                //结构体声明
  union  identifier  {  struct_declaration_list  }  //产生式0
  | union  {  struct_declaration_list  }          //产生式1
  | union  identifier                          //产生式2*/

void union_specifier()
{
	TEST_OUTPUT();
	skip(TS_UNION);

	if (scan == TS_ID || scan == TS_LBRACE) {
		if(scan == TS_ID)
			skip(TS_ID);
		if (scan == TS_LBRACE) {
			skip(TS_LBRACE);
			struct_declaration_list();
			skip(TS_RBRACE);
		} 
	}
	else
		output_error("error in union_specifier");

	if(is_follow(NTS_UNION_SPF, scan)) 
		return;
	else
		output_error("error: in union_specifier程序出错");
}

/*struct_declaration_list:
  struct_declaration
  | struct_declaration_list  struct_declaration*/
void struct_declaration_list() 
{
	TEST_OUTPUT();
	struct_declaration();
	while (is_first(NTS_STRUCT_DECLON,scan)) 
		struct_declaration( );
	if(is_follow(NTS_STRUCT_DECLON_L, scan)) 
		return;
	else
		output_error("error: in struct_declaration_list程序出错");
}

/*struct_declaration:
  specifier_qualifier_list  struct_declarator_list  ;*/

void struct_declaration() 
{
	TEST_OUTPUT();
	specifier_qualifier_list();

	if(!is_first(NTS_STRUCT_DECLOR_L,scan)) 
		output_error("error: in struct_declaration程序出错");

	struct_declarator_list();
	if (scan == TS_SEMICOLON && is_follow(NTS_STRUCT_DECLON, next)) {
		skip(TS_SEMICOLON);
		return;
	}
	else
		output_error("error: in struct_declaration_list程序出错");
}

/*specifier_qualifier_list:
  type_specifier 
  | type_specifier  specifier_qualifier_list*/

void specifier_qualifier_list() 
{
	TEST_OUTPUT();
	while (is_first(NTS_TP_SPF, scan)) {
		if(scan == TS_ID && !search_tb(NULL))
			//查询id是不是typedef类型,不是返回NULL
			break;
		type_specifier();
	}

	if(is_follow(NTS_SPF_QUAL_L, scan)) 
		return;
	else
		output_error("error: in specifier_qualifier_list程序出错");
}

/*struct_declarator_list:
  declarator
  | struct_declarator_list  ,  declarator*/
void struct_declarator_list() 
{
	TEST_OUTPUT();
	declarator();
	while (scan == TS_COMMA)
		declarator();
	if(is_follow(NTS_STRUCT_DECLOR_L, scan)) 
		return;
	else
		output_error("error: in struct_declarator_list程序出错");
}

/*enum_specifier:
  enum  {  enumerator_list  }            //产生式0
  | enum  identifier  {  enumerator_list  }  //产生式1
  | enum  identifier                      //产生式2*/

void enum_specifier( ) 
{
	TEST_OUTPUT();
	skip(TS_ENUM);

	if (scan == TS_ID || scan == TS_LBRACE) {
		if(scan == TS_ID)
			skip(TS_ID);
		if (scan == TS_LBRACE) {
			skip(TS_LBRACE);
			enumerator_list();
			skip(TS_RBRACE);
		} 
	}
	else
		output_error("error in enum_specifier");

	if(is_follow(NTS_ENUM_SPF, scan)) 
		return;
	else
		output_error("error: in enum_specifier程序出错");
}

/*enumerator_list:
	enumerator
	| enumerator_list  ,  enumerator*/
void enumerator_list()
{
	TEST_OUTPUT();
	enumerator();
	while (scan == TS_COMMA) {
		skip(TS_COMMA);
		enumerator();
	}
	if(is_follow(NTS_ENUMOR_L, scan)) 
		return;
	else
		output_error("error: in enumerator_list程序出错");
}

/*enumerator:
	enumeration_constant
	| enumeration_constant  =  constant_expression*/
void enumerator() 
{
	TEST_OUTPUT();
	enumeration_constant();
	if (scan == TS_ASSIGN) {
		skip(TS_ASSIGN);
		constant_expression();
		if(is_follow(NTS_ENUMOR, scan)) 
			return;
		else
			output_error("error: in enumerator程序出错");
	}
	else if(is_follow(NTS_ENUMOR, scan)) 
		return;
	else
		output_error("error: in enumerator程序出错");
}

/*enumeration_constant:  identifier*/

void enumeration_constant() 
{
	TEST_OUTPUT();
	if (scan == TS_ID && is_follow(NTS_ENUMON_CONST, next)) {
		skip(TS_ID);
		return;
	}
	else
		output_error("error: in enumeration_constant程序出错");
}

/*typedef_name:  identifier*/
void typedef_name() 
{
	TEST_OUTPUT();

	if (scan == TS_ID && is_follow(NTS_TYPEDEF_NAME, next)) {
		skip(TS_ID);
		return;
	}
	else
		output_error("error: in typedef_name程序出错");
}

/*init_declarator_list:
	init_declarator
	| init_declarator_list , init_declarator*/

void init_declarator_list()
{
	TEST_OUTPUT();
	init_declarator();
	while(scan == TS_COMMA) {
		skip(TS_COMMA);
		init_declarator();
	}

	if(is_follow(NTS_INIT_DECLOR_L, scan) || scan == TS_LBRACE)
		return;
	else  
		output_error("error: in init_declarator_list中出错");
}

/*init_declarator:
	declarator
	| declarator = initializer */ 

void init_declarator()
{
	TEST_OUTPUT();
	declarator();

	if ( scan == TS_ASSIGN) {
		skip(TS_ASSIGN);
		initializer();
	}

	if(is_follow(NTS_INIT_DECLOR, scan) || scan == TS_LBRACE) 
		return;
	else  
		output_error("error: in init_declarator中出错");
}

/*declarator:
	direct_declarator
	| pointer  direct_declarator*/

void declarator() 
{
	TEST_OUTPUT();
	if(is_first(NTS_POINTER,scan)) {
		pointer();
		if(is_first(NTS_DIRECT_DECLOR,scan)) 
			direct_declarator();
		if(is_follow(NTS_DECLOR, scan)) 
			return;
		else
			output_error("error: in declarator程序出错");
	}
	else if(is_first(NTS_DIRECT_DECLOR,scan)) {
		direct_declarator();
		if(is_follow(NTS_DECLOR, scan)) 
			return;
		else
			output_error("error: in declarator程序出错");
	}
	else
		output_error("error: in declarator程序出错");
}

/*pointer:
	* 
	| *  pointer*/

void pointer() 
{
	TEST_OUTPUT();
	while (scan == TS_MUL) {
		skip(TS_MUL);	
	}
	if(is_follow(NTS_POINTER, scan)) 
		return;
	else
		output_error("error: in pointer程序出错");
}

/*direct_declarator:                         
	identifier                             //id
	| ( declarator )                          //(id)
	| direct_declarator [ assignment_expression ]  // [ ]串用循环识别,左递归可以不改写
	| direct_declarator [  ]
	| direct_declarator ( parameter_type_list )  
	| direct_declarator (  )*/

void direct_declarator( )
{
	TEST_OUTPUT();

	if(scan == TS_ID) 
		skip(TS_ID);
	else if( scan == TS_LPAREN) {
		skip(TS_LPAREN);
		declarator( ) ;
		skip(TS_RPAREN);
	}
	else
		output_error("error in direct_declarator");
		
	while(scan == TS_LBRAKET || scan == TS_LPAREN) {
		if(scan == TS_LBRAKET) {
			skip(TS_LBRAKET);
			if(is_first(NTS_ASSIGNMENT_EXP, scan))
				assignment_expression();

			if (scan == TS_RBRAKET)
				skip(TS_RBRAKET);
			else
				output_error("数组定义、说明出错");
		}
		else {
			skip(TS_LPAREN);
			if(is_first(NTS_PRMT_TP_L, scan)) 
				parameter_type_list ( );

			if(scan == TS_RPAREN) 
				skip(TS_RPAREN);
			else
				output_error("函数定义、说明出错");
		}
	}  
	return;
}

/*parameter_type_list
	parameter_list 
	| parameter_list  ,  ...    //暂不支持*/
void parameter_type_list() 
{
	TEST_OUTPUT();
	parameter_list();
	if (scan == TS_COMMA) {
		skip(TS_COMMA);
		skip(TS_ELLIPSE);
	} 
	else if(is_follow(NTS_PRMT_TP_L, scan)) 
		return;
	else
		output_error("error: in parameter_type_list程序出错");
}

/*parameter_list:
	parameter_declaration
	| parameter_list  ,  parameter_declaration*/
void parameter_list() 
{
	TEST_OUTPUT();
	parameter_declaration();
	while (scan == TS_COMMA) {
		skip(TS_COMMA);
		parameter_declaration();
	}

	if(is_follow(NTS_PRMT_L, scan)) 
		return;
	else
		output_error("error: in parameter_list程序出错");
}

/*parameter_declaration:
	declaration_specifiers  declarator              //基本类型 
	| declaration_specifiers
	| declaration_specifiers  abstract_declarator*/
void parameter_declaration() 
{
	TEST_OUTPUT();
	declaration_specifiers();

	if(is_follow(NTS_PRMT_DECLON, scan))
		return;

	if (scan == TS_ID)
		declarator();
	else if (scan == TS_LBRAKET)
		abstract_declarator();
	else {
		struct seek old;
		mark(&old, scan, next);

		while (scan == TS_LPAREN || scan == TS_MUL)
			skip(scan);
		//search_tb的只是为了测试，表示查询符号表，仅仅返回参数；
		if (scan == TS_ID) {
			roll_back(&old, &scan, &next);

			declarator();
		}
		else if (scan == TS_LBRAKET || scan == TS_RPAREN || search_tb(NULL)) {
			roll_back(&old, &scan, &next);

			abstract_declarator();
		}
		else
			output_error("error: in parameter_list程序出错");
	}


	if(is_follow(NTS_PRMT_DECLON, scan))
		return;
	else
		output_error("error: in parameter_list程序出错");
}

/*type_name:
	specifier_qualifier_list
	| specifier_qualifier_list  abstract_declarator*/
void type_name() 
{
	TEST_OUTPUT();
	specifier_qualifier_list();
	if(is_first(NTS_ABSTRACT_DECLOR,scan)) {
		abstract_declarator();
		if(is_follow(NTS_TP_NAME, scan)) 
			return;
		else
			output_error("error: in type_name程序出错");
	}
	else if(is_follow(NTS_TP_NAME, scan)) 
		return;
	else
		output_error("error: in type_name程序出错");
}

/*abstract_declarator:
	pointer
	| direct_abstract_declarator
	| pointer  direct_abstract_declarator*/
void abstract_declarator() 
{
	TEST_OUTPUT();
	if(is_first(NTS_POINTER,scan)) {
		pointer();
		if(is_first(NTS_DIRECT_ABSTRACT_DECLOR, scan)) 
			direct_abstract_declarator();
		if(is_follow(NTS_ABSTRACT_DECLOR, scan)) 
			return;
		else
			output_error("error: in abstract_declarator程序出错");
	}
	else if(is_first(NTS_DIRECT_ABSTRACT_DECLOR,scan)) {
		direct_abstract_declarator();
		if(is_follow(NTS_ABSTRACT_DECLOR, scan)) 
			return;
		else
			output_error("error: in abstract_declarator程序出错");
	} 
	else
		output_error("error: in abstract_declarator程序出错");
}

/*direct_abstract_declarator:
	(  abstract_declarator  )
	| direct_abstract_declarator  [  assignment_expression  ]
	| direct_abstract_declarator  [  ]
	| [  assignment_expression  ]
	| [  ]
	| direct_abstract_declarator  [  *  ]
	| [  *  ]
	| direct_abstract_declarator  (  parameter_type_list  )
	| direct_abstract_declarator  (  )
	| (  parameter_type_list  )
	| (  )*/

void direct_abstract_declarator() 
{
	TEST_OUTPUT();
	if (scan  == TS_LPAREN) {
		skip(scan);
		if(is_first(NTS_ABSTRACT_DECLOR,scan)) 
			abstract_declarator();
		else if(is_first(NTS_PRMT_TP_L,scan)) 
			parameter_type_list();
		skip(TS_RPAREN);
	} 
	else if (scan == TS_LBRAKET) {
start_lbraket_loop:
		skip(scan);
		if (scan == TS_MUL) {
			if (next_tk != TS_RBRACE)
				assignment_expression();
		}
		skip(TS_RBRAKET);
	} 
	else
		output_error("error: in direct_abstract_declarator程序出错");
	if (scan == TS_LBRAKET)
		goto start_lbraket_loop;
	while (scan == TS_LBRAKET) {
		skip(scan);
		if(is_first(NTS_PRMT_TP_L,scan)) {
			parameter_type_list();
			skip(TS_RPAREN);
		}
		if(is_follow(NTS_DIRECT_ABSTRACT_DECLOR, scan)) 
			return;
		else
			output_error("error: in direct_abstract_declarator程序出错");
	}
}

/*
initializer:
	assignment_expression
	| {  initializer_list  }
	| {  initializer_list  ,  }*/

//函数结构：
void initializer( ) 
{
	TEST_OUTPUT();
	if(scan == TS_LBRACE) {
		skip(TS_LBRACE);
		initializer_list( );
		if (scan == TS_COMMA)
			skip(TS_COMMA);
		skip(TS_RBRACE);
	}
	else
		assignment_expression( );

	if(is_follow(NTS_INITER, scan))
		return;
	else
		output_error("在initializer程序出错1");
}

/*initializer_list: 
	initializer
	| designation  initializer
	| initializer_list  ,  initializer
	| initializer_list  ,  designation initializer*/

void initializer_list( ) 
{
	TEST_OUTPUT();
	if(is_first(NTS_DESIGON, scan))
		designation( );
	initializer( );
	while (scan == TS_COMMA) {
		skip(TS_COMMA);
		if(is_first(NTS_DESIGON, scan))
			designation( );
		initializer( );
	}
	if (is_follow(NTS_INITER_L, scan))
		return;
	else
		output_error("在initializer_list程序出错");
}

/*designation:
	designator_list  =*/

void designation( ) 
{
	TEST_OUTPUT();
	designator_list( );
	if (scan  == TS_ASSIGN ) {// && is_follow(NTS_DESIGON, scan)) {
		skip(TS_ASSIGN);
		return;
	}
	else
		output_error("在designation程序出错");
}

/*designator_list:
	designator 
	| designator_list  designator*/

void designator_list( ) 
{
	TEST_OUTPUT();
	designator( );
	while (is_first(NTS_DESIGOR, scan))
		designator( );
	if (is_follow(NTS_DESIGOR_L, scan))
		return;
	else
		output_error("在designator_list程序出错");
}

/*designator:
	[  constant_expression  ] 
	| .  identifier*/

void designator( ) 
{
	TEST_OUTPUT();
	if (scan == TS_LBRAKET) {
		skip(TS_LBRAKET);
		constant_expression( );
		if (scan == TS_RBRAKET)
			skip(TS_RBRAKET);
		else
			output_error("在designator程序出错");
	} 
	else if (scan == TS_DOT) {
		skip(TS_DOT);
		if (scan == TS_ID)
			skip(TS_ID);
		else
			output_error("在designator程序出错");
	} 
	else
		output_error("在designator程序出错");

	if (is_follow(NTS_DESIGOR, scan))
		return;
	else
		output_error("在designator程序出错");
}

/*statement	
	labeled_statement 
	| compound_statement 
	| expression_statement 
	| selection_statement
	| iteration_statement 
	| jump_statement*/

void statement()
{
	TEST_OUTPUT();
	//first(l_stm)和first(e_stm)冲突元素：ID
	//解决办法：分支到 l_stm需要判断next是不是'：'
	if(is_first(NTS_LAB_STM, scan) && next == TS_COLON) 
		labeled_statement();
	else if(is_first(NTS_COMP_STM, scan)) 
		compound_statement();
	else if(is_first(NTS_EXP_STM, scan))
		expression_statement();
	else if(is_first(NTS_SELECT_STM, scan))
		selection_statement();
	else if(is_first(NTS_ITER_STM, scan))
		iteration_statement();
	else if(is_first(NTS_JMP_STM, scan))
		jump_statement();
	else
		output_error("在statement中出错");
}

/*labeled_statement:　　　　　　　　　　　　//标签
	identifier : statement	//goto*/

void labeled_statement()
{
	TEST_OUTPUT();
	if(scan == TS_ID && next == TS_COLON) {
		skip(TS_ID);
		skip(TS_COLON);
		statement();
	}
	else
		output_error("在labeled_statement中出错");
}

/*compound_statement:
	{ }
	| { block_item_list }*/

void compound_statement()
{
	TEST_OUTPUT();
	if(scan == TS_LBRACE) {
		skip(TS_LBRACE);
		if(scan == TS_RBRACE && is_follow(NTS_COMP_STM, next)) {
			skip(TS_RBRACE);
			return;
		}
		block_item_list();
		if(scan == TS_RBRACE && (is_follow(NTS_COMP_STM, next) || next
					== 0)) {
		  
			skip(TS_RBRACE);
			return;
		}
		else
			output_error("在compound_statement中出错");
	}
	else
		output_error("在compound_statement中出错");
}

/*block_item_list:
	block_item
	| block_item_list  block_item*/

void block_item_list()
{
	TEST_OUTPUT();
	block_item();
	while (is_first(NTS_BLOCK_ITEM, scan))
		block_item();
	if(is_follow(NTS_BLOCK_ITEM_L, scan)) 
		return;
	else
		output_error("在block_item_list中出错");
}

/*block_item:　　　　　　　　　　　　　　//复合语句块
	declaration
	| statement*/

void block_item()
{
	int flag = FALSE;
	TEST_OUTPUT();
	//first(declaration)和first(statement)冲突元素：ID
	//在declaration中如果第一个标识符就是ID,则此ID是个typedef
	//解决办法：查符号表,确定ID是不是typedef从而选择分支
/*	if(scan == TS_ID)
		flag = search_tb(NULL);//falg返回空,则展开 declaration否则,展开
		statement*/
	if(scan == TS_ID) {
		if(next == TS_ID)
			declaration();
		else
			statement();
	}
	else {
		if(is_first(NTS_DECLON, scan)) {
			declaration();
		}
		else if(is_first(NTS_STM, scan))
			statement();
		else
			output_error("在block_item起始字符出错");
	}
	if(is_follow(NTS_BLOCK_ITEM, scan)) 
		return;
	else
		output_error("在block_item中出错");
}

/*expression_statement:
	 | ;
	 | expression  ;*/

void expression_statement()
{
	TEST_OUTPUT();
	if(scan == TS_SEMICOLON) {
		skip(TS_SEMICOLON);
		return;
	}
	if(is_first(NTS_EXP_STM, scan))
		expression();
	if(scan == TS_SEMICOLON && is_follow(NTS_EXP_STM, next)) {
		skip(TS_SEMICOLON);
		return;
	}
	else
		output_error("在expression_statement中出错");
}

/*selection_statement:　　　　　　　　　　//选择语句
	if (  expression  )  statement
	if (  expression  )  statement  else  statement
	switch (  expression  )  {  case_block  }*/

void selection_statement()
{
	TEST_OUTPUT();
	if(scan == TS_IF && next == TS_LPAREN) {
		skip(TS_IF);
		skip(TS_LPAREN);
		expression();
		if(scan == TS_RPAREN) {
			skip(TS_RPAREN);
			statement();
			if(scan != TS_ELSE && is_follow(NTS_SELECT_STM, scan)) 
				return;
			else if(scan == TS_ELSE) {
				skip(TS_ELSE);
				statement();
			}
			else
				output_error("在selection_statement中出错");
		}
	}else if(scan == TS_SWITCH && next == TS_LPAREN) {
		skip(TS_SWITCH);
		skip(TS_LPAREN);
		expression();
		if(scan == TS_RPAREN) {
			skip(TS_RPAREN);
			if(scan == TS_LBRACE) {
				skip(TS_LBRACE);
				case_block();
			}
			if(scan == TS_RBRACE)
				skip(TS_RBRACE);
			else
				output_error("在selection_statement中{}不skip(");
		}
	}
	else
		output_error("在selection_statement中出错");

	if(is_follow(NTS_SELECT_STM, scan)) 
		return;
	else
		output_error("在selection_statement中出错");
}

/*case_block:
	case_labeled_statement_list
	| case_labeled_statement_list  default  :  statement */

void case_block()
{
	TEST_OUTPUT();
	case_labeled_statement_list();
	if(scan == TS_DEFAULT) {
		skip(TS_DEFAULT);
		skip(TS_COLON);
		statement();
	}

	if(is_follow(NTS_CS_BLOCK, scan)) 
		return;
	else
		output_error("在case_block中出错");
}

/*case_label_statement_list:
	case_labeled_statement
	| case_labeled_statement_list  case_labeled_statement*/

void case_labeled_statement_list()
{
	TEST_OUTPUT();
	case_labeled_statement();
	while(is_first(NTS_CS_LAB_STM, scan))
		case_labeled_statement();

	if(is_follow(NTS_CS_LAB_STM_L, scan)) 
		return;
	else
		output_error("在case_label_statement_list中出错");
}

/*case_labeled_statement:
	case  constant_expression  :  statement*/

void case_labeled_statement()
{
	TEST_OUTPUT();
	skip(TS_CASE);
	constant_expression();
	skip(TS_COLON);
	statement();

	if(is_follow(NTS_CS_LAB_STM, scan)) 
		return;
	else
		output_error("在case_label_statement中出错");
}

/*iteration_statement:　　　　　　　　　　//循环语句
	while  (  expression  )  statement
	| do  statement  while (  expression  )  ;
	| for (  expression  ;  expression  ;  expression  )  statement
	| for (  ;  expression  ;  expression  )  statement
	| for ( expression  ;   ;  expression  )  statement
	| for ( expression  ;  expression  ;  )  statement
	| for ( expression  ;   ;  )  statement
	| for (  ;  expression  ;   )  statement
	| for (  ;  ;  expression )  statement
	| for (  ;   ;   )  statement*/

void iteration_statement()
{
	TEST_OUTPUT();
	if(scan == TS_WHILE) {
		skip(TS_WHILE);
		skip(TS_LPAREN);
		expression();
		skip(TS_RPAREN);
		statement();
	}
	else if(scan == TS_DO) {
		skip(TS_DO);
		statement();
		skip(TS_WHILE);
		skip(TS_LPAREN);
		expression();
		skip(TS_RPAREN);
		skip(TS_SEMICOLON);
	}
	else if(scan == TS_FOR) {
		int times = 2;
		skip(TS_FOR);
		skip(TS_LPAREN);
		while(1){
			if(is_first(NTS_EXP, scan))
				expression();
			if(!times)
				break;
			else
				--times;
			if(scan == TS_SEMICOLON)
				skip(TS_SEMICOLON);
			else
				output_error("在for语句中出错");
		}
		skip(TS_RPAREN);
		statement();
	}
	else
		output_error("在iteration_statement语句中出现不是while,do,for终结符");

	if(is_follow(NTS_ITER_STM, scan)) 
		return;
	else
		output_error("在iteration_statement中出错");
}

/*jump_statement:　　　　　　　　　　　//跳转语句
	goto  identifier  ; 
	| continue  ;
	| break  ; 
	| return  ; 
	| return  expression  ; */

void jump_statement()
{
	TEST_OUTPUT();
	if(scan == TS_GOTO && next == TS_ID) {
		skip(TS_GOTO);
		skip(TS_ID);
		if(scan == TS_SEMICOLON && is_follow(NTS_JMP_STM, next)) {
			skip(TS_SEMICOLON);
			return;

		}
	}

	if(scan == TS_CONTINUE || scan == TS_BREAK) {
		skip(scan);
		if(scan == TS_SEMICOLON && is_follow(NTS_JMP_STM, next)) {
			skip(TS_SEMICOLON);
			return;
		}
	}

	if(scan == TS_RETURN) {
		skip(TS_RETURN);
		if(is_first(NTS_EXP, scan))
			expression();
		if(scan == TS_SEMICOLON && is_follow(NTS_JMP_STM, next)) {
			skip(TS_SEMICOLON);
			return;
		}
	}
	else
		output_error("在jump_statement中出错");
}

void translation_unit()
{
	TEST_OUTPUT();
	declaration_specifiers ( );
	if(scan == TS_SEMICOLON) {
		declaration_list( );
		if(scan == -1)
			return;//程序结束编译
		else{
			declaration_specifiers( );
			declarator();
			function_definition_list( );
			if(scan == -1)
				return;//程序结束编译
			else
				output_error("function_definition_list结束后程序应该结束,出错");
		}
	} 

	declarator();
	if(scan == TS_COMMA || scan == TS_ASSIGN || scan == TS_SEMICOLON) {
		declaration_list( );
		if(scan == -1)
			return;//程序结束编译
		else{
			declaration_specifiers( );
			declarator( );
			function_definition_list( );
			if(scan == -1)
				return;//程序编译结束
			else
				output_error("function_definition_list之后,程序应该结束出错");
		}
	}

	function_definition_list( );
	if(scan == -1)
		return;//程序编译结束
	else
		output_error("function_definition_list之后,程序应该结束出错");
}

/*function_definition_list:
	function_definition
	| function_definition_list  function_definition

为了编程改写,左公因子超越规则被提前：
function_definition_list:
	function_definition
	| function_definition  declaration_specifiers  declarator   function_definition_list
*/
void function_definition_list( )
{
	TEST_OUTPUT();
	function_definition( );
	while(is_first(NTS_DECLON_SPF, scan)) {
		declaration_specifiers( );
		declarator( );
		function_definition();
	}
}
/*
function_definition:
declaration_specifiers   declarator  compound_statement

为了编程改写,左公因子超越规则被提前：
function_definition:
compound_statement
*/
void function_definition()
{
	TEST_OUTPUT();
	compound_statement();
}

int LL(void)
{
	TEST_OUTPUT();
	scan = get_next_token();
	next = get_next_token();
	//declaration_list();
	translation_unit();
}

int grammar(void)
{
	int ret;
	str_hash_tbl = create_str_hash_tbl();
	if (str_hash_tbl == NULL)
		return 0;

	ret = init_follow_data("./grammar/follow.txt");
	if (ret) {
		destroy_str_hash_tbl(str_hash_tbl);
		return -1;
	}
	deal_follow_data();

	ret = init_first_data("./grammar/first.txt");
	if (ret) {
		destroy_str_hash_tbl(str_hash_tbl);
		return -1;
	}
	deal_first_data();

	destroy_str_hash_tbl(str_hash_tbl);

	init_follow();
	init_first();

	LL();

	return ret;
}
