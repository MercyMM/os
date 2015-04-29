#include <stdio.h>
#include <stdlib.h>
#include "grammar.h"
#include "ll.h"

int scan;
extern int next_tk;
#define next next_tk
#define output_error(str) do { printf(str); exit(1);}while(0)

void skip(int tk)
{
	if(scan != tk) 
		output_error("token is not match\n");
	scan = next;
	if(scan == -1)
		exit(0);
	next = get_next_token();
}

/* declaration_list:
	declaration 
	| declaration_list  declaration*/

void declaration_list(void)
{
//	skip(TS_SEMICOLON); //;
	declaration();
	while(is_first(first, NTS_DECLON, scan))
		declaration();
	if(is_follow(follow, NTS_DECLON_L, scan))
		return;
	else  
		output_error("在declaration_list结束时出错");
}

/* declaration_specifiers:
	storage_class_specifier 
	| storage_class_specifier  declaration_specifiers
	| type_specifier
	| type_specifier  declaration_specifiers*/

void declaration_specifiers()
{
	if(is_first(first, NTS_STORE_CLASS_SPF, scan)) {
		storage_class_specifier();
		if(is_follow(follow, NTS_DECLON_SPF, scan))
			return;
	}else if(is_first(first, NTS_TP_NAME,scan)) {
		type_specifier();
		if(is_follow(follow, NTS_DECLON_SPF, scan)) {
			return;
		}else 
			output_error("类型定义说明declaration_specifiers的开始符号错");

		while(is_first(first, NTS_TP_SPF, scan) || 
				is_first(first, NTS_STORE_CLASS_SPF, scan)) {
			if(is_first(first, NTS_STORE_CLASS_SPF, scan)) 
				storage_class_specifier( );
			if(is_first(first, NTS_TP_SPF, scan)) {
				type_specifier();
			}
		}
	}
}

/* storage_class_specifier 
   typedef  |  extern  |  static*/

void storage_class_specifier( ) 
{
	if ((scan == TS_TYPEDEF || scan == TS_EXTERN || scan == TS_STATIC)
			&& is_follow(follow, NTS_STORE_CLASS_SPF, scan))
		return;
	else
		output_error("在storage_class_specifier程序应该结束出错");
}

/*declaration:                                  //声明  
  declaration_specifiers ; 
  | declaration_specifiers init_declarator_list ;*/

void declaration()
{
	declaration_specifiers();
	if(scan == TS_SEMICOLON && is_follow(follow, NTS_DECLON, next)) {
		skip(TS_SEMICOLON);
		return;
	}
	else if(is_first(first, NTS_INIT_DECLOR_L,scan)) {
		init_declarator_list( );
		if(scan == TS_SEMICOLON && is_follow(follow, NTS_DECLON, next)) {
			skip(TS_SEMICOLON);
			return;
		}else
			output_error("init_declarator_list没有以结束';',出错");	
	}else
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
	switch (scan) {
		case TS_VOID: 
		case TS_CHAR:
		case TS_SHORT: 
		case TS_INT:
		case TS_LONG:
		case TS_FLOAT:
		case TS_DOUBLE:
		case TS_UNSIGNED:             //情形0
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
			typedef_name();
			break;
		default:
			output_error("在type_specifier程序应该结束出错");
	}
	if(is_follow(follow, NTS_TP_SPF, scan)) 
		return;
	else
		output_error("在type_specifier程序应该结束出错");
}

/*struct_specifier:                                //结构体声明
  struct  identifier  {  struct_declaration_list  }  //产生式0
  | struct  {  struct_declaration_list  }          //产生式1
  | struct  identifier                          //产生式2*/
void struct_specifier()
{
	if (scan == TS_ID) { 
		skip(TS_ID);
		if (scan == TS_LBRACE) {
			skip(TS_LBRACE);
			struct_declaration_list();
			if (scan == TS_RBRACE && is_follow(follow, NTS_STRUCT_SPF, scan)) {
				skip(TS_RBRACE);
				return;
			}
			else
				output_error("在struct_specifier程序应该结束出错");
		} 
		else if(is_follow(follow, NTS_STRUCT_SPF, scan)) {
			return;
		}
		else
			output_error("在struct_specifier程序应该结束出错");
	} 
	else if (scan == TS_LBRACE) {
		skip(TS_LBRACE);
		struct_declaration_list();
		if(scan == TS_RBRACE && is_follow(follow, NTS_STRUCT_SPF, next)) {
			skip(TS_RBRACE);
			return;
		}
		else
			output_error("在struct_specifier程序应该结束出错");
	}
	else
		output_error("在struct_specifier程序应该结束出错");
}

/*union_specifier:                                //结构体声明
  union  identifier  {  struct_declaration_list  }  //产生式0
  | union  {  struct_declaration_list  }          //产生式1
  | union  identifier                          //产生式2*/

