GammarObj=./grammar/grammar.o
LexObj=./lex/lex.o

icc: icc.o lex.o grammar.o
	cc -o icc icc.o $(GammarObj) $(LexObj)
lex.o:
	cd ./lex/ && make
ll.o:
grammar.o:
	cd ./grammar/ && make

icc.o: icc.c icc.h tsym.h nontsym.h
	cc -c -g icc.c

clean:
	cd ./grammar/ && make clean 
	cd ./lex/ && make clean 
	rm icc *.o
