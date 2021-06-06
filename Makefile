CC=gcc
CFLAGS=-fsanitize=address -Wall -Werror -std=gnu11 -g -lm

tests: tests.c virtual_alloc.c
	$(CC) $(CFLAGS) $^ -o $@

run_tests: 
	./runit.sh

clean:
	rm tests
