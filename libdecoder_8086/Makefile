all: decoder

libdecoder.a: libdecoder.c *.h
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -c libdecoder.c -o libdecoder.o
	ar rcs libdecoder.a libdecoder.o

decoder: decoder.c libdecoder.a
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -L. decoder.c -o decoder -ldecoder

test: decoder
	./run_tests.sh

clean:
	rm -f decoder libdecoder.a *.o tests/*.bin tests/out_*
