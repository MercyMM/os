
void output_stack(void)
{
	int i;
	for(i = 0; i <= top; ++i)
		printf("%s ", vtn_tbl[stack[i].code].strname);
	printf("\n");
}

/*根据字符串获得它的hash整值*/
unsigned int JSHash(char *str)
{
	unsigned int hash = 1315423911;

	while (*str) {
		hash ^= ((hash << 5) + (*str++) + (hash >> 2));
	}

	return (hash & 0x7FFFFFFF);
}

/*创建hash表，并初始化，STRING_HASH_SPACE大小的选择使得
 所有的终结符和非终结符字符串散列值不发生冲突*/
unsigned char * create_str_hash_tbl(void)
{
	int i;
	unsigned char *tbl;
	
	tbl = malloc(STRING_HASH_SPACE);
	if(tbl == NULL) {
		fprintf(stderr, "Error: malloc fail!\n");
		return NULL;
	}

	for(i = 1; i <= VT_N_NR; ++i) //以字符串的散列值为下标，存储它的编码值
		tbl[JSHash(vtn_tbl[i].strname) % STRING_HASH_SPACE] = 
											           vtn_tbl[i].code;
	return tbl;
}

/*销毁字符串的hash表*/
void destroy_str_hash_tbl(void *addr)
{
	free(addr);
}

/*初始化bnf产生式集合*/

/*对vb_map排序用的交换记录的辅助函数*/
void swap(struct vtn_bnf_map *a, struct vtn_bnf_map *b)  
{  
	struct vtn_bnf_map tmp;
	int size;

	size = sizeof(struct vtn_bnf_map);
	//字节拷贝
	memcpy(&tmp, a, size);
	memcpy(a, b, size);
	memcpy(b, &tmp, size);
}  

/*对vb_map表以code成员值快速排序的辅助函数*/
int partition(struct vtn_bnf_map *vb_map, int low, int high)  
{  
    int privotKey = vb_map[low].code;

    while(low < high) {
        while(low < high  && vb_map[high].code >= privotKey) 
			--high;
        swap(vb_map + low, vb_map + high);  
        while(low < high  && vb_map[low].code <= privotKey)
			++low;  
        swap(vb_map + low, vb_map + high);  
    }  

    return low;  
}  

/*对vb_map表以成员l_flg,r_flg,r_off，乘以相应权值作为表排序依赖值
 进行快速排序的辅助函数*/
int partition2(struct vtn_bnf_map *vb_map, int low, int high)  
{  
    int privotKey;
	int comp;
	int r_off;

	r_off = vb_map[low].r_off;
 	privotKey = vb_map[low].l_flg * 1000 + vb_map[low].r_flg * 100 + 
		r_off * 10 + !!bnf_set[vb_map[low].bnf_idx].right[r_off + 1];

    while(low < high) {
		while(low < high) {
			r_off = vb_map[high].r_off;
			comp = vb_map[high].l_flg * 1000 + vb_map[high].r_flg * 100 + 
				r_off * 10 + !!bnf_set[vb_map[high].bnf_idx].right[r_off + 1];
			if(comp > privotKey)
				break;
			--high;
		}
        swap(vb_map + low, vb_map + high);  
		while(low < high) {
			r_off = vb_map[low].r_off;
			comp = vb_map[low].l_flg * 1000 + vb_map[low].r_flg * 100 + 
				r_off * 10 + !!bnf_set[vb_map[low].bnf_idx].right[r_off + 1];
			if(comp < privotKey)
				break;
			++low;  
		}
        swap(vb_map + low, vb_map + high);  
    }  

    return low;  
} 

/*对vb_map表的code快速排序*/
void sort_code(struct vtn_bnf_map *vb_map, int low, int high)
{  
	int privot;

    if(low < high){  
        privot = partition(vb_map, low, high);
        sort_code(vb_map, low,  privot - 1);
        sort_code(vb_map, privot + 1, high);
    }  
}

/*对vb_map表的l_flg,r_flg,r_off三者的加权值快速排序*/
void sort_three(struct vtn_bnf_map *vb_map, int low, int high)
{  
	int privot;

    if(low < high){  
        privot = partition2(vb_map, low, high);
        sort_three(vb_map, low,  privot - 1);
        sort_three(vb_map, privot + 1, high);
    }  
}

