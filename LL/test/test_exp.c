int postfix_exp(void)
{
	struct set *s;
	struct table *tb;
	int i, j, k;
	int nr;
	int code;

	code = stack[top].code;
	nr = tb_index[code + 1] - tb_index[code]; 

	
	for(i = 0; i < nr; ++i) {
		tb = table + tb_index[code] + i;
		s = set + tb->s_index;
		if(tb->r_index)
			continue; 
		if(is_follow(s, next_tk) && s->right[1] == 0) {
			if(s->code == NTS_UNARY_EXP) {
				stack[top].code = s->code;
				return 0;
			}
		}
	}
	return 1;
}

int unary_exp(void)
{
	struct set *s;
	struct table *tb;
	int i, j, k;
	int nr;
	int code;

	code = stack[top].code;
	nr = tb_index[code + 1] - tb_index[code]; 

	
	for(i = 0; i < nr; ++i) {
		tb = table + tb_index[code] + i;
		s = set + tb->s_index;
		if(tb->r_index)
			continue; 
		
		if(is_follow(s, next_tk) && s->right[1] == 0) {
			if(s->code == NTS_CST_EXP) {
				stack[top].code = s->code;
				return 0;
			}
		}
	}
	return 1;
}

int cast_exp(void)
{
	struct set *s;
	struct table *tb;
	int i, j, k;
	int nr;
	int code;

	code = stack[top].code;
	nr = tb_index[code + 1] - tb_index[code]; 

	
	for(i = 0; i < nr; ++i) {
		tb = table + tb_index[code] + i;
		s = set + tb->s_index;
		if(tb->r_index)
			continue; 
		
		if(is_follow(s, next_tk) && s->right[1] == 0) {
			if(s->code == NTS_MUL_EXP) {
				stack[top].code = s->code;
				return 0;
			}
		}
	}
	return 1;
}

int mul_exp(void)
{
	struct set *s;
	struct table *tb;
	int i, j, k;
	int nr;
	int code;

	code = stack[top].code;
	nr = tb_index[code + 1] - tb_index[code]; 

	
	for(i = 0; i < nr; ++i) {
		tb = table + tb_index[code] + i;
		s = set + tb->s_index;
		if(tb->r_index)
			continue; 
		
		if(is_follow(s, next_tk) && s->right[1] == 0) {
			if(s->code == NTS_ADD_EXP) {
				stack[top].code = s->code;
				return 0;
			}
		}
	}
	return 1;
}

int add_exp(void)
{
	struct set *s;
	struct table *tb;
	int i, j, k;
	int nr;
	int code;

	code = stack[top].code;
	nr = tb_index[code + 1] - tb_index[code]; 

	
	for(i = 0; i < nr; ++i) {
		tb = table + tb_index[code] + i;
		s = set + tb->s_index;
		if(tb->r_index)
			continue; 
		
		if(is_follow(s, next_tk) && s->right[1] == 0) {
			if(s->code == NTS_SHIFT_EXP) {
				stack[top].code = s->code;
				return 0;
			}
		}
	}
	return 1;
}

int shift_exp(void)
{
	struct set *s;
	struct table *tb;
	int i, j, k;
	int nr;
	int code;

	code = stack[top].code;
	nr = tb_index[code + 1] - tb_index[code]; 

	
	for(i = 0; i < nr; ++i) {
		tb = table + tb_index[code] + i;
		s = set + tb->s_index;
		if(tb->r_index)
			continue; 
		
		if(is_follow(s, next_tk) && s->right[1] == 0) {
			if(s->code == NTS_RELAT_EXP) {
				stack[top].code = s->code;
				return 0;
			}
		}
	}
	return 1;
}