void union_specifier()
{
	if (scan == TS_ID) {
		scan = get_next_token();
		if (scan == TS_LBRACE) {
			skip(TS_LBRACE);
			struct_declaration_list();
			if (scan == TS_RBRACE && is_follow(follow, NTS_UNION_SPF, next)) {
				skip(TS_RBRACE);
				return;
			}
			else
				output_error("在union_specifier程序应该结束出错");
		} 
		else if(is_follow(follow, NTS_UNION_SPF, scan)) {
			return;
		}
		else
			output_error("在union_specifier程序应该结束出错");
	}
	else if (scan  == TS_LBRACE) {
		skip(TS_LBRACE);
		struct_declaration_list();
		if (scan == TS_RBRACE && is_follow(follow, NTS_UNION_SPF, next)) {
			skip(TS_RBRACE);
			return;
		}
		else
			output_error("在union_specifier程序应该结束出错");
	} else
		output_error("在union_specifier程序应该结束出错");
}

/*struct_declaration_list:
  struct_declaration
  | struct_declaration_list  struct_declaration*/
void struct_declaration_list() 
{
	struct_declaration();
	while (is_first(first, NTS_STRUCT_DECLON,scan)) 
		struct_declaration( );
	if(is_follow(follow, NTS_STRUCT_DECLON_L, scan)) 
		return;
	else
		output_error("在struct_declaration_list程序应该结束出错");
}

/*struct_declaration:
  specifier_qualifier_list  struct_declarator_list  ;*/

void struct_declaration() 
{
	specifier_qualifier_list();

	if(!is_first(first, NTS_STRUCT_DECLOR_L,scan)) 
		output_error("在struct_declaration程序应该结束出错");

	struct_declarator_list();
	if (scan == TS_SEMICOLON && is_follow(follow, NTS_STRUCT_DECLON, next)) {
		skip(TS_SEMICOLON);
		return;
	}
	else
		output_error("在struct_declaration_list程序应该结束出错");
}

/*specifier_qualifier_list:
  type_specifier 
  | type_specifier  specifier_qualifier_list*/

void specifier_qualifier_list() 
{
	type_specifier(); //可以省略
	while (is_first(first, NTS_TP_SPF, scan)) 
		type_specifier();
	if(is_follow(follow, NTS_SPF_QUAL_L, scan)) 
		return;
	else
		output_error("在specifier_qualifier_list程序应该结束出错");
}

/*struct_declarator_list:
  declarator
  | struct_declarator_list  ,  declarator*/
void struct_declarator_list() 
{
	declarator();
	while (scan == TS_COMMA)
		declarator();
	if(is_follow(follow, NTS_STRUCT_DECLOR_L, scan)) 
		return;
	else
		output_error("在struct_declarator_list程序应该结束出错");
}

/*enum_specifier:
  enum  {  enumerator_list  }            //产生式0
  | enum  identifier  {  enumerator_list  }  //产生式1
  | enum  identifier                      //产生式2*/

void enum_specifier( ) 
{
	if (scan == TS_ID) {
		scan = get_next_token();
		if (scan == TS_LBRACE) {
			skip(TS_LBRACE);
			enumerator_list();
			if (scan  == TS_RBRACE && is_follow(follow, NTS_ENUM_SPF, next)) {
				skip(TS_RBRACE);
				return;
			}
			else
				output_error("在enum_specifier程序应该结束出错");
		}
		else if(is_follow(follow, NTS_ENUM_SPF, scan)) 
			return;
		else
			output_error("在enum_specifier程序应该结束出错");
	}
	else if (scan == TS_LBRACE) {
		skip(TS_LBRACE);
		enumerator_list();
		if (scan == TS_RBRACE && is_follow(follow, NTS_ENUM_SPF, next)) {
			skip(TS_RBRACE);
			return;
		}
		else
			output_error("在enum_specifier程序应该结束出错");
	}
}

/*enumerator_list:
	enumerator
	| enumerator_list  ,  enumerator*/
void enumerator_list()
{
	enumerator();
	while (scan == TS_COMMA)
		enumerator();
	if(is_follow(follow, NTS_ENUMOR_L, scan)) 
		return;
	else
		output_error("在enumerator_list程序应该结束出错");
}

/*enumerator:
	enumeration_constant
	| enumeration_constant  =  constant_expression*/
void enumerator() 
{
	enumeration_constant();
	if (scan == TS_ASSIGN) {
		skip(TS_ASSIGN);
		constant_expression();
		if(is_follow(follow, NTS_ENUMOR, scan)) 
			return;
		else
			output_error("在enumerator程序应该结束出错");
	}
	else if(is_follow(follow, NTS_ENUMOR, scan)) 
		return;
	else
		output_error("在enumerator程序应该结束出错");
}

/*enumeration_constant:  identifier*/

void enumeration_constant() 
{
	if (scan == TS_ID && is_follow(follow, NTS_ENUMON_CONST, next)) {
		skip(TS_ID);
		return;
	}
	else
		output_error("在enumeration_constant程序应该结束出错");
}

/*typedef_name:  identifier*/
void typedef_name() 
{
	if (scan == TS_ID && is_follow(follow, NTS_TYPEDEF_NAME, next)) {
		skip(TS_ID);
		return;
	}
	else
		output_error("在typedef_name程序应该结束出错");
}

/*init_declarator_list:
	init_declarator
	| init_declarator_list , init_declarator*/

