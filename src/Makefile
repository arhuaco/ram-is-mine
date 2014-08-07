CFLAGS+=-std=c99 -Wall -g

all: ram_is_mine

ram_is_mine: ram_is_mine.c
	$(CC) $(CFLAGS) -shared -fPIC -o ram-is-mine.so ram_is_mine.c -ldl -lpthread

test_malloc_build: test/test_malloc.c
	$(CC) $(CFLAGS) test/test_malloc.c -o test/test_malloc

.PHONY: clean test test_malloc

test: test_malloc
	@echo DONE! None of the tested programs crashed. That\'s what we know. Examine output!


test_malloc: ram_is_mine test_malloc_build
	MY_RAM_LIMIT=10000 LD_PRELOAD_64=./ram-is-mine.so LD_PRELOAD=./ram-is-mine.so test/test_malloc

clean:
	rm -f *.so *.o test/test_malloc
