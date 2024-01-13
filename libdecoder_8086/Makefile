all: test_decoder

libdecoder: libdecoder.c
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -c libdecoder.c -o libdecoder.o
	ar rcs libdecoder.a libdecoder.o

test_decoder: test_decoder.c libdecoder
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -L. test_decoder.c -o test_decoder -ldecoder

clean:
	rm -f test_decoder libdecoder.a *.o 