void init_declarator_list()
{
	init_declarator();
	while(scan == TS_COMMA)
		init_declarator();
	if(is_follow(follow, NTS_INIT_DECLOR_L, scan))
		return;
	else  
		output_error("在init_declarator_list中出错");
}

/*init_declarator:
	declarator
	| declarator = initializer  // initializer暂时未实现*/

void init_declarator()
{
	declarator( );
	if(is_follow(follow, NTS_INIT_DECLOR, scan)) 
		return;
	if ( scan == TS_ASSIGN)
		initializer();
	else  
		output_error("在init_declarator中出错");
}

/*declarator:
	direct_declarator
	| pointer  direct_declarator*/

void declarator() 
{
	if(is_first(first, NTS_POINTER,scan)) {
		pointer();
		if(is_first(first, NTS_DIRECT_DECLOR,scan)) 
			direct_declarator();
		if(is_follow(follow, NTS_DECLOR, scan)) 
			return;
		else
			output_error("在pointer程序应该结束出错");
	}
	else if(is_first(first, NTS_DIRECT_DECLOR,scan)) {
		direct_declarator();
		if(is_follow(follow, NTS_DECLOR, scan)) 
			return;
		else
			output_error("在pointer程序应该结束出错");
	}
	else
		output_error("在pointer程序应该结束出错");
}

/*pointer:
	* 
	| *  pointer*/

void pointer() 
{
	while (scan == TS_MUL) {
		scan = get_next_token();
	}
	if(is_follow(follow, NTS_POINTER, scan)) 
		return;
	else
		output_error("在pointer程序应该结束出错");
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
	if(scan == TS_ID) 
		skip(TS_ID);
	if( scan == TS_LPAREN) {
		skip(TS_LPAREN);
		declarator( ) ;
		skip(TS_RPAREN);
	}
	while( scan == TS_LBRAKET) {
		if(is_first(first, NTS_ASSIGNMENT_EXP,next)) {
			skip(TS_LBRAKET);
			assignment_expression ( );
			skip(TS_RBRAKET);
		}
		else if (next == TS_RBRAKET){
			skip(TS_LBRAKET);
			skip(TS_RBRAKET);
		}
		else
			output_error("数组定义、说明出错");
	}  
	while( scan == TS_LPAREN) {
		if(is_first(first, NTS_PRMT_TP_L, next)) {
			skip(TS_LPAREN);
			parameter_type_list ( );
			skip(TS_RPAREN);
		}
		else if( next == TS_RPAREN) {
			skip(TS_LPAREN);
			skip(TS_RPAREN);
		}
		else
			output_error("函数定义、说明出错");
	}  
	return;
}

/*parameter_type_list
	parameter_list 
	| parameter_list  ,  ...    //暂不支持*/
void parameter_type_list() 
{
	parameter_list();
	if (scan == TS_COMMA) {
		skip(TS_COMMA);
		skip(TS_ELLIPSE);
	} 
	else if(is_follow(follow, NTS_PRMT_TP_L, scan)) 
		return;
	else
		output_error("在parameter_type_list程序应该结束出错");
}

/*parameter_list:
	parameter_declaration
	| parameter_list  ,  parameter_declaration*/
void parameter_list() 
{
	parameter_declaration();
	while (scan == TS_COMMA)
		parameter_declaration();
	if(is_follow(follow, NTS_PRMT_L, scan)) 
		return;
	else
		output_error("在parameter_list程序应该结束出错");
}

/*parameter_declaration:
	declaration_specifiers  declarator              //基本类型 
	| declaration_specifiers   //抽象类型,用于函数说明
	| declaration_specifiers  abstract_declarator*/
void parameter_declaration() 
{
	decalaration_specifiers();
	if (scan == TS_ID)
		declarator();
	else if (scan == TS_LBRAKET)
		abstract_declarator();
	else {
		while (scan == TS_LPAREN || scan == TS_MUL)
			scan = get_next_token();
		//当退出上个循环后,scan是'identifier'情况时,可能是(declarator)||
		//(parameter_type_list),但是这种情况(parameter_type_list)中,first(parameter_type_list)
		//中的identifier是typedef_name<_identifier产生式的identifier,而之前就有了
		//declaration_specifiers,由于一条声明句子中不可能有两个类型,所以选择了
		//(declarator)
		if (scan == TS_ID)
			declarator();
		else if (scan == TS_LBRAKET || scan == TS_RPAREN)
			abstract_declarator();
		else
			output_error("在parameter_list程序应该结束出错");
	}
	if(is_follow(follow, NTS_PRMT_DECLON, scan))
		return;
	else
		output_error("在parameter_list程序应该结束出错");
}

/*type_name:
	specifier_qualifier_list
	| specifier_qualifier_list  abstract_declarator*/
void type_name() 
{
	specifier_qualifier_list();
	if(is_first(first, NTS_ABSTRACT_DECLOR,scan)) {
		abstract_declarator();
		if(is_follow(follow, NTS_TP_NAME, scan)) 
			return;
		else
			output_error("在type_name程序应该结束出错");
	}
	else if(is_follow(follow, NTS_TP_NAME, scan)) 
		return;
	else
		output_error("在type_name程序应该结束出错");
}