/*初始化vb_map表*/
void init_vb_map(struct vtn_bnf_map *vb_map, struct bnf *bnf_set)
{
	int i, j;
	int idx;
	int tmp;
	int nr;
	int num[32];

	for(idx = 0, i = 0; i < BNF_NR; ++i, ++idx) {//遍历bnf_set
		tmp = idx;
		vb_map[idx].code = bnf_set[i].right[0];
		vb_map[idx].l_flg = 1;
		vb_map[idx].bnf_idx = i;
		for(j = 1; j < 11 && bnf_set[i].right[j]; ++j) {//遍历right
				++idx;
				vb_map[idx].code = bnf_set[i].right[j];
				vb_map[idx].l_flg = 0;
				vb_map[idx].r_flg = 1;
				vb_map[idx].r_off = j;
				vb_map[idx].bnf_idx = i;
		}
	}

	//以code从小到大排序
	sort_code(vb_map, 0, idx - 1);

	//排序表中三成员l_flg,r_flg,r_off
	tmp = vb_map[0].code;
	for(i = 0; i < idx; i = j) { 
		for(j = i; j < idx && tmp == vb_map[j].code; ++j)
			;
		if(i != j - 1) {
			sort_three(vb_map, i, j - 1);
		}

		tmp = vb_map[j].code;
	}
}

/*初始化vtn_tbl表*/
void init_vtn_tbl(struct vtn *vtn_tbl, struct vtn_bnf_map *vb_map)
{
	int i, j = 2;
	int temp;

	temp = vb_map[0].code;
	vtn_tbl[1].map_idx = 0;

	for(i = 1; i < MAP_NR; ++i) {
		//提取每个code在vb_map首次出现的偏移值
		if(vb_map[i].code != temp) {
			temp = vb_map[i].code; 
			vtn_tbl[j++].map_idx = i;
		}
	}
}

/*初始化first集和follow集；first_data和follow_data是依据vtb_tbl的code顺序
 把非终结符的first和follow集放到first_data和follow_data中。每个非终结符
 的follow(first)起头是其元素的个数*/
void init_ff(struct first_follow *first, struct first_follow *follow)
{
	int i, j;
	int code;

	//遍历非终结符
	for(i = j = 0, code = 0; code < VN_NR; ++code, ++i, ++j) { 
		follow[code].nr = follow_data[i]; //提取code的follow集元素个数
		follow[code].idx = i + 1; //记录code的follow集在follow_data中偏移
		i += follow_data[i]; //跳过此code的follow集到下一个code的follow基

		first[code].nr = first_data[j];
		first[code].idx = j + 1;
		j += first_data[j];
	}
}

/*查询follow集，判定终结符vt_code是不是非终结符vn_code的follow集元素*/
static int is_follow(struct first_follow *follow, int vn_code, int vt_code)
{
	int *f;
	int start, end, mid;
	int nr;
	
	//获得vn_code的follow集的起始地址
	f = follow_data + follow[vn_code - VT_NR - 1].idx;
	//获得vn_code的follow集的元素个数
	nr = follow[vn_code - VT_NR - 1].nr;
	//如果个数大于5个，则采用折半查找，每个非终结符follow集以按编码值从小到大排序
	if(nr > 5) {
		start = 0; end = nr - 1;
		while(end >= start) {
			mid = (start + end) >> 1;
			if(f[mid] == vt_code) //找到
				return 1;
			if(f[mid] > vt_code) 
				end = mid - 1;
			else
				start = mid + 1;
		}
		if(end < start)
			return 0;
	}
	else { //个数小于等于5个，采用遍历查找
		for(end = nr; end; --end, ++f){
			if(*f == vt_code) //找到
				return 1;
		}
		return 0;
	}
}

/*查询first集，判定终结符vt_code是不是非终结符vn_code的first集元素*/
static int is_first(struct first_follow *first, int vn_code, int vt_code)
{
	int *f;
	int start, end, mid;
	int nr;
	
	//获得vn_code的first集合的起始地址
	f = first_data + first[vn_code - VT_NR - 1].idx;
	//获得vn_code的first集合元素个数
	nr = first[vn_code - VT_NR - 1].nr;
	if(nr > 5) { //元素个数大于5个，折半查找
		start = 0; end = nr - 1;
		while(end >= start) {
			mid = (start + end) >> 1;
			if(f[mid] == vt_code) //找到
				return 1;
			if(f[mid] > vt_code) 
				end = mid - 1;
			else
				start = mid + 1;
		}
		if(end < start) //未找到
			return 0;
	}
	else { //元素个数小于等于5个，遍历查找
		for(end = nr; end; --end, ++f){
			if(*f == vt_code)
				return 1;
		}
		return 0;
	}
}

