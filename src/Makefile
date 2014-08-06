# Silly Makefile.

all: ram_is_mine.c
	gcc -shared -fPIC -o ram-is-mine.so ram_is_mine.c  -ldl -Wall -lpthread

.PHONY: clean


clean:
	rm -f *.so *.o
