#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include "error.h"
#include "lex.h"

//字符串堆的大小
#define STRING_HEAP_SIZE 1024
//字符串堆
struct string_heap str_heap;

//字符串hash表大小
#define STRING_HASH_TABLE_SIZE 512
//字符串hash表
struct string_hash_node str_hash_table[STRING_HASH_TABLE_SIZE];

//哈希表散列函数
unsigned int DJBHash(char *str)
{
	unsigned int hash = 5381;

	while (*str) {
		hash += (hash << 5) + (*str++);
	}

	return (hash & 0x7FFFFFFF);
}

//查询字符串哈希表，如果找到返回字符串在内存的首址，否则返回NULL
char *search_get_string_table(char *name)
{
	unsigned int idx;
	struct string_hash_node *nd;

	idx = DJBHash(name) % STRING_HASH_TABLE_SIZE;
	nd = str_hash_table + idx;
	while(nd && nd->name) {
		if(strcmp(name, nd->name) == 0)
			return nd->name;
		nd = nd->next;
	}
	return NULL;
}

//将新获得的字符串放入符号表，不负责查询，因此要先执行search_get_string_table
void put_string_table(char *name)
{

	unsigned int idx;
	struct string_hash_node *nd;

	idx = DJBHash(name) % STRING_HASH_TABLE_SIZE;
	nd = str_hash_table + idx;
	//未冲突
	if(nd->name == NULL) {
		nd->name = name;
		return;
	}
	//冲突，挂载链表
	while(nd->next)
		nd = nd->next;
	//分配新的字符串节点结构
	nd->next = malloc(sizeof(struct string_hash_node));
	if(nd->next == NULL)
		exit(-1);
	//加入hash表
	nd = nd->next;
	nd->name = name;
	nd->next = NULL;
}

//初始化一个新的字符内存堆，可以存储size个字符
void init_string_heap(struct string_heap *str_heap, int size)
{
	str_heap->base = malloc(size);
	if(str_heap->base == NULL)
		exit(-1);
	memset(str_heap->base, 0, size);
	str_heap->size = size;
	str_heap->next = NULL;
}

//字符串内存分配函数
char *malloc_str(unsigned int size)
{
	struct string_heap *heap;

	heap = &str_heap;
	//链表末尾结点是未满结点
	while(heap->next)
		heap = heap->next;

	heap->size -= size + 1;
	if(heap->size < 0) { //不够，分配新字符内存堆
		heap->next = malloc(sizeof(struct string_heap));
		if(heap->next == NULL)
			return NULL;
		heap = heap->next;
		init_string_heap(heap, STRING_HEAP_SIZE);
		heap->size -= size + 1;
		return heap->base + heap->size;
	}
	//分配
	return heap->base + heap->size;
}

//返回字符串指针
int return_string(char *word, int size)
{
	string = search_get_string_table(word);
	if(string == NULL) { //哈希表中不存在此串
		string = malloc_str(size);//分配内存，拷贝
		if(string == NULL)
			return ERR_MEM;
		strncpy(string, word, size);
		string[size] = 0;
		put_string_table(string);//放入哈希表
	}
	//哈希表中存在此串
	return 0;
}

//释放字符内存堆
void release_string_heap(void)
{
	struct string_heap *heap, *tmp;

	heap = str_heap.next;
	while(heap) {//遍历逐个释放
		free(heap->base);	//释放字符串
		tmp = heap;
		heap = heap->next;
		free(tmp);			//释放堆节点
	}
	free(str_heap.base);
}

//获取编码对应的字符串名字
const char *get_vtn_string(int code)
{
	return vtn_tbl[code].strname;
}


//读取一个字符
static inline char fgetch(FILE *fp)
{
	char ch;
	ch = fgetc(fp);
	if(ch == '\n')
		++no;
	return ch;
}

//放回一个字符
static inline void ungetch(char ch, FILE *fp)
{
	if(ch == '\n')
		--no;
	ungetc(ch, fp);
}

struct seek{
	char ch;
	int scan;
	int next;
	long int pos;
};

void roll_back(struct seek *seek, int *scan, int *next)
{
	input.ch = seek->ch;
	*scan = seek->scan;
	*next = seek->next;
	fseek(input.fp, seek->pos, SEEK_SET);
}

