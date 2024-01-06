decoder: decoder.c
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L decoder.c -o decoder

test: decoder
	./run_tests.sh

clean:
	rm -f decoder tests/*.bin tests/out_*
