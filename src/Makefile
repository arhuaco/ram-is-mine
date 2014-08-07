# Silly Makefile.

all: ram_is_mine test_1

ram_is_mine: ram_is_mine.c
	gcc -shared -fPIC -o ram-is-mine.so ram_is_mine.c  -ldl -Wall -lpthread

test_1: test/test.c
	gcc -Wall test/test.c -o test_1


.PHONY: clean


clean:
	rm -f *.so *.o test_1
