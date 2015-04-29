struct a {
	int b;
	struct {
		char ch;
		int id;
	} me;
	struct a *next;

} ;

struct a student[10] = {
	1, {1, 2}, 0,
	1, {1, 2}, 0,
	1, {1, 2}, 0,
	1, {1, 2}, 3
};

struct a b[4] = {
	.b = 0,
	.me = {.ch = a,
		   .id = 0},
	.c
	.d
	.e = 1};

union c{
	int i;
	char ch;
} age_sex[5] = {1, 2, 3, 4, 5};

enum {
	WOMEN = 1,
	MAN,
	CHILD
};