int relational_exp(void)
{
	struct set *s;
	struct table *tb;
	int i, j, k;
	int nr;
	int code;

	code = stack[top].code;
	nr = tb_index[code + 1] - tb_index[code]; 

	
	for(i = 0; i < nr; ++i) {
		tb = table + tb_index[code] + i;
		s = set + tb->s_index;
		if(tb->r_index)
			continue; 
		
		if(is_follow(s, next_tk) && s->right[1] == 0) {
			if(s->code == NTS_EQ_EXP) {
				stack[top].code = s->code;
				return 0;
			}
		}
	}
	return 1;
}

int equality_exp(void)
{
	struct set *s;
	struct table *tb;
	int i, j, k;
	int nr;
	int code;

	code = stack[top].code;
	nr = tb_index[code + 1] - tb_index[code]; 

	
	for(i = 0; i < nr; ++i) {
		tb = table + tb_index[code] + i;
		s = set + tb->s_index;
		if(tb->r_index)
			continue; 
		
		if(is_follow(s, next_tk) && s->right[1] == 0) {
			if(s->code == NTS_BITAND_EXP) {
				stack[top].code = s->code;
				return 0;
			}
		}
	}
	return 1;
}

int bit_AND_exp(void)
{
	struct set *s;
	struct table *tb;
	int i, j, k;
	int nr;
	int code;

	code = stack[top].code;
	nr = tb_index[code + 1] - tb_index[code]; 

	
	for(i = 0; i < nr; ++i) {
		tb = table + tb_index[code] + i;
		s = set + tb->s_index;
		if(tb->r_index)
			continue; 
		
		if(is_follow(s, next_tk) && s->right[1] == 0) {
			if(s->code == NTS_XOR_EXP) {
				stack[top].code = s->code;
				return 0;
			}
		}
	}
	return 1;
}

int XOR_exp(void)
{
	struct set *s;
	struct table *tb;
	int i, j, k;
	int nr;
	int code;

	code = stack[top].code;
	nr = tb_index[code + 1] - tb_index[code]; 

	
	for(i = 0; i < nr; ++i) {
		tb = table + tb_index[code] + i;
		s = set + tb->s_index;
		if(tb->r_index)
			continue; 
		
		if(is_follow(s, next_tk) && s->right[1] == 0) {
			if(s->code == NTS_BITOR_EXP) {
				stack[top].code = s->code;
				return 0;
			}
		}
	}
	return 1;
}

int bit_OR_exp(void)
{
	struct set *s;
	struct table *tb;
	int i, j, k;
	int nr;
	int code;

	code = stack[top].code;
	nr = tb_index[code + 1] - tb_index[code]; 

	
	for(i = 0; i < nr; ++i) {
		tb = table + tb_index[code] + i;
		s = set + tb->s_index;
		if(tb->r_index)
			continue; 
		
		if(is_follow(s, next_tk) && s->right[1] == 0) {
			if(s->code == NTS_AND_EXP) {
				stack[top].code = s->code;
				return 0;
			}
		}
	}
	return 1;
}

int and_exp(void)
{
	struct set *s;
	struct table *tb;
	int i, j, k;
	int nr;
	int code;

	code = stack[top].code;
	nr = tb_index[code + 1] - tb_index[code]; 

	
	for(i = 0; i < nr; ++i) {
		tb = table + tb_index[code] + i;
		s = set + tb->s_index;
		if(tb->r_index)
			continue; 
		
		if(is_follow(s, next_tk) && s->right[1] == 0) {
			if(s->code == NTS_OR_EXP) {
				stack[top].code = s->code;
				return 0;
			}
		}
	}
	return 1;
}

int or_exp(void)
{
	struct set *s;
	struct table *tb;
	int i, j, k;
	int nr;
	int code;

	code = stack[top].code;
	nr = tb_index[code + 1] - tb_index[code]; 

	
	for(i = 0; i < nr; ++i) {
		tb = table + tb_index[code] + i;
		s = set + tb->s_index;
		if(tb->r_index)
			continue; 
		
		if(is_follow(s, next_tk) && s->right[1] == 0) {
			if(s->code == NTS_COND_EXP) {
				stack[top].code = s->code;
				return 0;
			}
		}
	}
	return 1;
}
