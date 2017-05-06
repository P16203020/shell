CFILES = $(wildcard *.c)
TARGETS = $(CFILES:.c=)
CC ?= gcc 
CFLAGS =-DDEBUG -g3

all: $(TARGETS)
	
$(TARGETS): $(CFILES)
	$(CC) $(CFLAGS) -o $@ $(@:=.c) -I/usr/lib/x86_64-linux-gnu/ -lreadline -ltermcap

clean: $(TARGETS)
	rm $(TARGETS)
