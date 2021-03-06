CFILES = $(wildcard *.c)
TARGETS = $(CFILES:.c=)
CC ?= gcc 
CFLAGS =-DDEBUG -g3
	
$(TARGETS): $(CFILES)
	bison -d bison.y
	flex lex.l
	gcc -o shell shell.c lex.yy.c bison.tab.c  execute.c -I/usr/lib/x86_64-linux-gnu/ -lfl -lreadline -ltermcap

clean: 
	rm -rf shell
	rm -rf *.o
	rm -rf bison.tab.h
	rm -rf bison.tab.c
	rm -rf lex.yy.c