void mark(struct seek *seek, int scan, int next)
{
	seek->ch = input.ch;
	seek->scan = scan;
	seek->next = next;
	seek->pos = ftell(input.fp);
}
//打开源程序文件，初始化input结构实例
void read_source(char *filename)
{
	FILE *fp;

	input.filename = filename;
	fp = fopen(filename, "r");
	if(fp == NULL) {
		perror("error:");
		exit(0);
	}
	input.fp = fp;
	input.ch = fgetch(fp);
}

//关闭打开的源程序文件
void close_source(void)
{
	fclose(input.fp);
}

//转换十六进制字符为十进制数值
static int hex(char ch)
{
	if ((ch >= '0') && (ch <= '9'))
		return ch - '0';
	if ((ch >= 'a') && (ch <= 'f'))
		return ch - 'a' + 10;
	if ((ch >= 'A') && (ch <= 'F'))
		return ch - 'A' + 10;
	return -1;
}

//解析数值
int digit_lex(void)
{
	int val= 0;
	char ch;
	FILE *fp;
	int ret;

	fp = input.fp;
	ch = input.ch;
	if(ch == '0') {
		ch = fgetch(fp);
		if(isdigit(ch)) {	//八进制
			do {
				val *= 8;
				val += ch - '0';
			}while(isdigit(ch = fgetch(fp)));
		} else if(ch == 'x' || ch == 'X') { //16进制
			ch = fgetch(fp);
			if(isalnum(ch)) {
				do {
					ret = hex(ch);
					if(ret == -1) {
						return ERR_HEX;
					}
					val *= 16;
					val += ret;
				}while(isalnum(ch = fgetch(fp)));
			}
		}
	}
	else { //十进制
		do {
			val *= 10;
			val += ch - '0';
		}while(isdigit(ch = fgetch(fp)));
	}

	input.ch = ch;
	value = val;		//词法分析器识别到的数值常量

	if(ispunct(ch) || isspace(ch)) {
		return TS_CONSTANT;
	}
	return ERR_ID;
}

