int (*reduction[])(int i) = {
	[33] = mul_star, 
	[68] = r_parenthesis,
	[73] = id,
//	[76] = exp13, //bit_AND_exp,
//	[78] = exp13, //add_exp,
	[80] = assignment_exp,
//	[87] = exp13, //cast_exp,
	[89] = conditional_exp,
	[91] = declaration,
	[94] = declarator,
//	[99] = direct_declarator,
	[130] = specifier_qualifier_list,
	[138] = type_specifier,
//	[140] = exp13, //unary_exp,
};

//0-归约成功继续下一次归约，1-无归约移入新tk，-1-错误
int main_reduction(int code)
{
	struct set *s;
	struct table *tb;
	int i, j;
	int tmp;
	int nr;
	int gn = 0;

	if(IS_DIRECT_IN(code))
		return 1;

	nr = tb_index[code + 1] - tb_index[code]; 

	if(top > 0) {
		for(i = 0; i < nr; ++i) { 
			tb = table + tb_index[code] + i;
			s = set + tb->s_index;
			if(tb->r_index == 0) 
				break;
			for(tmp = tb->r_index - 1, j = top - 1; 
					j >= 0 && tmp >= 0; --tmp, --j) {
				if(s->right[tmp] != stack[j].code)
					break; 
			}

			if(tmp == -1) { 
				switch(code) {
					case NTS_DIRECT_DECLOR:
						if(next_tk == TS_LBRAKET || next_tk == TS_LPAREN) {
							return 1;
						}
					case NTS_STM:
						if(next_tk == TS_ELSE)
							return 1;
					case NTS_ASSIGNMENT_EXP:
						{
						if(stack[top - 2].code == NTS_POSTFIX_EXP)
							gn = 1;
						break;
						}
				}
				if(s->right[tb->r_index + 1] == 0) {
					top -= tb->r_index;
					stack[top].code = s->code;
					return 0;
				}
				if(!gn)
					return 1; 
			}
		}
	}
	else {
		for(i = 0; i < nr; ++i) { 
			tb = table + tb_index[code] + i;
			s = set + tb->s_index;
			if(tb->r_index == 0) 
				break;
		}
	}

	for(; i < nr; ++i) {
		tb = table + tb_index[code] + i;
		s = set + tb->s_index;
		if(s->right[1] == 0)
			break;
	}

	if(i == nr)
		return 1;

	if(i + 1 == nr) {
		if(is_follow(s, next_tk)) {
			stack[top].code = s->code;
			return 0;
		}
		else
			return 1;
	}

	return reduction[stack[top].code](i);
}

int LR(void)
{
	int finished = 0;
	int over = 0;
	int nr;
	int i, j;
	int *set;
	int code;
	char *str = "id";

	next_tk = get_next_token();
	while(!finished) {
		if(top >= STACK_SIZE - 1)
			return -1;
		code = stack[++top].code = next_tk;
		next_tk = get_next_token();
		if(next_tk == -1)
			next_tk = 0;
	//	printf("\ncurr: %s   next: %s\n", 
	//			token_str[code], token_str[next_tk]);

		while(over == 0) {
			code = stack[top].code;
			switch(code) {
				case NTS_FUN_DEF_L:
					if(next_tk != 0) {
						over = 1;
						break;
					}
				default:
					over = main_reduction(code);
			}
			if(!over)
				output_stack();

		}
	}
	return -1;
}

int main(int argc, char *argv[]) 
{
	init_tb_index();
	init_ff();
	read_source(argv[1]);
	//printf("ret: %d\n", LR());
	close_source();
	return 0;
}
