#include <stdio.h>
#include "icc.h"

int main(int argc, char *argv[])
{
	int ret = 0;
	init_lex(argv[1]);
#if 1
	ret = grammar();
#endif
	release_lex();
	return ret;
}
