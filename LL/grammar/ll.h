#ifndef _LL_H_
#define _LL_H_

int scan;
extern int next_tk;
#define next next_tk
#define output_error(str) do { printf(str"\n"); exit(1);}while(0)

void skip(int tk);
void expression();
void assignment_expression();
int assignment_operator();
void constant_expression();
void conditional_expression();
void logical_OR_expression();
void logical_AND_expression();
void inclusive_OR_expression();
void exclusive_OR_expression();
void AND_expression();
void equality_expression();
void relational_expression();
void shift_expression();
void additive_expression();
void multiplicative_expression();
void cast_expression();
int unary_expression();
void postfix_expression();;
void argument_expression_list();
void primary_expression();
void declaration_list(void);
void declaration_specifiers();
void storage_class_specifier( ) ;
void declaration();
void type_specifier();
void struct_specifier();
void union_specifier();
void struct_declaration_list() ;
void struct_declaration() ;
void specifier_qualifier_list() ;
void struct_declarator_list() ;
void enum_specifier( ) ;
void enumerator_list();
void enumerator() ;
void enumeration_constant() ;
void typedef_name() ;
void init_declarator_list();
void init_declarator();
void declarator() ;
void pointer() ;
void direct_declarator( );
void parameter_type_list() ;
void parameter_list() ;
void parameter_declaration() ;
void type_name() ;
void abstract_declarator() ;
void direct_abstract_declarator() ;
void initializer();
void initializer_list();
void designation();
void designator_list();
void designator();
void statement();
void labeled_statement();
void compound_statement();
void block_item_list();
void block_item();
void expression_statement();
void selection_statement();
void case_block();
void case_labeled_statement_list();
void case_labeled_statement();
void iteration_statement();
void jump_statement();
void translation_unit();
void function_definition_list();
void function_definition();
#endif //#ifndef