//解析字母
int alpha_lex(void)
{
	char ch;
	FILE *fp;
	int i;
	int ret = TS_ID;
	char word[256];

	word[0] = input.ch;
	fp = input.fp;
	i = 1;
	ch = fgetch(fp);
	while((isalnum(ch) || ch == '_') && i < 256) {
		word[i++] = ch;
		ch = fgetch(fp);
	}

	if(ch == '\"' || ch == '\'' ||
			ch == '@'|| ch == '#' ||
			ch == '$'|| ch == '`')
		return ERR_ID;

	if(i >= 256)
		return ERR_ID_FLOW;
	word[i] = 0;
	input.ch = ch;

	if(i > 1 && i < 10)
		switch(word[0]) {
			case 'a':
				if(i == 4 && strncmp(word + 1, "uto", 3) == 0)
					ret = get_next_token();//auto跳过
				break;
			case 'b':
				if(i == 5 && strncmp(word + 1, "reak", 4) == 0)
					ret = TS_BREAK;
				break;
			case 'c':
				if(i == 4 && strncmp(word + 1, "har", 3) == 0)
					ret = TS_CHAR;
				else if(i == 4 && strncmp(word + 1, "ase", 3) == 0)
					ret = TS_CASE;
				if(i == 5 && strncmp(word + 1, "onst", 4) == 0)
					ret = get_next_token(); //const 跳过
				else if(i == 8 && strncmp(word + 1, "ontinue", 7) == 0)
					ret = TS_CONTINUE;
				break;
			case 'd':
				if(i == 2 && word[1] == 'o')
					ret = TS_DO;
				else if(i == 7 && strncmp(word + 1, "efault", 6) == 0)
					ret = TS_DEFAULT;
				else if(i == 6 && strncmp(word + 1, "ouble", 5) == 0)
					ret = TS_DOUBLE;
				break;
			case 'e':
				if(i == 4 && strncmp(word + 1, "num", 3) == 0)
					ret = TS_ENUM;
				else if(i == 4 && strncmp(word + 1, "lse", 3) == 0)
					ret = TS_ELSE;
				else if(i == 6 && strncmp(word + 1, "xtern",5) == 0)
					ret = TS_EXTERN;
				break;
			case 'f':
				if(i == 3 && strncmp(word + 1, "or", 2) == 0)
					ret = TS_FOR;
				else if(i == 5 && strncmp(word + 1, "loat", 4) == 0)
					ret = TS_FLOAT;
				break;
			case 'g':
				if(i == 4 && strncmp(word + 1, "oto", 3) == 0)
					ret = TS_GOTO;
				break;
			case 'i':
				if(i == 2 && word[1] == 'f')
					ret = TS_IF;
				else if(i == 3 && strncmp(word + 1, "nt", 2) == 0)
					ret = TS_INT;
					else if(i == 6 && strncmp(word + 1, "nline", 5) == 0)
						ret = get_next_token(); //inline跳过
				break;
			case 'l':
				if(i == 4 && strncmp(word + 1, "ong", 3) == 0)
					ret = TS_LONG;
				break;
			case 'r':
				if(i == 6 && strncmp(word + 1, "eturn", 5) == 0)
					ret = TS_RETURN;
				else if(strcmp(word, "register") == 0)
					ret = get_next_token();
				break;
			case 's':
				switch(word[1]){
					case 'i':
						if(i == 6 && strncmp(word + 2, "gned", 5) == 0)
							ret = TS_SIGNED;
						else if(i == 6 && strncmp(word + 2, "zeof", 5) == 0)
							ret = TS_SIZEOF;
						break;
					case 'h':
						if(i == 5 && strncmp(word + 2, "ort", 4) == 0)
							ret = TS_SHORT;
						break;
					case 't':

						if(i == 6 && strncmp(word + 2, "atic", 5) == 0)
							ret = TS_STATIC;
						else if(i == 6 && strncmp(word + 2, "ruct", 5) == 0)
							ret = TS_STRUCT;
						break;
					case 'w':
						if(i == 6 && strncmp(word + 2, "itch", 5) == 0)
							ret = TS_SWITCH;
						break;
				}
				break;
			case 't':
				if(i == 7 && strncmp(word + 1, "ypedef", 6) == 0)
					ret = TS_TYPEDEF;
				break;
			case 'u':
				if(i == 5 && strncmp(word + 1, "nion", 4) == 0)
					ret = TS_UNION;
				else if(i == 8 && strncmp(word + 1, "nsigned", 7) == 0)
					ret = TS_UNSIGNED;
				break;
			case 'v':
				if(i == 4 && strncmp(word + 1, "oid", 3) == 0)
					ret = TS_VOID;
				else if(i == 8 && strncmp(word + 1, "olatile", 7) == 0)
					ret = get_next_token();
				break;
			case 'w':
				if(i == 5 && strncmp(word + 1, "hile", 4) == 0)
					ret = TS_WHILE;
				break;
			default:
				ret = TS_ID;
		}

	if(ret == TS_ID) {
		ret = return_string(word, i);
		if(ret == 0)
			ret = TS_ID;
	}
	return ret;
}

//跳过注释
int skip_comment(void)
{
	char ch;
	FILE *fp;
	char line[256];
	char next;

	fp = input.fp;
	ch = input.ch;
	if(ch == '/') {
		fgets(line, 256, fp);
		++no;
		return 0;
	}

	if(ch == '*') {
		do {
			next = fgetch(fp);
			if(ch == '*' && next == '/')
				break;
			if(ch == '/' && next == '*')
				return ERR_COMMENT;
			ch = next;
		}while(ch != EOF);
		if(ch == EOF)
			return ERR_COMMENT;
	}
	return 0;
}

//转义字符解析
int escap_lex(void)
{
	char ch, next_ch;
	FILE *fp;

	fp = input.fp;
	ch = fgetch(fp);
	switch(ch) {
		case 'a':
			return '\a';
		case 'b':
			return '\b';
		case 'f':
			return '\f';
		case 'n':
			return '\n';
		case 'r':
			return '\r';
		case 't':
			return '\t';
		case 'v':
			return '\v';
		case '\\':
			return '\\';
		case '\'':
			return '\'';
		case '\"':
			return '\"';
		case 'x':
		case 'X':
			ungetch(ch, fp);
			input.ch = 0;
			digit_lex();
			ungetch(input.ch, fp);
			return value;
		default:
			if(isdigit(ch)) {
				input.ch = ch;
				digit_lex();
				ungetch(input.ch, fp);
				return value;
			}
			else {
				while(ch = fgetch(fp) && isspace(ch));
				return ' ';
			}
	}
}