/*abstract_declarator:
	pointer
	| direct_abstract_declarator
	| pointer  direct_abstract_declarator*/
void abstract_declarator() 
{
	if(is_first(first, NTS_POINTER,scan)) {
		pointer();
		if(is_first(first, NTS_DIRECT_ABSTRACT_DECLOR,scan)) 
			direct_abstract_declarator();
		if(is_follow(follow, NTS_ABSTRACT_DECLOR, scan)) 
			return;
		else
			output_error("在type_name程序应该结束出错");
	}
	else if(is_first(first, NTS_DIRECT_ABSTRACT_DECLOR,scan)) {
		direct_abstract_declarator();
		if(is_follow(follow, NTS_ABSTRACT_DECLOR, scan)) 
			return;
		else
			output_error("在type_name程序应该结束出错");
	} 
	else
		output_error("在type_name程序应该结束出错");
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
	if (scan  == TS_LPAREN) {
		scan = get_next_token();
		if(is_first(first, NTS_ABSTRACT_DECLOR,scan)) 
			abstract_declarator();
		else if(is_first(first, NTS_PRMT_TP_L,scan)) 
			parameter_type_list();
		skip(TS_RPAREN);
	} 
	else if (scan == TS_LBRAKET) {
start_lbraket_loop:
		scan = get_next_token();
		if (scan == TS_MUL) {
			if (next_tk != TS_RBRACE)
				assignment_expression();
		}
		skip(TS_RBRAKET);
	} 
	else
		output_error("在direct_abstract_declarator程序应该结束出错");
	if (scan == TS_LBRAKET)
		goto start_lbraket_loop;
	while (scan == TS_LBRAKET) {
		scan = get_next_token();
		if(is_first(first, NTS_PRMT_TP_L,scan)) {
			parameter_type_list();
			skip(TS_RPAREN);
		}
		if(is_follow(follow, NTS_DIRECT_ABSTRACT_DECLOR, scan)) 
			return;
		else
			output_error("在direct_abstract_declarator程序应该结束出错");
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
	if (scan == TS_LBRACE) {
		skip(TS_LBRACE);
		initializer( );
		if (scan == TS_COMMA || scan == TS_RBRACE)
			skip(scan);
		else
			output_error("在initializer程序出错");
	}
	else
		assignment_expression( );
	if(is_follow(follow, NTS_INITER, scan))
		return;
	else
		output_error("在initializer程序出错");
}

/*initializer_list: 
	initializer
	| designation  initializer
	| initializer_list  ,  initializer
	| initializer_list  ,  designation initializer*/

void initializer_list( ) 
{
	if(is_first(first, NTS_DESIGON, scan))
		designation( );
	initializer( );
	while (scan == TS_COMMA) {
		skip(TS_COMMA);
		if(is_first(first, NTS_DESIGON, scan))
			designation( );
		initializer( );
	}
	if (is_follow(follow, NTS_INITER_L, scan))
		return;
	else
		output_error("在initializer_list程序出错");
}

/*designation:
	designator_list  =*/

void designation( ) 
{
	designator_list( );
	if (scan  == TS_ASSIGN && is_follow(follow, NTS_DESIGON, next)) {
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
	designator( );
	while (is_first(first, NTS_DESIGOR, scan))
		designator( );
	if (is_follow(follow, NTS_DESIGOR_L, scan))
		return;
	else
		output_error("在designator_list程序出错");
}

/*designator:
	[  constant_expression  ] 
	| .  identifier*/

void designator( ) 
{
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

	if (is_follow(follow, NTS_DESIGOR, scan))
		return;
	else
		output_error("在designator程序出错");
}
#if 0
/*expression:
	 assignment_expression                  
	| expression ,  assignment_expression*/

void expression()
{
	assignment_expression();
	while(scan是',') {
		skip(',');
		assignment_expression();
	}

	if(is_follow(follow, NTS_EXP, scan)) 
		return;
	else
		output_error("在expression中出错");
}

/*assignment_expression:
	conditional_expression  //1  conditional_expression的部分语言以unary_expression开头
	| unary_expression   assignment_operator   assignment_expression  //2
	*/

void assignment_expression()
{
		//unary_expression的first集是conditional_expression子集(相同),导致如下解决方案
		if( identify_ assign_exp( ) != TRUE ){// identify_ assign_exp( )通过预扫描识别unary_expression跟随赋值符
        		conditional_expression(  ); //1   
			return;
		}else{
			if(scan是first(unary_expression)){ //2
if(is_first(first, , scan))
				unary_expression( );
		assignment_operator( );
		assignment_expression( );//右结合语义,用递归实现
	}
	if(scan是follow(assignment_expression))
if(is_follow(follow, , scan)) 
		return;
	else
		output_error("在assignment_expression中出错");
}
}
	int  identify_ assign_exp( );
	功能：通过预扫描识别出赋值表达式左部的unary_expression和赋值符
	  识别出unary_expression时,scan是assignment_operator,则返回TRUE否则,返回FALSE
       预识别前,保存源代码扫描指针、scan和next退出函数时,恢复源代码扫描指针、scan和next
	int  identify_ assign_exp ( ){// 识别赋值表达式左部
		old_seek = 源码扫描指针; old_scan = scan;  	old_next = next;//局部保存扫描起点
		af_stk_top _=4;//PUSH发现赋值运算符全局标志,初值FALSE
		* af_stk_top = FALSE;// 全局变量：af_stk_top ,即assign_flg_stk_top
		pre_scan = TRUE;//设置预扫描全局标志pre_scan = TRUE,运行unary_expression(  )
		unary_expression(  );
		pre_scan = FALSE;//撤销预扫描全局标志pre_scan = FALSE
		源码扫描指针= old_seek; scan = old_scan; next = old_next;//恢复扫描起点
		assign_flg = * af_stk_top;// 全局变量：af_stk_top ,即assign_flg_stk_top
		af_stk_top +=4;//POP, 取预扫描结果
		return  assign_flg;//由 assign_flg标志是否发现赋值表达式左部！
	}
assignment_operator:  /* one of */            // 赋值操作符
=  *=  /=  %=  +=  _=  <<=  >>=  &=  ^=  |=

函数结构
	assignment_operator(){
if(scan是= ,*= ,/= ,%=, +=, _= ,<<=, >>= ,&=, ^=, |=){
	skip(上面token
		return;
		}
		else
	output_error("在assignment_operator中现不合法运算符");
}

/*constant_expression: 
		conditional_expression*/

void constant_expression()
{
	conditional_expression();
	if(is_follow(follow, NTS_CONST_EXP, scan)) 
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
	logical_OR_expression();
	if(is_follow(follow, NTS_COND_EXP, scan)) 
		return;

	if(scan == TS_QUESTION) {
		skip('？');
		expression();
	}
	else
		output_error("在conditional_expression中缺失'？'");

	if(scan == TS_COLON) {
		skip('：');
		while(is_first(first, NTS_COND_EXP, scan)) {
			logical_OR_expression();
			if(is_follow(follow, NTS_COND_EXP, scan)) 
				break;//2展开结束

			if(scan == TS_QUESTION) {
				skip('？');
				expression();
			}
			else
				output_error("在conditional_expression中缺失'？'");
		}
	}
	else
		output_error("在conditional_expression中缺失'：'");

	if(is_follow(follow, NTS_COND_EXP, scan)) 
		return;
	else
		output_error("在conditional_expression中出错");
}

/*logical_OR_expression: 
		logical_AND_expression
	| logical_OR_expression  ||  logical_AND_expression */

void logical_OR_expression()
{
	logical_AND_expression();

	while(scan == TS_OR) {
		skip('||');
		logical_AND_expression();
	}

	if(is_follow(follow, NTS_OR_EXP, scan)) 
		return;
	else
		output_error("在logical_OR_expression中出错");
}

/*logical_AND_expression: 
		inclusive_OR_expression
	| logical_AND_expression  &&  inclusive_OR_expression  //2*/

void logical_AND_expression()
{
	inclusive_OR_expression();

	while(scan == TS_AND) {
		skip('&&');
		inclusive_OR_expression();
	}

	if(is_follow(follow, NTS_AND_EXP, scan)) 
		return;		
	else
		output_error("在logical_AND_expression中出错");
}

/*inclusive_OR_expression: 
	exclusive_OR_expression
	| inclusive_OR_expression  |  exclusive_OR_expression  //2*/

void inclusive_OR_expression()
{
	exclusive_OR_expression();

	while(scan == TS_BITOR) {
		skip('|');
		exclusive_OR_expression();
	}

	if(is_follow(follow, NTS_BITOR_EXP, scan)) 
		return;		
	else
		output_error("在inclusive_OR_expression中出错");
}

/*	exclusive_OR_expression: 
		AND_expression 
		| exclusive_OR_expression  ^  AND_expression  //2*/

void exclusive_OR_expression()
{
	AND_expression();

	while(scan == TS_BITXOR) {
		skip('^');
		AND_expression();
	}

	if(is_follow(follow, NTS_XOR_EXP, scan)) 
		return;		
	else
		output_error("在exclusive_OR_expression中出错");
}

/*AND_expression:
		 equality_expression 
		| AND_expression  &  equality_expression  //2*/

void AND_expression()
{
	equality_expression();

	while(scan == TS_BITAND) {
		skip('&');
		equality_expression();
	}

	if(is_follow(follow, NTS_BITAND_EXP, scan)) 
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
	relational_expression();

	while(scan == TS_EQ || scan == TS_NOT_EQ) {
		skip('= ='||'!=');
		relational_expression();
	}

	if(is_follow(follow, NTS_EQ_EXP, scan)) 
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
	shift_expression();

	while(scan == TS_LESS || scan == TS_GREATER || 
			scan == TS_LESS_EQ || scan == TS_GREATER_EQ) {
		skip('<'或'>'或 '<='或'>=');
		shift_expression();
	}

	if(is_follow(follow, NTS_RELAT_EXP, scan)) 
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
	additive_expression();

	while(scan == TS_LSHIFT || scan == TS_RSHIFT) {
		skip('<<'或'>>');
		additive_expression();
	}

	if(is_follow(follow, NTS_SHIFT_EXP, scan)) 
		return;		
	else
		output_error("在shift _expression中出错");
}

/*additive_expression: 
	multiplicative_expression
	| additive_expression  +  multiplicative_expression  //2
	| additive_expression  _  multiplicative_expression  //3*/

void additive_expression()
{
	multiplicative _expression();
		
	while(scan == TS_ADD || scan == TS_SUB) {
		multiplicative _expression();

		if(is_follow(follow, NTS_ADD_EXP, scan)) 
			return;		
		else
			output_error("在additive _expression中出错");
	}
}

/*multiplicative_expression: 
	cast_expression
	| multiplicative_expression  *  cast_expression  //2
	| multiplicative_expression  /  cast_expression  //3
	| multiplicative_expression  %  cast_expression  //4*/

void multiplicative_expression()
{
	cast _expression();

	while(scan == TS_MUL || scan == TS_DIV || scan == TS_MOD) {
		skip('*'或'/'或'%');
		cast _expression();
	}

	if(is_follow(follow, NTS_MUL_EXP, scan)) 
		return;		
	else
		output_error("在multiplicative _expression中出错");
}

/*cast_expression: 
		unary_expression 
		| (  type_name  )  cast_expression  //强制类型转换*/
	
void cast_expression()
{
	while(is_first(first, NTS_UNARY_EXP, scan) || scan == TS_LPAREN) {
		//first(unary_exp)中包含'(', 解决办法：使用next判断展开分支
		if(is_first(first, NTS_UNARY_EXP, scan) && scan != TS_LPAREN) {
			unary_expression();
			if(is_follow(follow, NTS_CST_EXP, scan)) 
				return;
			else
				output_error("在cast_expression中出错");
			break;//展开结束
		}

		if(scan == TS_LPAREN) {
			skip('(');
			if(is_first(first, NTS_TP_NAME, scan))
				type_name();
			else
				output_error("在cast_expression中出错");
			if(scan == TS_RPAREN)
				skip(')');
			else
				output_error("在cast_expression中()不skip(");
		}
	}
}

/*unary_expression:                     //一元运算
	postfix_expression//1
	| ++  unary_expression//2
	| __  unary_expression //3
	| unary_operator  cast_expression//4
	| sizeof  unary_expression//5
	| sizeof  (  type_name  )//6*/

void unary_expression()
{  // 利用pre_scan增加预识别赋值表达式左部的功能
if(is_first(first, NTS_POSTFIX_EXP, scan)) {
			局部保存全局预扫描标志pre_scan;//为postfix_expression( )营造环境     !!!!!!这两句可提前,优化将来！！！！！！
			pre_scan = FALSE;//撤销预扫描全局标志     !!!!!!这两句可提前,优化将来！！！！！！
			postfix_expression();
			恢复pre_scan标志;
			if(scan是follow(unary_expression)){
if(is_follow(follow, , scan)) 
				if( pre_scan  &&  scan是assignment_operator )   //设置发现赋值符全局标志
					* af_stk_top  = TRUE;       //unary_expression 是赋值表达式左部！
				return;
			}elseif(pre_scan)
				return;//FALSE出口
			else
				output_errer("unary_expression之后的字符出错");
}elseif(scan是'++'||'__'){//2、3
			skip('++' ||'__';
			局部保存全局预扫描标志pre_scan;//为unary_expression(  )营造环境   !!!!!!这两句可提前,优化将来！！！！！！
			pre_scan = FALSE;//撤销预扫描全局标志     !!!!!!这两句可提前,优化将来！！！！！！
			unary_expression( );
			恢复pre_scan标志;
			if(scan是follow(unary_expression)){
if(is_follow(follow, , scan)) 
				if( pre_scan  &&  scan是assignment_operator )   // 设置发现赋值符全局标志
					* af_stk_top  = TRUE;       //unary_expression 是赋值表达式左部！
				return;
			}else if(pre_scan)
				return;//FALSE出口
			else
				output_errer("unary_expression之后的字符出错");
		}elseif(scan是一元运算符：& * + _ ˜ !) { //4    
			skip(一元运算符：& * + _ ˜ ! ;
			局部保存全局预扫描标志pre_scan;//为cast_expression()营造环境    !!!!!!这两句可提前,优化将来！！！！！！
			pre_scan = FALSE;//撤销预扫描全局标志     !!!!!!这两句可提前,优化将来！！！！！！
			cast_expression();
			恢复pre_scan标志;
			if(scan是follow(unary_expression)){
if(is_follow(follow, , scan)) 
				if( pre_scan  &&  scan是assignment_operator )   // 设置发现赋值符全局标志
					* af_stk_top  = TRUE;       //unary_expression 是赋值表达式左部！
				return;
			}else if(pre_scan)
				return;//FALSE出口
			else
				output_errer("unary_expression之后的字符出错");
		}elseif(scan是  sizeof){//5
			skip('sizeof';
			局部保存全局预扫描标志pre_scan;//为unary_expression(  )营造环境    !!!!!!这两句可提前,优化将来！！！！！！ 
			pre_scan = FALSE;//撤销预扫描全局标志    !!!!!!这两句可提前,优化将来！！！！！！
			unary_expression(  );
			恢复pre_scan标志;
			if(scan是follow(unary_expression)){
if(is_follow(follow, , scan)) 
				if( pre_scan  &&  scan是assignment_operator )   //设置发现赋值符全局标志
					* af_stk_top  = TRUE;       //unary_expression 是赋值表达式左部！
				return;
			}else if(pre_scan)
				return;//FALSE出口
			else
				output_errer("unary_expression之后的字符出错");
		}elseif(scan是'sizeof' 并且 next是'('){//6
			skip('sizeof';
			skip('(';
			局部保存全局预扫描标志pre_scan;//为type_name()营造环境   !!!!!!这两句可提前,优化将来！！！！！！
			pre_scan = FALSE;//撤销预扫描全局标志    !!!!!!这两句可提前,优化将来！！！！！！
			type_name();
			恢复pre_scan标志;
			if(scan是')')
skip(')';
		}else	if(pre_scan)//7 起始字符不skip(
			return;//FALSE出口
		else
			output_errer("unary_expression的起始字符出错！");//通常错误出口
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
	primary_expression();
	if(is_follow(follow, NTS_POSTFIX_EXP, scan)) 
		return;
	//else   此处不能退出！！！！
	//output_error("在postfix_expression中出错");

	while(scan == TS_LBRAKET || scan == TS_LPAREN ||
			scan == TS_DOT || scan == TS_POINTER ||
			scan == TS_INC || scan == TS_DEC) {
		switch(scan){
			case TS_LBRAKET: //'[': //展开2
				skip('[');
				expression();
				if(scan == TS_RBRAKET)
					skip(']');
				else
					output_error("在postfix_expression中'[ ]'不skip(");
				break;

			case TS_LPAREN: //'('：
				skip('(');
				if(is_first(first, NTS_ARGUMENT_EXP_L, scan))
					argument_expression_list();
				else if(scan == TS_RPAREN) {
					skip(')');
					break; 
				}
				else
					output_error("在postfix_expression中'()'不skip(");
				break;

			case  TS_DOT: //'.': //展开5
				skip('.');
			case  TS_POINTER://'_>': //展开6
				skip('_>');
				if(scan == TS_ID) {
					skip('identifier');
					break;
				}
				else
					output_error("在postfix_expression中_>.期望ID");
				break;
			case TS_INC: //'++'：//展开7
				skip('++');
				break;
			case TS_DEC://'__'：//展开8
				skip('__');
		}//switch
	}//while

	if(is_follow(follow, NTS_POSTFIX_EXP, scan)) 
		return;
	else
		output_error("在postfix_expression中出错");
}

/*argument_expression_list:          
	assignment_expression
	| argument_expression_list  ,  assignment_expression  //参数表达式表*/

void argument_expression_list()
{
	assignment_expression();
	while(scan == TS_COMMA) {
		skip(',');
		assignment_expression();
	}
	if(is_follow(follow, NTS_ARGUMENT_EXP_L, scan)) 
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
	if(scan == TS_LPAREN) {
		skip('(');
		expression();
		if(scan == TS_RPAREN)
			skip(')');
		else
			output_error("在primary_expression中'()'不skip(");
	}
	else if((scan == TS_ID || scan == TS_CONSTANT ||
				scan == TS_STR) && is_follow(follow, NTS_PRIMARY_EXP, scan)) 
		skip('identifier '或CONST 或 STRING);

	if(is_follow(follow, NTS_PRIMARY_EXP, scan)) 
		return;
	else
		output_error("在primary_expression中出错");
}

/*labeled_statement 
	| compound_statement 
	| expression_statement 
	| selection_statement
	| iteration_statement 
	| jump_statement*/

void statement()
{
	//first(l_stm)和first(e_stm)冲突元素：ID
	//解决办法：分支到 l_stm需要判断next是不是'：'
	if(is_first(first, NTS_LAB_STM, scan) && next == TS_COLON) 
		labeled_statement();
	else if(is_first(first, NTS_COMP_STM, scan)) 
		compound_statement();
	else if(is_first(first, NTS_EXP_STM, scan))
		expression_statement();
	else if(is_first(first, NTS_SELECT_STM, scan))
		selection_statement();
	else if(is_first(first, NTS_ITER_STM, scan))
		iteration_statement();
	else if(is_first(first, NTS_JMP_STM, scan))
		jump_statement();
	else
		output_error("在statement中出错");
}

/*labeled_statement:　　　　　　　　　　　　//标签
	identifier : statement	//goto*/

void labeled_statement()
{
	if(scan == TS_ID && next == TS_COLON) {
		skip('identifier');
		skip('：');
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
	if(scan == TS_LBRACE) {
		skip('{');
		if(scan == TS_RBRACE && is_follow(follow, NTS_COMP_STM, next)) {
			skip('}');
			return;
		}
		block_item_list();
		if(scan == TS_RBRACE && is_follow(follow, NTS_COMP_STM, next))  {
			skip('}');
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

void block_item_list()\
{
	block_item();
	while (is_first(first, NTS_BLOCK_ITEM, scan))
		block_item();
	if(is_follow(follow, NTS_BLOCK_ITEM_L, scan)) 
		return;
	else
		output_error("在block_item_list中出错");
}

/*block_item:　　　　　　　　　　　　　　//复合语句块
	declaration
	| statement*/

void block_item()
{
	//first(declaration)和first(statement)冲突元素：ID
	//在declaration中如果第一个标识符就是ID,则此ID是个typedef
	//解决办法：查符号表,确定ID是不是typedef从而觉得下面分支
	if(scan == TS_ID)
		flag = search_table(scan);//falg返回空,则展开 declaration否则,展开 statement
	if(flag && is_first(first, NTS_DECLON, scan)) {
		declaration();
	}
	else if(!flag && is_first(first, NTS_STM, scan))
		statement();
	else
		output_error("在block_item起始字符出错");

	if(is_follow(follow, NTS_BLOCK_ITEM, scan)) 
		return;
	else
		output_error("在block_item中出错");
}

/*expression_statement:
	 | ;
	 | expression  ;*/

void expression_statement()
{
	if(scan == TS_SEMICOLON) {
		skip(';');
		return;
	}
	if(is_first(first, NTS_EXP_STM, scan))
		expression();
	if(scan == TS_SEMICOLON && is_follow(follow, NTS_EXP_STM, next)) {
		skip(';');
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
	if(scan == TS_IF && next == TS_LPAREN) {
		skip('if');
		skip('(');
		expression();
		if(scan == TS_RPAREN) {
			skip(')');
			statement();
			if(next != TS_ELSE && is_follow(follow, NTS_SELECT_STM, next)) 
				return;
			else if(next == TS_ELSE) {
				skip('else');
				statement();
			}
			else
				output_error("在selection_statement中出错");
		}
	}else if(scan == TS_SWITCH && next == TS_LPAREN) {
		skip('switch');
		skip('(');
		expression();
		if(scan == TS_RPAREN) {
			skip(')');
			if(scan == TS_LBRACE) {
				skip('{');
				case_block();
			}
			if(scan == TS_RBRACE)
				skip('}');
			else
				output_error("在selection_statement中{}不skip(");
		}
	}
	else
		output_error("在selection_statement中出错");

	if(is_follow(follow, NTS_SELECT_STM, scan)) 
		return;
	else
		output_error("在selection_statement中出错");
}

/*case_block:
	case_labeled_statement_list
	| case_labeled_statement_list  default  :  statement */

void case_block()
{
	case_labeled_statement_list();
	if(scan == TS_DEFAULT) {
		skip('default');
		skip('：');
		statement();
	}

	if(is_follow(follow, NTS_CS_BLOCK, scan)) 
		return;
	else
		output_error("在case_block中出错");
}

/*case_label_statement_list:
	case_labeled_statement
	| case_labeled_statement_list  case_labeled_statement*/

void case_label_statement_list()
{
	case_labeled_statement();
	while(is_first(first, NTS_CS_LAB_STM, scan))
		case_labeled_statement();

	if(is_follow(follow, NTS_CS_LAB_STM_L, scan)) 
		return;
	else
		output_error("在case_label_statement_list中出错");
}

/*case_labeled_statement:
	case  constant_expression  :  statement*/

void case_labeled_statement()
{
	skip('case');
	constant_expression();
	skip('：');
	statement();

	if(is_follow(follow, NTS_CS_LAB_STM, scan)) 
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
	if(scan == TS_WHILE) {
		skip('while');
		skip('(');
		expression();
		skip(')');
		statement();
	}
	else if(scan == TS_DO) {
		skip('do');
		statement();
		skip('while');
		skip('(');
		expression();
		skip(')');
		skip(';');
	}
	else if(scan == TS_FOR) {
		skip('for');
		skip('(');
		while(循环3次){
			if(is_first(first, NTS_EXP, scan))
				expression();
			if(是第三次循环)
				break;
			if(scan == TS_SEMICOLON)
				skip(';');
			else
				output_error("在for语句中出错");
		}
		skip(')');
		statement();
	}
	else
		output_error("在iteration_statement语句中出现不是while,do,for终结符");

	if(is_follow(follow, NTS_ITER_STM, scan)) 
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
	if(scan == TS_GOTO && next == TS_ID) {
		skip('goto');
		skip('identifier ');
		if(scan == TS_SEMICOLON && is_follow(follow, NTS_JMP_STM, next)) {
			skip(';');
			return;

		}
	}

	if(scan == TS_CONTINUE || scan == TS_BREAK) {
		skip('continue'||'break');
		if(scan == TS_SEMICOLON && is_follow(follow, NTS_JMP_STM, next)) {
			skip(';');
			return;
		}
	}

	if(scan == TS_RETURN) {
		skip('return');
		if(is_first(first, NTS_EXP, scan))
			expression();
		if(scan == TS_SEMICOLON && is_follow(follow, NTS_JMP_STM, next)) {
			skip(';');
			return;
		}
	}
	else
		output_error("在jump_statement中出错");
}
#endif

void translation_unit()
{
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
	function_definition( );
	while(is_first(first, NTS_DECLON_SPF, scan)) {
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
	compound_statement();
}
