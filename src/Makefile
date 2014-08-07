CFLAGS=-std=c99 -Wall

all: ram_is_mine test_1

ram_is_mine: ram_is_mine.c
	$(CC) $(CFLAGS) -shared -fPIC -o ram-is-mine.so ram_is_mine.c -ldl -lpthread

test_1: test/test.c
	$(CC) $(CFLAGS) test/test.c -o test_1

.PHONY: clean test

test: all
	MY_RAM_LIMIT=10000 LD_PRELOAD_64=./ram-is-mine.so LD_PRELOAD=./ram-is-mine.so ./test_1

clean:
	rm -f *.so *.o test_1