//跳过预处理部分，比如头文件，宏定义,条件编译
int preprocess(void)
{
	char ch;
	FILE *fp;
	char line[256];

	fp = input.fp;
	ch = input.ch;

	if(ch == 'd') {
		do{
			fgets(line, 256, fp);
			++no;
			//printf("no: %d\n", no);
		} while(ch = line[strlen(line) - 1] == '\\');

		if(ch == EOF)
			return ERR_COMMENT;
	}
	else {
		fgets(line, 256, fp);
		++no;
	}
	return 0;
}

//字符串解析
int string_lex(char ch)
{
	char next_ch;
	FILE *fp;
	int ret;
	int i;
	char *str;

	fp = input.fp;
	next_ch = input.ch;
	switch(ch) {
		case '\\':
			{
				char line[256];
				memset(line, 0, 256);
				str = line;
				fgets(str, 256, fp);
				++no;
				while(*str) {
					if(!isspace(*str++))
						return ERR_ESCAP;
				}
			}
			break;
		case '\"':
			{
				char str[256] = {0};
				i = 0;
				while(next_ch != '\"' && i < 256) {
					switch(next_ch) {
						case EOF:
							printf("Error: error string\n");
							input.ch = next_ch;
							return ERR_STRING;
						case '\\':
							str[i++] = escap_lex();
							break;
							//case '\"':字符串合成
						default:
							str[i++] = next_ch;
					}
					next_ch = fgetch(fp);
				}
				ret = return_string(str, 255);
				if(ret == 0)
					ret = TS_STR;
			}
			break;
		case '\'':
			if(next_ch == '\'') {
				value = 0;
				ret = TS_CONSTANT;
				break;
			}
			if(next_ch == '\\')
				value = escap_lex();
			else
				value = next_ch;
			while((next_ch = fgetch(fp)) != '\'' && next_ch != EOF);
			if(next_ch == EOF) {
				input.ch = next_ch;
				return ERR_CONST_CHAR;
			}
			ret = TS_CONSTANT;
			break;
	}

	return ret;
}