/*获取code产生式索引在vb_map中项个数*/
static int get_code_bnf_nr(struct vtn *vtn_tbl, int code)
{
	return vtn_tbl[code + 1].map_idx - vtn_tbl[code].map_idx; 
}

/*终结符*自身向上归约函数*/

/*非终结符type_specifier自身向上归约函数*/

/*终结符ID自身向上归约函数*/

/*非终结符declarator自身向上归约函数*/

/*非终结符declaration自身向上归约函数*/

/*非终结符conditional_expression自身向上归约函数*/

/*非终结符assignment_expression 自身向上归约函数*/

/*需要向栈后看，决定自身向上归约方向的符号*/

/*下面的终结符出现的时候，不用考虑是否归约，一定不会归约，直接移进*/

/*向前看才能决定归约方向的归约处理函数*/

/*测试输出函数*/
void output_product(struct bnf *set)
{
	int i;
	for(i = 0; i < NR; ++i) { 
		if(i == 1)
			printf(" -> ");
		if(set->right[i])
			printf("%s ", get_vtn_string(vtn_tbl, set->right[i]));
		else
			break;
	}
	printf("\n");
}

/*主归约函数*/
int main_reduction(int code)
{
	struct bnf *set;
	struct vtn_bnf_map *map;
	int i, j;
	int tmp;
	int nr;
	int ret;

	if(is_direct_in(code))
		return 1;

	nr = get_code_bnf_nr(vtn_tbl, code);

	if(top > 0) {
		for(i = 0; i < nr; ++i) { 
			map = vb_map + vtn_tbl[code].map_idx + i;
			if(map->l_flg) //跳过产生式左部的索引
				continue;
			
			set = bnf_set + map->bnf_idx;
			if(map->r_off == 1) //向后匹配的产生式匹配完毕 
				break;

			//向后匹配产生式
			for(tmp = map->r_off - 1, j = top - 1; 
					j >= 0 && tmp >= 1; --tmp, --j) {
				if(set->right[tmp] != stack[j].code)
					break; 
			}

			//成功向后匹配产生式
			if(tmp == 0) { 
				ret = look_front(i); //解决归约歧义
				if(ret != 2) {
					if(ret == 1)
						ret = check_in(i);
					return ret;
				}
			}
		}
	}
	else {
		for(i = 0; i < nr; ++i) { 
			map = vb_map + vtn_tbl[code].map_idx + i;
			if(map->l_flg) //跳过产生式左部的索引
				continue;
			set = bnf_set + map->bnf_idx;
			if(map->r_off == 1) 
				break;
		}
	}

	for(; i < nr; ++i) {
		map = vb_map + vtn_tbl[code].map_idx + i;
		set = bnf_set + map->bnf_idx;
		if(set->right[2] == 0)
			break;
	}

	if(i == nr)
		return 1; //不需要自身向上归约，移进

	if(i + 1 == nr) { //仅有一个自身向上归约的产生式
		if(is_follow(follow, set->right[0], next_tk)) {
			//next_tk是此产生式左部的follow基，则归约
			stack[top].code = set->right[0]; 
			return 0;
		}
		else //不是follow基，则继续移进
			return 1;
	}
	return look_back(i); //处理自身向上归约多个方向的字符
}


//语法归约模块主函数
int grammar(void)
{
	int ret;

	str_hash_tbl = create_str_hash_tbl();
	if(str_hash_tbl == NULL) 
		return 0;
	ret = init_bnf_set("bnf.txt", bnf_set);
	if(ret) {
		destroy_str_hash_tbl(str_hash_tbl);
		return -1;
	}
	destroy_str_hash_tbl(str_hash_tbl);

	init_vb_map(vb_map, bnf_set);
	init_vtn_tbl(vtn_tbl, vb_map);
	init_ff(first, follow);
	LR();

	return ret;
}
