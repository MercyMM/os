//#include <stdio.h>

typedef struct me {
	int a;
} SA;

typedef void(*fun)(int);
/*typedef *first[10] AA;
typedef AA[10] BB;*/
typedef int INT;

void you(int a)
{
	printf("%d\n", a);
	return;
}

int main(int argc, char *argv[])
{
	SA *sb;
	fun we;
	we = you;
//	BB s;

	we(10);

	return 0;
}