//标点符号解析
int punct_lex(void)
{
	char ch, next_ch;
	FILE *fp;
	int ret;

	fp = input.fp;
	ch = input.ch;
	next_ch = fgetch(fp);

	switch(ch) {
		case '+':
			if(next_ch == '+')
				ret = TS_INC;
			else if(next_ch == '=')
				ret = TS_ADD_ASSIGN;
			else
				ret = TS_ADD;
			break;
		case '-':
			if(next_ch == '-')
				ret = TS_DEC;
			else if(next_ch == '=')
				ret = TS_SUB_ASSIGN;
			else if(next_ch == '>')
				ret = TS_POINTER;
			else
				ret = TS_SUB;
			break;
		case '*':
			if(next_ch == '=')
				ret = TS_MUL_ASSIGN;
			else
				ret = TS_MUL;
			break;
		case '/':
			if(next_ch == '=')
				ret = TS_DIV_ASSIGN;
			else if(next_ch == '/' || next_ch == '*') {
				input.ch = next_ch;
				if(skip_comment() < 0)
					return ERR_COMMENT;
				input.ch = fgetch(fp);
				return get_next_token();
			}
			else
				ret = TS_DIV;
			break;
		case '%':
			if(next_ch == '=')
				ret = TS_MOD_ASSIGN;
			else
				ret = TS_MOD;
			break;
		case '<':
			if(next_ch == '=')
				ret = TS_LESS_EQ;
			else if(next_ch == '<') {
				next_ch = fgetch(fp);
				if(next_ch == '=')
					ret = TS_LSHIFT_ASSIGN;
				else
				{
					ungetch(next_ch, fp);
					ret = TS_LSHIFT;
				}
			}
			else
				ret = TS_LESS;
			break;
		case '>':
			if(next_ch == '=')
				ret = TS_GREATER_EQ;
			else if(next_ch == '>') {
				next_ch = fgetch(fp);
				if(next_ch == '=')
					ret = TS_RSHIFT_ASSIGN;
				else
				{
					ungetch(next_ch, fp);
					ret = TS_RSHIFT;
				}
			}
			else
				ret = TS_GREATER;
			break;
		case '&':
			if(next_ch == '=')
				ret = TS_BITAND_ASSIGN;
			else if(next_ch == '&')
				ret = TS_AND;
			else
				ret = TS_BITAND;
			break;
		case '|':
			if(next_ch == '=')
				ret = TS_BITOR_ASSIGN;
			else if(next_ch == '|')
				ret = TS_OR;
			else
				ret = TS_BITOR;
			break;
		case '=':
			if(next_ch == '=')
				ret = TS_EQ;
			else
				ret = TS_ASSIGN;
			break;
		case '!':
			if(next_ch == '=')
				ret = TS_NOT_EQ;
			else
				ret = TS_NOT;
			break;
		case '^':
			if(next_ch == '=')
				ret = TS_BITXOR_ASSIGN;
			else
				ret = TS_BITXOR;
			break;
		case '{':
			ret = TS_LBRACE;
			break;
		case '}':
			ret = TS_RBRACE;
			break;
		case '(':
			ret = TS_LPAREN;
			break;
		case ')':
			ret = TS_RPAREN;
			break;
		case '[':
			ret = TS_LBRAKET;
			break;
		case ']':
			ret = TS_RBRAKET;
			break;
		case '.':
			if(next_ch == '.') {
				next_ch = fgetch(fp);
				if(next_ch == '.') {
					next_ch = fgetch(fp);
					ret = TS_ELLIPSE;
				}
				else {
					ungetch(next_ch, fp);
					ret = ERR_ELLIPSE;
				}
			}
			else
				ret = TS_DOT;
			break;

		case '?':
			ret = TS_QUESTION;
			break;
		case '~':
			ret = TS_COMPL;
			break;
		case ':':
			ret = TS_COLON;
			break;
		case ';':
			ret = TS_SEMICOLON;
			break;
		case ',':
			ret = TS_COMMA;
			break;
		case '#':
			//预编译范畴
			input.ch = next_ch;
			if(preprocess() < 0)
				return ERR_ID;
			input.ch = fgetch(fp);
			return get_next_token();
		case '`':
		case '@':
		case '$':
			return ERR_ID;

		default: //  \ " '
			input.ch = next_ch;
			ret = string_lex(ch);
			next_ch = fgetch(fp);
	}

	switch(next_ch) {
		case '+':
			if(ret != TS_INC) 
				input.ch = next_ch;
			else
				input.ch = fgetch(fp);
			break;
		case '-':
			if(ret != TS_DEC) 
				input.ch = next_ch;
			else
				input.ch = fgetch(fp);
			break;

		case '&':
			if(ret != TS_AND) 
				input.ch = next_ch;
			else
				input.ch = fgetch(fp);
			break;

		case '=':
		case '>':
		case '<':
		case '|':
			input.ch = fgetch(fp);
			break;
		default:
			input.ch = next_ch;
	}

	return ret;
}

//获取下一个词素
int get_next_token(void)
{
	FILE *fp;
	char ch;

	fp = input.fp;
	ch = input.ch;
	while(isspace(ch)){
		ch = fgetch(fp);
	}
	input.ch = ch;
	if(isalpha(ch) || ch == '_')
		return alpha_lex();
	if(ispunct(ch))
		return punct_lex();
	if(isdigit(ch))
		return digit_lex();
	return END_OF_FILE;
}

//词法器测试函数
int lex(void)
{
	int ret = 0;
	int count = 0;

	while(ret >= 0) {
		ret = get_next_token();
		++count;
		if(count % 30 == 0)
			printf("\n");
		switch(ret) {
			case TS_STR:
			case TS_ID:
				if(string) {
					printf("%s ", string);
				}
				break;
			case TS_CONSTANT:
				printf("%d ", value);
				break;
			default:
				printf("%s ", vtn_tbl[ret].strname);

		}
	}

	return ret;
}


//初始化词法器
void init_lex(char *file)
{
	init_string_heap(&str_heap, STRING_HEAP_SIZE);
	read_source(file);
#if 0
	lex();
#endif
}

//析构词法器
void release_lex(void)
{
	release_string_heap();
	close_source();
}
