CC = gcc 
CFLAGS = -g -std=c99 

OBJs = parse.tab.o attr.o instruction.o lex.yy.o 

default: compiler

compiler: ${OBJs}
	${CC} ${CFLAGS} ${OBJs} -o compiler

lex.yy.c: scan.l parse.tab.h
	flex -i scan.l

parse.tab.c: parse.y attr.h instruction.h
	bison -dv parse.y

parse.tab.h: parse.tab.c

clean:
	rm -f compiler lex.yy.c *.o parse.tab.[ch] parse.output

depend:
	makedepend -I. *.c



