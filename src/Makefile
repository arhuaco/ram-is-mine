# Silly Makefile.

CFLAGS=-std=c99 -Wall

all: ram_is_mine test_1

ram_is_mine: ram_is_mine.c
	gcc $(CFLAGS) -shared -fPIC -o ram-is-mine.so ram_is_mine.c -ldl -lpthread

test_1: test/test.c
	gcc $(CFLAGS) test/test.c -o test_1

.PHONY: clean
clean:
	rm -f *.so *.o test_1
