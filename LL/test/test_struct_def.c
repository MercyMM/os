struct a{
	int a;
	struct a *next;
};

struct {
	char *name;
	struct a data[8];
};

union A{
	struct a a1;
	int b;
	char c;
};
